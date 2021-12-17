/* VM library: native code patching for x86_64 .

   Copyright (C) 2017, 2019, 2020 Luca Saiu
   Updated in 2021 by Luca Saiu
   Written by Luca Saiu

   This file is part of GNU Jitter.

   GNU Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   GNU Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <jitter/jitter-fatal.h>

#include <jitter/jitter-arithmetic.h>
#include <jitter/jitter-patch.h>
#include <jitter/jitter-machine-common.h>

#include "jitter-machine.h"


void
jitter_invalidate_icache (char *from, size_t byte_no)
{
  /* This doesn't need to do anything on x86_64. */
}

/* Return the distance in bytes, from the pointed address to the destination
   address.  This is natural for branching instructions, but is in fact also
   usable to materalise any constant which happens to be expressible as the
   address of the instruction materialising plus a short offset.
   Notice that on x86_64 %rip points to the byte right *after* the instruction
   being executed, at all times. */
static jitter_int
jitter_distance_from (const char *destination_p, const char *from_p)
{
  return destination_p - from_p;
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_register (const char *immediate_pointer,
                                     unsigned int residual_register_index,
                                     const char *loading_code_to_write)
{
  int64_t immediate = * (int64_t *) immediate_pointer;
  int64_t distance
    = jitter_distance_from ((const char *) immediate, loading_code_to_write);

  if (immediate == 0)
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_0_to_64bit_residual_register_0;
      case 1:  return jitter_snippet_load_0_to_64bit_residual_register_1;
      case 2:  return jitter_snippet_load_0_to_64bit_residual_register_2;
      case 3:  return jitter_snippet_load_0_to_64bit_residual_register_3;
      default: jitter_fatal ("impossible A");
      }
  if (immediate == -1)
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_minus_1_to_64bit_residual_register_0;
      case 1:  return jitter_snippet_load_minus_1_to_64bit_residual_register_1;
      case 2:  return jitter_snippet_load_minus_1_to_64bit_residual_register_2;
      case 3:  return jitter_snippet_load_minus_1_to_64bit_residual_register_3;
      default: jitter_fatal ("impossible B");
      }
  else if (JITTER_FITS_IN_BITS_ZERO_EXTENDED (immediate, 32))
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_set_32bit_residual_register_0;
      case 1:  return jitter_snippet_set_32bit_residual_register_1;
      case 2:  return jitter_snippet_set_32bit_residual_register_2;
      case 3:  return jitter_snippet_set_32bit_residual_register_3;
      default: jitter_fatal ("impossible C");
      }
  else if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (immediate, 32))
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_set_32bit_sign_extended_residual_register_0;
      case 1:  return jitter_snippet_set_32bit_sign_extended_residual_register_1;
      case 2:  return jitter_snippet_set_32bit_sign_extended_residual_register_2;
      case 3:  return jitter_snippet_set_32bit_sign_extended_residual_register_3;
      default: jitter_fatal ("impossible C");
      }
  else if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (distance, 32))
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_set_pcrel_address_residual_register_0;
      case 1:  return jitter_snippet_set_pcrel_address_residual_register_1;
      case 2:  return jitter_snippet_set_pcrel_address_residual_register_2;
      case 3:  return jitter_snippet_set_pcrel_address_residual_register_3;
      default: jitter_fatal ("impossible D");
      }
  else
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_set_64bit_residual_register_0;
      case 1:  return jitter_snippet_set_64bit_residual_register_1;
      case 2:  return jitter_snippet_set_64bit_residual_register_2;
      case 3:  return jitter_snippet_set_64bit_residual_register_3;
      default: jitter_fatal ("impossible E");
      }
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_memory (const char *immediate_pointer,
                                   unsigned int index,
                                   const char *loading_code_to_write)
{
  if (index >= 15)
    jitter_fatal ("you have an awful lot of memory residuals: not implemented yet");
  /* FIXME: check elsewhere, at machine initialization time, that we don't have
     enough memory residuals to potentially fail here.  The check is safe to
     perform only once. */

  int64_t immediate = * (int64_t*) immediate_pointer;
  if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (immediate, 32))
    return jitter_snippet_set_32bit_sign_extended_residual_memory;
  else
    return jitter_snippet_set_64bit_residual_memory_two_32bit_stores;
}

void
jitter_patch_load_immediate_to_register (char *native_code,
                                         size_t native_code_size,
                                         const char *immediate_pointer,
                                         enum jitter_snippet_to_patch snippet)
{
  /* See the comments in machine-assembly.S about offsets. */
  switch (snippet)
    {
    case jitter_snippet_load_0_to_64bit_residual_register_0:
    case jitter_snippet_load_0_to_64bit_residual_register_1:
    case jitter_snippet_load_0_to_64bit_residual_register_2:
    case jitter_snippet_load_0_to_64bit_residual_register_3:
    case jitter_snippet_load_minus_1_to_64bit_residual_register_0:
    case jitter_snippet_load_minus_1_to_64bit_residual_register_1:
    case jitter_snippet_load_minus_1_to_64bit_residual_register_2:
    case jitter_snippet_load_minus_1_to_64bit_residual_register_3:
      /* Do nothing: these snippets do not need patching. */
      break;

    case jitter_snippet_set_32bit_residual_register_0:
    case jitter_snippet_set_32bit_residual_register_1:
    case jitter_snippet_set_32bit_residual_register_2:
    case jitter_snippet_set_32bit_residual_register_3:
    case jitter_snippet_set_32bit_sign_extended_residual_register_0:
    case jitter_snippet_set_32bit_sign_extended_residual_register_1:
    case jitter_snippet_set_32bit_sign_extended_residual_register_2:
    case jitter_snippet_set_32bit_sign_extended_residual_register_3:
      /* Since x86_64 is little-endian I can truncate a 64-bit value to its
         lower 32 bits by just taking its *first* four bytes.
         The immediate is 32-bit, coming at the end of the instruction, for
         both the movl and the movq case. */
      memcpy (native_code + native_code_size - 4, immediate_pointer, 4);
      break;

    case jitter_snippet_set_64bit_residual_register_0:
    case jitter_snippet_set_64bit_residual_register_1:
    case jitter_snippet_set_64bit_residual_register_2:
    case jitter_snippet_set_64bit_residual_register_3:
      memcpy (native_code + native_code_size - 8, immediate_pointer, 8);
      break;

    case jitter_snippet_set_pcrel_address_residual_register_0:
    case jitter_snippet_set_pcrel_address_residual_register_1:
    case jitter_snippet_set_pcrel_address_residual_register_2:
    case jitter_snippet_set_pcrel_address_residual_register_3:
      {
        uint64_t immediate = * (uint64_t *) immediate_pointer;
        int64_t distance = jitter_distance_from ((const char *) immediate,
                                                 native_code + native_code_size);
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (distance, 32))
          jitter_fatal ("%%rip-relative lea: displacement too large: %li %p", (long) distance, (void *) (unsigned long) distance);
        int32_t distance_32 = distance;
        memcpy (native_code + native_code_size - 4, & distance_32, 4);
        break;
      }

    default:
      jitter_unimplemented ("jitter_patch_load_immediate_to_register");
    }
}

void
jitter_patch_load_immediate_to_memory (char *native_code,
                                       size_t native_code_size,
                                       unsigned int memory_index,
                                       const char *immediate_pointer,
                                       enum jitter_snippet_to_patch snippet)
{
  switch (snippet)
    {
    case jitter_snippet_set_64bit_residual_memory_two_32bit_stores:
      {
        /* I have to patch two consecutive instructions with the same identical
           format; I don't want to make assumption on their size. */
        assert ((native_code_size & 1) == 0);
        size_t each_instruction_size = native_code_size / 2;

        /* Each of the two instructions has two immediates at the end: the last
           four bytes (little-endian) are the 32-bit value immediate to be stored;
           before them comes the offset from the register, as one byte, signed. */
        off_t value_offset = each_instruction_size - 4;
        off_t offset_offset = each_instruction_size - 5;

        /* This is a little-endian architecture: the low half comes first, and
           has a smaller offset from the base. */
        int8_t low_half_offset = memory_index * 8;
        int8_t high_half_offset = low_half_offset + 4;
        uint32_t low_half_value = * (uint32_t *) immediate_pointer;
        uint32_t high_half_value = * (uint32_t *) (immediate_pointer + 4);

        /* Patch immediates first for the low half, then for the high half. */
        memcpy (native_code + value_offset,
                & low_half_value, 4);
        memcpy (native_code + offset_offset,
                & low_half_offset, 1);
        memcpy (native_code + value_offset + each_instruction_size,
                & high_half_value, 4);
        memcpy (native_code + offset_offset + each_instruction_size,
                & high_half_offset, 1);
        break;
      }

    case jitter_snippet_set_32bit_sign_extended_residual_memory:
      {
        /* Similar to the previous case.  [FIXME: factor?].  Patch two
           immediates into the end of the instruction.  Here I am generating
           an instruction storing a 64-bit value, but in effect only mentioning
           its low half -- this depends on endianness -- in terms of offset, and
           of the value to be stored. */
        int8_t low_half_offset = memory_index * 8;
        uint32_t low_half_value = * (uint32_t *) immediate_pointer;
        off_t offset_offset = native_code_size - 5;
        off_t value_offset = native_code_size - 4;
        memcpy (native_code + offset_offset, & low_half_offset, 1);
        memcpy (native_code + value_offset, & low_half_value, 4);
        break;
      }

    default:
      jitter_unimplemented ("jitter_patch_load_immediate_to_memory");
    }
}

/* I keep this conditional to be able to test with patch-in disabled, by just
   commenting one line in the header. */
#ifdef JITTER_HAVE_PATCH_IN
enum jitter_snippet_to_patch
jitter_snippet_for_patch_in (const struct jitter_patch_in_descriptor *dp)
{
  jitter_uint patch_in_case = dp->patch_in_case;
  switch (patch_in_case)
    {
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL:
      return jitter_snippet_jump_unconditional_32bit_offset;

    case JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY:
      return jitter_snippet_empty_after_conditional_jump_32bit_offset;

    case JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK:
      return jitter_snippet_call_32bit_offset;

    default:
      jitter_fatal ("jitter_snippet_for_patch_in: unsupported patch-in case %li",
                    (long) patch_in_case);
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN

/* I keep this conditional to be able to test with patch-in disabled, by just
   commenting one line in the header. */
#ifdef JITTER_HAVE_PATCH_IN
void
jitter_patch_patch_in (char *native_code,
                       const char *immediate_pointer,
                       const struct jitter_patch_in_descriptor *descriptor,
                       enum jitter_snippet_to_patch snippet)
{
  char *jump_target = * (char**) immediate_pointer;

  switch (snippet)
    {
    case jitter_snippet_jump_unconditional_32bit_offset:
      {
        /* On x86_64 jump targets are encoded as signed distances from the *end*
           of the jumping instruction--in this case in 32 bits.  This jmp
           instruction takes 5 bytes, of which the last 4 are the offset. */
        int32_t offset = jump_target - (native_code + 5);
        memcpy (native_code + 1, & offset, sizeof (offset));
        break;
      }

    case jitter_snippet_empty_after_conditional_jump_32bit_offset:
      {
        /* On x86_64 jump targets are encoded as signed distances from the *end*
           of the jumping instruction--in this case in 32 bits.  This patch-in
           records the *end* of the conditional branching instruction,
           therefore, independently from how many bytes this conditional jump
           instruction takes (it should be 5), the address to patch in comes
           right *before* the native_code pointer.
           By using only one (trivial) snippet for every conditional jump I
           have moved the complexity from patching to instruction generation. */
        int32_t offset = jump_target - native_code;
        memcpy (native_code - 4, & offset, sizeof (offset));
        break;
      }

    case jitter_snippet_call_32bit_offset:
      {
        /* On x86_64 jump targets are encoded as signed distances from the *end*
           of the jumping instruction--in this case in 32 bits.  This call
           instruction takes 5 bytes, of which the last 4 are the offset. */
        int32_t offset = jump_target - (native_code + 5);
        memcpy (native_code + 1, & offset, sizeof (offset));
        break;
      }

    default:
      jitter_fatal ("jitter_patch_patch_in: unsupported snippet %li",
                    (long) snippet);
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN
