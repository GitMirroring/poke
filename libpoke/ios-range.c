#include "pk-utils.h"
#include "ios.h"
#include "ios-dev.h"
#include "pvm-val.h"
#include "ios-range.h"


struct NODE_IMPL;
struct ios_rangetbl
{
  size_t count;
  struct NODE_IMPL *root;
};

typedef struct ios_rangetbl * CONTAINER_T;
#define NODE_PAYLOAD_FIELDS pvm_val val;
#define NODE_PAYLOAD_ASSIGN(node) \
  node->val = val;
#define NODE_PAYLOAD_PARAMS \
  const pvm_val val
#define NODE_PAYLOAD_ARGS val
#define NODE_PAYLOAD_ACCESS(node) \
  node->val
#define NODE_PAYLOAD_COMPARE(node) \
  (node->val == val)

typedef void (*node_visitor_fn)(NODE_PAYLOAD_PARAMS);
#define NODE_VISITOR_FN node_visitor_fn

#include "ios-ivtree.h"


/* ************** Interface via ios_rangetbl ******************** */

int
ios_rangetbl_insert (struct ios_rangetbl *tbl, pvm_val val,
		     ios_off begin, ios_off end)
{
  return ios_ivtree_insert (tbl, tbl->root, begin, end, val);
}

void
ios_rangetbl_remove (struct ios_rangetbl *tbl, pvm_val val, ios_off offs)
{

  NODE_T target = ios_ivtree_lookup (tbl->root, offs, val);
  if (target)
    gl_tree_remove_node (tbl, target);
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

}

void
mark_dirty (const pvm_val val)
{
  PVM_VAL_SET_DIRTY_P (val, 1);
}

void
ios_rangetbl_dirty (struct ios_rangetbl *tbl, ios_off begin, ios_off end)
{
  if (!tbl)
    return;

  ios_ivtree_visit_overlaps (tbl->root, begin, end, mark_dirty);
}

void
ios_rangetbl_dirty_all (struct ios_rangetbl *tbl)
{
  if (!tbl)
    return;

  ios_ivtree_visit_all (tbl->root, mark_dirty);
}

size_t
ios_rangetbl_nentries (struct ios_rangetbl *tbl)
{
  return tbl->count;
}
