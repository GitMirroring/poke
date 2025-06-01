/* pvm-val.c - Values for the PVM.  */

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
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include "xalloc.h"

#include "pkt.h"
#include "pkl.h"
#include "pvm.h"
#include "pvm-program.h"
#include "pvm-val.h"
#include "pvm-alloc.h"
#include "pk-utils.h"

// XXX ?
#include "pvm-vm.h"

/* Unitary special values used internally by GC.
   Here we're re-using PVM_NULL's "tag".  */

#define PVM_VAL_INVALID_OBJECT          0x17
#define PVM_VAL_UNINITIALIZED_OBJECT    0x27
#define PVM_VAL_BROKEN_HEART_TYPE_CODE  0x37
#define PVM_VAL_PADDING                 0x47

/* Unitary values that are always reused.

   These values are created in pvm_val_initialize and disposed in
   pvm_val_finalize.  */

static pvm_val string_type;
static pvm_val void_type;

/* We are currently only supporting a relatively small number of
   integral types, i.e. signed and unsigned types of sizes 1 to 64
   bits.  It is therefore possible to cache these types to avoid
   allocating them again and again.

   The contents of this table are initialized in pvm_val_initialize
   and finalized in pvm_val_finalize.  New types are installed by
   pvm_make_integral_type.

   Note that the first entry of the table is unused; it would
   correspond to an integer of "zero" bits.  This is more efficient
   than correcting the index at every access at the cost of only
   64-bits.  */

static pvm_val common_int_types[65][2];

/* Jitter GC heap.

   TODO Move this to `pvm'.
 */

static struct jitter_gc_shape_table *gc_shapes;
static struct jitter_gc_heap *gc_heap;
struct jitter_gc_heaplet *gc_heaplet; // keep in-sync with pvm-alloc.c

#define GC_ALLOCATION_POINTER(HEAPLET)                                        \
  ((HEAPLET)->convenience_runtime_allocation_pointer)
#define GC_RUNTIME_LIMIT(HEAPLET) ((HEAPLET)->convenience_runtime_limit)

static inline void *
pvm_heaplet_alloc (struct jitter_gc_heaplet *heaplet, size_t nbytes)
{
  void *p;

  _JITTER_GC_ALLOCATE (heaplet, GC_ALLOCATION_POINTER (heaplet),
                       GC_RUNTIME_LIMIT (heaplet), p, nbytes);
  assert (p); // TODO Add error handling.
  return p;
}

/* INT */

pvm_val
pvm_make_int (int32_t value, int size)
{
  assert (0 < size && size <= 32);
  return PVM_MAKE_INT (value, size);
}

/* UINT */

pvm_val
pvm_make_uint (uint32_t value, int size)
{
  assert (0 < size && size <= 32);
  return PVM_MAKE_UINT (value, size);
}

/* LONG */

static bool
pvm_gc_is_long_p (pvm_val v)
{
  return PVM_IS_LONG (v);
}

static bool
pvm_gc_is_long_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_LONG;
}

#define PVM_GC_SIZEOF_LONG JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_long)

static size_t
pvm_gc_sizeof_long (pvm_val v __attribute__ ((unused)))
{
  return PVM_GC_SIZEOF_LONG;
}

static size_t
pvm_gc_copy_long (struct jitter_gc_heaplet *heaplet __attribute__ ((unused)),
                  pvm_val *new_val, void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_LONG);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_LONG;
}

static size_t
pvm_gc_update_fields_long (struct jitter_gc_heaplet *heaplet
                           __attribute__ ((unused)),
                           void *p __attribute__ ((unused)))
{
  /* Longs are leaf objects; no fields to update.  */
  return PVM_GC_SIZEOF_LONG;
}

pvm_val
pvm_make_long (int64_t value, int size)
{
  struct pvm_long *lng;

  assert (0 < size && size <= 64);

  {
    void *p;

    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p, PVM_GC_SIZEOF_LONG);
    assert (p); // FIXME FIXME FIXME Handle error.
    lng = p;
  }

  lng->type_code = PVM_VAL_TAG_LONG;
  lng->value = (uint64_t)value;
  lng->size_minus_one = (size - 1) & 0x3f;
  return PVM_BOX (lng);
}

/* ULONG */

static bool
pvm_gc_is_ulong_p (pvm_val v)
{
  return PVM_IS_ULONG (v);
}

static bool
pvm_gc_is_ulong_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_ULONG;
}

#define PVM_GC_SIZEOF_ULONG                                                   \
  JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_long /* not pvm_ulong */)

static size_t
pvm_gc_sizeof_ulong (pvm_val v __attribute__ ((unused)))
{
  return PVM_GC_SIZEOF_ULONG;
}

static size_t
pvm_gc_copy_ulong (struct jitter_gc_heaplet *heaplet __attribute__ ((unused)),
                    pvm_val *new_val, void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_ULONG);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_ULONG;
}

static size_t
pvm_gc_update_fields_ulong (struct jitter_gc_heaplet *heaplet
                            __attribute__ ((unused)),
                            void *p __attribute__ ((unused)))
{
  /* Ulongs are leaf objects; no fields to update.  */
  return PVM_GC_SIZEOF_ULONG;
}

pvm_val
pvm_make_ulong (uint64_t value, int size)
{
  struct pvm_long *ulng;

  assert (0 < size && size <= 64);

  {
    void *p;

    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p, PVM_GC_SIZEOF_LONG);
    assert (p); // FIXME FIXME FIXME Handle error.
    ulng = p;
  }

  ulng->type_code = PVM_VAL_TAG_ULONG;
  ulng->value = value;
  ulng->size_minus_one = (size - 1) & 0x3f;
  return PVM_BOX (ulng);
}

pvm_val
pvm_make_signed_integral (int64_t value, int size)
{
  if (size > 64)
    return PK_NULL;

  if (size <= 32)
    return PVM_MAKE_INT ((int32_t)value, size);
  else
    return pvm_make_long (value, size);
}

pvm_val
pvm_make_unsigned_integral (uint64_t value, int size)
{
  if (size > 64)
    return PK_NULL;

  if (size <= 32)
    return PVM_MAKE_UINT ((uint32_t)value, size);
  else
    return pvm_make_ulong (value, size);
}

pvm_val
pvm_make_integral (uint64_t value, int size, int signed_p)
{
  if (size > 64)
    return PK_NULL;

  if (size <= 32)
    return signed_p ? PVM_MAKE_INT ((int32_t)value, size)
                    : PVM_MAKE_UINT ((uint32_t)value, size);
  else
    return signed_p ? pvm_make_long (value, size)
                    : pvm_make_ulong (value, size);
}

/* STRING */

#define PVM_GC_SIZEOF_STRING JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_string)

static bool
pvm_gc_is_string_p (pvm_val v)
{
  return PVM_IS_STR (v);
}

static bool
pvm_gc_is_string_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_STR;
}

static size_t
pvm_gc_sizeof_string (pvm_val v __attribute__ ((unused)))
{
  return PVM_GC_SIZEOF_STRING;
}

static size_t
pvm_gc_copy_string (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                    void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_STRING);
  JITTER_GC_FINALIZABLE_COPY (pvm_string, finalization_data, heaplet, from,
                              to);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_STRING;
}

static size_t
pvm_gc_update_fields_string (struct jitter_gc_heaplet *heaplet
                             __attribute__ ((unused)),
                             void *p __attribute__ ((unused)))
{
  /* Strings are leaf objects; no fields to update.  */
  return PVM_GC_SIZEOF_STRING;
}

static void
pvm_gc_finalize_string (struct jitter_gc_heap *heap __attribute__ ((unused)),
                        struct jitter_gc_heaplet *heaplet
                        __attribute__ ((unused)),
                        void *obj)
{
  struct pvm_string *header = obj;

  free (header->data);
}

pvm_val
pvm_make_string_nodup (/*pvm apvm, */ char *str)
{
  struct pvm_string *header;

  {
    void *p;

    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p,
                         PVM_GC_SIZEOF_STRING);
    header = p;
  }

  header->type_code = PVM_VAL_TAG_STR;
  JITTER_GC_FINALIZABLE_INITIALIZE (pvm_string, finalization_data, header);
  header->data = str;
  return PVM_BOX (header);
}

pvm_val
pvm_make_string (/*pvm apvm, */ const char *str)
{
  return pvm_make_string_nodup (strdup (str));
}

/* ARRAY */

#define PVM_GC_SIZEOF_ARRAY JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_array)

static bool
pvm_gc_is_array_p (pvm_val v)
{
  return PVM_IS_ARR (v);
}

static size_t
pvm_gc_sizeof_array (pvm_val v __attribute__ ((unused)))
{
  return PVM_GC_SIZEOF_ARRAY;
}

static bool
pvm_gc_is_array_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_ARR;
}

static size_t
pvm_gc_copy_array (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                   void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_ARRAY);
  JITTER_GC_FINALIZABLE_COPY (pvm_array, finalization_data, heaplet, from, to);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_ARRAY;
}

static size_t
pvm_gc_update_fields_array (struct jitter_gc_heaplet *heaplet, void *obj)
{
  pvm_array arr = obj;
  size_t num_elems;

  // FIXME Use accessor macros.
  jitter_gc_handle_word (heaplet, &arr->mapinfo.ios);
  jitter_gc_handle_word (heaplet, &arr->mapinfo.offset);
  jitter_gc_handle_word (heaplet, &arr->mapinfo_back.ios);
  jitter_gc_handle_word (heaplet, &arr->mapinfo_back.offset);
  jitter_gc_handle_word (heaplet, &arr->elems_bound);
  jitter_gc_handle_word (heaplet, &arr->size_bound);
  jitter_gc_handle_word (heaplet, &arr->mapper);
  jitter_gc_handle_word (heaplet, &arr->writer);
  jitter_gc_handle_word (heaplet, &arr->type);
  jitter_gc_handle_word (heaplet, &arr->nelem);

  num_elems = PVM_VAL_ULONG (arr->nelem);
  for (size_t i = 0; i < num_elems; ++i)
    {
      jitter_gc_handle_word (heaplet, &arr->elems[i].offset);
      jitter_gc_handle_word (heaplet, &arr->elems[i].offset_back);
      jitter_gc_handle_word (heaplet, &arr->elems[i].value);
    }
  return PVM_GC_SIZEOF_ARRAY;
}

static void
pvm_gc_finalize_array (struct jitter_gc_heap *heap __attribute__ ((unused)),
                       struct jitter_gc_heaplet *heaplet
                       __attribute__ ((unused)),
                       void *obj)
{
  pvm_array arr = obj;

  free (arr->elems);
  arr->nallocated = 0;
  arr->elems = NULL;
}

pvm_val
pvm_make_array (pvm_val nelem, pvm_val type)
{
  size_t num_elems = PVM_VAL_ULONG (nelem);
  size_t num_allocated = num_elems > 0 ? num_elems : 16;
  size_t nbytes = sizeof (struct pvm_array_elem) * num_allocated;
  size_t i;
  pvm_array arr;
  pvm_val arr_nelem;
  pvm_val arr_mapinfo_off;

  /* All GC-related allocation should happen here.  */
  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &type);

    arr_nelem = pvm_make_ulong (0, 64);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &arr_nelem);

    arr_mapinfo_off = pvm_make_ulong (0, 64);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &arr_mapinfo_off);

    {
      void *p;

      _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                           GC_RUNTIME_LIMIT (gc_heaplet), p,
                           PVM_GC_SIZEOF_ARRAY);
      arr = p;
    }
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  /* Do not trigger GC-related allocation after this point.  */

  arr->type_code = PVM_VAL_TAG_ARR;
  JITTER_GC_FINALIZABLE_INITIALIZE (pvm_array, finalization_data, arr);

  PVM_MAPINFO_MAPPED_P (arr->mapinfo) = 0;
  PVM_MAPINFO_STRICT_P (arr->mapinfo) = 1;
  PVM_MAPINFO_IOS (arr->mapinfo) = PVM_NULL;
  PVM_MAPINFO_OFFSET (arr->mapinfo) = arr_mapinfo_off;

  PVM_MAPINFO_MAPPED_P (arr->mapinfo_back) = 0;
  PVM_MAPINFO_STRICT_P (arr->mapinfo_back) = 1;
  PVM_MAPINFO_IOS (arr->mapinfo_back) = PVM_NULL;
  PVM_MAPINFO_OFFSET (arr->mapinfo_back) = PVM_NULL;

  arr->elems_bound = PVM_NULL;
  arr->size_bound = PVM_NULL;
  arr->mapper = PVM_NULL;
  arr->writer = PVM_NULL;
  arr->nelem = arr_nelem;
  arr->nallocated = num_allocated;
  arr->type = type;

  arr->elems = malloc (nbytes);
  assert (arr->elems); // FIXME Check malloc result.
  for (i = 0; i < num_allocated; ++i)
    {
      arr->elems[i].offset = PVM_NULL;
      arr->elems[i].value = PVM_NULL;
    }

  return PVM_BOX (arr);
}

int
pvm_array_insert (pvm_val arr, pvm_val idx, pvm_val val)
{
  size_t index = PVM_VAL_ULONG (idx);
  size_t nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (arr));
  size_t nallocated = PVM_VAL_ARR_NALLOCATED (arr);
  size_t nelem_to_add = index - nelem + 1;
  size_t nelem_to_allocate = index >= nallocated ? index - nallocated + 1 : 0;
  size_t val_size = pvm_sizeof (val);
  size_t array_boffset = PVM_VAL_ULONG (PVM_VAL_ARR_OFFSET (arr));
  size_t elem_boffset
      = (nelem > 0 ? (PVM_VAL_ULONG (PVM_VAL_ARR_ELEM_OFFSET (arr, nelem - 1))
                      + pvm_sizeof (PVM_VAL_ARR_ELEM_VALUE (arr, nelem - 1)))
                   : array_boffset);
  size_t i;

  /* First of all, make sure that the given index doesn't correspond
     to an existing element.  If that is the case, return 0 now.  */
  if (index < nelem)
    return 0;

  /* We have a hard-limit in the number of elements to allocate, in
     order to avoid malicious code or harmful bugs.  */
  if (nelem_to_allocate > 1024)
    return 0;

  /* Make sure there is enough room in the array for the new elements.
     Otherwise, make space for the new elements, plus a buffer of 16
     elements more.  */
  /* No GC-related allocation should happen here.  */
  if ((nallocated - nelem) < nelem_to_add)
    {
      PVM_VAL_ARR_NALLOCATED (arr) += nelem_to_add + 16;
      PVM_VAL_ARR_ELEMS (arr) = realloc (PVM_VAL_ARR_ELEMS (arr),
                                         PVM_VAL_ARR_NALLOCATED (arr)
                                             * sizeof (struct pvm_array_elem));
      assert (PVM_VAL_ARR_ELEMS (arr)); // FIXME Handle error.

      for (i = index + 1; i < PVM_VAL_ARR_NALLOCATED (arr); ++i)
        {
          PVM_VAL_ARR_ELEM_VALUE (arr, i) = PVM_NULL;
          PVM_VAL_ARR_ELEM_OFFSET (arr, i) = PVM_NULL;
        }
    }

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &arr);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &val);

    /* Initialize the new elements with the given value, also setting
       their bit-offset.  */
    for (i = nelem; i <= index; ++i)
      {
        PVM_VAL_ARR_ELEM_VALUE (arr, i) = val;
        PVM_VAL_ARR_ELEM_OFFSET (arr, i) = pvm_make_ulong (elem_boffset, 64);
        elem_boffset += val_size;
      }

    /* Finally, adjust the number of elements.  */
    PVM_VAL_ARR_NELEM (arr) = pvm_make_ulong (
        PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (arr)) + nelem_to_add, 64);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  return 1;
}

int
pvm_array_set (pvm_val arr, pvm_val idx, pvm_val val)
{
  size_t index = PVM_VAL_ULONG (idx);
  size_t nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (arr));
  size_t i;
  ssize_t size_diff;

  /* Make sure that the given index is within bounds.  */
  if (index >= nelem)
    return 0;

  /* Calculate the difference of size introduced by the new
     elemeent.  */
  size_diff = ((ssize_t)pvm_sizeof (val)
               - (ssize_t)pvm_sizeof (PVM_VAL_ARR_ELEM_VALUE (arr, index)));

  /* Update the element with the given value.  */
  PVM_VAL_ARR_ELEM_VALUE (arr, index) = val;

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &arr);

    /* Recalculate the bit-offset of all the elements following the
       element just updated.  */
    for (i = index + 1; i < nelem; ++i)
      {
        size_t elem_boffset
            = (ssize_t)PVM_VAL_ULONG (PVM_VAL_ARR_ELEM_OFFSET (arr, i))
              + size_diff;
        PVM_VAL_ARR_ELEM_OFFSET (arr, i) = pvm_make_ulong (elem_boffset, 64);
      }
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  return 1;
}

int
pvm_array_rem (pvm_val arr, pvm_val idx)
{
  size_t index = PVM_VAL_ULONG (idx);
  size_t nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (arr));
  size_t i;

  /* Make sure the given index is within bounds.  */
  if (index >= nelem)
    return 0;

  for (i = index; i < (nelem - 1); i++)
    PVM_VAL_ARR_ELEM (arr, i) = PVM_VAL_ARR_ELEM (arr, i + 1);

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &arr);

    PVM_VAL_ARR_NELEM (arr) = pvm_make_ulong (nelem - 1, 64);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  return 1;
}

/* STRUCT */

#define PVM_GC_SIZEOF_STRUCT JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_struct)

static bool
pvm_gc_is_struct_p (pvm_val v)
{
  return PVM_IS_SCT (v);
}

static size_t
pvm_gc_sizeof_struct (pvm_val v)
{
  return PVM_GC_SIZEOF_STRUCT;
}

static bool
pvm_gc_is_struct_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_SCT;
}

static size_t
pvm_gc_copy_struct (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                    void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_STRUCT);
  JITTER_GC_FINALIZABLE_COPY (pvm_struct, finalization_data, heaplet, from,
                              to);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_STRUCT;
}

// FIXME struct: move elems and methods to the elements of the?
// headered object (to be like fast_vector) to avoid finalizability.

static size_t
pvm_gc_update_fields_struct (struct jitter_gc_heaplet *heaplet, void *obj)
{
  pvm_struct sct = obj;
  size_t num_fields;
  size_t num_methods;

  jitter_gc_handle_word (heaplet, &sct->nfields);
  jitter_gc_handle_word (heaplet, &sct->nmethods);
  jitter_gc_handle_word (heaplet, &sct->mapinfo.ios);
  jitter_gc_handle_word (heaplet, &sct->mapinfo.offset);
  jitter_gc_handle_word (heaplet, &sct->mapinfo_back.ios);
  jitter_gc_handle_word (heaplet, &sct->mapinfo_back.offset);
  jitter_gc_handle_word (heaplet, &sct->mapper);
  jitter_gc_handle_word (heaplet, &sct->writer);
  jitter_gc_handle_word (heaplet, &sct->type);

  assert (PVM_IS_ULONG (sct->nfields));
  num_fields = PVM_VAL_ULONG (sct->nfields);
  for (size_t i = 0; i < num_fields; ++i)
    {
      jitter_gc_handle_word (heaplet, &sct->fields[i].offset);
      jitter_gc_handle_word (heaplet, &sct->fields[i].offset_back);
      jitter_gc_handle_word (heaplet, &sct->fields[i].name);
      jitter_gc_handle_word (heaplet, &sct->fields[i].value);
      jitter_gc_handle_word (heaplet, &sct->fields[i].modified);
      jitter_gc_handle_word (heaplet, &sct->fields[i].modified_back);
    }

  assert (PVM_IS_ULONG (sct->nmethods));
  num_methods = PVM_VAL_ULONG (sct->nmethods);
  for (size_t i = 0; i < num_methods; ++i)
    {
      jitter_gc_handle_word (heaplet, &sct->methods[i].name);
      jitter_gc_handle_word (heaplet, &sct->methods[i].value);
    }
  return PVM_GC_SIZEOF_STRUCT;
}

static void
pvm_gc_finalize_struct (struct jitter_gc_heap *heap __attribute__ ((unused)),
                        struct jitter_gc_heaplet *heaplet
                        __attribute__ ((unused)),
                        void *obj)
{
  pvm_struct sct = obj;

  free (PVM_VAL_SCT_FIELDS (sct));
  free (PVM_VAL_SCT_METHODS (sct));
  PVM_VAL_SCT_FIELDS (sct) = NULL;
  PVM_VAL_SCT_METHODS (sct) = NULL;
  PVM_VAL_SCT_NFIELDS (sct) = PVM_NULL;
  PVM_VAL_SCT_NMETHODS (sct) = PVM_NULL;
}

pvm_val
pvm_make_struct (pvm_val nfields, pvm_val nmethods, pvm_val type)
{
  assert (PVM_IS_ULONG (nfields));
  assert (PVM_IS_ULONG (nmethods));

  size_t i;
  size_t nfieldbytes
      = sizeof (struct pvm_struct_field) * PVM_VAL_ULONG (nfields);
  size_t nmethodbytes
      = sizeof (struct pvm_struct_method) * PVM_VAL_ULONG (nmethods);
  pvm_struct sct;
  pvm_val sct_mapinfo_off;

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &nfields);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &nmethods);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &type);

    sct_mapinfo_off = pvm_make_ulong (0, 64);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &sct_mapinfo_off);

    {
      void *p;

      _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                           GC_RUNTIME_LIMIT (gc_heaplet), p,
                           PVM_GC_SIZEOF_STRUCT);
      sct = p;
    }
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  sct->type_code = PVM_VAL_TAG_SCT;
  JITTER_GC_FINALIZABLE_INITIALIZE (pvm_struct, finalization_data, sct);

  PVM_MAPINFO_MAPPED_P (sct->mapinfo) = 0;
  PVM_MAPINFO_STRICT_P (sct->mapinfo) = 1;
  PVM_MAPINFO_IOS (sct->mapinfo) = PVM_NULL;
  PVM_MAPINFO_OFFSET (sct->mapinfo) = sct_mapinfo_off;

  PVM_MAPINFO_MAPPED_P (sct->mapinfo_back) = 0;
  PVM_MAPINFO_STRICT_P (sct->mapinfo_back) = 1;
  PVM_MAPINFO_IOS (sct->mapinfo_back) = PVM_NULL;
  PVM_MAPINFO_OFFSET (sct->mapinfo_back) = PVM_NULL;

  sct->mapper = PVM_NULL;
  sct->writer = PVM_NULL;
  sct->type = type;

  sct->nfields = nfields;
  sct->fields = malloc (nfieldbytes);
  assert (sct->fields); // FIXME Check allocation error.

  sct->nmethods = nmethods;
  sct->methods = malloc (nmethodbytes);
  assert (sct->methods); // FIXME Check for allocation error.

  for (i = 0; i < PVM_VAL_ULONG (sct->nfields); ++i)
    {
      sct->fields[i].offset = PVM_NULL;
      sct->fields[i].name = PVM_NULL;
      sct->fields[i].value = PVM_NULL;
      sct->fields[i].modified = PVM_MAKE_INT (0, 32);
      sct->fields[i].modified_back = PVM_NULL;
      sct->fields[i].offset_back = PVM_NULL;
    }

  for (i = 0; i < PVM_VAL_ULONG (sct->nmethods); ++i)
    {
      sct->methods[i].name = PVM_NULL;
      sct->methods[i].value = PVM_NULL;
    }

  return PVM_BOX (sct);
}

pvm_val
pvm_ref_struct_cstr (pvm_val sct, const char *name)
{
  size_t nfields, nmethods, i;
  struct pvm_struct_field *fields;
  struct pvm_struct_method *methods;

  assert (PVM_IS_SCT (sct));

  /* Lookup fields.  */
  assert (PVM_IS_ULONG (PVM_VAL_SCT_NFIELDS (sct)));
  nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (sct));
  fields = PVM_VAL_SCT (sct)->fields;

  for (i = 0; i < nfields; ++i)
    {
      if (!PVM_VAL_SCT_FIELD_ABSENT_P (sct, i) && fields[i].name != PVM_NULL
          && STREQ (PVM_VAL_STR (fields[i].name), name))
        return fields[i].value;
    }

  /* Lookup methods.  */
  nmethods = PVM_VAL_ULONG (PVM_VAL_SCT_NMETHODS (sct));
  methods = PVM_VAL_SCT (sct)->methods;

  for (i = 0; i < nmethods; ++i)
    {
      if (STREQ (PVM_VAL_STR (methods[i].name), name))
        return methods[i].value;
    }

  return PVM_NULL;
}

void
pvm_ref_set_struct_cstr (pvm_val sct, const char *fname, pvm_val value)
{
  size_t nfields, i;
  struct pvm_struct_field *fields;

  assert (PVM_IS_SCT (sct));

  nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (sct));
  fields = PVM_VAL_SCT (sct)->fields;

  for (i = 0; i < nfields; ++i)
    {
      if (!PVM_VAL_SCT_FIELD_ABSENT_P (sct, i) && fields[i].name != PVM_NULL
          && STREQ (PVM_VAL_STR (fields[i].name), fname))
        fields[i].value = value;
    }
}

pvm_val
pvm_ref_struct (pvm_val sct, pvm_val name)
{
  assert (PVM_IS_SCT (sct) && PVM_IS_STR (name));
  return pvm_ref_struct_cstr (sct, PVM_VAL_STR (name));
}

pvm_val
pvm_refo_struct (pvm_val sct, pvm_val name)
{
  size_t nfields, i;
  struct pvm_struct_field *fields;

  assert (PVM_IS_SCT (sct) && PVM_IS_STR (name));

  assert (PVM_IS_ULONG (PVM_VAL_SCT_NFIELDS (sct)));
  nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (sct));
  fields = PVM_VAL_SCT (sct)->fields;

  for (i = 0; i < nfields; ++i)
    {
      if (!PVM_VAL_SCT_FIELD_ABSENT_P (sct, i) && fields[i].name != PVM_NULL
          && STREQ (PVM_VAL_STR (fields[i].name), PVM_VAL_STR (name)))
        return fields[i].offset;
    }

  return PVM_NULL;
}

int
pvm_set_struct (pvm_val sct, pvm_val name, pvm_val val)
{
  size_t nfields, i;
  struct pvm_struct_field *fields;

  assert (PVM_IS_SCT (sct) && PVM_IS_STR (name));

  assert (PVM_IS_ULONG (PVM_VAL_SCT_NFIELDS (sct)));
  nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (sct));
  fields = PVM_VAL_SCT (sct)->fields;

  for (i = 0; i < nfields; ++i)
    {
      if (fields[i].name != PVM_NULL
          && STREQ (PVM_VAL_STR (fields[i].name), PVM_VAL_STR (name)))
        {
          PVM_VAL_SCT_FIELD_VALUE (sct, i) = val;
          PVM_VAL_SCT_FIELD_MODIFIED (sct, i) = PVM_MAKE_INT (1, 32);
          return 1;
        }
    }

  return 0;
}

pvm_val
pvm_get_struct_method (pvm_val sct, const char *name)
{
  assert (PVM_IS_SCT (sct));
  assert (PVM_IS_ULONG (PVM_VAL_SCT_NMETHODS (sct)));

  size_t i, nmethods = PVM_VAL_ULONG (PVM_VAL_SCT_NMETHODS (sct));
  struct pvm_struct_method *methods = PVM_VAL_SCT (sct)->methods;

  for (i = 0; i < nmethods; ++i)
    {
      if (STREQ (PVM_VAL_STR (methods[i].name), name))
        return methods[i].value;
    }

  return PVM_NULL;
}

/* TYPE */

#define PVM_GC_SIZEOF_TYPE JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_type)

static bool
pvm_gc_is_type_p (pvm_val v)
{
  return PVM_IS_TYP (v);
}

static size_t
pvm_gc_sizeof_type (pvm_val v)
{
  return PVM_GC_SIZEOF_TYPE;
}

static bool
pvm_gc_is_type_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_TYP;
}

static size_t
pvm_gc_copy_type (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                  void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_TYPE);
  JITTER_GC_FINALIZABLE_COPY (pvm_type, finalization_data, heaplet, from, to);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_TYPE;
}

// TODO
// Make TYPE a non-finalizable headered object (like fast_vector).
// TYPE has a defined extent at construction time (e.g., the number of
// args of a closure and the type of them is known at construction time),
// so this should be easily possible.

static size_t
pvm_gc_update_fields_type (struct jitter_gc_heaplet *heaplet, void *obj)
{
  pvm_type typ = obj;

  switch (typ->code)
    {
    case PVM_TYPE_INTEGRAL:
      jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_I_SIZE (typ));
      jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_I_SIGNED_P (typ));
      break;

    case PVM_TYPE_ARRAY:
      jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_A_BOUND (typ));
      jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_A_ETYPE (typ));
      break;

    case PVM_TYPE_STRUCT:
      {
        pvm_val nfields_val;
        uint64_t nfields;

        jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_S_NAME (typ));
        jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_S_NFIELDS (typ));
        jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_S_CONSTRUCTOR (typ));

        nfields_val = PVM_VAL_TYP_S_NFIELDS (typ);
        assert (PVM_IS_ULONG (nfields_val));
        nfields = PVM_VAL_ULONG (nfields_val);
        for (uint64_t i = 0; i < nfields; ++i)
          {
            jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_S_FNAME (typ, i));
            jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_S_FTYPE (typ, i));
          }
      }
      break;

    case PVM_TYPE_OFFSET:
      jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_O_BASE_TYPE (typ));
      jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_O_UNIT (typ));
      jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_O_REF_TYPE (typ));
      break;

    case PVM_TYPE_CLOSURE:
      {
        uint64_t nargs;

        jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_C_NARGS (typ));
        jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_C_RETURN_TYPE (typ));

        nargs = PVM_VAL_ULONG (PVM_VAL_TYP_C_NARGS (typ));
        for (uint64_t i = 0; i < nargs; ++i)
          jitter_gc_handle_word (heaplet, &PVM_VAL_TYP_C_ATYPE (typ, i));
      }
      break;

    case PVM_TYPE_STRING:
    case PVM_TYPE_VOID:
      /* Nothing to do.  */
      break;
    }
  return PVM_GC_SIZEOF_TYPE;
}

static void
pvm_gc_finalize_type (struct jitter_gc_heap *heap __attribute__ ((unused)),
                      struct jitter_gc_heaplet *heaplet
                      __attribute__ ((unused)),
                      void *obj)
{
  pvm_type typ = obj;

  switch (typ->code)
    {
    case PVM_TYPE_INTEGRAL:
    case PVM_TYPE_STRING:
    case PVM_TYPE_ARRAY:
    case PVM_TYPE_OFFSET:
    case PVM_TYPE_VOID:
      /* No finalization.  */
      break;

    case PVM_TYPE_STRUCT:
      free (PVM_VAL_TYP_S_FNAMES (typ));
      free (PVM_VAL_TYP_S_FTYPES (typ));
      break;

    case PVM_TYPE_CLOSURE:
      free (PVM_VAL_TYP_C_ATYPES (typ));
      break;
    }
}

static pvm_val
pvm_make_type (enum pvm_type_code code)
{
  pvm_type typ;

  {
    void *p;

    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p, PVM_GC_SIZEOF_TYPE);
    typ = p;
  }
  typ->type_code = PVM_VAL_TAG_TYP;
  JITTER_GC_FINALIZABLE_INITIALIZE (pvm_type, finalization_data, typ);

  typ->code = code;
  /* The rest will be filled by the caller.  */

  return PVM_BOX (typ);
}

/* Only will be called in pvm_val_initialize to fill COMMON_INT_TYPES array.
 */

static inline pvm_val
pvm_make_integral_type_1 (pvm_val size, pvm_val signed_p)
{
  pvm_val itype;

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &size);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &signed_p);

    itype = pvm_make_type (PVM_TYPE_INTEGRAL);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  PVM_VAL_TYP_I_SIZE (itype) = size;
  PVM_VAL_TYP_I_SIGNED_P (itype) = signed_p;
  return itype;
}

pvm_val
pvm_make_integral_type (pvm_val size, pvm_val signed_p)
{
  uint64_t bits = PVM_VAL_ULONG (size);
  int32_t sign = PVM_VAL_INT (signed_p);

  return common_int_types[bits][sign];
}

pvm_val
pvm_make_string_type (void)
{
  return string_type;
}

pvm_val
pvm_make_void_type (void)
{
  return void_type;
}

pvm_val
pvm_make_offset_type (pvm_val base_type, pvm_val unit, pvm_val ref_type)
{
  pvm_val otype;

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &base_type);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &unit);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &ref_type);

    otype = pvm_make_type (PVM_TYPE_OFFSET);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  PVM_VAL_TYP_O_BASE_TYPE (otype) = base_type;
  PVM_VAL_TYP_O_UNIT (otype) = unit;
  PVM_VAL_TYP_O_REF_TYPE (otype) = ref_type;
  return otype;
}

pvm_val
pvm_make_array_type (pvm_val type, pvm_val bounder)
{
  pvm_val atype;

  assert (PVM_IS_CLS (bounder));

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &type);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &bounder);

    atype = pvm_make_type (PVM_TYPE_ARRAY);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  PVM_VAL_TYP_A_ETYPE (atype) = type;
  PVM_VAL_TYP_A_BOUND (atype) = bounder;
  return atype;
}

pvm_val
pvm_make_struct_type_unsafe (pvm_val nfields, pvm_val **fnames_ptr,
                             pvm_val **ftypes_ptr)
{
  size_t nfields_val = PVM_VAL_ULONG (nfields);
  size_t nbytes = sizeof (pvm_val) * nfields_val;
  pvm_val *fnames;
  pvm_val *ftypes;
  pvm_val stype;

  assert (PVM_IS_ULONG (nfields));

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &nfields);

    stype = pvm_make_type (PVM_TYPE_STRUCT);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  fnames = malloc (nbytes);
  assert (fnames); // FIXME Handle error.
  ftypes = malloc (nbytes);
  assert (ftypes); // FIXME Handle error.

  PVM_VAL_TYP_S_NFIELDS (stype) = nfields;
  PVM_VAL_TYP_S_CONSTRUCTOR (stype) = PVM_NULL;
  PVM_VAL_TYP_S_FNAMES (stype) = fnames;
  PVM_VAL_TYP_S_FTYPES (stype) = ftypes;
  PVM_VAL_TYP_S_NAME (stype) = PVM_NULL;

  if (fnames_ptr)
    *fnames_ptr = fnames;
  if (ftypes_ptr)
    *ftypes_ptr = ftypes;

  return stype;
}

pvm_val
pvm_make_struct_type (pvm_val nfields, pvm_val **fnames, pvm_val **ftypes)
{
  size_t nfields_val = PVM_VAL_ULONG (nfields);
  pvm_val stype;

  stype = pvm_make_struct_type_unsafe (nfields, fnames, ftypes);
  if (fnames)
    for (size_t i = 0; i < nfields_val; ++i)
      (*fnames)[i] = PVM_NULL;
  if (ftypes)
    for (size_t i = 0; i < nfields_val; ++i)
      (*ftypes)[i] = PVM_NULL;

  return stype;
}

pvm_val
pvm_make_closure_type_unsafe (pvm_val rtype, pvm_val nargs,
                              pvm_val **atypes_ptr)
{
  uint64_t nbytes = sizeof (pvm_val) * PVM_VAL_ULONG (nargs);
  pvm_val *atypes;
  pvm_val ctype;

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &rtype);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &nargs);

    ctype = pvm_make_type (PVM_TYPE_CLOSURE);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  atypes = malloc (nbytes);
  assert (atypes); // FIXME Handle error.

  PVM_VAL_TYP_C_RETURN_TYPE (ctype) = rtype;
  PVM_VAL_TYP_C_NARGS (ctype) = nargs;
  PVM_VAL_TYP_C_ATYPES (ctype) = atypes;

  if (atypes_ptr)
    *atypes_ptr = atypes;

  return ctype;
}

pvm_val
pvm_make_closure_type (pvm_val rtype, pvm_val nargs, pvm_val **atypes)
{
  uint64_t nargs_val = PVM_VAL_ULONG (nargs);
  pvm_val ctype;

  ctype = pvm_make_closure_type_unsafe (rtype, nargs, atypes);
  for (size_t i = 0; i < nargs_val; ++i)
    (*atypes)[i] = PVM_NULL;

  return ctype;
}

/* CLOSURE */

#define PVM_GC_SIZEOF_CLOSURE JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_cls)

static bool
pvm_gc_is_closure_p (pvm_val v)
{
  return PVM_IS_CLS (v);
}

static size_t
pvm_gc_sizeof_closure (pvm_val v)
{
  return PVM_GC_SIZEOF_CLOSURE;
}

static bool
pvm_gc_is_closure_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_CLS;
}

static size_t
pvm_gc_copy_closure (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                     void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_CLOSURE);
  JITTER_GC_FINALIZABLE_COPY (pvm_cls, finalization_data, heaplet, from, to);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_CLOSURE;
}

static size_t
pvm_gc_update_fields_closure (struct jitter_gc_heaplet *heaplet, void *obj)
{
  pvm_cls cls = (pvm_cls)(uintptr_t)obj;

  jitter_gc_handle_word (heaplet, &PVM_VAL_CLS_NAME (cls));
  jitter_gc_handle_word (heaplet, &PVM_VAL_CLS_ENV (cls));
  jitter_gc_handle_word (heaplet, &PVM_VAL_CLS_PROGRAM (cls));
  return PVM_GC_SIZEOF_CLOSURE;
}

static void
pvm_gc_finalize_closure (struct jitter_gc_heap *heap __attribute__ ((unused)),
                         struct jitter_gc_heaplet *heaplet
                         __attribute__ ((unused)),
                         void *obj)
{
  pvm_cls cls = obj;

  pvm_destroy_program (PVM_VAL_CLS_PROGRAM (cls));

  // FIXME FIXME FIXME
  PVM_VAL_CLS_NAME (cls) = PVM_NULL;
  PVM_VAL_CLS_ENV (cls) = PVM_NULL;
  PVM_VAL_CLS_PROGRAM (cls) = PVM_NULL;
}

pvm_val
pvm_make_cls (pvm_val program, pvm_val name)
{
  pvm_cls cls;

  assert (PVM_IS_PRG (program));

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    void *p;

    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &program);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &name);

    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p,
                         PVM_GC_SIZEOF_CLOSURE);
    cls = p;
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  cls->type_code = PVM_VAL_TAG_CLS;
  JITTER_GC_FINALIZABLE_INITIALIZE (pvm_cls, finalization_data, cls);

  // FIXME FIXME FIXME Use macros.
  cls->name = name;
  cls->program = program;
  cls->entry_point = pvm_program_beginning (program);
  cls->env = PVM_NULL; /* This should be set by a PEC instruction
                          before using the closure.  */
  return PVM_BOX (cls);
}

/* OFFSET */

#define PVM_GC_SIZEOF_OFFSET JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_off)

static bool
pvm_gc_is_offset_p (pvm_val v)
{
  return PVM_IS_OFF (v);
}

static size_t
pvm_gc_sizeof_offset (pvm_val v)
{
  return PVM_GC_SIZEOF_OFFSET;
}

static bool
pvm_gc_is_offset_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_OFF;
}

static size_t
pvm_gc_copy_offset (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                    void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_OFFSET);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_OFFSET;
}

static size_t
pvm_gc_update_fields_offset (struct jitter_gc_heaplet *heaplet, void *obj)
{
  pvm_off off = obj;

  jitter_gc_handle_word (heaplet, &PVM_VAL_OFF_TYPE (off));
  jitter_gc_handle_word (heaplet, &PVM_VAL_OFF_MAGNITUDE (off));
  return PVM_GC_SIZEOF_OFFSET;
}

pvm_val
pvm_make_offset (pvm_val magnitude, pvm_val type)
{
  pvm_off off;

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    void *p;

    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &magnitude);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &type);

    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p,
                         PVM_GC_SIZEOF_OFFSET);
    off = p;
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  off->type_code = PVM_VAL_TAG_OFF;
  off->type = type;
  off->magnitude = magnitude;
  return PVM_BOX (off);
}

/* IARRAY */

#define PVM_GC_SIZEOF_IARRAY JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_iarray)

static bool
pvm_gc_is_iarray_p (pvm_val v)
{
  return PVM_IS_IAR (v);
}

static size_t
pvm_gc_sizeof_iarray (pvm_val v)
{
  return PVM_GC_SIZEOF_IARRAY;
}

static bool
pvm_gc_is_iarray_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_IAR;
}

static size_t
pvm_gc_copy_iarray (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                    void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_IARRAY);
  JITTER_GC_FINALIZABLE_COPY (pvm_iarray, finalization_data, heaplet, from,
                              to);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_IARRAY;
}

static size_t
pvm_gc_update_fields_iarray (struct jitter_gc_heaplet *heaplet, void *obj)
{
  pvm_iarray iar = obj;

  for (size_t i = 0; i < iar->nelem; ++i)
    jitter_gc_handle_word (heaplet, &iar->elems[i]);
  return PVM_GC_SIZEOF_IARRAY;
}

static void
pvm_gc_finalize_iarray (struct jitter_gc_heap *heap __attribute__ ((unused)),
                        struct jitter_gc_heaplet *heaplet
                        __attribute__ ((unused)),
                        void *obj)
{
  pvm_iarray iar = obj;

  free (iar->elems);
  iar->elems = NULL;
}

pvm_val
pvm_make_iarray (int hint)
{
  pvm_iarray iar;

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    void *p;

    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p,
                         PVM_GC_SIZEOF_IARRAY);
    iar = p;
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  iar->type_code = PVM_VAL_TAG_IAR;
  JITTER_GC_FINALIZABLE_INITIALIZE (pvm_iarray, finalization_data, iar);

  iar->nelem = 0;
  iar->nallocated = hint == 0 ? 16 : hint;
  iar->elems = malloc (iar->nallocated * sizeof (pvm_val));
  assert (iar->elems); // FIXME Handle error.

  return PVM_BOX (iar);
}

size_t
pvm_iarray_push (pvm_val iar, pvm_val val)
{
  assert (PVM_IS_IAR (iar));

  if (PVM_VAL_IAR_NELEM (iar) == PVM_VAL_IAR_NALLOCATED (iar))
    {
      PVM_VAL_IAR_NALLOCATED (iar) += 32;
      PVM_VAL_IAR_ELEMS (iar)
          = realloc (PVM_VAL_IAR_ELEMS (iar),
                     PVM_VAL_IAR_NALLOCATED (iar) * sizeof (pvm_val));
      assert (PVM_VAL_IAR_ELEMS (iar) != NULL); // FIXME Handle error.
    }
  PVM_VAL_IAR_ELEM (iar, PVM_VAL_IAR_NELEM (iar)) = val;
  return PVM_VAL_IAR_NELEM (iar)++;
}

pvm_val
pvm_iarray_pop (pvm_val iar)
{
  assert (PVM_IS_IAR (iar));

  if (PVM_VAL_IAR_NELEM (iar))
    {
      PVM_VAL_IAR_NELEM (iar) -= 1;
      return PVM_VAL_IAR_ELEM (iar, PVM_VAL_IAR_NELEM (iar));
    }
  return PVM_NULL;
}

/* ENV */

#define PVM_GC_SIZEOF_ENV JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_env_)

static bool
pvm_gc_is_env_p (pvm_val v)
{
  return PVM_IS_ENV (v);
}

static size_t
pvm_gc_sizeof_env (pvm_val v)
{
  return PVM_GC_SIZEOF_ENV;
}

static bool
pvm_gc_is_env_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_ENV;
}

static size_t
pvm_gc_copy_env (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                 void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_ENV);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_ENV;
}

static size_t
pvm_gc_update_fields_env (struct jitter_gc_heaplet *heaplet, void *obj)
{
  pvm_env_ env = obj;

  jitter_gc_handle_word (heaplet, &env->vars);
  jitter_gc_handle_word (heaplet, &env->env_up);
  return PVM_GC_SIZEOF_ENV;
}

pvm_val
pvm_make_env (int hint)
{
  pvm_env_ env;
  pvm_val vars;

  vars = pvm_make_iarray (hint);

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    void *p;

    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &vars);
    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p, PVM_GC_SIZEOF_ENV);
    env = p;
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  env->type_code = PVM_VAL_TAG_ENV;
  env->vars = vars;
  env->env_up = PVM_NULL;

  return PVM_BOX (env);
}

pvm_val
pvm_env_push_frame (pvm_val env, int hint)
{
  pvm_val frame;

  assert (PVM_IS_ENV (env));

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &env);
    frame = pvm_make_env (hint);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  PVM_VAL_ENV_UP (frame) = env;
  return frame;
}

pvm_val
pvm_env_pop_frame (pvm_val env)
{
  pvm_val up;

  assert (PVM_IS_ENV (env));
  up = PVM_VAL_ENV_UP (env);
  assert (up != PVM_NULL);
  return up;
}

void
pvm_env_register (pvm_val env, pvm_val val)
{
  assert (PVM_IS_ENV (env));

  pvm_iarray_push (PVM_VAL_ENV_VARS (env), val);
}

/* Given an environment return the frame back frames up from the bottom
   one.  back is allowed to be zero, but not negative. */

static pvm_val
pvm_env_back (pvm_val env, int back)
{
  pvm_val frame = env;
  int i;

  for (i = 0; i < back; i++)
    frame = PVM_VAL_ENV_UP (frame);
  return frame;
}

pvm_val
pvm_env_lookup (pvm_val env, int back, int over)
{
  env = pvm_env_back (env, back);
  return PVM_VAL_ENV_VAR (env, over);
}

void
pvm_env_set_var (pvm_val env, int back, int over, pvm_val val)
{
  env = pvm_env_back (env, back);
  PVM_VAL_ENV_VAR (env, over) = val;
}

int
pvm_env_toplevel_p (pvm_val env)
{
  assert (PVM_IS_ENV (env));
  return PVM_VAL_ENV_UP (env) == PVM_NULL;
}

pvm_val
pvm_env_toplevel (pvm_val env)
{
  assert (PVM_IS_ENV (env));
  while (PVM_VAL_ENV_UP (env) != PVM_NULL)
    env = PVM_VAL_ENV_UP (env);
  return env;
}

/* PROGRAM */

#define PVM_GC_SIZEOF_PROGRAM                                                 \
  JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES (pvm_program_)

static bool
pvm_gc_is_program_p (pvm_val v)
{
  return PVM_IS_PRG (v);
}

static size_t
pvm_gc_sizeof_program (pvm_val v)
{
  return PVM_GC_SIZEOF_PROGRAM;
}

static bool
pvm_gc_is_program_type_code_p (uintptr_t v)
{
  return v == PVM_VAL_TAG_PRG;
}

static size_t
pvm_gc_copy_program (struct jitter_gc_heaplet *heaplet, pvm_val *new_val,
                     void *from, void *to)
{
  memcpy (to, from, PVM_GC_SIZEOF_PROGRAM);
  JITTER_GC_FINALIZABLE_COPY (pvm_program_, finalization_data, heaplet, from,
                              to);
  *new_val = PVM_BOX (to);
  return PVM_GC_SIZEOF_PROGRAM;
}

static size_t
pvm_gc_update_fields_program (struct jitter_gc_heaplet *heaplet, void *obj)
{
  pvm_program_ prg = obj;

  jitter_gc_handle_word (heaplet, &prg->insn_params);
  return PVM_GC_SIZEOF_PROGRAM;
}

static void
pvm_gc_finalize_program (struct jitter_gc_heap *heap __attribute__ ((unused)),
                         struct jitter_gc_heaplet *heaplet
                         __attribute__ ((unused)),
                         void *obj)
{
  pvm_program_ prg = obj;

  pvm_destroy_routine (prg->routine);
  free (prg->labels);
  prg->labels = NULL;
}

pvm_val
pvm_make_program (void)
{
  pvm_program_ prg;
  pvm_val insn_params;

  insn_params = pvm_make_iarray (64);

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    void *p;

    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &insn_params);
    _JITTER_GC_ALLOCATE (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                         GC_RUNTIME_LIMIT (gc_heaplet), p,
                         PVM_GC_SIZEOF_PROGRAM);
    prg = p;
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  JITTER_GC_FINALIZABLE_INITIALIZE (pvm_program_, finalization_data, prg);

  prg->type_code = PVM_VAL_TAG_PRG;
  prg->insn_params = insn_params;
  prg->routine = pvm_make_routine ();
  prg->nlabels_max = 128 * 1024;
  prg->nlabels = 0;
  prg->labels = malloc (prg->nlabels_max * sizeof (pvm_val));
  assert (prg->labels); // FIXME Handle error.

  return PVM_BOX (prg);
}

/* Keep track of PVM values provided to the instructions as parameter(s).  */

static size_t
pvm_program_collect_val (pvm_val program, pvm_val val)
{
  pvm_val insn_params;
  size_t nelem;

  assert (PVM_IS_PRG (program));

  insn_params = PVM_VAL_PRG_INSN_PARAMS (program);
  nelem = PVM_VAL_IAR_NELEM (insn_params);
  for (size_t i = 0; i < nelem; ++i)
    if (PVM_VAL_IAR_ELEM (insn_params, i) == val)
      return i;
  return pvm_iarray_push (PVM_VAL_PRG_INSN_PARAMS (program), val);
}

int
pvm_program_append_instruction (pvm_val program, const char *insn_name)
{
  assert (PVM_IS_PRG (program));

  /* For push instructions, pvm_program_append_push_instruction shall
     be used instead.  That function is to remove when the causing
     limitation in jitter gets fixed.  */
  assert (STRNEQ (insn_name, "push"));

  /* XXX Jitter should provide error codes so we can return PVM_EINVAL
     and PVM_EINSN properly.  */
  pvm_routine_append_instruction_name (PVM_VAL_PRG_ROUTINE (program),
                                       insn_name);

  return PVM_OK;
}

int
pvm_program_append_push_instruction (pvm_val program, pvm_val val)
{
  pvm_routine routine;

  assert (PVM_IS_PRG (program));

  routine = PVM_VAL_PRG_ROUTINE (program);
  PVM_ROUTINE_APPEND_INSTRUCTION (routine, push);
  pvm_routine_append_unsigned_literal_parameter (
      routine, pvm_program_collect_val (program, val));

  return PVM_OK;
}

int
pvm_program_append_val_parameter (pvm_val program, pvm_val val)
{
  assert (PVM_IS_PRG (program));

  pvm_routine_append_unsigned_literal_parameter (
      PVM_VAL_PRG_ROUTINE (program), pvm_program_collect_val (program, val));

  return PVM_OK;
}

int
pvm_program_append_unsigned_parameter (pvm_val program, unsigned int n)
{
  assert (PVM_IS_PRG (program));
  pvm_routine_append_unsigned_literal_parameter (PVM_VAL_PRG_ROUTINE (program),
                                                 (jitter_uint)n);

  return PVM_OK;
}

int
pvm_program_append_register_parameter (pvm_val program, pvm_register reg)
{
  assert (PVM_IS_PRG (program));
  /* XXX Jitter should return an error code here so we can return
     PVM_EINVAL whenever appropriate.  */
  PVM_ROUTINE_APPEND_REGISTER_PARAMETER (PVM_VAL_PRG_ROUTINE (program), r,
                                         reg);

  return PVM_OK;
}

int
pvm_program_append_label_parameter (pvm_val program, pvm_program_label label)
{
  assert (PVM_IS_PRG (program));
  /* XXX Jitter should return an error code here so we can return
     PVM_EINVAL whenever appropriate.  */
  pvm_routine_append_label_parameter (PVM_VAL_PRG_ROUTINE (program),
                                      PVM_VAL_PRG_LABEL (program, label));

  return PVM_OK;
}

pvm_program_program_point
pvm_program_beginning (pvm_val program)
{
  assert (PVM_IS_PRG (program));
  return (pvm_program_program_point)PVM_ROUTINE_BEGINNING (
      PVM_VAL_PRG_ROUTINE (program));
}

int
pvm_program_make_executable (pvm_val program)
{
  assert (PVM_IS_PRG (program));
  /* XXX Jitter should return an error code here.  */
  jitter_routine_make_executable_if_needed (PVM_VAL_PRG_ROUTINE (program));

  return PVM_OK;
}

void
pvm_destroy_program (pvm_val program)
{
  assert (PVM_IS_PRG (program));
  pvm_destroy_routine (PVM_VAL_PRG_ROUTINE (program));
  PVM_VAL_PRG_ROUTINE (program) = NULL;
}

pvm_routine
pvm_program_routine (pvm_val program)
{
  assert (PVM_IS_PRG (program));
  return PVM_VAL_PRG_ROUTINE (program);
}

pvm_program_label
pvm_program_fresh_label (pvm_val program)
{
  assert (PVM_IS_PRG (program));
  if (PVM_VAL_PRG_NLABELS (program) == PVM_VAL_PRG_NLABELS_MAX (program))
    {
      void *p;

      PVM_VAL_PRG_NLABELS_MAX (program) += 32;
      p = realloc (PVM_VAL_PRG_LABELS (program),
                   PVM_VAL_PRG_NLABELS_MAX (program));
      assert (p); // FIXME Handle error.
      PVM_VAL_PRG_LABELS (program) = p;
    }

  PVM_VAL_PRG_LABEL (program, PVM_VAL_PRG_NLABELS (program))
      = jitter_fresh_label (PVM_VAL_PRG_ROUTINE (program));
  return PVM_VAL_PRG_NLABELS (program)++;
}

int
pvm_program_append_label (pvm_val program, pvm_program_label label)
{
  assert (PVM_IS_PRG (program));
  if (label >= PVM_VAL_PRG_NLABELS (program))
    return PVM_EINVAL;

  pvm_routine_append_label (PVM_VAL_PRG_ROUTINE (program),
                            PVM_VAL_PRG_LABEL (program, label));
  return PVM_OK;
}

/* --- */

int
pvm_val_equal_p (pvm_val val1, pvm_val val2)
{
  if (val1 == PVM_NULL && val2 == PVM_NULL)
    return 1;
  else if (PVM_IS_INT (val1) && PVM_IS_INT (val2))
    return (PVM_VAL_INT_SIZE (val1) == PVM_VAL_INT_SIZE (val2))
           && (PVM_VAL_INT (val1) == PVM_VAL_INT (val2));
  else if (PVM_IS_UINT (val1) && PVM_IS_UINT (val2))
    return (PVM_VAL_UINT_SIZE (val1) == PVM_VAL_UINT_SIZE (val2))
           && (PVM_VAL_UINT (val1) == PVM_VAL_UINT (val2));
  else if (PVM_IS_LONG (val1) && PVM_IS_LONG (val2))
    return (PVM_VAL_LONG_SIZE (val1) && PVM_VAL_LONG_SIZE (val2))
           && (PVM_VAL_LONG (val1) == PVM_VAL_LONG (val2));
  else if (PVM_IS_ULONG (val1) && PVM_IS_ULONG (val2))
    return (PVM_VAL_ULONG_SIZE (val1) == PVM_VAL_ULONG_SIZE (val2))
           && (PVM_VAL_ULONG (val1) == PVM_VAL_ULONG (val2));
  else if (PVM_IS_STR (val1) && PVM_IS_STR (val2))
    return STREQ (PVM_VAL_STR (val1), PVM_VAL_STR (val2));
  else if (PVM_IS_OFF (val1) && PVM_IS_OFF (val2))
    {
      pvm_val val1_type = PVM_VAL_OFF_TYPE (val1);
      pvm_val val2_type = PVM_VAL_OFF_TYPE (val2);
      int pvm_off_mag_equal, pvm_off_unit_equal;

      pvm_off_mag_equal = pvm_val_equal_p (PVM_VAL_OFF_MAGNITUDE (val1),
                                           PVM_VAL_OFF_MAGNITUDE (val2));
      pvm_off_unit_equal = pvm_val_equal_p (PVM_VAL_TYP_O_UNIT (val1_type),
                                            PVM_VAL_TYP_O_UNIT (val2_type));

      return pvm_off_mag_equal && pvm_off_unit_equal;
    }
  else if (PVM_IS_SCT (val1) && PVM_IS_SCT (val2))
    {
      size_t pvm_sct1_nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (val1));
      size_t pvm_sct2_nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (val2));
      size_t pvm_sct1_nmethods = PVM_VAL_ULONG (PVM_VAL_SCT_NMETHODS (val1));
      size_t pvm_sct2_nmethods = PVM_VAL_ULONG (PVM_VAL_SCT_NMETHODS (val2));

      if ((pvm_sct1_nfields != pvm_sct2_nfields)
          || (pvm_sct1_nmethods != pvm_sct2_nmethods))
        return 0;

      if (!pvm_val_equal_p (PVM_VAL_SCT_IOS (val1), PVM_VAL_SCT_IOS (val2)))
        return 0;

      if (!pvm_val_equal_p (PVM_VAL_SCT_TYPE (val1), PVM_VAL_SCT_TYPE (val2)))
        return 0;

      if (!pvm_val_equal_p (PVM_VAL_SCT_OFFSET (val1),
                            PVM_VAL_SCT_OFFSET (val2)))
        return 0;

      for (size_t i = 0; i < pvm_sct1_nfields; i++)
        {
          if (PVM_VAL_SCT_FIELD_ABSENT_P (val1, i)
              != PVM_VAL_SCT_FIELD_ABSENT_P (val2, i))
            return 0;

          if (!PVM_VAL_SCT_FIELD_ABSENT_P (val1, i))
            {
              if (!pvm_val_equal_p (PVM_VAL_SCT_FIELD_NAME (val1, i),
                                    PVM_VAL_SCT_FIELD_NAME (val2, i)))
                return 0;

              if (!pvm_val_equal_p (PVM_VAL_SCT_FIELD_VALUE (val1, i),
                                    PVM_VAL_SCT_FIELD_VALUE (val2, i)))
                return 0;

              if (!pvm_val_equal_p (PVM_VAL_SCT_FIELD_OFFSET (val1, i),
                                    PVM_VAL_SCT_FIELD_OFFSET (val2, i)))
                return 0;
            }
        }

      for (size_t i = 0; i < pvm_sct1_nmethods; i++)
        {
          if (!pvm_val_equal_p (PVM_VAL_SCT_METHOD_NAME (val1, i),
                                PVM_VAL_SCT_METHOD_NAME (val2, i)))
            return 0;
        }

      return 1;
    }
  else if (PVM_IS_ARR (val1) && PVM_IS_ARR (val2))
    {
      size_t pvm_arr1_nelems = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val1));
      size_t pvm_arr2_nelems = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val2));

      if (pvm_arr1_nelems != pvm_arr2_nelems)
        return 0;

      if (!pvm_val_equal_p (PVM_VAL_ARR_TYPE (val1), PVM_VAL_ARR_TYPE (val2)))
        return 0;

      if (!pvm_val_equal_p (PVM_VAL_ARR_IOS (val1), PVM_VAL_ARR_IOS (val2)))
        return 0;

      if (!pvm_val_equal_p (PVM_VAL_ARR_OFFSET (val1),
                            PVM_VAL_ARR_OFFSET (val2)))
        return 0;

      if (!pvm_val_equal_p (PVM_VAL_ARR_ELEMS_BOUND (val1),
                            PVM_VAL_ARR_ELEMS_BOUND (val2)))
        return 0;

      if (!pvm_val_equal_p (PVM_VAL_ARR_SIZE_BOUND (val1),
                            PVM_VAL_ARR_SIZE_BOUND (val2)))
        return 0;

      for (size_t i = 0; i < pvm_arr1_nelems; i++)
        {
          if (!pvm_val_equal_p (PVM_VAL_ARR_ELEM_VALUE (val1, i),
                                PVM_VAL_ARR_ELEM_VALUE (val2, i)))
            return 0;

          if (!pvm_val_equal_p (PVM_VAL_ARR_ELEM_OFFSET (val1, i),
                                PVM_VAL_ARR_ELEM_OFFSET (val2, i)))
            return 0;
        }

      return 1;
    }
  else if (PVM_IS_TYP (val1) && PVM_IS_TYP (val2))
    return pvm_type_equal_p (val1, val2);
  else
    return 0;
}

pvm_val
pvm_elemsof (pvm_val val)
{
  if (PVM_IS_ARR (val))
    return PVM_VAL_ARR_NELEM (val);
  else if (PVM_IS_SCT (val))
    {
      size_t nfields;
      size_t i, present_fields = 0;

      nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (val));
      for (i = 0; i < nfields; ++i)
        {
          if (!PVM_VAL_SCT_FIELD_ABSENT_P (val, i))
            present_fields++;
        }

      return pvm_make_ulong (present_fields, 64);
    }
  else if (PVM_IS_STR (val))
    return pvm_make_ulong (strlen (PVM_VAL_STR (val)), 64);
  else
    return pvm_make_ulong (1, 64);
}

pvm_val
pvm_val_mapper (pvm_val val)
{
  if (PVM_IS_ARR (val))
    return PVM_VAL_ARR_MAPPER (val);
  if (PVM_IS_SCT (val))
    return PVM_VAL_SCT_MAPPER (val);

  return PVM_NULL;
}

pvm_val
pvm_val_writer (pvm_val val)
{
  if (PVM_IS_ARR (val))
    return PVM_VAL_ARR_WRITER (val);
  if (PVM_IS_SCT (val))
    return PVM_VAL_SCT_WRITER (val);

  return PVM_NULL;
}

void
pvm_val_unmap (pvm_val val)
{
  PVM_VAL_SET_MAPPED_P (val, 0);

  if (PVM_IS_ARR (val))
    {
      size_t nelem, i;

      nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val));
      for (i = 0; i < nelem; ++i)
        pvm_val_unmap (PVM_VAL_ARR_ELEM_VALUE (val, i));
    }
  else if (PVM_IS_SCT (val))
    {
      size_t nfields, i;

      nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (val));
      for (i = 0; i < nfields; ++i)
        pvm_val_unmap (PVM_VAL_SCT_FIELD_VALUE (val, i));
    }
}

void
pvm_val_reloc (pvm_val val, pvm_val ios, pvm_val boffset)
{
  uint64_t boff = PVM_VAL_ULONG (boffset);

  if (PVM_IS_ARR (val))
    {
      size_t nelem, i;
      uint64_t array_offset = PVM_VAL_ULONG (PVM_VAL_ARR_OFFSET (val));

      nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val));
      for (i = 0; i < nelem; ++i)
        {
          pvm_val elem_value = PVM_VAL_ARR_ELEM_VALUE (val, i);
          pvm_val elem_offset = PVM_VAL_ARR_ELEM_OFFSET (val, i);
          uint64_t elem_new_offset
              = boff
                + (PVM_VAL_ULONG (PVM_VAL_ARR_ELEM_OFFSET (val, i))
                   - array_offset);

          PVM_VAL_ARR_ELEM_OFFSET_BACK (val, i) = elem_offset;
          PVM_VAL_ARR_ELEM_OFFSET (val, i)
              = pvm_make_ulong (elem_new_offset, 64);

          pvm_val_reloc (elem_value, ios,
                         pvm_make_ulong (elem_new_offset, 64));
        }

      PVM_VAL_ARR_MAPINFO_BACK (val) = PVM_VAL_ARR_MAPINFO (val);

      PVM_VAL_ARR_MAPPED_P (val) = 1;
      PVM_VAL_ARR_IOS (val) = ios;
      PVM_VAL_ARR_OFFSET (val) = pvm_make_ulong (boff, 64);
    }
  else if (PVM_IS_SCT (val))
    {
      size_t nfields, i;
      uint64_t struct_offset = PVM_VAL_ULONG (PVM_VAL_SCT_OFFSET (val));

      nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (val));
      for (i = 0; i < nfields; ++i)
        {
          pvm_val field_value = PVM_VAL_SCT_FIELD_VALUE (val, i);
          pvm_val field_offset = PVM_VAL_SCT_FIELD_OFFSET (val, i);
          uint64_t field_new_offset
              = boff
                + (PVM_VAL_ULONG (PVM_VAL_SCT_FIELD_OFFSET (val, i))
                   - struct_offset);

          /* Do not relocate absent fields.  */
          if (PVM_VAL_SCT_FIELD_ABSENT_P (val, i))
            continue;

          PVM_VAL_SCT_FIELD_OFFSET_BACK (val, i) = field_offset;
          PVM_VAL_SCT_FIELD_OFFSET (val, i)
              = pvm_make_ulong (field_new_offset, 64);
          PVM_VAL_SCT_FIELD_MODIFIED_BACK (val, i)
              = PVM_VAL_SCT_FIELD_MODIFIED (val, i);
          PVM_VAL_SCT_FIELD_MODIFIED (val, i) = PVM_MAKE_INT (1, 32);

          pvm_val_reloc (field_value, ios,
                         pvm_make_ulong (field_new_offset, 64));
        }

      PVM_VAL_SCT_MAPINFO_BACK (val) = PVM_VAL_SCT_MAPINFO (val);

      PVM_VAL_SCT_MAPPED_P (val) = 1;
      PVM_VAL_SCT_IOS (val) = ios;
      PVM_VAL_SCT_OFFSET (val) = pvm_make_ulong (boff, 64);
    }
}

void
pvm_val_ureloc (pvm_val val)
{
  if (PVM_IS_ARR (val))
    {
      size_t nelem, i;

      nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val));
      for (i = 0; i < nelem; ++i)
        {
          pvm_val elem_value = PVM_VAL_ARR_ELEM_VALUE (val, i);

          PVM_VAL_ARR_ELEM_OFFSET (val, i)
              = PVM_VAL_ARR_ELEM_OFFSET_BACK (val, i);
          pvm_val_ureloc (elem_value);
        }

      PVM_VAL_ARR_MAPINFO (val) = PVM_VAL_ARR_MAPINFO_BACK (val);
    }
  else if (PVM_IS_SCT (val))
    {
      size_t nfields, i;

      nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (val));
      for (i = 0; i < nfields; ++i)
        {
          pvm_val field_value = PVM_VAL_SCT_FIELD_VALUE (val, i);

          PVM_VAL_SCT_FIELD_OFFSET (val, i)
              = PVM_VAL_SCT_FIELD_OFFSET_BACK (val, i);
          PVM_VAL_SCT_FIELD_MODIFIED (val, i)
              = PVM_VAL_SCT_FIELD_MODIFIED_BACK (val, i);

          pvm_val_ureloc (field_value);
        }

      PVM_VAL_ARR_MAPINFO (val) = PVM_VAL_ARR_MAPINFO_BACK (val);
    }
}

uint64_t
pvm_sizeof (pvm_val val)
{
  if (PVM_IS_INT (val))
    return PVM_VAL_INT_SIZE (val);
  else if (PVM_IS_UINT (val))
    return PVM_VAL_UINT_SIZE (val);
  else if (PVM_IS_LONG (val))
    return PVM_VAL_LONG_SIZE (val);
  else if (PVM_IS_ULONG (val))
    return PVM_VAL_ULONG_SIZE (val);
  else if (PVM_IS_STR (val))
    return (strlen (PVM_VAL_STR (val)) + 1) * 8;
  else if (PVM_IS_ARR (val))
    {
      size_t nelem, i;
      size_t size = 0;

      nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val));
      for (i = 0; i < nelem; ++i)
        size += pvm_sizeof (PVM_VAL_ARR_ELEM_VALUE (val, i));

      return size;
    }
  else if (PVM_IS_SCT (val))
    {
      pvm_val sct_offset = PVM_VAL_SCT_OFFSET (val);
      size_t nfields, i, size, sct_offset_bits;

      if (sct_offset == PVM_NULL)
        sct_offset_bits = 0;
      else
        sct_offset_bits = PVM_VAL_ULONG (sct_offset);

      nfields = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (val));

      size = 0;
      for (i = 0; i < nfields; ++i)
        {
          pvm_val elem_value = PVM_VAL_SCT_FIELD_VALUE (val, i);
          pvm_val elem_offset = PVM_VAL_SCT_FIELD_OFFSET (val, i);

          if (!PVM_VAL_SCT_FIELD_ABSENT_P (val, i))
            {
              uint64_t elem_size_bits = pvm_sizeof (elem_value);

              if (elem_offset == PVM_NULL)
                size += elem_size_bits;
              else
                {
                  uint64_t elem_offset_bits = PVM_VAL_ULONG (elem_offset);

#define MAX(A, B) ((A) > (B) ? (A) : (B))
                  size = MAX (size, elem_offset_bits - sct_offset_bits
                                        + elem_size_bits);
                }
            }
        }

      return size;
    }
  else if (PVM_IS_OFF (val))
    return pvm_sizeof (PVM_VAL_OFF_MAGNITUDE (val));
  else if (PVM_IS_TYP (val))
    /* By convention, type values have size zero.  */
    return 0;
  else if (PVM_IS_CLS (val))
    /* By convention, closure values have size zero.  */
    return 0;
  else if (val == PVM_NULL)
    /* By convention, PVM_NULL values have size zero.  */
    return 0;

  PK_UNREACHABLE ();
  return 0;
}

static void
print_unit_name (uint64_t unit)
{
  switch (unit)
    {
    case PVM_VAL_OFF_UNIT_BITS:
      pk_puts ("b");
      break;
    case PVM_VAL_OFF_UNIT_NIBBLES:
      pk_puts ("N");
      break;
    case PVM_VAL_OFF_UNIT_BYTES:
      pk_puts ("B");
      break;
    case PVM_VAL_OFF_UNIT_KILOBITS:
      pk_puts ("Kb");
      break;
    case PVM_VAL_OFF_UNIT_KILOBYTES:
      pk_puts ("KB");
      break;
    case PVM_VAL_OFF_UNIT_MEGABITS:
      pk_puts ("Mb");
      break;
    case PVM_VAL_OFF_UNIT_MEGABYTES:
      pk_puts ("MB");
      break;
    case PVM_VAL_OFF_UNIT_GIGABITS:
      pk_puts ("Gb");
      break;
    case PVM_VAL_OFF_UNIT_GIGABYTES:
      pk_puts ("GB");
      break;
    case PVM_VAL_OFF_UNIT_KIBIBITS:
      pk_puts ("Kib");
      break;
    case PVM_VAL_OFF_UNIT_KIBIBYTES:
      pk_puts ("KiB");
      break;
    case PVM_VAL_OFF_UNIT_MEBIBITS:
      pk_puts ("Mib");
      break;
    case PVM_VAL_OFF_UNIT_MEBIBYTES:
      pk_puts ("MiB");
      break;
    case PVM_VAL_OFF_UNIT_GIGIBITS:
      pk_puts ("Gib");
      break;
    case PVM_VAL_OFF_UNIT_GIGIBYTES:
      pk_puts ("GiB");
      break;
    default:
      /* XXX: print here the name of the base type of the
         offset.  */
      pk_printf ("%" PRIu64, unit);
    }
}

#define PVM_PRINT_VAL_1(...)                                                  \
  pvm_print_val_1 (vm, depth, mode, base, indent, acutoff, flags,             \
                   exit_exception, __VA_ARGS__)

static void
pvm_print_val_1 (pvm vm, int depth, int mode, int base, int indent,
                 int acutoff, uint32_t flags, pvm_val *exit_exception,
                 pvm_val val, int ndepth)
{
  const char *long64_fmt, *long_fmt;
  const char *ulong64_fmt, *ulong_fmt;
  const char *int32_fmt, *int16_fmt, *int8_fmt, *int4_fmt, *int_fmt;
  const char *uint32_fmt, *uint16_fmt, *uint8_fmt, *uint4_fmt, *uint_fmt;

  /* Extract configuration settings from FLAGS.  */
  int maps = flags & PVM_PRINT_F_MAPS;
  int pprint = flags & PVM_PRINT_F_PPRINT;

  /* Select the appropriate formatting templates for the given
     base.  */
  switch (base)
    {
    case 8:
      long64_fmt = "0o%" PRIo64 "L";
      long_fmt = "0o%" PRIo64 " as int<%d>";
      ulong64_fmt = "0o%" PRIo64 "UL";
      ulong_fmt = "0o%" PRIo64 " as uint<%d>";
      int32_fmt = "0o%" PRIo32;
      int16_fmt = "0o%" PRIo32 "H";
      int8_fmt = "0o%" PRIo32 "B";
      int4_fmt = "0o%" PRIo32 "N";
      int_fmt = "0o%" PRIo32 " as int<%d>";
      uint32_fmt = "0o%" PRIo32 "U";
      uint16_fmt = "0o%" PRIo32 "UH";
      uint8_fmt = "0o%" PRIo32 "UB";
      uint4_fmt = "0o%" PRIo32 "UN";
      uint_fmt = "0o%" PRIo32 " as uint<%d>";
      break;
    case 10:
      long64_fmt = "%" PRIi64 "L";
      long_fmt = "%" PRIi64 " as int<%d>";
      ulong64_fmt = "%" PRIu64 "UL";
      ulong_fmt = "%" PRIu64 " as uint<%d>";
      int32_fmt = "%" PRIi32;
      int16_fmt = "%" PRIi32 "H";
      int8_fmt = "%" PRIi32 "B";
      int4_fmt = "%" PRIi32 "N";
      int_fmt = "%" PRIi32 " as int<%d>";
      uint32_fmt = "%" PRIu32 "U";
      uint16_fmt = "%" PRIu32 "UH";
      uint8_fmt = "%" PRIu32 "UB";
      uint4_fmt = "%" PRIu32 "UN";
      uint_fmt = "%" PRIu32 " as uint<%d>";
      break;
    case 16:
      long64_fmt = "0x%" PRIx64 "L";
      long_fmt = "0x%" PRIx64 " as int<%d>";
      ulong64_fmt = "0x%" PRIx64 "UL";
      ulong_fmt = "0x%" PRIx64 " as uint<%d>";
      int32_fmt = "0x%" PRIx32;
      int16_fmt = "0x%" PRIx32 "H";
      int8_fmt = "0x%" PRIx32 "B";
      int4_fmt = "0x%" PRIx32 "N";
      int_fmt = "0x%" PRIx32 " as int<%d>";
      uint32_fmt = "0x%" PRIx32 "U";
      uint16_fmt = "0x%" PRIx32 "UH";
      uint8_fmt = "0x%" PRIx32 "UB";
      uint4_fmt = "0x%" PRIx32 "UN";
      uint_fmt = "0x%" PRIx32 " as uint<%d>";
      break;
    case 2:
      /* This base doesn't use printf's formatting strings, but its
         own printer.  */
      long64_fmt = "";
      long_fmt = "";
      ulong64_fmt = "";
      ulong_fmt = "";
      int32_fmt = "";
      int16_fmt = "";
      int8_fmt = "";
      int4_fmt = "";
      int_fmt = "";
      uint32_fmt = "";
      uint16_fmt = "";
      uint8_fmt = "";
      uint4_fmt = "";
      uint_fmt = "";
      break;
    default:
      PK_UNREACHABLE ();
      break;
    }

  /* And print out the value in the given stream..  */
  if (val == PVM_NULL)
    pk_puts ("null");
  else if (PVM_IS_LONG (val))
    {
      int size = PVM_VAL_LONG_SIZE (val);
      int64_t longval = PVM_VAL_LONG (val);
      uint64_t ulongval;

      pk_term_class ("integer");

      if (size == 64)
        ulongval = (uint64_t) longval;
      else
        ulongval = (uint64_t) longval & ((((uint64_t) 1) << size) - 1);

      if (base == 2)
        {
          pk_puts ("0b");
          PK_PRINT_BINARY (ulongval, size);
          pk_puts (PK_INTEGRAL_SUFFIX (size, /*signed_p*/ 1));
        }
      else
        {
          if (size == 64)
            pk_printf (long64_fmt, base == 10 ? longval : ulongval);
          else
            pk_printf (long_fmt, base == 10 ? longval : ulongval,
                       PVM_VAL_LONG_SIZE (val));
        }

      pk_term_end_class ("integer");
    }
  else if (PVM_IS_INT (val))
    {
      int size = PVM_VAL_INT_SIZE (val);
      int32_t intval = PVM_VAL_INT (val);
      uint32_t uintval;

      pk_term_class ("integer");

      if (size == 32)
        uintval = (uint32_t) intval;
      else
        uintval = (uint32_t) intval & ((((uint32_t) 1) << size) - 1);

      if (base == 2)
        {
          pk_puts ("0b");
          PK_PRINT_BINARY ((uint64_t) uintval, size);
          pk_puts (PK_INTEGRAL_SUFFIX (size, /*signed_p*/ 1));
        }
      else
        {
          if (size == 32)
            pk_printf (int32_fmt, base == 10 ? intval : uintval);
          else if (size == 16)
            pk_printf (int16_fmt, base == 10 ? intval : uintval);
          else if (size == 8)
            pk_printf (int8_fmt, base == 10 ? intval : uintval);
          else if (size == 4)
            pk_printf (int4_fmt, base == 10 ? intval : uintval);
          else
            pk_printf (int_fmt, base == 10 ? intval : uintval,
                       PVM_VAL_INT_SIZE (val));
        }

      pk_term_end_class ("integer");
    }
  else if (PVM_IS_ULONG (val))
    {
      int size = PVM_VAL_ULONG_SIZE (val);
      uint64_t ulongval = PVM_VAL_ULONG (val);

      pk_term_class ("integer");

      if (base == 2)
        {
          pk_puts ("0b");
          PK_PRINT_BINARY (ulongval, size);
          pk_puts (PK_INTEGRAL_SUFFIX (size, /*signed_p*/ 0));
        }
      else
        {
          if (size == 64)
            pk_printf (ulong64_fmt, ulongval);
          else
            pk_printf (ulong_fmt, ulongval, PVM_VAL_LONG_SIZE (val));
        }

      pk_term_end_class ("integer");
    }
  else if (PVM_IS_UINT (val))
    {
      int size = PVM_VAL_UINT_SIZE (val);
      uint32_t uintval = PVM_VAL_UINT (val);

      pk_term_class ("integer");

      if (base == 2)
        {
          pk_puts ("0b");
          PK_PRINT_BINARY (uintval, size);
          pk_puts (PK_INTEGRAL_SUFFIX (size, /*signed_p*/ 0));
        }
      else
        {
          if (size == 32)
            pk_printf (uint32_fmt, uintval);
          else if (size == 16)
            pk_printf (uint16_fmt, uintval);
          else if (size == 8)
            pk_printf (uint8_fmt, uintval);
          else if (size == 4)
            pk_printf (uint4_fmt, uintval);
          else
            pk_printf (uint_fmt, uintval, PVM_VAL_UINT_SIZE (val));
        }

      pk_term_end_class ("integer");
    }
  else if (PVM_IS_STR (val))
    {
      const char *str = PVM_VAL_STR (val);
      char *str_printable;
      size_t str_size = strlen (PVM_VAL_STR (val));
      size_t printable_size, i, j;

      pk_term_class ("string");

      /* Calculate the length (in bytes) of the printable string
         corresponding to the string value.  */
      for (printable_size = 0, i = 0; i < str_size; i++)
        {
          switch (str[i])
            {
            case '\n': printable_size += 2; break;
            case '\t': printable_size += 2; break;
            case '\\': printable_size += 2; break;
            case '\"': printable_size += 2; break;
            default: printable_size += 1; break;
            }
        }

      /* Now build the printable string.  */
      str_printable = xmalloc (printable_size + 1);
      for (i = 0, j = 0; i < str_size; i++)
        {
          switch (str[i])
            {
            case '\n':
              str_printable[j] = '\\';
              str_printable[j+1] = 'n';
              j += 2;
              break;
            case '\t':
              str_printable[j] = '\\';
              str_printable[j+1] = 't';
              j += 2;
              break;
            case '\\':
              str_printable[j] = '\\';
              str_printable[j+1] = '\\';
              j += 2;
              break;
            case '"':
              str_printable[j] = '\\';
              str_printable[j+1] = '\"';
              j += 2;
              break;
            default:
              str_printable[j] = str[i];
              j++;
              break;
            }
        }
      assert (j == printable_size);
      str_printable[j] = '\0';

      pk_printf ("\"%s\"", str_printable);
      free (str_printable);

      pk_term_end_class ("string");
    }
  else if (PVM_IS_ARR (val))
    {
      size_t nelem, idx;
      pvm_val array_offset = PVM_VAL_ARR_OFFSET (val);

      nelem = PVM_VAL_ULONG (PVM_VAL_ARR_NELEM (val));
      pk_term_class ("array");

      pk_puts ("[");
      for (idx = 0; idx < nelem; idx++)
        {
          pvm_val elem_value = PVM_VAL_ARR_ELEM_VALUE (val, idx);
          pvm_val elem_offset = PVM_VAL_ARR_ELEM_OFFSET (val, idx);

          if (idx != 0)
            pk_puts (",");

          if ((acutoff != 0) && (acutoff <= idx))
            {
              pk_term_class ("ellipsis");
              pk_puts ("...");
              pk_term_end_class ("ellipsis");
              break;
            }

          PVM_PRINT_VAL_1 (elem_value, ndepth);

          if (maps && elem_offset != PVM_NULL)
            {
              pk_puts (" @ ");
              pk_term_class ("offset");
              PVM_PRINT_VAL_1 (elem_offset, ndepth);
              pk_puts ("#b");
              pk_term_end_class ("offset");
            }
        }
      pk_puts ("]");

      if (maps && array_offset != PVM_NULL)
        {
          /* The struct offset is a bit-offset.  Do not bother to
             create a real offset here.  */
          pk_puts (" @ ");
          pk_term_class ("offset");
          PVM_PRINT_VAL_1 (array_offset, ndepth);
          pk_puts ("#b");
          pk_term_end_class ("offset");
        }

      pk_term_end_class ("array");
    }
  else if (PVM_IS_SCT (val))
    {
      size_t nelem, idx, nabsent;
      pvm_val struct_type = PVM_VAL_SCT_TYPE (val);
      pvm_val struct_type_name = PVM_VAL_TYP_S_NAME (struct_type);
      pvm_val struct_offset = PVM_VAL_SCT_OFFSET (val);

      /* If the struct has a pretty printing method (called _print)
         then use it, unless the PVM is configured to not do so.  */
      if (pprint)
        {
          if (pvm_call_pretty_printer (vm, val, exit_exception))
            return;
        }

      nelem = PVM_VAL_ULONG (PVM_VAL_SCT_NFIELDS (val));

      pk_term_class ("struct");

      if (struct_type_name != PVM_NULL)
        {
          pk_term_class ("struct-type-name");
          pk_puts ( PVM_VAL_STR (struct_type_name));
          pk_term_end_class ("struct-type-name");
        }
      else
        pk_puts ("struct");

      if (ndepth >= depth && depth != 0)
        {
          pk_puts (" {...}");
          pk_term_end_class ("struct");
          return;
        }

      pk_puts (" ");
      pk_printf ("{");

      nabsent = 0;
      for (idx = 0; idx < nelem; ++idx)
        {
          pvm_val name = PVM_VAL_SCT_FIELD_NAME (val, idx);
          pvm_val value = PVM_VAL_SCT_FIELD_VALUE (val, idx);
          pvm_val offset = PVM_VAL_SCT_FIELD_OFFSET (val, idx);

          if (PVM_VAL_SCT_FIELD_ABSENT_P (val, idx))
            nabsent++;
          else
            {
              if ((idx - nabsent) != 0)
                pk_puts (",");

              if (mode == PVM_PRINT_TREE)
                pk_term_indent (ndepth + 1, indent);

              if (name != PVM_NULL)
                {
                  pk_term_class ("struct-field-name");
                  pk_printf ("%s", PVM_VAL_STR (name));
                  pk_term_end_class ("struct-field-name");
                  pk_puts ("=");
                }
              PVM_PRINT_VAL_1 (value, ndepth + 1);

              if (maps && offset != PVM_NULL)
                {
                  pk_puts (" @ ");
                  pk_term_class ("offset");
                  PVM_PRINT_VAL_1 (offset, ndepth);
                  pk_puts ("#b");
                  pk_term_end_class ("offset");
                }
            }
        }

      if (mode == PVM_PRINT_TREE)
        pk_term_indent (ndepth, indent);
      pk_puts ("}");

      if (maps && struct_offset != PVM_NULL)
        {
          /* The struct offset is a bit-offset.  Do not bother to
             create a real offset here.  */
          pk_puts (" @ ");
          pk_term_class ("offset");
          PVM_PRINT_VAL_1 (struct_offset, ndepth);
          pk_puts ("#b");
          pk_term_end_class ("offset");
        }

      pk_term_end_class ("struct");
    }
  else if (PVM_IS_TYP (val))
    {
      pk_term_class ("type");

      switch (PVM_VAL_TYP_CODE (val))
        {
        case PVM_TYPE_INTEGRAL:
          {
            if (!(PVM_VAL_INT (PVM_VAL_TYP_I_SIGNED_P (val))))
              pk_puts ("u");

            switch (PVM_VAL_ULONG (PVM_VAL_TYP_I_SIZE (val)))
              {
              case 8: pk_puts ("int8"); break;
              case 16: pk_puts ("int16"); break;
              case 32: pk_puts ("int32"); break;
              case 64: pk_puts ("int64"); break;
              default: PK_UNREACHABLE (); break;
              }
          }
          break;
        case PVM_TYPE_STRING:
          pk_puts ("string");
          break;
        case PVM_TYPE_VOID:
          pk_puts ("void");
          break;
        case PVM_TYPE_ARRAY:
          PVM_PRINT_VAL_1 (PVM_VAL_TYP_A_ETYPE (val), ndepth);
          pk_puts ("[");
          if (PVM_VAL_TYP_A_BOUND (val) != PVM_NULL)
            PVM_PRINT_VAL_1 (PVM_VAL_TYP_A_BOUND (val), ndepth);
          pk_puts ("]");
          break;
        case PVM_TYPE_OFFSET:
          pk_puts ("[");
          PVM_PRINT_VAL_1 (PVM_VAL_TYP_O_BASE_TYPE (val), ndepth);
          pk_puts (" ");
          print_unit_name (PVM_VAL_ULONG (PVM_VAL_TYP_O_UNIT (val)));
          pk_puts ("]");
          break;
        case PVM_TYPE_CLOSURE:
          {
            size_t i, nargs;

            nargs = PVM_VAL_ULONG (PVM_VAL_TYP_C_NARGS (val));

            pk_puts ("(");
            for (i = 0; i < nargs; ++i)
              {
                pvm_val atype = PVM_VAL_TYP_C_ATYPE (val, i);
                if (i != 0)
                  pk_puts (",");
                PVM_PRINT_VAL_1 (atype, ndepth);
              }
            pk_puts (")");

            PVM_PRINT_VAL_1 (PVM_VAL_TYP_C_RETURN_TYPE (val), ndepth);
            break;
          }
        case PVM_TYPE_STRUCT:
          {
            size_t i, nelem;
            pvm_val type_name = PVM_VAL_TYP_S_NAME (val);

            nelem = PVM_VAL_ULONG (PVM_VAL_TYP_S_NFIELDS (val));

            if (type_name != PVM_NULL)
              pk_puts (PVM_VAL_STR (type_name));
            else
              pk_puts ("struct");

            pk_puts (" {");
            for (i = 0; i < nelem; ++i)
              {
                pvm_val ename = PVM_VAL_TYP_S_FNAME (val, i);
                pvm_val etype = PVM_VAL_TYP_S_FTYPE (val, i);

                if (i != 0)
                  pk_puts (" ");

                PVM_PRINT_VAL_1 (etype, ndepth);
                if (ename != PVM_NULL)
                  pk_printf (" %s", PVM_VAL_STR (ename));
                pk_puts (";");
              }
            pk_puts ("}");
            break;
          }
        default:
          PK_UNREACHABLE ();
        }

      pk_term_end_class ("type");
    }
  else if (PVM_IS_OFF (val))
    {
      pvm_val val_type = PVM_VAL_OFF_TYPE (val);

      pk_term_class ("offset");
      PVM_PRINT_VAL_1 (PVM_VAL_OFF_MAGNITUDE (val), ndepth);
      pk_puts ("#");
      print_unit_name (PVM_VAL_ULONG (PVM_VAL_TYP_O_UNIT (val_type)));
      pk_term_end_class ("offset");
    }
  else if (PVM_IS_CLS (val))
    {
      pvm_val name = PVM_VAL_CLS_NAME (val);

      pk_term_class ("special");
      if (name == PVM_NULL)
        pk_puts ("#<closure>");
      else
        pk_printf ("#<closure:%s>", PVM_VAL_STR (name));
      pk_term_end_class ("special");
    }
  else
    PK_UNREACHABLE ();
}

#undef PVM_PRINT_VAL_1

void
pvm_print_val (pvm vm, pvm_val val, pvm_val *exit_exception)
{
  if (exit_exception)
    *exit_exception = PVM_NULL;
  pvm_print_val_1 (vm, pvm_odepth (vm), pvm_omode (vm), pvm_obase (vm),
                   pvm_oindent (vm), pvm_oacutoff (vm),
                   (pvm_omaps (vm) << (PVM_PRINT_F_MAPS - 1)
                    | (pvm_pretty_print (vm) << (PVM_PRINT_F_PPRINT - 1))),
                   exit_exception, val, 0 /* ndepth */);
}

void
pvm_print_val_with_params (pvm vm, pvm_val val, int depth, int mode, int base,
                           int indent, int acutoff, uint32_t flags,
                           pvm_val *exit_exception)
{
  if (exit_exception)
    *exit_exception = PVM_NULL;

  pvm_print_val_1 (vm, depth, mode, base, indent, acutoff, flags,
                   exit_exception, val, 0 /* ndepth */);
}

pvm_val
pvm_typeof (pvm_val val)
{
  pvm_val type;

  if (PVM_IS_INT (val))
    type = pvm_make_integral_type (pvm_make_ulong (PVM_VAL_INT_SIZE (val), 64),
                                   PVM_MAKE_INT (1, 32));
  else if (PVM_IS_UINT (val))
    type = pvm_make_integral_type (
        pvm_make_ulong (PVM_VAL_UINT_SIZE (val), 64), PVM_MAKE_INT (0, 32));
  else if (PVM_IS_LONG (val))
    type = pvm_make_integral_type (
        pvm_make_ulong (PVM_VAL_LONG_SIZE (val), 64), PVM_MAKE_INT (1, 32));
  else if (PVM_IS_ULONG (val))
    type = pvm_make_integral_type (
        pvm_make_ulong (PVM_VAL_ULONG_SIZE (val), 64), PVM_MAKE_INT (0, 32));
  else if (PVM_IS_STR (val))
    type = pvm_make_string_type ();
  else if (PVM_IS_OFF (val))
    type = PVM_VAL_OFF_TYPE (val);
  else if (PVM_IS_ARR (val))
    type = PVM_VAL_ARR_TYPE (val);
  else if (PVM_IS_SCT (val))
    type = PVM_VAL_SCT_TYPE (val);
  else if (PVM_IS_TYP (val))
    type = val;
  else if (PVM_IS_CLS (val))
    type = PVM_NULL;
  else
    PK_UNREACHABLE ();

  return type;
}

int
pvm_type_equal_p (pvm_val type1, pvm_val type2)
{
  enum pvm_type_code type_code_1 = PVM_VAL_TYP_CODE (type1);
  enum pvm_type_code type_code_2 = PVM_VAL_TYP_CODE (type2);

  if (type_code_1 != type_code_2)
    return 0;

  switch (type_code_1)
    {
    case PVM_TYPE_INTEGRAL:
      {
        size_t t1_size = PVM_VAL_ULONG (PVM_VAL_TYP_I_SIZE (type1));
        size_t t2_size = PVM_VAL_ULONG (PVM_VAL_TYP_I_SIZE (type2));
        int32_t t1_signed = PVM_VAL_INT (PVM_VAL_TYP_I_SIGNED_P (type1));
        int32_t t2_signed = PVM_VAL_INT (PVM_VAL_TYP_I_SIGNED_P (type2));

        return (t1_size == t2_size && t1_signed == t2_signed);
      }
    case PVM_TYPE_STRING:
    case PVM_TYPE_VOID:
      return 1;
    case PVM_TYPE_ARRAY:
      {
        pvm_val etype1 = PVM_VAL_TYP_A_ETYPE (type1);
        pvm_val etype2 = PVM_VAL_TYP_A_ETYPE (type2);

        /* Note that arrays whose elements can be of any
           type have etype PVM_NULL.  */
        if (etype1 == PVM_NULL && etype2 == PVM_NULL)
          return 1;
        else if (etype1 == PVM_NULL || etype2 == PVM_NULL)
          return 0;
        else
          return pvm_type_equal_p (etype1, etype2);
      }
    case PVM_TYPE_STRUCT:
      {
        pvm_val name1 = PVM_VAL_TYP_S_NAME (type1);
        pvm_val name2 = PVM_VAL_TYP_S_NAME (type2);

        if (name1 == PVM_NULL || name2 == PVM_NULL)
          return 0;
        return (STREQ (PVM_VAL_STR (name1), PVM_VAL_STR (name2)));
      }
    case PVM_TYPE_OFFSET:
      return (pvm_type_equal_p (PVM_VAL_TYP_O_BASE_TYPE (type1),
                                PVM_VAL_TYP_O_BASE_TYPE (type2))
              && (PVM_VAL_ULONG (PVM_VAL_TYP_O_UNIT (type1))
                  == PVM_VAL_ULONG (PVM_VAL_TYP_O_UNIT (type2))));
    case PVM_TYPE_CLOSURE:
      {
        size_t i, nargs;

        if (PVM_VAL_ULONG (PVM_VAL_TYP_C_NARGS (type1))
            != PVM_VAL_ULONG (PVM_VAL_TYP_C_NARGS (type2)))
          return 0;

        if (!pvm_type_equal_p (PVM_VAL_TYP_C_RETURN_TYPE (type1),
                               PVM_VAL_TYP_C_RETURN_TYPE (type2)))
          return 0;

        nargs = PVM_VAL_ULONG (PVM_VAL_TYP_C_NARGS (type1));
        for (i = 0; i < nargs; i++)
          {
            if (!pvm_type_equal_p (PVM_VAL_TYP_C_ATYPE (type1, i),
                                   PVM_VAL_TYP_C_ATYPE (type2, i)))
              return 0;
          }

        return 1;
      }
    default:
      PK_UNREACHABLE ();
    }
}

void
pvm_print_string (pvm_val string)
{
  pk_puts (PVM_VAL_STR (string));
}

/* Call a struct pretty-print function in the closure CLS,
   corresponding to the struct VAL.  */

int
pvm_call_pretty_printer (pvm vm, pvm_val val, pvm_val *exit_exception)
{
  pvm_val cls = pvm_get_struct_method (val, "_print");

  if (cls == PVM_NULL)
    return 0;

  pvm_call_closure (vm, cls, exit_exception, val, PVM_NULL);
  return 1;
}

/* IMPORTANT: please keep pvm_make_exception in sync with the
   definition of the struct Exception in pkl-rt.pk.  */

pvm_val
pvm_make_exception (int code, const char *name, int exit_status,
                    const char *location, const char *msg)
{
  pvm_val exception;

  JITTER_GC_BLOCK_BEGIN (gc_heaplet);
  {
    pvm_val nfields;
    pvm_val type;
    pvm_val nmethods;

    nfields = pvm_make_ulong (5, 64);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &nfields);

    type = pvm_make_struct_type (nfields, /*field_names*/ NULL,
                                 /*field_types*/ NULL);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &type);

    PVM_VAL_TYP_S_NAME (type) = pvm_make_string ("Exception");

    PVM_VAL_TYP_S_FNAME (type, 0) = pvm_make_string ("code");
    PVM_VAL_TYP_S_FTYPE (type, 0) = pvm_make_integral_type (
        pvm_make_ulong (32, 64), PVM_MAKE_INT (1, 32));
    PVM_VAL_TYP_S_FNAME (type, 1) = pvm_make_string ("name");
    PVM_VAL_TYP_S_FTYPE (type, 1) = pvm_make_string_type ();
    PVM_VAL_TYP_S_FNAME (type, 2) = pvm_make_string ("exit_status");
    PVM_VAL_TYP_S_FTYPE (type, 2) = pvm_make_integral_type (
        pvm_make_ulong (32, 64), PVM_MAKE_INT (1, 32));
    PVM_VAL_TYP_S_FNAME (type, 3) = pvm_make_string ("location");
    PVM_VAL_TYP_S_FTYPE (type, 3) = pvm_make_string_type ();
    PVM_VAL_TYP_S_FNAME (type, 4) = pvm_make_string ("msg");
    PVM_VAL_TYP_S_FTYPE (type, 4) = pvm_make_string_type ();

    nmethods = pvm_make_ulong (0, 64);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &nmethods);

    exception = pvm_make_struct (nfields, nmethods, type);
    JITTER_GC_BLOCK_ROOT_1 (gc_heaplet, &exception);

    PVM_VAL_SCT_FIELD_NAME (exception, 0) = PVM_VAL_TYP_S_FNAME (type, 0);
    PVM_VAL_SCT_FIELD_VALUE (exception, 0) = PVM_MAKE_INT (code, 32);

    PVM_VAL_SCT_FIELD_NAME (exception, 1) = PVM_VAL_TYP_S_FNAME (type, 1);
    PVM_VAL_SCT_FIELD_VALUE (exception, 1) = pvm_make_string (name);

    PVM_VAL_SCT_FIELD_NAME (exception, 2) = PVM_VAL_TYP_S_FNAME (type, 2);
    PVM_VAL_SCT_FIELD_VALUE (exception, 2) = PVM_MAKE_INT (exit_status, 32);

    PVM_VAL_SCT_FIELD_NAME (exception, 3) = PVM_VAL_TYP_S_FNAME (type, 3);
    PVM_VAL_SCT_FIELD_VALUE (exception, 3)
        = pvm_make_string (location == NULL ? "" : location);

    PVM_VAL_SCT_FIELD_NAME (exception, 4) = PVM_VAL_TYP_S_FNAME (type, 4);
    PVM_VAL_SCT_FIELD_VALUE (exception, 4)
        = pvm_make_string (msg == NULL ? "" : msg);
  }
  JITTER_GC_BLOCK_END (gc_heaplet);

  /* FIXME FIXME FIXME */
  assert (PVM_IS_SCT (exception));
  assert (PVM_IS_ULONG (PVM_VAL_SCT_NFIELDS (exception)));
  assert (PVM_IS_INT (PVM_VAL_SCT_FIELD_VALUE (exception, 0)));
  assert (PVM_IS_STR (PVM_VAL_SCT_FIELD_VALUE (exception, 1)));
  assert (PVM_IS_INT (PVM_VAL_SCT_FIELD_VALUE (exception, 2)));
  assert (PVM_IS_STR (PVM_VAL_SCT_FIELD_VALUE (exception, 3)));
  assert (PVM_IS_STR (PVM_VAL_SCT_FIELD_VALUE (exception, 4)));

  return exception;
}

pvm_val
pvm_val_cls_program (pvm_val cls)
{
  return PVM_VAL_CLS_PROGRAM (cls);
}

static bool
pvm_gc_is_unboxed_p (pvm_val v)
{
  if (PVM_VAL_TAG (v) == 7)
    return true;
  return !PVM_VAL_BOXED_P (v);
}

/* */

void *
pvm_alloc_uncollectable (size_t nelem)
{
  uintptr_t *us;
  size_t nbytes;

  nbytes = nelem * sizeof (uintptr_t);

  us = pvm_heaplet_alloc (gc_heaplet, sizeof (uintptr_t) + nbytes);
  assert (us); // FIXME Handle error.
  us[0]
      = (uintptr_t)jitter_gc_register_global_root (gc_heaplet, us + 1, nbytes);
  return us + 1;
}

void
pvm_free_uncollectable (void *ptr)
{
  uintptr_t *us;
  jitter_gc_global_root root;

  us = (uintptr_t *)ptr;
  root = (jitter_gc_global_root)us[0];
  jitter_gc_deregister_global_root (gc_heaplet, root);
}

void
pvm_alloc_gc (void)
{
  JITTER_GC_COLLECT_EITHER (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                            GC_RUNTIME_LIMIT (gc_heaplet));
}

static struct
{
  jitter_gc_global_root void_type;
  jitter_gc_global_root string_type;
  jitter_gc_global_root common_int_types;

#define NSTACKS 3
  struct
  {
    void *memory;
    size_t nelems;
    void **tos_ptr;
  } stacks[NSTACKS];
  size_t nstacks;

#if 0
  size_t roots_nelem;
  size_t roots_nallocated;
  jitter_gc_global_root *roots;
#endif
} gc_global_roots;

void *
pvm_gc_register_vm_stack (void *pointer, size_t nelems, void **tos_ptr)
{
  assert (gc_global_roots.nstacks < NSTACKS);
  gc_global_roots.stacks[gc_global_roots.nstacks].memory = pointer;
  gc_global_roots.stacks[gc_global_roots.nstacks].nelems = nelems;
  gc_global_roots.stacks[gc_global_roots.nstacks].tos_ptr = tos_ptr;
  return (void *)(uintptr_t)gc_global_roots.nstacks++;
}

void
pvm_gc_deregister_vm_stack (void *handle)
{
  uintptr_t i = (uintptr_t)handle;

  assert (i < NSTACKS);
  memset (&gc_global_roots.stacks[i], 0, sizeof (gc_global_roots.stacks[i]));
}

// Debugging.
#if 1

static void
pvm_gc_hook_pre (struct jitter_gc_heaplet *b, void *useless,
                 enum jitter_gc_collection_kind k)
{
  fprintf (stderr, "[GC-PRE] heaplet:%p kind:%s\n", b,
           jitter_gc_collection_kind_to_string (k));

  if (k == jitter_gc_collection_kind_ssb_flush)
    return;

  for (int i = 0; i < 2; ++i)
    {
      pvm_val *begin;
      pvm_val *end;
      ptrdiff_t nelem;

      /* Half-open interval [begin, end).  */
      begin = (pvm_val *)gc_global_roots.stacks[i].memory;
      end = (pvm_val *)*gc_global_roots.stacks[i].tos_ptr + 1;
      nelem = end - begin;

      fprintf (stderr, "[GC-PRE] [stack%d] nelem:%td\n", i, nelem);

      for (ptrdiff_t j = 0; j < nelem; ++j)
        jitter_gc_handle_word (gc_heaplet, &begin[j]);
    }

  {
    struct pvm_exception_handler *begin;
    struct pvm_exception_handler *end;
    ptrdiff_t nelem;

    /* Half-open interval [begin, end).  */
    begin = (struct pvm_exception_handler *)gc_global_roots.stacks[2].memory;
    end = (struct pvm_exception_handler *)*gc_global_roots.stacks[2].tos_ptr + 1;
    nelem = end - begin;

    fprintf (stderr, "[GC-PRE] stack2 nelem:%td\n", nelem);

    for (ptrdiff_t j = 0; j < nelem; ++j)
      jitter_gc_handle_word (gc_heaplet, &begin[j].env);
  }
}

static void
pvm_gc_hook_post (struct jitter_gc_heaplet *b, void *useless,
                      enum jitter_gc_collection_kind k)
{
  fprintf (stderr, "[GC-POST] heaplet:%p kind:%s\n", b,
           jitter_gc_collection_kind_to_string (k));
}

#endif
void
pvm_val_initialize (void)
{
  gc_shapes = jitter_gc_shape_table_make (
      PVM_VAL_INVALID_OBJECT, PVM_VAL_UNINITIALIZED_OBJECT,
      PVM_VAL_BROKEN_HEART_TYPE_CODE, pvm_gc_is_unboxed_p);
  assert (gc_shapes); // FIXME Handle error.

#if 0
  jitter_gc_shape_add_headered_quickly_finalizable (gc_shapes,

  // Cannot be headerless.
  jitter_gc_shape_add_headerless (gc_shapes, "big",
                                  pvm_val_is_big_p,
                                  pvm_val_big_sizeof,
                                  pvm_val_big_copy);
  jitter_gc_shape_add_headerless (gc_shapes, "ubig",
                                  pvm_val_is_ubig_p,
                                  pvm_val_ubig_sizeof,
                                  pvm_val_ubig_copy);
#endif

  // FIXME Use the same function sets for long/ulong.
  jitter_gc_shape_add_headered_non_finalizable (
      gc_shapes, "long", pvm_gc_is_long_p, pvm_gc_sizeof_long,
      pvm_gc_is_long_type_code_p, pvm_gc_copy_long, pvm_gc_update_fields_long);
  jitter_gc_shape_add_headered_non_finalizable (
      gc_shapes, "ulong", pvm_gc_is_ulong_p, pvm_gc_sizeof_ulong,
      pvm_gc_is_ulong_type_code_p, pvm_gc_copy_ulong,
      pvm_gc_update_fields_ulong);

  jitter_gc_shape_add_headered_quickly_finalizable (
      gc_shapes, "string", pvm_gc_is_string_p, pvm_gc_sizeof_string,
      pvm_gc_is_string_type_code_p, pvm_gc_copy_string,
      pvm_gc_update_fields_string, pvm_gc_finalize_string);

  jitter_gc_shape_add_headered_quickly_finalizable (
      gc_shapes, "array", pvm_gc_is_array_p, pvm_gc_sizeof_array,
      pvm_gc_is_array_type_code_p, pvm_gc_copy_array,
      pvm_gc_update_fields_array, pvm_gc_finalize_array);

  jitter_gc_shape_add_headered_quickly_finalizable (
      gc_shapes, "struct", pvm_gc_is_struct_p, pvm_gc_sizeof_struct,
      pvm_gc_is_struct_type_code_p, pvm_gc_copy_struct,
      pvm_gc_update_fields_struct, pvm_gc_finalize_struct);

  jitter_gc_shape_add_headered_quickly_finalizable (
      gc_shapes, "type", pvm_gc_is_type_p, pvm_gc_sizeof_type,
      pvm_gc_is_type_type_code_p, pvm_gc_copy_type, pvm_gc_update_fields_type,
      pvm_gc_finalize_type);

  /* FIXME Change to non_finalizable.  */
  jitter_gc_shape_add_headered_quickly_finalizable (
      gc_shapes, "closure", pvm_gc_is_closure_p, pvm_gc_sizeof_closure,
      pvm_gc_is_closure_type_code_p, pvm_gc_copy_closure,
      pvm_gc_update_fields_closure, pvm_gc_finalize_closure);

  jitter_gc_shape_add_headered_non_finalizable (
      gc_shapes, "offset", pvm_gc_is_offset_p, pvm_gc_sizeof_offset,
      pvm_gc_is_offset_type_code_p, pvm_gc_copy_offset,
      pvm_gc_update_fields_offset);

  jitter_gc_shape_add_headered_quickly_finalizable (
      gc_shapes, "iarray", pvm_gc_is_iarray_p, pvm_gc_sizeof_iarray,
      pvm_gc_is_iarray_type_code_p, pvm_gc_copy_iarray,
      pvm_gc_update_fields_iarray, pvm_gc_finalize_iarray);

  jitter_gc_shape_add_headered_quickly_finalizable (
      gc_shapes, "program", pvm_gc_is_program_p, pvm_gc_sizeof_program,
      pvm_gc_is_program_type_code_p, pvm_gc_copy_program,
      pvm_gc_update_fields_program, pvm_gc_finalize_program);

  jitter_gc_shape_add_headered_non_finalizable (
      gc_shapes, "env", pvm_gc_is_env_p, pvm_gc_sizeof_env,
      pvm_gc_is_env_type_code_p, pvm_gc_copy_env, pvm_gc_update_fields_env);

  gc_heap = jitter_gc_heap_make (gc_shapes);
  assert (gc_heap); // FIXME Handle error.

  gc_heaplet = jitter_gc_heaplet_make (gc_heap);
  assert (gc_heaplet); // FIXME Handle error.

  JITTER_GC_HEAPLET_TO_RUNTIME (gc_heaplet, GC_ALLOCATION_POINTER (gc_heaplet),
                                GC_RUNTIME_LIMIT (gc_heaplet));

  jitter_gc_hook_register_pre_collection (gc_heaplet, pvm_gc_hook_pre,
                                          NULL);
  jitter_gc_hook_register_post_collection (gc_heaplet, pvm_gc_hook_post,
                                           NULL);
  jitter_gc_hook_register_pre_ssb_flush (gc_heaplet, pvm_gc_hook_pre,
                                         NULL);
  jitter_gc_hook_register_post_ssb_flush (gc_heaplet, pvm_gc_hook_post,
                                          NULL);

  string_type = pvm_make_type (PVM_TYPE_STRING);
  gc_global_roots.string_type
      = jitter_gc_register_global_root_1 (gc_heaplet, &string_type);

  void_type = pvm_make_type (PVM_TYPE_VOID);
  gc_global_roots.void_type
      = jitter_gc_register_global_root_1 (gc_heaplet, &void_type);

  for (int bits = 0; bits < 65; ++bits)
    {
      common_int_types[bits][/*signed_p*/ 0] = PVM_NULL;
      common_int_types[bits][/*signed_p*/ 1] = PVM_NULL;
    }
  gc_global_roots.common_int_types = jitter_gc_register_global_root (
      gc_heaplet, common_int_types, sizeof (common_int_types));
  /* Now, after registering COMMON_INT_TYPES as a global root, we can safely
     make all the integral types.  */
  for (int bits = 1; bits <= 64; ++bits)
    {
      pvm_val size;

      size = pvm_make_ulong (bits, 64);
      common_int_types[bits][0]
          = pvm_make_integral_type_1 (size, /*signed_p*/ PVM_MAKE_INT (0, 32));
      common_int_types[bits][1]
          = pvm_make_integral_type_1 (size, /*signed_p*/ PVM_MAKE_INT (1, 32));
    }

#if 0
  gc_global_roots.roots_nallocated = 128;
  gc_global_roots.roots
      = malloc (gc_global_roots.roots_nallocated * sizeof (uintptr_t));
  assert (gc_global_rotos.roots); // FIXME Handle error properly.
#endif
}

void
pvm_val_finalize (void)
{
#if 0
  while (gc_global_roots.roots_nelem)
    {
      gc_global_roots.roots_nelem--;
      jitter_gc_deregister_global_root (
          gc_heaplet, gc_global_roots.roots[gc_global_roots.roots_nelem]);
    }
  free (gc_global_roots.roots);
#endif

  jitter_gc_deregister_global_root (gc_heaplet,
                                    gc_global_roots.common_int_types);
  jitter_gc_deregister_global_root (gc_heaplet, gc_global_roots.void_type);
  jitter_gc_deregister_global_root (gc_heaplet, gc_global_roots.string_type);

  jitter_gc_heaplet_destroy (gc_heaplet);
  jitter_gc_heap_destroy (gc_heap);
  jitter_gc_shape_table_destroy (gc_shapes);
}
