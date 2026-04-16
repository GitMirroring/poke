/* ios-range.c - IO mapped value range tracking for poke.  */

/* Copyright (C) 2026 Free Software Foundation, Inc.  */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

/* The IOS range table tracks, for every pvm_val mapped in a given
   IOS, the interval where that val is mapped.  Its purpose is to
   be able to quickly determine which mapped values are affected
   by a write to the IOS at some interval.

   The table is implemented as an augmented interval tree, a type of
   self-balancing binary search tree specialized to storing intervals,
   and facilitating quickly identifying all stored intervals which
   overlap with some query interval.  */

struct NODE_IMPL;
struct ios_rangetbl
{
  size_t count;			/* Number of values currently tracked.  */
  struct NODE_IMPL *root;	/* Root of the tree.  */
  payload_compar_fn compar;	/* Payload comparison function.  */
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

/* Prototype for visitor functions which operate on entries of the
   range table.  */
typedef void (*node_visitor_fn)(NODE_PAYLOAD_PARAMS);
#define NODE_VISITOR_FN node_visitor_fn

/* Payload (i.e., pvm_val) comparator a la qsort.  */

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

/* The actual tree implementation.  */
#include "ios-ivtree.h"

/* ************** Interface via ios_rangetbl ******************** */

int
ios_rangetbl_insert (struct ios_rangetbl *tbl, pvm_val val,
		     ios_off begin, ios_off end)
{
  int res = ios_ivtree_insert (tbl, begin, end, val);
  return res;
}

void
ios_rangetbl_remove (struct ios_rangetbl *tbl, pvm_val val, ios_off offs)
{
  NODE_T target = ios_ivtree_lookup (tbl, offs, val);
  if (target)
    gl_tree_remove_node (tbl, target);

  /* N.B. Attempting to remove an entry which is not in the tree is most
     likely an error.  */
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

  return tbl;
}

void
ios_rangetbl_destroy (struct ios_rangetbl *tbl)
{
  if (!tbl)
    return;

  ios_ivtree_destroy (tbl);
  GC_remove_roots (tbl, tbl + sizeof (struct ios_rangetbl));
  free (tbl);
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

static void
notify_ios_closed (pvm_val val)
{
  PVM_VAL_SET_IOSLIVE_P (val, 0);
}

void
ios_rangetbl_notify_close (struct ios_rangetbl *tbl)
{
  ios_ivtree_visit_all (tbl->root, notify_ios_closed);
}

