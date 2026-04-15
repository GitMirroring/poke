/* ios-range.c - IO mapped value range tracking for poke.  */

/* Copyright (C) 2026 Free Software Foundation, Inc.
   Written by David Faust <david.faust@oracle.com>, 2026.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "pk-utils.h"
#include "ios.h"
#include "ios-dev.h"
#include "pvm-val.h"
#include "ios-range.h"

#define GC_THREADS
#include <gc/gc.h>

#include <stdio.h>

typedef int (*payload_compar_fn)(pvm_val v1, pvm_val v2);

struct NODE_IMPL;
struct ios_rangetbl
{
  size_t count;
  struct NODE_IMPL *root;
  payload_compar_fn compar;

  /* struct NODE_IMPL *tail; */
};

typedef struct ios_rangetbl * CONTAINER_T;
#define NODE_PAYLOAD_FIELDS pvm_val val;
#define NODE_PAYLOAD_ASSIGN(node) \
  node->val = val;
#define NODE_PAYLOAD_PARAMS \
  pvm_val val
#define NODE_PAYLOAD_ARGS val
#define NODE_PAYLOAD_ACCESS(node) \
  node->val
#define NODE_PAYLOAD_EQUALS(node) \
  (node->val == val)

#define NODE_PAYLOAD_DISPOSE(container, node) ;

typedef void (*node_visitor_fn)(NODE_PAYLOAD_PARAMS);

#define NODE_VISITOR_FN node_visitor_fn


int
ivtree_payload_compar (pvm_val v1, pvm_val v2)
{
  if (v1 < v2)
    return -1;
  else if (v1 == v2)
    return 0;
  else
    return 1;
}

#include "ios-ivtree.h"
/* #include "ios-ivlist.h" */


/* ************** Interface via ios_rangetbl ******************** */

int
ios_rangetbl_insert (struct ios_rangetbl *tbl, pvm_val val,
		     ios_off begin, ios_off end)
{
  /* ios_rangetbl_debug (tbl); */
  /* printf ("ios_rangetbl_insert val=%lx offset=%lu end=%lu\n", */
	  /* val, begin, end); */
  /* return ios_ivtree_insert (tbl, tbl->root, begin, end, val); */

  int res = ios_ivtree_insert_c (tbl, begin, end, val);

  /* int res = ios_ivlist_insert (tbl, begin, end, val); */
  return res;
}

void
ios_rangetbl_remove (struct ios_rangetbl *tbl, pvm_val val, ios_off offs)
{

  /* NODE_T target = ios_ivtree_lookup (tbl->root, offs, val); */
  NODE_T target = ios_ivtree_lookup_c (tbl, offs, val);
  /* NODE_T target = ios_ivlist_lookup (tbl, val); */
  if (target)
    {
      /* printf ("ios_rangetbl_remove found valid target: val=%lx offset=%lu\n", */
	      /* val, offs); */
      gl_tree_remove_node (tbl, target);
      /* ios_ivlist_remove (tbl, target); */
    }
  else
    {
      printf ("XXX ios_rangetbl_remove no target val=%lx offset=%lu\n", val, offs);
    }
}


struct ios_rangetbl *
ios_rangetbl_create (void)
{
  struct ios_rangetbl *tbl = malloc (sizeof (struct ios_rangetbl));
  if (!tbl)
    return NULL;

  GC_add_roots (tbl, tbl + sizeof (struct ios_rangetbl));

  tbl->root = NULL;
  tbl->count = 0;
  tbl->compar = ivtree_payload_compar;

  /* tbl->tail = NULL; */

  return tbl;
}

void
ios_rangetbl_destroy (struct ios_rangetbl *tbl)
{
  if (!tbl)
    return;

  ios_ivtree_destroy (tbl);
  /* ios_ivlist_destroy (tbl); */
}

static void
mark_dirty (pvm_val val)
{
  PVM_VAL_SET_DIRTY_P (val, 1);
}

void
ios_rangetbl_dirty (struct ios_rangetbl *tbl, ios_off begin, ios_off end)
{
  if (!tbl)
    return;

  ios_ivtree_visit_overlaps (tbl->root, begin, end, mark_dirty);
  /* ios_ivlist_visit_overlaps (tbl, begin, end, mark_dirty); */
}

void
ios_rangetbl_dirty_all (struct ios_rangetbl *tbl)
{
  if (!tbl)
    return;

  ios_ivtree_visit_all (tbl->root, mark_dirty);
  /* ios_ivlist_visit_all (tbl, mark_dirty); */
}

size_t
ios_rangetbl_nentries (struct ios_rangetbl *tbl)
{
  return tbl->count;
}


static void
notify_ios_closed (pvm_val val)
{
  PVM_VAL_SET_IOSLIVE_P (val, 0);
}

/* Visit all entries in the table and update their mapinfo to
   reflect that the IOS where they are mapped has been closed.  */
void
ios_rangetbl_notify_close (struct ios_rangetbl *tbl)
{
  ios_ivtree_visit_all (tbl->root, notify_ios_closed);
  /* ios_ivlist_visit_all (tbl, notify_ios_closed); */
}

static void
debug (pvm_val val)
{
  struct pvm_mapinfo *mi;
  uint8_t tag = PVM_VAL_TAG (val);
  printf ("%lx tag=%x\n", val, tag);

  if (PVM_IS_ARR (val))
    {
      mi = &(PVM_VAL_ARR_MAPINFO (val));
      printf (" arr, mi=%p\n", mi);
      printf ("  mapped_p=%i, strict_p=%i, dirty_p=%i, ioslive_p=%i, offset=%lu\n",
	      mi->mapped_p, mi->strict_p, mi->dirty_p, mi->ioslive_p,
	      PVM_VAL_INTEGRAL (PVM_VAL_ARR_OFFSET (val)));
    }
  else if (PVM_IS_SCT (val))
    {
      mi = &(PVM_VAL_SCT_MAPINFO (val));
      printf (" sct, mi=%p\n", mi);
      printf ("  mapped_p=%i, strict_p=%i, dirty_p=%i, ioslive_p=%i, offset=%lu\n",
	      mi->mapped_p, mi->strict_p, mi->dirty_p, mi->ioslive_p,
	      PVM_VAL_INTEGRAL (PVM_VAL_ARR_OFFSET (val)));
    }
  else
    {
      printf ("  ?????\n");
    }
}

void
ios_rangetbl_debug (struct ios_rangetbl *tbl)
{
  /* ios_ivtree_visit_all (tbl->root, debug); */
  /* ios_ivlist_visit_all (tbl, debug); */
}
