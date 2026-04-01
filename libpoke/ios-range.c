#include "pk-utils.h"
#include "ios.h"
#include "ios-dev.h"
#include "pvm-val.h"
#include "ios-range.h"

#if 0
/* Range table implemented as an augmented interval tree.
   See e.g.
    https://en.wikipedia.org/wiki/Interval_tree#Augmented_tree

   Every interval is represented as a node in a binary search tree, ordered
   by the low end of the interval (which corresponds to the offset where
   a value is mapped; the high end is offset + size).

   Each node additionally stores the highest high-value of any range of itself
   or any of its children.  This "high-water" mark gives a fast way to avoid
   checking subtrees for interval overlaps.  */

struct interval_node {
  uint64_t low;	  /* Low end of interval,  i.e. mapped value offset. */
  uint64_t high;  /* High end of interval, i.e. mapped value offset + size.  */

  /* The highest high-value of any interval in this node or its subtrees.
     Used to speed up interval overlap checks: if the low end of an interval we
     want to check is higher than this, then it can never overlap any interval
     in the subtree rooted at this node.  */
  uint64_t highest;

  /* The relevant mapped pvm_val this node represents.  */
  uint64_t val;

  struct interval_node *left;
  struct interval_node *right;
  struct interval_node *parent;
};


struct ios_rangetbl {
  uint64_t num_entries;

  struct interval_node *tree;

  /* Could store multiple trees for each ios, e.g. if the offset of this
     value/write is at tree->highest * 4, then start a new tree.
     Then when checking overlaps we check each tree, but all trees are
     potentially smaller and easier to balance.
     Helps in cases where workloads have many changes in local areas
     but those areas are each disjoint.
     Suggested by Jose.  */
};


/* TODO: The tree implemented here does not (yet) do any self-balancing.
   This may lead to very degraded performance by imbalancing the tree.
   For example, if the first value mapped in an IOS is mapped at offset 0,
   then every subsequently mapped value will go in the right subtree... */

/* Allocate and return a new interval node.  */
static struct interval_node *
interval_mknode (uint64_t val, uint64_t low, uint64_t high)
{
  struct interval_node *new
    = (struct interval_node *) malloc (sizeof (struct interval_node));

  if (!new)
    return NULL;

  new->val = val;
  new->low = low;
  new->high = high;
  new->highest = high;
  new->left = NULL;
  new->right = NULL;
  new->parent = NULL;

  return new;
}

/* Insert the interval [LOW, HIGH] with value VAL into the interval tree
   rooted at NODE.  */
static int
interval_tree_insert (struct interval_node *node,
		      uint64_t val, uint64_t low, uint64_t high)
{
  if (!node)
    return IOS_ERROR;

  /* Update high-water marker for this node.  */
  if (high > node->highest)
    node->highest = high;

  /* TODO: this tree is simple and does not do any sort of self-balancing.  */

  if (low < node->low)
    {
      if (node->left)
	return interval_tree_insert (node->left, val, low, high);
      else
	{
	  node->left = interval_mknode (val, low, high);
	  if (!node->left)
	    return IOS_ENOMEM;
	  node->left->parent = node;
	}
    }
  else
    {
      if (node->right)
	return interval_tree_insert (node->right, val, low, high);
      else
	{
	  node->right = interval_mknode (val, low, high);
	  if (!node->left)
	    return IOS_ENOMEM;
	  node->right->parent = node;
	}
    }

  return IOS_OK;
}

/* Locate the node containing VAL in the tree rooted at NODE, using OFFS as
   the offset of the mapped value to guide the search.
   VAL is assumed to be unique in the tree.
   If OFFS is wrong, the node may not be located.  */
static struct interval_node *
interval_tree_lookup (struct interval_node *node, uint64_t val, uint64_t offs)
{
  if (!node)
    return NULL;

  /* If the low offset is above the high-water of this node, it cannot be
     anywhere in the subtree from this node.  */
  if (offs > node->highest)
    return NULL;

  /* Check this node, assuming val unique */
  if (node->val == val)
    return node;

  if (offs < node->low)
    return interval_tree_lookup (node->left, val, offs);
  else
    return interval_tree_lookup (node->right, val, offs);
}

static uint64_t
recalc_max (struct interval_node *node)
{
  uint64_t hw = node->high;
  if (node->left && node->left->highest > hw)
    hw = node->left->highest;
  if (node->right && node->right->highest > node->highest)
    hw = node->right->highest;

  return hw;
}

/* Return the minimum node in the tree rooted at NODE.  */
static struct interval_node *
interval_tree_min (struct interval_node *node)
{
  while (node->left)
    node = node->left;
  return node;
}

/* Replace node OLD with NEW in place. Helper for deletion.
   Does not free OLD.  */
static void
replace_node (struct interval_node *old, struct interval_node *new)
{
  if (old->parent == NULL)
    ;
  else if (old == old->parent->left)
    old->parent->left = new;
  else /* u == u->parent->right */
    old->parent->right = new;

  if (new)
    new->parent = old->parent;
}

/* Delete NODE from its tree (and maintain the tree).
   Return whatever node now occupies its place.  */
static struct interval_node *
interval_tree_delete (struct interval_node *node)
{
  struct interval_node *s = NULL;
  if (node->left == NULL && node->right == NULL)
    {
      /* No children, replace with NULL (updates the parent).  */
      replace_node (node, NULL);
    }
  else if (node->left == NULL)
    {
      /* No left subtree -> replace with right child.  */
      replace_node (node, node->right);
      s = node->right;
    }
  else if (node->right == NULL)
    {
      /* No right subtree -> replace with left child.  */
      replace_node (node, node->left);
      s = node->left;
    }
  else
    {
      /* NODE is somewhere in the middle, with both left and right subtrees.
	 Replace it with the in-order successor, S, which is just the minimum
	 node in the right subtree.
	 Note that S does not have a left subtree.  */
      s = interval_tree_min (node->right);

      if (s != node->right)
	{
	  /* Replace S by its own right child (it has no left). */
	  replace_node (s, s->right);

	  /* S takes NODE's right subtree.  */
	  s->right = node->right;
	  s->right->parent = s;
	}

      replace_node (node, s);

      /* S takes NODE's left subtree.  */
      s->left = node->left;
      s->left->parent = s;

      /* Recalculate the high-water mark for S since it has new children.  */
      s->highest =  recalc_max (s);
    }

  /* After replacing NODE, recalculate the high-water mark in the parent,
     which may have been from NODE's interval.  */
  if (node->parent)
    node->parent->highest = recalc_max (node->parent);

  free (node);
  return s;
}

/* Remove the node containing VAL if it is in the tree rooted at NODE,
   which should be mapped at OFFS.
   VAL is assumed to be unique in the tree.
   If OFFS is incorrect, the node may not be removed even if VAL was
   present in the tree.

   Return value is the new root of the tree, in case the node we are
   removing was the root.  */

static struct interval_node *
interval_tree_remove (struct interval_node *node, uint64_t val, uint64_t offs)
{
  if (!node)
    return NULL;

  struct interval_node *target;
  target = interval_tree_lookup (node, val, offs);

  if (!target)
    return node;

  if (!target->parent)
    {
      /* We are removing the root of the tree. */
      return interval_tree_delete (target);
    }

  interval_tree_delete (target);
  return node;
}

/* Check a single interval node for overlap against the given interval.  */

static int
interval_node_overlap (struct interval_node *node, uint64_t low, uint64_t high)
{
  return (node->low <= high && node->high >= low);
}

/* Traverse the interval tree rooted at ROOT and mark any values in it dirty
   if their interval overlaps with the one provided by [LOW,HIGH].  */

static void
interval_tree_mark (struct interval_node *node, uint64_t low,
		    uint64_t high)
{
  if (!node)
    return;

  /* If the low end of the interval is above the high-water mark of this node,
     then it cannot overlap any interval in this (sub)tree.  */
  if (low > node->highest)
    return;

  /* Check left subtree.  */
  interval_tree_mark (node->left, low, high);

  /* Check this node.  */
  if (interval_node_overlap (node, low, high))
    /* pvm_val_set_dirty (node->val); */
    PVM_VAL_SET_DIRTY_P (node->val, 1);

  /* If the high end of the interval is less than the low of this node,
     there will be no matches in the right subtree.  */
  if (high < node->low)
    return;

  /* Check right subtree.  */
  interval_tree_mark (node->right, low, high);
}

static void
interval_tree_mark_all (struct interval_node *node)
{
  if (!node)
    return;

  PVM_VAL_SET_DIRTY_P (node->val, 1);
  interval_tree_mark_all (node->left);
  interval_tree_mark_all (node->right);
}

static void
interval_tree_destroy (struct interval_node *node)
{
  if (!node)
    return;

  interval_tree_destroy (node->left);
  interval_tree_destroy (node->right);
  free (node);
}
#endif


/* ************** Interface via ios_rangetbl ******************** */

struct NODE_IMPL;
struct ios_rangetbl
{
  size_t count;
  struct NODE_IMPL *root;
};

/* typedef struct ios_rangetbl_impl * container_t; */

/* #define CONTAINER_T struct ios_rangetbl_impl * */
typedef struct ios_rangetbl * CONTAINER_T;
#define NODE_PAYLOAD_FIELDS pvm_val val;
#define NODE_PAYLOAD_ASSIGN(node) \
  node->val = val;
#define NODE_PAYLOAD_PARAMS \
  const pvm_val val
#define NODE_PAYLOAD_ARGS val
#define NODE_PAYLOAD_COMPARE(node) \
  (node->val == val)

#include "ios-ivtree.h"

/* struct ios_rangetbl_impl */
/* { */
/*   size_t count; */
/*   NODE_T root; */
/* }; */

int
ios_rangetbl_insert (struct ios_rangetbl *tbl, uint64_t val,
		     ios_off begin, ios_off end)
{
  /* ios_ivtree_insert (tbl, begin, end, val) */

  return ios_ivtree_insert (tbl, tbl->root, begin, end, val);

  #if 0
  int res = IOS_OK;
  if (tbl->root)
    {
      res = ios_ivtree_insert (tbl, begin, end, val);
      tbl->count++;
    }
  else
    {
      tbl->root = interval_mknode (begin, end);
      if (!tbl->root)
	res = IOS_ENOMEM;
      tbl->count = 1;
    }

  return res;
  #endif
}

void
ios_rangetbl_remove (struct ios_rangetbl *tbl, uint64_t val, ios_off offs)
{

  NODE_T target = ios_ivtree_lookup (tbl->root, offs, val);
  if (target)
    gl_tree_remove_node (tbl, target);

  #if 0
  struct interval_node *new_root = interval_tree_remove (tbl->tree, val, offs);
  tbl->tree = new_root;
  #endif
}


struct ios_rangetbl *
ios_rangetbl_create (void)
{
  struct ios_rangetbl *tbl = malloc (sizeof (struct ios_rangetbl));
  if (!tbl)
    return NULL;

  tbl->root = NULL;
  tbl->count = 0;
  return tbl;
}

void
ios_rangetbl_destroy (struct ios_rangetbl *tbl)
{
  if (!tbl)
    return;

  ios_ivtree_destroy (tbl);

  /* struct interval_node *node = tbl->tree; */
  /* interval_tree_destroy (node); */
  /* tbl->count = 0; */
  /* free (tbl); */
}

void
ios_rangetbl_dirty (struct ios_rangetbl *tbl, ios_off begin, ios_off end)
{
  if (!tbl)
    return;

  ios_ivtree_mark_overlaps (tbl->root, begin, end);

  /* interval_tree_mark (tbl->tree, (uint64_t) begin, (uint64_t) end); */
}

void
ios_rangetbl_dirty_all (struct ios_rangetbl *tbl)
{
  if (!tbl)
    return;

  ios_ivtree_mark_all (tbl->root);
  /* interval_tree_mark_all (tbl->tree); */
}

uint64_t
ios_rangetbl_nentries (struct ios_rangetbl *tbl)
{
  /* return tbl->num_entries; */
  return tbl->count;
}
