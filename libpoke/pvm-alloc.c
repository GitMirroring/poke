/* pvm-val.c - Memory allocator for the PVM.  */

/* Copyright (C) 2019, 2020, 2021, 2022, 2023, 2024, 2025 Jose E.
 * Marchesi */

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

#include <config.h>
#include <assert.h>

#include <stdlib.h>
#include <string.h>

#include "pvm-alloc.h"
#include "pvm.h"
#include "pvm-val.h"

extern struct jitter_gc_heaplet *gc_heaplet; // keep in-sync with pvm-val.c

void
pvm_alloc_initialize ()
{
  assert (gc_heaplet == NULL);
}

void
pvm_alloc_finalize ()
{
  pvm_alloc_gc ();
}

void *
pvm_alloc_add_gc_roots (void *pointer, size_t nelems)
{
  return jitter_gc_register_global_root (gc_heaplet, pointer,
                                         nelems * sizeof (void *));
}

void
pvm_alloc_remove_gc_roots (void *handle)
{
  jitter_gc_deregister_global_root (gc_heaplet, handle);
}

int
pvm_alloc_register_thread ()
{
  return 0;
}

int
pvm_alloc_unregister_thread ()
{
  return 0;
}
