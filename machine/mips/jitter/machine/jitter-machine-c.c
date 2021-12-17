/* VM library: native code patching for MIPS .

   Copyright (C) 2017, 2019, 2020, 2021 Luca Saiu
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


/* Instruction encoding.
 * ************************************************************************** */

/* In the specification "MIPS bit designations are always little-endian"; here I
   must explicitly reverse the field order for big-endian configurations.  I
   wish there was a prettier way to do it, but luckily the instruction encodings
   to patch are very few in number. */

/* Expand to a struct definition with the given name, suitable to represent an
   instruction encoded with an immediate of the given represented size (in the
   base of branch displacements two low 0 bits may not be represented) in the
   little-endian-low bits. */
#if JITTER_WORDS_BIGENDIAN
# define JITTER_MIPS_INSTRUCTION_STRUCT(struct_name, offset_represented_width)  \
    __attribute__ ((packed))                                                    \
    struct struct_name                                                          \
    {                                                                           \
      /* Other fields, including the opcode. */                                 \
      unsigned uninteresting  : (32 - (offset_represented_width));              \
      /* The displacement, of which two low bits are not represented. */        \
      unsigned immediate      : (offset_represented_width);                     \
    };
#else /* little-endian */
# define JITTER_MIPS_INSTRUCTION_STRUCT(struct_name, offset_represented_width)  \
    __attribute__ ((packed))                                                    \
    struct struct_name                                                          \
    {                                                                           \
      /* The same fields above, swapped for the little-endian case. */          \
      unsigned immediate      : (offset_represented_width);                     \
      unsigned uninteresting  : (32 - (offset_represented_width));              \
    };
#endif // #if JITTER_WORDS_BIGENDIAN

/* Encoding for the instructions we have to patch. */

/* An or, xor or add instruction having a 16-bit immediate. */
JITTER_MIPS_INSTRUCTION_STRUCT (jitter_mips_16_bit_immediate_instruction, 16);

/* An addiupc instruction having a 21-bit immediate, of which the low 19 bits
   are encoded. */
JITTER_MIPS_INSTRUCTION_STRUCT (jitter_mips_21_bit_immediate_instruction, 19);

/* A branch instruction having a 28-bit immediate displacement, of which the low
   26 bits are encoded. */
JITTER_MIPS_INSTRUCTION_STRUCT (jitter_mips_28_bit_branch_instruction, 26);

/* A branch instruction having a 18-bit immediate displacement, of which the low
   16 bits are encoded. */
JITTER_MIPS_INSTRUCTION_STRUCT (jitter_mips_18_bit_branch_instruction, 16);

/* A branch instruction having a 23-bit immediate displacement, of which the low
   21 bits are encoded. */
JITTER_MIPS_INSTRUCTION_STRUCT (jitter_mips_23_bit_branch_instruction, 21);




/* Icache invalidation.
 * ************************************************************************** */

void
jitter_invalidate_icache (char *from, size_t byte_no)
{
  /* This doesn't need to do anything on MIPS.  The GCC builtin
     __builtin___clear_cache seems sufficient, on the machines I've tested. */
}




/* Snippet selection.
 * ************************************************************************** */

enum jitter_snippet_to_patch
jitter_snippet_for_loading_register (const char *immediate_pointer,
                                     unsigned int residual_register_index,
                                     const char *loading_code_to_write)
{
  int32_t immediate = * (int32_t*) immediate_pointer;
  if (JITTER_FITS_IN_BITS_ZERO_EXTENDED ((uint32_t) immediate, 16))
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_zero_extended_16bit_to_register_0;
      case 1:  return jitter_snippet_load_zero_extended_16bit_to_register_1;
      case 2:  return jitter_snippet_load_zero_extended_16bit_to_register_2;
      case 3:  return jitter_snippet_load_zero_extended_16bit_to_register_3;
      case 4:  return jitter_snippet_load_zero_extended_16bit_to_register_4;
      case 5:  return jitter_snippet_load_zero_extended_16bit_to_register_5;
      default: jitter_fatal ("impossible");
      }
  else if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (immediate, 16))
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_sign_extended_16bit_to_register_0;
      case 1:  return jitter_snippet_load_sign_extended_16bit_to_register_1;
      case 2:  return jitter_snippet_load_sign_extended_16bit_to_register_2;
      case 3:  return jitter_snippet_load_sign_extended_16bit_to_register_3;
      case 4:  return jitter_snippet_load_sign_extended_16bit_to_register_4;
      case 5:  return jitter_snippet_load_sign_extended_16bit_to_register_5;
      default: jitter_fatal ("impossible");
      }
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
  else if (/* The immediate must be representable as the instruction address
              plus a 21-bit signed constant having its two low bits set to zero;
              on MIPS the instruction is also aligned to 4 bytes. */
           JITTER_FITS_IN_BITS_ZERO_EXTENDED
             ((jitter_int) immediate - (jitter_int) loading_code_to_write,
              21)
           && (immediate & 0x3) == 0)
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_pcrel_address_to_register_0;
      case 1:  return jitter_snippet_load_pcrel_address_to_register_1;
      case 2:  return jitter_snippet_load_pcrel_address_to_register_2;
      case 3:  return jitter_snippet_load_pcrel_address_to_register_3;
      case 4:  return jitter_snippet_load_pcrel_address_to_register_4;
      case 5:  return jitter_snippet_load_pcrel_address_to_register_5;
      default: jitter_fatal ("impossible");
      }
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
  else
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_32bit_to_register_0;
      case 1:  return jitter_snippet_load_32bit_to_register_1;
      case 2:  return jitter_snippet_load_32bit_to_register_2;
      case 3:  return jitter_snippet_load_32bit_to_register_3;
      case 4:  return jitter_snippet_load_32bit_to_register_4;
      case 5:  return jitter_snippet_load_32bit_to_register_5;
      default: jitter_fatal ("impossible");
      }
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_memory (const char *immediate_pointer,
                                   unsigned int index,
                                   const char *loading_code_to_write)
{
  jitter_fatal ("MIPS residual memory: not implemented yet");
}

void
jitter_patch_load_immediate_to_register (char *native_code,
                                         size_t native_code_size,
                                         const char *immediate_pointer,
                                         enum jitter_snippet_to_patch snippet)
{
  uint32_t u = * (uint32_t*) immediate_pointer;
  uint16_t low = u & ((1 << 16) - 1);
  uint16_t high = u >> 16;
  /* See the comments in machine-assembly.S about offsets. */
  switch (snippet)
    {
    case jitter_snippet_load_zero_extended_16bit_to_register_0:
    case jitter_snippet_load_zero_extended_16bit_to_register_1:
    case jitter_snippet_load_zero_extended_16bit_to_register_2:
    case jitter_snippet_load_zero_extended_16bit_to_register_3:
    case jitter_snippet_load_zero_extended_16bit_to_register_4:
    case jitter_snippet_load_zero_extended_16bit_to_register_5:
    case jitter_snippet_load_sign_extended_16bit_to_register_0:
    case jitter_snippet_load_sign_extended_16bit_to_register_1:
    case jitter_snippet_load_sign_extended_16bit_to_register_2:
    case jitter_snippet_load_sign_extended_16bit_to_register_3:
    case jitter_snippet_load_sign_extended_16bit_to_register_4:
    case jitter_snippet_load_sign_extended_16bit_to_register_5:
    case jitter_snippet_load_zero_extended_16bit_to_memory:
    case jitter_snippet_load_sign_extended_16bit_to_memory:
      {
        struct jitter_mips_16_bit_immediate_instruction *instruction
          = (struct jitter_mips_16_bit_immediate_instruction *) native_code;
        instruction->immediate = low;
        break;
      }
    case jitter_snippet_load_32bit_to_register_0:
    case jitter_snippet_load_32bit_to_register_1:
    case jitter_snippet_load_32bit_to_register_2:
    case jitter_snippet_load_32bit_to_register_3:
    case jitter_snippet_load_32bit_to_register_4:
    case jitter_snippet_load_32bit_to_register_5:
    case jitter_snippet_load_32bit_to_memory:
      {
        /* Not much more difficult than the previous case.  Here we have two
           32-bit instructions to patch, each with a 16-bit immediate always in
           the same position.  The high part comes first. */
        struct jitter_mips_16_bit_immediate_instruction *first
          = (struct jitter_mips_16_bit_immediate_instruction *) native_code;
        struct jitter_mips_16_bit_immediate_instruction *second = first + 1;
        first->immediate = high;
        second->immediate = low;
        break;
      }
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    case jitter_snippet_load_pcrel_address_to_register_0:
    case jitter_snippet_load_pcrel_address_to_register_1:
    case jitter_snippet_load_pcrel_address_to_register_2:
    case jitter_snippet_load_pcrel_address_to_register_3:
    case jitter_snippet_load_pcrel_address_to_register_4:
    case jitter_snippet_load_pcrel_address_to_register_5:
      {
        jitter_int immediate = * (jitter_int *) immediate_pointer;
        jitter_int instruction_address_as_integer = (jitter_int) native_code;
        jitter_int distance = immediate - instruction_address_as_integer;
        if (! JITTER_FITS_IN_BITS_ZERO_EXTENDED (distance, 21))
          jitter_fatal ("cannot use addiupc to load an address not at a "
                        "distance fitting in 21 bits from the program counter: "
                        "this should never happen");
        if ((distance & 0x3) != 0)
          jitter_fatal ("cannot use addiupc to load an odd number: this should "
                        "never happen");
        struct jitter_mips_21_bit_immediate_instruction *addiupc_instruction
          = (struct jitter_mips_21_bit_immediate_instruction *) native_code;
        addiupc_instruction->immediate = distance >> 2; /* With GCC this is an
                                                           arithmetic shift. */
        break;
      }
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    default:
      jitter_fatal ("impossible");
    }
}

void
jitter_patch_load_immediate_to_memory (char *native_code,
                                       size_t native_code_size,
                                       unsigned int memory_index,
                                       const char *immediate_pointer,
                                       enum jitter_snippet_to_patch snippet)
{
  jitter_fatal ("MIPS residual memory: not implemented yet");
}

#ifdef JITTER_HAVE_PATCH_IN
enum jitter_snippet_to_patch
jitter_snippet_for_patch_in (const struct jitter_patch_in_descriptor *dp)
{
  jitter_uint patch_in_case = dp->patch_in_case;
  switch (patch_in_case)
    {
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL:
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
      return jitter_snippet_branch_unconditional_28bit_compact;
#else
      return jitter_snippet_jump_unconditional_28bit_pseudo_direct;
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_ZERO:
      return jitter_snippet_branch_conditional_compact_23bit_offset;
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_OTHER:
      return jitter_snippet_branch_conditional_compact_18bit_offset;
#else
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY:
      return jitter_snippet_branch_conditional_18bit_offset;
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

    case JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK:
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
      return jitter_snippet_branch_and_link_28bit_compact;
#else
      return jitter_snippet_jump_and_link_28bit_pseudo_direct;
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

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
  char *jump_target = * (char**) immediate_pointer;
  char *jump_address __attribute__ ((unused)) = native_code;

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
  /* Every R6 branch is patched in exactly the same way.  Only the displacement
     width changes. */
# define JITTER_R6_CASE_(snippet_name, branch_width)                           \
    case snippet_name:                                                         \
      {                                                                        \
        /* The branch displacement is measured from the beginning of the       \
           instruction following the branch. */                                \
        char *jump_origin = native_code + 4;                                   \
        int64_t displacement = jump_target - jump_origin;                      \
                                                                               \
        /* Make a quick sanity check then patch the branching instruction      \
           with the offset. */                                                 \
        struct JITTER_CONCATENATE_THREE (jitter_mips_,                         \
                                         branch_width,                         \
                                         _bit_branch_instruction)              \
          *instruction                                                         \
          = (struct JITTER_CONCATENATE_THREE (jitter_mips_,                    \
                                              branch_width,                    \
                                              _bit_branch_instruction)         \
             *) native_code;                                                   \
        if (displacement % 4 != 0)                                             \
          jitter_fatal (JITTER_STRINGIFY (snippet_name)                        \
                        " %i-bit displacement branch: misaligned "             \
                        "instruction", (int) (branch_width));                  \
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (displacement, branch_width))  \
          jitter_fatal (JITTER_STRINGIFY (snippet_name)                        \
                        " %i-bit displacement branch: far branch",             \
                        (int) (branch_width));                                 \
        /* GCC guarantees that this is an arithmetic right shift, and this     \
           code is only used with GCC. */                                      \
        instruction->immediate = displacement >> 2;                            \
        break;                                                                 \
      }
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

  switch (snippet)
    {
#if ! defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    case jitter_snippet_branch_conditional_18bit_offset:
      {
        char *delay_slot_address = native_code + 4;

        /* Check that the jump origin and target addresses are 32-bit
           aligned. */
        uint32_t jump_target_u = (uint32_t) (jitter_uint) jump_target;
        uint32_t jump_address_u = (uint32_t) (jitter_uint) jump_address;
        if (jump_address_u % 4 != 0)
          jitter_fatal ("conditional branch instruction not 32-bit aligned");
        if (jump_target_u % 4 != 0)
          jitter_fatal ("conditional branch target not 32-bit aligned");

        /* Compute the shifted branch offset (the two lowest bits, always zero,
           are not represented), and fail if doesn't fit in the instruction. */
        int64_t jump_offset = jump_target - delay_slot_address;
        int64_t jump_offset_shifted_64bit = jump_offset >> 2;
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (jump_offset_shifted_64bit, 16))
          jitter_fatal ("conditional branch target too far");
        uint16_t jump_offset_shifted = jump_offset_shifted_64bit;

        /* Compute the patched instruction, by replacing the shifted offset into
           the old instruction. */
        uint32_t old_instruction = * (uint32_t*) native_code;
        uint32_t instruction
          = (old_instruction & ~ 0xffff) | jump_offset_shifted;

        /* Write the patched instruction in place of the original instruction.
           This store is aligned to 32 bits if the native_code pointer actually
           points to a MIPS instruction. */
        * (uint32_t *) native_code = instruction;
        break;
      }
#endif // #if ! defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

#if ! defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    case jitter_snippet_jump_unconditional_28bit_pseudo_direct:
    case jitter_snippet_jump_and_link_28bit_pseudo_direct:
      {
        /* Check that the jump origin and target addresses are 32-bit
           aligned. */
        if ((jitter_uint) jump_address % 4 != 0)
          jitter_fatal ("j instruction not 32-bit aligned");
        if ((jitter_uint) jump_target % 4 != 0)
          jitter_fatal ("j instruction target not 32-bit aligned");

        /* Perform an arithmetic right shift of the two address, so as to ignore
           the two least significatn zero bits.  This is GCC, so we can rely on
           signed right shift. */
        uint32_t jump_target_u = ((jitter_int) jump_target) >> 2;
        uint32_t jump_address_u = ((jitter_int) jump_address) >> 2;

#define JITTER_INSTRUCTION_INDEX(address_as_uint32) \
  (address_as_uint32 & ((1u << 26) - 1u))
#define JITTER_REGION(address_as_uint32) \
  (address_as_uint32 & ~ ((1u << 26) - 1u))

        /* Check that the six most significant bits of the jump targets are the
           same as the six most significant target of the jumping
           instruction. */
        if (JITTER_REGION(jump_address_u) != JITTER_REGION(jump_target_u))
          jitter_fatal ("can't jump out of the current 256MB region");

        /* Encode the instruction. */
        uint32_t unscaled_opcode
          = ((snippet == jitter_snippet_jump_unconditional_28bit_pseudo_direct)
             ? 2u
             : 3u);
        uint32_t instruction
          = (  (unscaled_opcode << 26)                    /* opcode. */
             | JITTER_INSTRUCTION_INDEX(jump_target_u));  /* instruction index. */

#undef JITTER_INSTRUCTION_INDEX
#undef JITTER_REGION

        /* Write the patched instruction in place of the original instruction.
           This store is aligned to 32 bits if the native_code pointer actually
           points to a MIPS instruction. */
        * (uint32_t *) native_code = instruction;
        break;
      }
#endif // #if ! defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    /* Handle every r6 branch. */
    JITTER_R6_CASE_ (jitter_snippet_branch_unconditional_28bit_compact, 28);
    JITTER_R6_CASE_ (jitter_snippet_branch_and_link_28bit_compact, 28);
    JITTER_R6_CASE_ (jitter_snippet_branch_conditional_compact_18bit_offset, 18);
    JITTER_R6_CASE_ (jitter_snippet_branch_conditional_compact_23bit_offset, 23);
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

    default:
      jitter_fatal ("jitter_patch_patch_in: unsupported snippet %li",
                    (long) snippet);
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN
