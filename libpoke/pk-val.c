/* pk-val.c - poke values.  */

/* Copyright (C) 2020, 2021, 2022, 2023, 2024, 2025 Jose E. Marchesi */

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

#include "pvm.h"
#include "pvm-val.h"
#include "libpoke.h"

#if 0

pk_val
_pk_add_to_current_gc_scope (pk_compiler pkc, pvm_val v)
{
  size_t index;

  index = pvm_iarray_push (pkc->iarray_stack[pkc->iarray_stack_index], v);
  return (pk_val)pkc->iarray_stack_index << 32 | index;
}

#endif

pk_val
pk_make_int (pk_compiler pkc __attribute__ ((unused)), int64_t value, int size)
{
  pvm_val v = pvm_make_signed_integral (value, size);

#if 0
  pk_val i = _pk_add_to_current_gc_scope (pkc, v);
  return i;
#endif

  return v;
}

int64_t
pk_int_value (pk_val val)
{
  if (PVM_IS_INT (val))
    return PVM_VAL_INT (val);
  else
    return PVM_VAL_LONG (val);
}

int
pk_int_size (pk_val val)
{
  if (PVM_IS_INT (val))
    return PVM_VAL_INT_SIZE (val);
  else
    return PVM_VAL_LONG_SIZE (val);
}

pk_val
pk_make_uint (pk_compiler pkc __attribute__ ((unused)), uint64_t value,
              int size)
{
  return pvm_make_unsigned_integral (value, size);
}

uint64_t
pk_uint_value (pk_val val)
{
  if (PVM_IS_UINT (val))
    return PVM_VAL_UINT (val);
  else
    return PVM_VAL_ULONG (val);
}

int
pk_uint_size (pk_val val)
{
  if (PVM_IS_UINT (val))
    return PVM_VAL_UINT_SIZE (val);
  else
    return PVM_VAL_ULONG_SIZE (val);
}

pk_val
pk_make_string (pk_compiler pkc __attribute__ ((unused)), const char *str)
{
  return pvm_make_string (str);
}

const char *
pk_string_str (pk_val val)
{
  return PVM_VAL_STR (val);
}

pk_val
pk_make_offset (pk_compiler pkc __attribute__ ((unused)), pk_val magnitude,
                pk_val unit)
{
  if (!PVM_IS_INTEGRAL (magnitude)
      || !PVM_IS_ULONG (unit)
      || PVM_VAL_ULONG (unit) == 0
      || PVM_VAL_ULONG_SIZE (unit) != 64)
    return PK_NULL;
  else
    {
      pvm_val type = pvm_make_offset_type (pvm_typeof (magnitude),
                                           unit, PVM_NULL /* ref_type */);
      return pvm_make_offset (magnitude, type);
    }
}

pk_val
pk_offset_magnitude (pk_val val)
{
  return PVM_VAL_OFF_MAGNITUDE (val);
}

pk_val
pk_offset_unit (pk_val val)
{
  pvm_val val_type = PVM_VAL_OFF_TYPE (val);
  return PVM_VAL_TYP_O_UNIT (val_type);
}

int
pk_val_mappable_p (pk_val val)
{
  return PVM_VAL_MAPPABLE_P (val);
}

int
pk_val_mapped_p (pk_val val)
{
  return PVM_VAL_MAPPED_P (val);
}

void
pk_val_set_mapped (pk_val val, int mapped_p)
{
  PVM_VAL_SET_MAPPED_P (val, !!mapped_p);
}

int
pk_val_strict_p (pk_val val)
{
  return PVM_VAL_STRICT_P (val);
}

void
pk_val_set_strict (pk_val val, int strict_p)
{
  PVM_VAL_SET_STRICT_P (val, !!strict_p);
}

pk_val
pk_val_ios (pk_val val)
{
  return PVM_VAL_IOS (val);
}

void
pk_val_set_ios (pk_val val, pk_val ios)
{
  if (PVM_IS_INT (ios) && PVM_VAL_INT_SIZE (ios) == 32)
    PVM_VAL_SET_IOS (val, ios);
}

pk_val
pk_val_offset (pk_val val)
{
  pvm_val val_offset;
  uint64_t bit_offset;

  if (!PVM_VAL_MAPPED_P (val))
    return PK_NULL;

  val_offset = PVM_VAL_OFFSET (val);
  assert (val_offset != PVM_NULL);

  /* The offset in the PVM value is a bit-offset.  Convert to a proper
     offset.  */
  bit_offset = PVM_VAL_ULONG (val_offset);

  /* XXX "upunit" properly so we get a nice unit, not just bytes or
     bits.  */
  if (bit_offset % 8 == 0)
    return pk_make_offset (/*pkc*/ NULL, pvm_make_ulong (bit_offset / 8, 64),
                           pvm_make_ulong (8, 64));
  else
    return pk_make_offset (/*pkc*/ NULL, val_offset, pvm_make_ulong (1, 64));
}

void
pk_val_set_offset (pk_val val, pk_val off)
{
  uint64_t boff;

  if (PVM_IS_OFF (off))
    {
      pvm_val off_type = PVM_VAL_OFF_TYPE (off);

      boff = PVM_VAL_INTEGRAL (PVM_VAL_OFF_MAGNITUDE (off))
        * PVM_VAL_ULONG (PVM_VAL_TYP_O_UNIT (off_type));
      PVM_VAL_SET_OFFSET (val, pvm_make_ulong (boff, 64));
    }
}

pk_val
pk_val_boffset (pk_val val)
{
  pvm_val val_boffset = PVM_VAL_OFFSET (val);

  return val_boffset == PVM_NULL ? PK_NULL : val_boffset;
}

void
pk_val_set_boffset (pk_val val, pk_val boff)
{
  if (PVM_IS_ULONG (boff) && PVM_VAL_ULONG_SIZE (boff) == 64)
    PVM_VAL_SET_OFFSET (val, boff);
}

int
pk_type_code (pk_val val)
{
  switch (PVM_VAL_TYP_CODE (val))
    {
    case PVM_TYPE_INTEGRAL:
      if ((pk_int_value (pk_integral_type_signed_p (val))))
        return PK_TYPE_INT;
      else
        return PK_TYPE_UINT;
    case PVM_TYPE_STRING:
      return PK_TYPE_STRING;
    case PVM_TYPE_ARRAY:
      return PK_TYPE_ARRAY;
    case PVM_TYPE_STRUCT:
      return PK_TYPE_STRUCT;
    case PVM_TYPE_OFFSET:
      return PK_TYPE_OFFSET;
    case PVM_TYPE_CLOSURE:
      return PK_TYPE_CLOSURE;
    case PVM_TYPE_VOID:
      return PK_TYPE_VOID;
    default:
      return PK_TYPE_UNKNOWN;
    }
}

pk_val
pk_type_name (pk_val type)
{
  switch (PVM_VAL_TYP_CODE (type))
    {
    case PVM_TYPE_STRUCT:
      return PVM_VAL_TYP_S_NAME (type);
      break;
    default:
      break;
    }

  return PK_NULL;
}

int
pk_val_kind (pk_val val)
{
  if (PVM_IS_INT (val) || PVM_IS_LONG (val))
    return PK_VAL_INT;
  else if (PVM_IS_UINT (val) || PVM_IS_ULONG (val))
    return PK_VAL_UINT;
  else if (PVM_IS_OFF (val))
    return PK_VAL_OFFSET;
  else if (PVM_IS_STR (val))
    return PK_VAL_STRING;
  else if (PVM_IS_ARR (val))
    return PK_VAL_ARRAY;
  else if (PVM_IS_SCT (val))
    return PK_VAL_STRUCT;
  else if (PVM_IS_CLS (val))
    return PK_VAL_CLOSURE;
  else if (PVM_IS_TYP (val))
    return PK_VAL_TYPE;
  else
    return PK_VAL_UNKNOWN;
}

int
pk_val_equal_p (pk_val val1, pk_val val2)
{
  return pvm_val_equal_p (val1, val2);
}

pk_val
pk_make_struct (pk_compiler pkc __attribute__ ((unused)), pk_val nfields,
                pk_val type)
{
  return pvm_make_struct (nfields, pvm_make_ulong (0, 64), type);
}

pk_val
pk_struct_nfields (pk_val sct)
{
  return PVM_VAL_SCT_NFIELDS (sct);
}

pk_val
pk_struct_ref_field_value (pk_val sct, const char *fname)
{
  return pvm_ref_struct_cstr (sct, fname);
}

void
pk_struct_ref_set_field_value (pk_val sct, const char *fname,
                               pk_val value)
{
  pvm_ref_set_struct_cstr (sct, fname, value);
}

pk_val
pk_struct_field_boffset (pk_val sct, uint64_t idx)
{
  if (idx < pk_uint_value (pk_struct_nfields (sct)))
    return PVM_VAL_SCT_FIELD_OFFSET (sct, idx);
  else
    return PK_NULL;
}

void
pk_struct_set_field_boffset (pk_val sct, uint64_t idx, pk_val boffset)
{
  if (idx < pk_uint_value (pk_struct_nfields (sct)))
    PVM_VAL_SCT_FIELD_OFFSET (sct, idx) = boffset;
}

pk_val
pk_struct_field_name (pk_val sct, uint64_t idx)
{
  if (idx < pk_uint_value (pk_struct_nfields (sct)))
    return PVM_VAL_SCT_FIELD_NAME (sct, idx);
  else
    return PK_NULL;
}

void
pk_struct_set_field_name (pk_val sct, uint64_t idx, pk_val name)
{
  if (idx < pk_uint_value (pk_struct_nfields (sct)))
    PVM_VAL_SCT_FIELD_NAME (sct, idx) = name;
}

pk_val
pk_struct_field_value (pk_val sct, uint64_t idx)
{
  if (idx < pk_uint_value (pk_struct_nfields (sct)))
    return PVM_VAL_SCT_FIELD_VALUE (sct, idx);
  else
    return PK_NULL;
}

void
pk_struct_set_field_value (pk_val sct, uint64_t idx, pk_val value)
{
  if (idx < pk_uint_value (pk_struct_nfields (sct)))
    PVM_VAL_SCT_FIELD_VALUE (sct, idx) = value;
}

pk_val
pk_make_array (pk_compiler pkc __attribute__ ((unused)), pk_val nelem,
               pk_val array_type)
{
  return pvm_make_array (nelem, array_type);
}

pk_val
pk_make_integral_type (pk_compiler pkc __attribute__ ((unused)), pk_val size,
                       pk_val signed_p)
{
  return pvm_make_integral_type (size, signed_p);
}

pk_val
pk_integral_type_size (pk_val type)
{
  return PVM_VAL_TYP_I_SIZE (type);
}

pk_val
pk_integral_type_signed_p (pk_val type)
{
  return PVM_VAL_TYP_I_SIGNED_P (type);
}

pk_val
pk_make_string_type (pk_compiler pkc __attribute__ ((unused)))
{
  return pvm_make_string_type ();
}

pk_val
pk_make_offset_type (pk_compiler pkc __attribute__ ((unused)),
                     pk_val base_type, pk_val unit, pk_val ref_type)
{
  return pvm_make_offset_type (base_type, unit, ref_type);
}

pk_val
pk_offset_type_base_type (pk_val type)
{
  return PVM_VAL_TYP_O_BASE_TYPE (type);
}

pk_val
pk_offset_type_unit (pk_val type)
{
  return PVM_VAL_TYP_O_UNIT (type);
}

pk_val
pk_make_struct_type (pk_compiler pkc __attribute__ ((unused)), pk_val nfields,
                     pk_val name, pk_val **fnames, pk_val **ftypes)
{
  pvm_val struct_type = pvm_make_struct_type (nfields, fnames, ftypes);

  PVM_VAL_TYP_S_NAME (struct_type) = name;
  return struct_type;
}

pk_val
pk_struct_type (pk_val sct)
{
  return PVM_VAL_SCT_TYPE (sct);
}

pk_val
pk_struct_type_name (pk_val type)
{
  return PVM_VAL_TYP_S_NAME (type);
}

pk_val
pk_struct_type_nfields (pk_val type)
{
  return PVM_VAL_TYP_S_NFIELDS (type);
}

pk_val
pk_struct_type_fname (pk_val type, uint64_t idx)
{
  if (idx < pk_uint_value (pk_struct_type_nfields (type)))
    return PVM_VAL_TYP_S_FNAME (type, idx);
  else
    return PK_NULL;
}

void
pk_struct_type_set_fname (pk_val type, uint64_t idx, pk_val field_name)
{
  if (idx < pk_uint_value (pk_struct_type_nfields (type)))
    PVM_VAL_TYP_S_FNAME (type, idx) = field_name;
}

pk_val
pk_struct_type_ftype (pk_val type, uint64_t idx)
{
  if (idx < pk_uint_value (pk_struct_type_nfields (type)))
    return PVM_VAL_TYP_S_FTYPE (type, idx);
  else
    return PK_NULL;
}

void
pk_struct_type_set_ftype (pk_val type, uint64_t idx, pk_val field_type)
{
  if (idx < pk_uint_value (pk_struct_type_nfields (type)))
    PVM_VAL_TYP_S_FTYPE (type, idx) = field_type;
}

pk_val
pk_make_array_type (pk_compiler pkc __attribute__ ((unused)), pk_val etype,
                    pk_val bound)
{
  return pvm_make_array_type (etype, bound);
}

pk_val
pk_array_type_etype (pk_val type)
{
  return PVM_VAL_TYP_A_ETYPE (type);
}

pk_val
pk_array_type_bound (pk_val type)
{
  return PVM_VAL_TYP_A_BOUND (type);
}

pk_val
pk_typeof (pk_val val)
{
  return pvm_typeof (val);
}

pk_val
pk_array_nelem (pk_val array)
{
  return PVM_VAL_ARR_NELEM (array);
}

pk_val
pk_array_elem_value (pk_val array, uint64_t idx)
{
  if (idx < pk_uint_value (pk_array_nelem (array)))
    return PVM_VAL_ARR_ELEM_VALUE (array, idx);
  else
    return PK_NULL;
}

void
pk_array_insert_elem (pk_val array, uint64_t idx, pk_val val)
{
  (void) pvm_array_insert (array, pvm_make_ulong (idx, 64), val);
}

void
pk_array_set_elem (pk_val array, uint64_t idx, pk_val val)
{
  (void) pvm_array_set (array, pvm_make_ulong (idx, 64), val);
}

pk_val
pk_array_elem_boffset (pk_val array, uint64_t idx)
{
  if (idx < pk_uint_value (pk_array_nelem (array)))
    return PVM_VAL_ARR_ELEM_OFFSET (array, idx);
  else
    return PK_NULL;
}

uint64_t
pk_sizeof (pk_val val)
{
  return pvm_sizeof (val);
}
