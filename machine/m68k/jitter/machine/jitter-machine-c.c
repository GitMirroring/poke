/* VM library: native code patching for m68k .

   Copyright (C) 2017, 2019, 2020 Luca Saiu
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
#include <jitter/jitter-bitwise.h>
#include <jitter/jitter-patch.h>
#include <jitter/jitter-machine-common.h>

#include <jitter/jitter-fatal.h>

#include "jitter-machine.h"




/* Instruction formats for m68k.
 * ************************************************************************** */

/* The m68k is big-endian only.  Here I am defining just enough of the
   instruction formats to be able to patch instructions in a convenient way; the
   fields not to be patched are not specified in detail. */

/* A moveq instruction, from immediate to register. */
__attribute__ ((packed))
struct jitter_m68k_moveq_instruction
{
  unsigned not_to_be_patched:  8;
  unsigned immediate:          8;
};

/* A movel instruction, from immediate to register. */
__attribute__ ((packed))
struct jitter_m68k_movel_immediate_register_instruction
{
  unsigned not_to_be_patched:  16;
  unsigned immediate:          32;
};

/* A clrl instruction with a register+offset destination operand. */
__attribute__ ((packed))
struct jitter_m68k_clrl_memory_instruction
{
  unsigned not_to_be_patched:  16;
  unsigned offset:             16;
};

/* A movel instruction with a literl source operand and a register+offset
   destination operand. */
__attribute__ ((packed))
struct jitter_m68k_movel_immediate_memory_instruction
{
  unsigned not_to_be_patched:  16;
  unsigned immediate:          32;
  unsigned offset:             16;
};

/* Not all m68k branch instructions have this form, but Jitter only uses these.
   In particular the actually used branch displacement is 32-bits, and the 8-bit
   displacement field must be set to 0xff to mean that it is not used.  Another
   alternative for medium-sized displacements would be a 16-bit field specified
   in place of the beginning of the 32-bit displacement, which is used when the
   8-bit displacement is set to 0x00.  Jitter never uses the 16-bit displacement
   either. */
__attribute__ ((packed))
struct jitter_m68k_branch_instruction
{
  unsigned not_to_be_patched:  8;
  unsigned displacement_8:     8;
  unsigned displacement_32:    32;
};




/* Patch functions.
 * ************************************************************************** */

void
jitter_invalidate_icache (char *from, size_t byte_no)
{
  /* I am assuming that this does not need to do anything on m68k.  Not
     tested on real hardware yet. */
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_register (const char *immediate_pointer,
                                     unsigned int residual_register_index,
                                     const char *loading_code_to_write)
{
  jitter_int immediate = * (jitter_int *) immediate_pointer;
  if (immediate == 0)
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_zero_to_register_0;
      case 1:  return jitter_snippet_load_zero_to_register_1;
      case 2:  return jitter_snippet_load_zero_to_register_2;
      case 3:  return jitter_snippet_load_zero_to_register_3;
      case 4:  return jitter_snippet_load_zero_to_register_4;
      case 5:  return jitter_snippet_load_zero_to_register_5;
      default: jitter_fatal ("impossible");
      }
  else if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (immediate, 8))
    switch (residual_register_index)
      {
      case 0:   return jitter_snippet_load_8bit_sign_extended_to_register_0;
      case 1:   return jitter_snippet_load_8bit_sign_extended_to_register_1;
      case 2:   return jitter_snippet_load_8bit_sign_extended_to_register_2;
      case 3:   return jitter_snippet_load_8bit_sign_extended_to_register_3;
      case 4:   return jitter_snippet_load_8bit_sign_extended_to_register_4;
      case 5:   return jitter_snippet_load_8bit_sign_extended_to_register_5;
      default:  jitter_fatal ("impossible");
      }
  else
    switch (residual_register_index)
      {
      case 0:   return jitter_snippet_load_32bit_to_register_0;
      case 1:   return jitter_snippet_load_32bit_to_register_1;
      case 2:   return jitter_snippet_load_32bit_to_register_2;
      case 3:   return jitter_snippet_load_32bit_to_register_3;
      case 4:   return jitter_snippet_load_32bit_to_register_4;
      case 5:   return jitter_snippet_load_32bit_to_register_5;
      default:  jitter_fatal ("impossible");
      }
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_memory (const char *immediate_pointer,
                                   unsigned int index,
                                   const char *loading_code_to_write)
{
  /* This checks against a conservative bound: the actual limit, with a 16-bit
     offset, is much higher; however it makes no sense to support instructions
     with so many arguments which would not work anyway on most other
     architectures. */
  if (index >= 1000)
    jitter_fatal ("you have unreasonably many memory residuals: not supported");
  /* FIXME: check elsewhere, at machine initialization time, that we don't have
     enough memory residuals to potentially fail here.  The check is safe to
     perform only once. */

  jitter_int immediate = * (jitter_int *) immediate_pointer;
  if (immediate == 0)
    return jitter_snippet_load_zero_to_memory;
  else
    return jitter_snippet_load_32bit_to_memory;
}

void
jitter_patch_load_immediate_to_register (char *native_code,
                                         size_t native_code_size,
                                         const char *immediate_pointer,
                                         enum jitter_snippet_to_patch snippet)
{
  jitter_int immediate = * (jitter_int *) immediate_pointer;
  switch (snippet)
    {
    case jitter_snippet_load_zero_to_register_0:
    case jitter_snippet_load_zero_to_register_1:
    case jitter_snippet_load_zero_to_register_2:
    case jitter_snippet_load_zero_to_register_3:
    case jitter_snippet_load_zero_to_register_4:
    case jitter_snippet_load_zero_to_register_5:
      /* There is nothing to patch. */
      break;

    case jitter_snippet_load_8bit_sign_extended_to_register_0:
    case jitter_snippet_load_8bit_sign_extended_to_register_1:
    case jitter_snippet_load_8bit_sign_extended_to_register_2:
    case jitter_snippet_load_8bit_sign_extended_to_register_3:
    case jitter_snippet_load_8bit_sign_extended_to_register_4:
    case jitter_snippet_load_8bit_sign_extended_to_register_5:
      {
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (immediate, 8))
          jitter_fatal ("jitter_patch_load_immediate_to_register: impossible");
        struct jitter_m68k_moveq_instruction *moveq
          = (struct jitter_m68k_moveq_instruction *) native_code;
        moveq->immediate = immediate & 0xff;
        break;
      }

    case jitter_snippet_load_32bit_to_register_0:
    case jitter_snippet_load_32bit_to_register_1:
    case jitter_snippet_load_32bit_to_register_2:
    case jitter_snippet_load_32bit_to_register_3:
    case jitter_snippet_load_32bit_to_register_4:
    case jitter_snippet_load_32bit_to_register_5:
      {
        struct jitter_m68k_movel_immediate_register_instruction *movel
          = ((struct jitter_m68k_movel_immediate_register_instruction *)
             native_code);
        movel->immediate = immediate;
        break;
      }

    default:
      jitter_fatal ("jitter_patch_load_immediate_to_register: case not yet implemented");
    }
}

void
jitter_patch_load_immediate_to_memory (char *native_code,
                                       size_t native_code_size,
                                       unsigned int memory_index,
                                       const char *immediate_pointer,
                                       enum jitter_snippet_to_patch snippet)
{
  /* All the memory instructions we support encode the offset the same way. */
  jitter_int offset = memory_index * 4;
  if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (offset, 16))
    jitter_fatal ("jitter_patch_load_immediate_to_memory: offset too "
                  "large: this should never happen");

  switch (snippet)
    {
    case jitter_snippet_load_zero_to_memory:
      {
        struct jitter_m68k_clrl_memory_instruction *clrl
          = ((struct jitter_m68k_clrl_memory_instruction *) native_code);
        clrl->offset = offset;
        break;
      }

    case jitter_snippet_load_32bit_to_memory:
      {
        jitter_uint immediate = * (jitter_uint *) immediate_pointer;

        struct jitter_m68k_movel_immediate_memory_instruction *movel
          = ((struct jitter_m68k_movel_immediate_memory_instruction *)
             native_code);
        movel->immediate = immediate;
        movel->offset = offset;
        break;
      }

    default:
      jitter_fatal ("jitter_patch_load_immediate_to_memory: impossible");
    }
}

#ifdef JITTER_HAVE_PATCH_IN
enum jitter_snippet_to_patch
jitter_snippet_for_patch_in (const struct jitter_patch_in_descriptor *dp)
{
  jitter_uint patch_in_case = dp->patch_in_case;
  switch (patch_in_case)
    {
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL:
      return jitter_snippet_branch_unconditional_32bit_offset;
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK:
      return jitter_snippet_branch_and_link_32bit_offset;
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY:
      return jitter_snippet_branch_conditional_32bit_offset;

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
  /* The distance is computed from the end of the first 16 bits of the
     instruction; technically speaking the 32-bit displacement is optional. */
  char *target = * (char **) immediate_pointer;
  int32_t distance = target - native_code;
  distance -= 2; /* Account for the first 16 bits of the instruction. */
  switch (snippet)
    {
    /* The branching instructions we patch all share the same format. */
    case jitter_snippet_branch_unconditional_32bit_offset:
    case jitter_snippet_branch_and_link_32bit_offset:
    case jitter_snippet_branch_conditional_32bit_offset:
      {
        struct jitter_m68k_branch_instruction *branch
          = ((struct jitter_m68k_branch_instruction *) native_code);
        branch->displacement_8 = 0xff; /* Do not use the 8-bit displacment. */
        branch->displacement_32 = distance;
        break;
      }
    default:
      jitter_fatal ("jitter_patch_patch_in: invalid patch-in");
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN
