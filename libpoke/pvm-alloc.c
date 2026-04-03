/* pvm-val.c - Memory allocator for the PVM.  */

/* Copyright (C) 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026 Jose E.
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

#define GC_THREADS
#include <gc/gc.h>

#include "pvm.h"
#include "pvm-val.h"

void *
pvm_alloc (size_t size)
{
  return GC_MALLOC (size);
}

void *
pvm_alloc_uncollectable (size_t size)
{
  return GC_MALLOC_UNCOLLECTABLE (size);
}

void pvm_free_uncollectable (void *ptr)
{
  GC_FREE (ptr);
}

void *
pvm_realloc (void *ptr, size_t size)
{
  return GC_REALLOC (ptr, size);
}

char *
pvm_alloc_strdup (const char *string)
{
  return GC_strdup (string);
}

static void
pvm_alloc_finalize_closure (void *object, void *client_data)
{
  /* XXX this causes a crash because of a cycle in the finalizers:
     routines of recursive PVM programs contain a reference to
     themselves, be it directly or indirectly.  */
  /* pvm_cls cls = (pvm_cls) object; */
  /*  pvm_destroy_program (PVM_VAL_CLS_PROGRAM (cls)); */
}

static void
pvm_alloc_finalize_struct (void *object, void *client_data)
{
  /* Remove from range map table.  */
  pvm_struct sct = (pvm_struct) object;
  pvm_val val = (uint64_t) sct | PVM_VAL_TAG_SCT;

  /* if (sct->mapinfo.mapped_p && sct->mapinfo.ioslive_p) */
  if (sct->mapinfo.io)
    ios_deregister_range (val, sct->mapinfo.io, sct->mapinfo.offset);
}

static void
pvm_alloc_finalize_array (void *object, void *client_data)
{
  /* Remove from range map table.  */
  pvm_array arr = (pvm_array) object;
  pvm_val val = (uint64_t) arr | PVM_VAL_TAG_ARR;

  /* if (arr->mapinfo.mapped_p && arr->mapinfo.ioslive_p) */
  if (arr->mapinfo.io)
    ios_deregister_range (val, arr->mapinfo.io, arr->mapinfo.offset);
}

void *
pvm_alloc_cls (void)
{
  pvm_cls cls = pvm_alloc (sizeof (struct pvm_cls));

  GC_register_finalizer_no_order (cls, pvm_alloc_finalize_closure, NULL,
                                  NULL, NULL);
  return cls;
}

void *
pvm_alloc_arr (void)
{
  pvm_array arr = pvm_alloc (sizeof (struct pvm_array));
  GC_register_finalizer_no_order (arr, pvm_alloc_finalize_array, NULL,
				  NULL, NULL);
  return arr;
}

void *
pvm_alloc_sct (void)
{
  pvm_struct sct = pvm_alloc (sizeof (struct pvm_struct));
  GC_register_finalizer_no_order (sct, pvm_alloc_finalize_struct, NULL,
				  NULL, NULL);
  return sct;
}

static void
pvm_alloc_finalize_boxed (void *object, void *client_data)
{
  pvm_val_box box = (pvm_val_box) object;
  pvm_val val = PVM_BOX (box);

  if (PVM_IS_SCT (val))
    {
      if (PVM_VAL_SCT_MAPPED_P (val) && PVM_VAL_SCT_IOSLIVE_P (val))
	ios_deregister_range (val, PVM_VAL_SCT_IOS_PTR (val),
			      PVM_VAL_INTEGRAL (PVM_VAL_SCT_OFFSET (val)));
    }
  else if (PVM_IS_ARR (val))
    {
      if (PVM_VAL_ARR_MAPPED_P (val) && PVM_VAL_ARR_IOSLIVE_P (val))
	ios_deregister_range (val, PVM_VAL_ARR_IOS_PTR (val),
			      PVM_VAL_INTEGRAL (PVM_VAL_ARR_OFFSET (val)));
    }
}

void *
pvm_alloc_boxed (uint8_t tag)
{
  pvm_val_box box = pvm_alloc (sizeof (struct pvm_val_box));
  if (tag == PVM_VAL_TAG_SCT)
    {
      /* pvm_struct sct = pvm_alloc_sct (); */
      pvm_struct sct = pvm_alloc (sizeof (struct pvm_struct));
      PVM_VAL_BOX_SCT (box) = sct;
    }
  else if (tag == PVM_VAL_TAG_ARR)
    {
      /* pvm_array arr = pvm_alloc_arr (); */
      pvm_array arr = pvm_alloc (sizeof (struct pvm_array));
      PVM_VAL_BOX_ARR (box) = arr;
    }
  else
    {
      assert (false);
    }
  PVM_VAL_BOX_TAG (box) = tag;

  GC_register_finalizer_no_order (box, pvm_alloc_finalize_boxed, NULL,
				  NULL, NULL);
  return box;
}


void
pvm_alloc_initialize ()
{
  /* Initialize the Boehm Garbage Collector, but only if it hasn't
     been already initialized.  The later may happen if some other
     library or program uses the boehm GC.  */
  if (!GC_is_init_called ())
    {
      GC_INIT ();
      GC_allow_register_threads ();
    }
}

void
pvm_alloc_finalize ()
{
  GC_gcollect ();
}

void
pvm_alloc_add_gc_roots (void *pointer, size_t nelems)
{
  GC_add_roots (pointer,
                ((char*) pointer) + sizeof (void*) * nelems);
}

void
pvm_alloc_remove_gc_roots (void *pointer, size_t nelems)
{
  GC_remove_roots (pointer,
                   ((char*) pointer) + sizeof (void*) * nelems);
}

void
pvm_alloc_register_thread ()
{
  struct GC_stack_base sb;
  int ok_p __attribute__ ((unused));

  ok_p = GC_get_stack_base (&sb) == GC_SUCCESS;
  assert (ok_p);
  /* The following call may return GC_SUCCESS or GC_DUPLICATE.  */
  GC_register_my_thread (&sb);
}

void
pvm_alloc_unregister_thread ()
{
  int ok_p __attribute__ ((unused));

  ok_p = GC_unregister_my_thread () == GC_SUCCESS;
  assert (ok_p);
}

void
pvm_alloc_gc ()
{
  GC_gcollect ();
}
