/* VM library: native code patching for RISC-V .

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
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#include <stdint.h>
#include <assert.h>
#include <string.h>

#include <jitter/jitter-fatal.h>

#include <jitter/jitter-arithmetic.h>
#include <jitter/jitter-bitwise.h>
#include <jitter/jitter-patch.h>
#include <jitter/jitter-machine-common.h>

#include "jitter-machine.h"




/* RISC-V instruction formats.
 * ************************************************************************** */

/* RISC-V is always little-endian.  Here I am writing the instruction fields
   starting from the least significant bit, numbered 0.  By reading the diagrams
   from the specification left-to-right (from bit 31 to bit 0) one sees fields
   reversed compared to my definitions here. */

/* RISC-V funct3 field values, only for the few instructions where I need to
   change them. */
enum jitter_riscv_funct3
  {
    jitter_riscv_funct3_xor_xori = 0b100,
    jitter_riscv_funct3_or_ori   = 0b110
  };

/* A U-type RISC-V instruction.  Importantly for us, lui has this format. */
__attribute__ ((packed))
struct jitter_riscv_u_type_instruction
{
  unsigned opcode:   7;
  unsigned rd:       5;
  unsigned imm12_31: 20;
};

/* An I-type RISC-V instruction.  Importantly for us the following instructions
   have this format: ori, xori, jalr. */
__attribute__ ((packed))
struct jitter_riscv_i_type_instruction
{
  unsigned opcode:  7;
  unsigned rd:      5;
  unsigned funct3:  3;
  unsigned rs1:     5;
  unsigned imm0_11: 12;
};

/* An R-type RISC-V instruction.  Importantly for us, or and xor have this
   format. */
__attribute__ ((packed))
struct jitter_riscv_r_type_instruction
{
  unsigned opcode:  7;
  unsigned rd:      5;
  unsigned funct3:  3;
  unsigned rs1:     5;
  unsigned rs2:     5;
  unsigned funct7:  7;
};

/* A J-type RISC-V instruction.  The jal instruction has this format, and j is
   encoded as jal with rd == 0. */
__attribute__ ((packed))
struct jitter_riscv_j_type_instruction
{
  unsigned opcode:   7;
  unsigned rd:       5;
  unsigned imm12_19: 8;
  unsigned imm11:    1;
  unsigned imm1_10:  10;
  unsigned imm20:    1;
};

/* A B-type RISC-V instruction.  Conditional branches use this format: beq, bne,
   blt, bge, bltu, bgeu. */
__attribute__ ((packed))
struct jitter_riscv_b_type_instruction
{
  unsigned opcode:   7;
  unsigned imm11:    1;
  unsigned imm1_4:   4;
  unsigned funct3:   3;
  unsigned rs1:      5;
  unsigned rs2:      5;
  unsigned imm5_10:  6;
  unsigned imm12:    1;
};




/* Patch functions.
 * ************************************************************************** */

void
jitter_invalidate_icache (char *from, size_t byte_no)
{
  /* I am assuming that this does not need to do anything on RISC-V.  Not
     tested on real hardware yet. */
}

/* Return the distance in bytes, from the pointed instruction, to be patched,
   to the destination address.  This is natural for branching instructions, but
   is in fact also usable to materalise any constant which happens to be
   expressible as the address of the instruction materialising plus a short
   offset. */
static jitter_int
jitter_distance_from (const char *destination_p,
                      const char *branch_instruction_p)
{
  return destination_p - branch_instruction_p;
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_register (const char *immediate_pointer,
                                     unsigned int residual_register_index,
                                     const char *loading_code_to_write)
{
  jitter_uint immediate = * (jitter_uint *) immediate_pointer;
  jitter_int jitter_distance_from_pc
    = jitter_distance_from ((const char *) immediate, loading_code_to_write);
  uint32_t distance_high __attribute__ ((unused)) // See [imprecise] below.
    = ((uint32_t) jitter_distance_from_pc) >> 12;
  uint32_t distance_low __attribute__ ((unused)) // See [imprecise] below.
    = ((uint32_t) jitter_distance_from_pc) & ((1LU << 12) - 1);
  if (JITTER_FITS_IN_BITS_SIGN_EXTENDED ((jitter_int) immediate, 12))
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_sign_extended_12bit_to_register_0;
      case 1:  return jitter_snippet_load_sign_extended_12bit_to_register_1;
      case 2:  return jitter_snippet_load_sign_extended_12bit_to_register_2;
      case 3:  return jitter_snippet_load_sign_extended_12bit_to_register_3;
      case 4:  return jitter_snippet_load_sign_extended_12bit_to_register_4;
      case 5:  return jitter_snippet_load_sign_extended_12bit_to_register_5;
      default: jitter_fatal ("impossible");
      }
#if 0  /* [imprecise] FIXME: these extremely precise choices are performed too
          late for the current specialiser, which decides which snippet to use
          before knowing the exact address.  I might re-enable them later after
          I rewrite the specialiser if the new design makes this possible -- I
          should rewrite the specialiser for other reasons as well. */
  else if (JITTER_FITS_IN_BITS_SIGN_EXTENDED ((jitter_int) immediate, 32)
           && (immediate & ((1LU << 12) - 1)) == 0)
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_lui_only_to_register_0;
      case 1:  return jitter_snippet_load_lui_only_to_register_1;
      case 2:  return jitter_snippet_load_lui_only_to_register_2;
      case 3:  return jitter_snippet_load_lui_only_to_register_3;
      case 4:  return jitter_snippet_load_lui_only_to_register_4;
      case 5:  return jitter_snippet_load_lui_only_to_register_5;
      default: jitter_fatal ("impossible");
      }
#endif // #if 0
#if 0 /* FIXME: see [imprecise] above. */
  else if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (((jitter_int)
                                               jitter_distance_from_pc),
                                              32)
           && distance_low == 0)
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_pcrel_address_no_add_to_register_0;
      case 1:  return jitter_snippet_load_pcrel_address_no_add_to_register_1;
      case 2:  return jitter_snippet_load_pcrel_address_no_add_to_register_2;
      case 3:  return jitter_snippet_load_pcrel_address_no_add_to_register_3;
      case 4:  return jitter_snippet_load_pcrel_address_no_add_to_register_4;
      case 5:  return jitter_snippet_load_pcrel_address_no_add_to_register_5;
      default: jitter_fatal ("impossible");
      }
#endif // #if 0
  else if (JITTER_XLEN == 32
           || JITTER_FITS_IN_BITS_SIGN_EXTENDED ((jitter_int) immediate, 32))
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_sign_extended_32bit_to_register_0;
      case 1:  return jitter_snippet_load_sign_extended_32bit_to_register_1;
      case 2:  return jitter_snippet_load_sign_extended_32bit_to_register_2;
      case 3:  return jitter_snippet_load_sign_extended_32bit_to_register_3;
      case 4:  return jitter_snippet_load_sign_extended_32bit_to_register_4;
      case 5:  return jitter_snippet_load_sign_extended_32bit_to_register_5;
      default: jitter_fatal ("impossible");
      }
  else if (JITTER_FITS_IN_BITS_SIGN_EXTENDED (((jitter_int)
                                               jitter_distance_from_pc),
                                              32))
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
#if JITTER_XLEN == 64
  else
    switch (residual_register_index)
      {
      case 0:  return jitter_snippet_load_64bit_to_register_0;
      case 1:  return jitter_snippet_load_64bit_to_register_1;
      case 2:  return jitter_snippet_load_64bit_to_register_2;
      case 3:  return jitter_snippet_load_64bit_to_register_3;
      case 4:  return jitter_snippet_load_64bit_to_register_4;
      case 5:  return jitter_snippet_load_64bit_to_register_5;
      default: jitter_fatal ("impossible");
      }
#else
  else
    jitter_fatal ("jitter_snippet_for_loading_register: impossible");
#endif // #if JITTER_XLEN == 64
}

enum jitter_snippet_to_patch
jitter_snippet_for_loading_memory (const char *immediate_pointer,
                                   unsigned int index,
                                   const char *loading_code_to_write)
{
  jitter_fatal ("RISC-V residual memory: not implemented yet");
}

/* Patch the instruction sequence beginning at native_code, having the form
     lui xN, HIGH
   to materialise the given 32-bit constant. */
__attribute__ ((unused )) /* FIXME: see [imprecise] above. */
static void
jitter_patch_load_lui (char *native_code, uint32_t immediate)
{
  uint32_t immediate_low = JITTER_WORD_FIELD (immediate, 0, 11);
  if (immediate_low != 0)
    jitter_fatal ("jitter_patch_load_lui: this should never happen");
  uint32_t immediate_high = JITTER_WORD_FIELD (immediate, 12, 31);
  struct jitter_riscv_u_type_instruction *lui_instruction
    = (struct jitter_riscv_u_type_instruction *) native_code;
  lui_instruction->imm12_31 = immediate_high;
}

/* Patch the instruction sequence beginning at native_code, having the form
     lui xN, HIGH
     ori xN, xN, LOW
   to materialise the given 32-bit constant.  Change the ori instruction into
   a xori instruction if needed, along with the HIGH part. */
static void
jitter_patch_load_lui_ori (char *native_code, uint32_t immediate)
{
  uint32_t immediate_low = JITTER_WORD_FIELD (immediate, 0, 11);
  uint32_t immediate_high = JITTER_WORD_FIELD (immediate, 12, 31);
  struct jitter_riscv_u_type_instruction *lui_instruction
    = (struct jitter_riscv_u_type_instruction *) native_code;
  struct jitter_riscv_i_type_instruction *ori_or_xori_instruction
    = (struct jitter_riscv_i_type_instruction *) (native_code + 4);
  if (JITTER_WORD_FIELD (immediate, 11, 11) != 0)
    {
      immediate_high = ~ immediate_high;
      ori_or_xori_instruction->funct3 = jitter_riscv_funct3_xor_xori;
    }
  lui_instruction->imm12_31 = immediate_high;
  ori_or_xori_instruction->imm0_11 = immediate_low;
}

void
jitter_patch_load_immediate_to_register (char *native_code,
                                         size_t native_code_size,
                                         const char *immediate_pointer,
                                         enum jitter_snippet_to_patch snippet)
{
  jitter_uint immediate = * (jitter_uint *) immediate_pointer;
  jitter_int jitter_distance_from_pc
    = jitter_distance_from ((const char *) immediate, native_code);
  uint32_t distance_high = JITTER_WORD_FIELD (jitter_distance_from_pc, 12, 31);
  uint32_t distance_low = JITTER_WORD_FIELD (jitter_distance_from_pc, 0, 11);

  switch (snippet)
    {
    case jitter_snippet_load_sign_extended_12bit_to_register_0:
    case jitter_snippet_load_sign_extended_12bit_to_register_1:
    case jitter_snippet_load_sign_extended_12bit_to_register_2:
    case jitter_snippet_load_sign_extended_12bit_to_register_3:
    case jitter_snippet_load_sign_extended_12bit_to_register_4:
    case jitter_snippet_load_sign_extended_12bit_to_register_5:
      {
        struct jitter_riscv_i_type_instruction *ori_instruction
          = (struct jitter_riscv_i_type_instruction *) native_code;
        ori_instruction->imm0_11 = immediate;
        break;
      }
#if 0 /* FIXME: see [imprecise] above. */
    case jitter_snippet_load_lui_only_to_register_0:
    case jitter_snippet_load_lui_only_to_register_1:
    case jitter_snippet_load_lui_only_to_register_2:
    case jitter_snippet_load_lui_only_to_register_3:
    case jitter_snippet_load_lui_only_to_register_4:
    case jitter_snippet_load_lui_only_to_register_5:
      jitter_patch_load_lui (native_code, immediate);
      break;
#endif // #if 0
    case jitter_snippet_load_sign_extended_32bit_to_register_0:
    case jitter_snippet_load_sign_extended_32bit_to_register_1:
    case jitter_snippet_load_sign_extended_32bit_to_register_2:
    case jitter_snippet_load_sign_extended_32bit_to_register_3:
    case jitter_snippet_load_sign_extended_32bit_to_register_4:
    case jitter_snippet_load_sign_extended_32bit_to_register_5:
      jitter_patch_load_lui_ori (native_code, immediate);
      break;
    case jitter_snippet_load_pcrel_address_to_register_0:
    case jitter_snippet_load_pcrel_address_to_register_1:
    case jitter_snippet_load_pcrel_address_to_register_2:
    case jitter_snippet_load_pcrel_address_to_register_3:
    case jitter_snippet_load_pcrel_address_to_register_4:
    case jitter_snippet_load_pcrel_address_to_register_5:
#if 0 /* FIXME: see [imprecise] above. */
    case jitter_snippet_load_pcrel_address_no_add_to_register_0:
    case jitter_snippet_load_pcrel_address_no_add_to_register_1:
    case jitter_snippet_load_pcrel_address_no_add_to_register_2:
    case jitter_snippet_load_pcrel_address_no_add_to_register_3:
    case jitter_snippet_load_pcrel_address_no_add_to_register_4:
    case jitter_snippet_load_pcrel_address_no_add_to_register_5:
#endif // #if 0
      {
        struct jitter_riscv_u_type_instruction *lui_instruction
          = (struct jitter_riscv_u_type_instruction *) native_code;
        struct jitter_riscv_i_type_instruction *addi_instruction
          = (struct jitter_riscv_i_type_instruction *) (native_code + 4);
        if (JITTER_WORD_FIELD (jitter_distance_from_pc, 11, 11))
          /* The low part will be sign-extended, which is just like adding -1 to
             the high part.  Compensate for this. */
          lui_instruction->imm12_31 = distance_high + 1;
        else
          lui_instruction->imm12_31 = distance_high;
#if 0 /* FIXME: see [imprecise] above. */
        if (distance_low != 0)
#endif // #if 0
          addi_instruction->imm0_11 = distance_low;
        break;
      }
#if JITTER_XLEN == 64
    case jitter_snippet_load_64bit_to_register_0:
    case jitter_snippet_load_64bit_to_register_1:
    case jitter_snippet_load_64bit_to_register_2:
    case jitter_snippet_load_64bit_to_register_3:
    case jitter_snippet_load_64bit_to_register_4:
    case jitter_snippet_load_64bit_to_register_5:
      {
        uint32_t low = JITTER_WORD_FIELD (immediate, 0, 31);
        uint32_t high = JITTER_WORD_FIELD (immediate, 32, 63);
        bool bit_31 = JITTER_WORD_FIELD (immediate, 31, 31);
        if (bit_31)
          high = ~ high;
        jitter_patch_load_lui_ori (native_code, low);
        jitter_patch_load_lui_ori (native_code + 4 * 2, high);
        struct jitter_riscv_i_type_instruction *final_or_or_xor_instruction
          = (struct jitter_riscv_i_type_instruction *) (native_code + 4 * 5);
        if (bit_31)
          final_or_or_xor_instruction->funct3 = jitter_riscv_funct3_xor_xori;
        break;
      }
#endif // #if JITTER_XLEN == 64
    default:
      jitter_fatal ("patch load-immediate to register: impossible case");
    }
}

void
jitter_patch_load_immediate_to_memory (char *native_code,
                                       size_t native_code_size,
                                       unsigned int memory_index,
                                       const char *immediate_pointer,
                                       enum jitter_snippet_to_patch snippet)
{
  jitter_fatal ("RISC-V residual memory: not implemented yet, maybe not "
                "needed at all on such a register-rich architecture");
}

#ifdef JITTER_HAVE_PATCH_IN
enum jitter_snippet_to_patch
jitter_snippet_for_patch_in (const struct jitter_patch_in_descriptor *dp)
{
  jitter_uint patch_in_case = dp->patch_in_case;
  switch (patch_in_case)
    {
    case JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL:
      return jitter_snippet_jump_unconditional_20bit_pcrel;

    case JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK:
      return jitter_snippet_jump_and_link_20bit_pcrel;

    case JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY:
      return jitter_snippet_jump_conditional_13bit_pcrel;

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
  const char * immediate = * (const char **) immediate_pointer;
  jitter_int distance = jitter_distance_from (immediate, native_code);
  switch (snippet)
    {
    case jitter_snippet_jump_unconditional_20bit_pcrel:
    case jitter_snippet_jump_and_link_20bit_pcrel:
      {
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (distance, 20))
          jitter_fatal ("far jump from J-type: not supported yet");
        if (distance & 1)
          jitter_fatal ("odd distance (J-type): this should never happen");
        struct jitter_riscv_j_type_instruction *jal
          = (struct jitter_riscv_j_type_instruction *) native_code;

        jal->imm1_10 = JITTER_WORD_FIELD (distance, 1, 10);
        jal->imm11 = JITTER_WORD_FIELD (distance, 11, 11);
        jal->imm12_19 = JITTER_WORD_FIELD (distance, 12, 19);
        jal->imm20 = JITTER_WORD_FIELD (distance, 20, 20);
        break;
      }
    case jitter_snippet_jump_conditional_13bit_pcrel:
      {
        if (! JITTER_FITS_IN_BITS_SIGN_EXTENDED (distance, 13))
          jitter_fatal ("far jump from B-type: not supported yet");
        if (distance & 1)
          jitter_fatal ("odd distance (B-type): this should never happen");
        struct jitter_riscv_b_type_instruction *instr
          = (struct jitter_riscv_b_type_instruction *) native_code;

        instr->imm1_4 = JITTER_WORD_FIELD (distance, 1, 4);
        instr->imm5_10 = JITTER_WORD_FIELD (distance, 5, 10);
        instr->imm11 = JITTER_WORD_FIELD (distance, 11, 11);
        instr->imm12 = JITTER_WORD_FIELD (distance, 12, 12);
        break;
      }
    default:
      jitter_fatal ("jitter_patch_patch_in: unsupported snippet %li",
                    (long) snippet);
    }
}
#endif // #ifdef JITTER_HAVE_PATCH_IN
