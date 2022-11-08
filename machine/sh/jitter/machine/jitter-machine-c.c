/* VM library: native code patching for SH .

   Copyright (C) 2017, 2019 Luca Saiu
   Updated in 2020 by Luca Saiu
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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <jitter/jitter-fatal.h>

#include <jitter/jitter.h>
#include <jitter/jitter-arithmetic.h>
#include <jitter/jitter-patch.h>
#include <jitter/jitter-machine-common.h>

#include "jitter-machine.h"


void
jitter_invalidate_icache (char *from, size_t byte_no)
{
  /* FIXME: I don't know if I need to do anything on SH, since I can't test on
     real hardware.  For the time being I'll assume that the GCC builtin
     __builtin___clear_cache is sufficient. */
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_register (const char *immediate_pointer,
                                     unsigned int residual_register_index,
                                     const char *loading_code_to_write)
{
  int32_t immediate = * (int32_t*) immediate_pointer;

  if (residual_register_index >= JITTER_RESIDUAL_REGISTER_NO)
    jitter_fatal ("jitter_snippet_for_loading_register: unhandled register;"
                  " this should not happen");

  /* The rest of this function assumes that the order of the enum
     jitter_snippet_to_patch cases doesn't change. */

  /* The SH can load 8-bit sign-extended values to any register, in one
     instruction.  That's the easiest and fastest way. */
  if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (immediate, 8))
    return (jitter_snippet_load_signed_8bit_to_register_0
            + residual_register_index);

  /* It's much less convenient, but I can load a sign-extended 16-bit value.
     This takes five instructions if the 8th bit of the immediate is 1, four
     otherwise.  I wonder whether the fallback snippets below are actually
     faster. */
  if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (immediate, 16))
    {
      if ((uint32_t) immediate & 128)
        return (jitter_snippet_load_signed_16bit_8th_bit_1_to_register_0
                + residual_register_index);
      else
        return (jitter_snippet_load_signed_16bit_8th_bit_0_to_register_0
                + residual_register_index);
    }

  /* This is a fallback solution doing a PC-relative load, and branching over
     the datum.  Probably not very fast, but it works for any immediate. */
  bool aligned = ((jitter_uint) loading_code_to_write) % 4 == 0;
  int misalignment_addend = aligned ? 0 : +1;
  return (jitter_snippet_load_pcrel_to_register_0_pc_aligned
          + 2 * residual_register_index
          + misalignment_addend);
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_memory (const char *immediate_pointer,
                                   unsigned int index,
                                   const char *loading_code_to_write)
{
  jitter_fatal ("jitter_snippet_for_loading_memory: still unimplemented on SH");
}

/* Replace the immediate in the pointed SH instruction, assuming that the
   immediate takes the least significant byte of the two, indepdendently from
   the endianness. */
static void
jitter_patch_sh_immediate (uint16_t *instruction_p,
                           uint8_t new_immediate)
{
  /* Fetch a copy of the instruction.  In order to not rely on endianness, let's
     just fetch and replace the whole 16-bit instruction. */
  uint16_t instruction = * instruction_p;

  /* Mask away the old argument, and put ours in. */
  instruction &= ~ 0xff;
  instruction |= new_immediate;

  /* Store the modified instruction back. */
  * instruction_p = instruction;
}

void
jitter_patch_load_immediate_to_register (char *native_code,
                                         size_t native_code_size,
                                         const char *immediate_pointer,
                                         enum jitter_snippet_to_patch snippet)
{
  switch (snippet)
    {
    case jitter_snippet_load_signed_8bit_to_register_0:
    case jitter_snippet_load_signed_8bit_to_register_1:
    case jitter_snippet_load_signed_8bit_to_register_2:
      {
        /* Get the immediate.  If we're using this snippet it fits in 8 bits
           sign-extended; but here we want the low bits set to zero, since we
           are setting the field thru a bitmask.  */
        const int32_t *immediate_p = (const int32_t *) immediate_pointer;
        const uint8_t immediate = * immediate_p;

        /* Replace the immediate of the last (16-bit) instruction. */
        jitter_patch_sh_immediate ((uint16_t *)
                                   (native_code + native_code_size - 2),
                                   immediate);
        break;
      }
    case jitter_snippet_load_signed_16bit_8th_bit_1_to_register_0:
    case jitter_snippet_load_signed_16bit_8th_bit_1_to_register_1:
    case jitter_snippet_load_signed_16bit_8th_bit_1_to_register_2:
    case jitter_snippet_load_signed_16bit_8th_bit_0_to_register_0:
    case jitter_snippet_load_signed_16bit_8th_bit_0_to_register_1:
    case jitter_snippet_load_signed_16bit_8th_bit_0_to_register_2:
      {
        /* Get the immediate, and only keep its low 16 bits. */
        const int32_t *immediate_p = (const int32_t *) immediate_pointer;
        const int16_t immediate = * immediate_p;

        /* Split the immediate into two bytes.  The high byte will be
           sign-extended in the end, but here we work better with unsigned
           values since we are playing with bit masks. */
        const uint8_t high_byte = immediate >> 8;
        const uint8_t low_byte = ((uint16_t) immediate) & 0xff;

        /* Replace the immediate arguments for the two mov instructions; they
           are the first and the second instruction in the snippet.  The high
           byte is written first by the snippet, with a sign-extending mov ; the
           low one is zero-extended and or-ed in after shifting left logically,
           which leaves zeroes on the right. */
        uint16_t *instructions = (uint16_t*) native_code;
        jitter_patch_sh_immediate (instructions + 0, high_byte);
        jitter_patch_sh_immediate (instructions + 1, low_byte);

        /* Old alternative version using r0. */
        /* /\* Replace the immediate arguments for the mov (second) and or (fourth) */
        /*    instructions.  The high byte is written first by the snippet, with a */
        /*    sign-extending mov ; the low one is or-ed in after shifting left */
        /*    logically, which leaves zeroes on the right. *\/ */
        /* uint16_t *instructions = (uint16_t*) native_code; */
        /* jitter_patch_sh_immediate (instructions + 1, high_byte); */
        /* jitter_patch_sh_immediate (instructions + 3, low_byte); */
        break;
      }

    case jitter_snippet_load_pcrel_to_register_0_pc_aligned:
    case jitter_snippet_load_pcrel_to_register_0_pc_misaligned:
    case jitter_snippet_load_pcrel_to_register_1_pc_aligned:
    case jitter_snippet_load_pcrel_to_register_1_pc_misaligned:
    case jitter_snippet_load_pcrel_to_register_2_pc_aligned:
    case jitter_snippet_load_pcrel_to_register_2_pc_misaligned:
      {
        /* This snippet ends with a datum .long, of course skipped by the actual
           instructions, and that is what we need to patch.  The datum word is
           32 bit, the same as two SH instructions. */
        char *datum_p = native_code + native_code_size - 4;
        memcpy (datum_p, immediate_pointer, 4);
        break;
      }
    default:
      jitter_fatal ("jitter_patch_load_immediate_to_register:"
                    " unhandled case; this should not happen");
    }
}

void
jitter_patch_load_immediate_to_memory (char *native_code,
                                       size_t native_code_size,
                                       unsigned int memory_index,
                                       const char *immediate_pointer,
                                       enum jitter_snippet_to_patch snippet)
{
  jitter_fatal ("jitter_patch_load_immediate_to_memory: still unimplemented on SH");
}

#ifdef JITTER_HAVE_PATCH_IN
enum jitter_snippet_to_patch
jitter_snippet_for_patch_in (const struct jitter_patch_in_descriptor *dp)
{
  jitter_uint patch_in_case = dp->patch_in_case;
  switch (patch_in_case)
    {
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL:
      return jitter_snippet_branch_unconditional_13bit_offset;

    case JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK:
      return jitter_snippet_branch_and_link_13bit_offset;

    default:
      jitter_fatal ("jitter_patch_patch_in: this should never happen");
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
  switch (snippet)
    {
    case jitter_snippet_branch_unconditional_13bit_offset:
    case jitter_snippet_branch_and_link_13bit_offset:
      {
        char *jump_target = * (char**) immediate_pointer;
        char *jump_address = native_code;
        char *delay_slot_end_address = jump_address + 4;

        /* Check that the jump origin and target addresses are 16-bit
           aligned. */
        uint32_t jump_target_u = (uint32_t) (jitter_uint) jump_target;
        uint32_t jump_address_u = (uint32_t) (jitter_uint) jump_address;
        if (jump_address_u % 2 != 0)
          jitter_fatal ("unconditional branch instruction not 16-bit aligned");
        if (jump_target_u % 2 != 0)
          jitter_fatal ("unconditional branch target not 16-bit aligned");

        /* Compute the shifted and masked 12-bit signed branch offset (the
           lowest bit, always zero, is not represented) from the end of the
           delay slot; fail if doesn't fit in the instruction. */
        uint16_t low_12_bits_mask = (1u << 12u) - 1;
        int64_t jump_offset = jump_target - delay_slot_end_address;
        int64_t jump_offset_shifted_64bit = jump_offset >> 1;
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (jump_offset_shifted_64bit, 12))
          jitter_fatal ("conditional branch target too far");
        uint16_t jump_offset_shifted_masked
          = jump_offset_shifted_64bit & low_12_bits_mask;

        /* Compute the patched instruction, by replacing the shifted offset into
           the old instruction.  The displacement is in the 12 least significant
           bits of the first instruction of the snippet. */
        uint16_t old_instruction = * (uint16_t*) native_code;
        uint16_t instruction
          = ((old_instruction & ~ low_12_bits_mask)
             | jump_offset_shifted_masked);

        /* Write the patched instruction in place of the original instruction. */
        * (uint16_t *) native_code = instruction;
        break;
      }

    default:
      jitter_fatal ("jitter_patch_patch_in: this should never happen");
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN
