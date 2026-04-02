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

static void
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
}
