/* VM library: native code patching for SPARC (either bitness).

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

#include <jitter/jitter-arithmetic.h>
#include <jitter/jitter-fatal.h>

#include <jitter/jitter.h>
#include <jitter/jitter-patch.h>
#include <jitter/jitter-machine-common.h>

#include "jitter-machine.h"


void
jitter_invalidate_icache (char *from, size_t byte_no)
{
  /* This doesn't need to do anything on SPARC.  The GCC builtin
     __builtin___clear_cache seems sufficient, on the machines I've tested. */
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_register (const char *immediate_pointer,
                                     unsigned int residual_register_index,
                                     const char *loading_code_to_write)
{
  if (residual_register_index >= JITTER_RESIDUAL_REGISTER_NO)
    jitter_fatal ("invalid residual register index");

  jitter_uint immediate = * (jitter_int*) immediate_pointer;
  if (JITTER_FITS_IN_BITS_SIGN_EXTENDED ((jitter_int) immediate, 13))
    return jitter_snippet_load_or_to_reg_0 + 4 * residual_register_index;
  else if (JITTER_FITS_IN_BITS_ZERO_EXTENDED ((jitter_int) immediate, 32))
    return jitter_snippet_load_sethi_or_to_reg_0 + 4 * residual_register_index;
  // FIXME: also use the sethi-only version.
  else
    return jitter_snippet_load_64bit_to_reg_0 + 4 * residual_register_index;
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_memory (const char *immediate_pointer,
                                   unsigned int index,
                                   const char *loading_code_to_write)
{
  jitter_fatal ("SPARC residual memory: not implemented yet");
}

void
jitter_patch_load_immediate_to_register (char *native_code,
                                         size_t native_code_size,
                                         const char *immediate_pointer,
                                         enum jitter_snippet_to_patch snippet)
{
  jitter_uint u = * (jitter_uint *) immediate_pointer;
  uint32_t low13_mask = ((1u << 13) - 1);
  uint32_t low22_mask = ((1u << 22) - 1);
  uint32_t low10_mask = ((1u << 10) - 1);
  uint32_t *first_instruction_p = (uint32_t *) native_code;

  /* See the comments in machine-assembly.S about offsets. */
  switch (snippet)
    {
    case jitter_snippet_load_or_to_reg_0:
    case jitter_snippet_load_or_to_reg_1:
    case jitter_snippet_load_or_to_reg_2:
    case jitter_snippet_load_or_to_reg_3:
    case jitter_snippet_load_or_to_reg_4:
    case jitter_snippet_load_or_to_reg_5:
    case jitter_snippet_load_or_to_reg_6:
    case jitter_snippet_load_or_to_reg_7:
    case jitter_snippet_load_or_to_reg_8:
    case jitter_snippet_load_or_to_reg_9:
    case jitter_snippet_load_or_to_reg_10:
    case jitter_snippet_load_or_to_reg_11:
      {
        uint32_t low13 = u & low13_mask;
        uint32_t instruction = * first_instruction_p;
        instruction = (instruction & ~ low13_mask) | low13;
        * first_instruction_p = instruction;
      }
      break;

    case jitter_snippet_load_sethi_or_to_reg_0:
    case jitter_snippet_load_sethi_or_to_reg_1:
    case jitter_snippet_load_sethi_or_to_reg_2:
    case jitter_snippet_load_sethi_or_to_reg_3:
    case jitter_snippet_load_sethi_or_to_reg_4:
    case jitter_snippet_load_sethi_or_to_reg_5:
    case jitter_snippet_load_sethi_or_to_reg_6:
    case jitter_snippet_load_sethi_or_to_reg_7:
    case jitter_snippet_load_sethi_or_to_reg_8:
    case jitter_snippet_load_sethi_or_to_reg_9:
    case jitter_snippet_load_sethi_or_to_reg_10:
    case jitter_snippet_load_sethi_or_to_reg_11:
      {
        uint32_t low10 = u & low10_mask;
        uint32_t high22 = u >> 10;

        uint32_t sethi_instruction = first_instruction_p [0];
        uint32_t or_instruction = first_instruction_p [1];
        sethi_instruction = (sethi_instruction & ~ low22_mask) | high22;
        or_instruction = (or_instruction & ~ low13_mask) | low10;
        first_instruction_p [0] = sethi_instruction;
        first_instruction_p [1] = or_instruction;
      }
      break;

    case jitter_snippet_load_sethi_to_reg_0:
    case jitter_snippet_load_sethi_to_reg_1:
    case jitter_snippet_load_sethi_to_reg_2:
    case jitter_snippet_load_sethi_to_reg_3:
    case jitter_snippet_load_sethi_to_reg_4:
    case jitter_snippet_load_sethi_to_reg_5:
    case jitter_snippet_load_sethi_to_reg_6:
    case jitter_snippet_load_sethi_to_reg_7:
    case jitter_snippet_load_sethi_to_reg_8:
    case jitter_snippet_load_sethi_to_reg_9:
    case jitter_snippet_load_sethi_to_reg_10:
    case jitter_snippet_load_sethi_to_reg_11:
      {
        jitter_fatal ("sethi: patching unimplemented");
      }
      break;

    case jitter_snippet_load_64bit_to_reg_0:
    case jitter_snippet_load_64bit_to_reg_1:
    case jitter_snippet_load_64bit_to_reg_2:
    case jitter_snippet_load_64bit_to_reg_3:
    case jitter_snippet_load_64bit_to_reg_4:
    case jitter_snippet_load_64bit_to_reg_5:
    case jitter_snippet_load_64bit_to_reg_6:
    case jitter_snippet_load_64bit_to_reg_7:
    case jitter_snippet_load_64bit_to_reg_8:
    case jitter_snippet_load_64bit_to_reg_9:
    case jitter_snippet_load_64bit_to_reg_10:
    case jitter_snippet_load_64bit_to_reg_11:
      {
#if   JITTER_SIZEOF_VOID_P == 4
        jitter_fatal ("attempting to patch a 64-bit loading snippet on 32 bit");
#elif JITTER_SIZEOF_VOID_P == 8
        uint32_t low_half = (int32_t) u;
        uint32_t high_half = (int32_t) (u >> 32);

        uint32_t high22_low_half = low_half >> 10;
        uint32_t high22_high_half = high_half >> 10;
        uint32_t low13_mask = ((1u << 13) - 1);
        uint32_t low22_mask = ((1u << 22) - 1);
        uint32_t low10_mask = ((1u << 10) - 1);
        uint32_t low10_low_half = low_half & low10_mask;
        uint32_t low10_high_half = high_half & low10_mask;

        uint32_t sethi1_instruction = first_instruction_p [0];
        uint32_t sethi2_instruction = first_instruction_p [1];
        uint32_t or1_instruction = first_instruction_p [2];
        uint32_t or2_instruction = first_instruction_p [3];
        sethi1_instruction = (sethi1_instruction & ~ low22_mask) | high22_high_half;
        sethi2_instruction = (sethi2_instruction & ~ low22_mask) | high22_low_half;
        or1_instruction = (or1_instruction & ~ low13_mask) | low10_high_half;
        or2_instruction = (or2_instruction & ~ low13_mask) | low10_low_half;
        first_instruction_p [0] = sethi1_instruction;
        first_instruction_p [1] = sethi2_instruction;
        first_instruction_p [2] = or1_instruction;
        first_instruction_p [3] = or2_instruction;
#else
# error "word size not 32 or 64 bits"
#endif // #if   JITTER_SIZEOF_VOID_P ==
      }
      break;

    default:
      jitter_fatal ("jitter_patch_load_immediate_to_register: unknown snippet");
    }
}

void
jitter_patch_load_immediate_to_memory (char *native_code,
                                       size_t native_code_size,
                                       unsigned int memory_index,
                                       const char *immediate_pointer,
                                       enum jitter_snippet_to_patch snippet)
{
  jitter_fatal ("SPARC residual memory: not implemented yet");
}

#ifdef JITTER_HAVE_PATCH_IN
enum jitter_snippet_to_patch
jitter_snippet_for_patch_in (const struct jitter_patch_in_descriptor *dp)
{
  jitter_uint patch_in_case = dp->patch_in_case;
  switch (patch_in_case)
    {
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL:
      /* This has some subtle problems, sometimes generating an incorrect
         immediate; I'm not sure where my mistake is.  Anyway the other patch-in
         snippet below works reliably, despite its narrower range. */
      //return jitter_snippet_branch_unconditional_21bits_offset;
      return jitter_snippet_branch_unconditional_18bits_offset;

    default:
      jitter_fatal ("jitter_snippet_for_patch_in: unsupported patch-in case");
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
    case jitter_snippet_branch_unconditional_18bits_offset:
      {
        char *jump_target = * (char**) immediate_pointer;
        char *jump_address = native_code;

        jitter_uint jump_target_u = (jitter_uint) jump_target;
        jitter_uint jump_address_u = (jitter_uint) jump_address;

        /* Check that the jump origin and target addresses are 32-bit
           aligned. */
        if (jump_address_u % 4 != 0)
          jitter_fatal ("jmpl instruction not 32-bit aligned");
        if (jump_target_u % 4 != 0)
          jitter_fatal ("jmpl instruction target not 32-bit aligned");

        /* Compute the branch distance, and check that the destination is
           near enough to fit in the instruction immediate. */
        jitter_int jump_distance
          = (jitter_int) jump_target_u - (jitter_int) jump_address_u;
        jitter_int shifted_jump_distance = jump_distance >> 2;
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (shifted_jump_distance, 16))
          jitter_fatal ("branch too far");

        /* Compute the immediate, in its split encoding.  Its 2 high bits are at
           positions [21,20], while its low 14 are at [13,0].  The entire
           displacement is represented without the two least significant
           bits. */
        uint32_t low14_mask = ((1 << 14) - 1);
        uint32_t immediate_low14 = shifted_jump_distance & low14_mask;
        uint32_t low2_mask = ((1 << 2) - 1);
        uint32_t high2_mask = low2_mask << 20;
        uint32_t immediate_high2 =
          ((shifted_jump_distance >> 14) & low2_mask) << 20;
        uint32_t immediate = immediate_high2 | immediate_low14;

        /* Patch the branching instruction.  We need two replace the two split
           fields encoding the displacement, but we can keep the rest of the
           instruction coming from the assembly snippet. */
        uint32_t old_instruction = * (uint32_t *) native_code;
        uint32_t instruction
          = ((old_instruction & ~ low14_mask) & ~ high2_mask) | immediate;
        * (uint32_t *) native_code = instruction;
        break;
      }

    /* FIXME: this case is currently not used, as the destination is not
       always correct; see the comment at the end of the case. */
    case jitter_snippet_branch_unconditional_21bits_offset:
      {
        char *jump_target = * (char**) immediate_pointer;
        char *jump_address = native_code;

        jitter_uint jump_target_u = (jitter_uint) jump_target;
        jitter_uint jump_address_u = (jitter_uint) jump_address;

        /* Check that the jump origin and target addresses are 32-bit
           aligned. */
        if (jump_address_u % 4 != 0)
          jitter_fatal ("jmpl instruction not 32-bit aligned");
        if (jump_target_u % 4 != 0)
          jitter_fatal ("jmpl instruction target not 32-bit aligned");

        /* Compute the branch distance, and check that the destination is
           near enough to fit in the instruction immediate. */
        jitter_int jump_distance
          = (jitter_int) jump_target_u - (jitter_int) jump_address_u;
        jitter_int shifted_jump_distance = jump_distance >> 2;
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (shifted_jump_distance, 19))
          jitter_fatal ("branch too far");

        /* Compute the immediate. */
        uint32_t low19_mask = ((1 << 19) - 1);
        uint32_t immediate19 = shifted_jump_distance & low19_mask;

        /* Patch the branching instruction.  We only have to replace the
           immediate; the other fields we can copy from the current
           instruction, which came from the assembly snippet.  */
        uint32_t old_instruction = * (uint32_t *) native_code;
        uint32_t instruction = (old_instruction & ~ low19_mask) | immediate19;
        * (uint32_t *) native_code = instruction;

        /* The result is sometimes wrong.  When I disassemble I sometimes see
           a destination off by exactly 0x200000 bytes; for example
             0x0000004000ed4008 when expecting
             0x0000004000cd4008 .
           I must have misunderstood some detail in the instruction encoding.
           The code here seems easy enough. */
        break;
      }

    default:
      jitter_fatal ("jitter_patch_patch_in: unsupported snippet %li",
                    (long) snippet);
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN
