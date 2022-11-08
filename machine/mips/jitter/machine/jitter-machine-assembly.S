/* VM library: assembly support for MIPS .

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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


/* Include headers and common code.
 * ************************************************************************** */

/* Include the architecture-dependent CPP macro definitions. */
#include <jitter/machine/jitter-machine.h>

/* Include the architecture-independent Gas macro definitions. */
#include <jitter/jitter-machine-common.S>


/* Global configuration.
 * ************************************************************************** */

.text

/* Here I use $1/$at explicitly; in this kind of low-level file, where I care
   about the encoding of each assembly instruction, I will certainly not use
   pseudo-instructions expanding to hidden uses of $1. */
.set noat

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
/* Make sure we are using the correct MIPS variant. */
.set arch=mips32r6
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)


/* Snippets.
 * ************************************************************************** */

/* Here come the actual snippet definitions, containing the code to be copied
   and patched.  Notice that the order matters, and the calls to jitter_snippet
   here must follow the same order as the enum jitter_snippet_to_patch cases in
   native.h . */
jitter_arrays

/* Load a zero-extended 16-bit literal into a 32-bit register.  This
   assembles to something like
     34 02 ff ff   # ori $2, $0, 0xffff
   where the rightmost 16 bits, here all ones, are the literal. */
#define LOAD_ZERO_EXTENDED_16BIT_TO_REGISTER(register)  \
  <ori register, $0, 0>
jitter_snippet load_zero_extended_16bit_to_register_0,               \
  LOAD_ZERO_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_zero_extended_16bit_to_register_1,               \
  LOAD_ZERO_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_zero_extended_16bit_to_register_2,               \
  LOAD_ZERO_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_zero_extended_16bit_to_register_3,               \
  LOAD_ZERO_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_zero_extended_16bit_to_register_4,               \
  LOAD_ZERO_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_zero_extended_16bit_to_register_5,               \
  LOAD_ZERO_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)

/* Load a sign-extended 16-bit literal into a 32-bit register.  This
   assembles to something like
     24 02 00 00   # addiu $2, $0, 0x0000
   where the rightmost 16 bits, here zero, are the literal. */
#define LOAD_SIGN_EXTENDED_16BIT_TO_REGISTER(register)  \
  <addiu register, $0, 0>
jitter_snippet load_sign_extended_16bit_to_register_0,               \
  LOAD_SIGN_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_sign_extended_16bit_to_register_1,               \
  LOAD_SIGN_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_sign_extended_16bit_to_register_2,               \
  LOAD_SIGN_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_sign_extended_16bit_to_register_3,               \
  LOAD_SIGN_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_sign_extended_16bit_to_register_4,               \
  LOAD_SIGN_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_sign_extended_16bit_to_register_5,               \
  LOAD_SIGN_EXTENDED_16BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)

/* Load a 32-bit literal into a 32-bit register.  This assembles to
   something like
     3c 10 ab cd   # lui $16,      0xabcd
     36 10 12 34   # ori $16, $16, 0x1234
   where the first instruction loads the high half and the second the low
   half.  Both halves are encoded in the rightmost 16 bits of each
   instruction.  The 32-bit literal in this example is 0xabcd1234 . */
#define LOAD_32BIT_TO_REGISTER(register)  \
  <lui register, 0>,                      \
  <ori register, register, 0>
jitter_snippet load_32bit_to_register_0,               \
  LOAD_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_32bit_to_register_1,               \
  LOAD_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_32bit_to_register_2,               \
  LOAD_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_32bit_to_register_3,               \
  LOAD_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_32bit_to_register_4,               \
  LOAD_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_32bit_to_register_5,               \
  LOAD_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
/* Load a 21-bit PC-relative aligned address into a 32-bit register.  Only the
   19 most significant bits of the immediate are represented.
   This assembles to something like
     ee200003   # addiupc $17, 12  */
#define LOAD_PCREL_ADDRESS_TO_REGISTER(register)  \
  <addiupc register, 0>
jitter_snippet load_pcrel_address_to_register_0,               \
  LOAD_PCREL_ADDRESS_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_pcrel_address_to_register_1,               \
  LOAD_PCREL_ADDRESS_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_pcrel_address_to_register_2,               \
  LOAD_PCREL_ADDRESS_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_pcrel_address_to_register_3,               \
  LOAD_PCREL_ADDRESS_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_pcrel_address_to_register_4,               \
  LOAD_PCREL_ADDRESS_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_pcrel_address_to_register_5,               \
  LOAD_PCREL_ADDRESS_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

/* FIXME: these are not implemented yet. */
jitter_snippet load_zero_extended_16bit_to_memory,  \
           <nop>
jitter_snippet load_sign_extended_16bit_to_memory,  \
           <nop>
jitter_snippet load_32bit_to_memory,  \
           <nop>

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
/* Branch relative using a 26-bit displacement from the next instruction,
   left-shifted by two bits.  No delay slot. */
jitter_snippet branch_unconditional_28bit_compact,  \
           <1: bc 1b> 

/* The linking equivalent of the instruction above. */
jitter_snippet branch_and_link_28bit_compact,  \
           <1: balc 1b>
#else /* pre-r6 */
/* Jump to an absolute address whose six most significant bits are the same
   as the ones in the address of the jumping instruction, the next 26 are
   specified as an immediate, and the last two are zero.  The opcode takes
   the most significant 6 bits, and its value is 2.
     08000000   # j to the first address in the same region.
     00200825   # or $1, $1, $0
   Here instruction encodings are shown big-endian.
   The second instruction is a nop for the delay slot.  Some old Loongson2F
   version has a bug for which the recommended workaround is using this
   encoding for a nop, rather than the traditional
     00000000   # sll $0, $0, 0x0
   Of course the observable behavior is the same. */
jitter_snippet jump_unconditional_28bit_pseudo_direct,  \
           <.long 0x8000000>,                           \
           <JITTER_MIPS_NOP>

/* Identical to jump_unconditional_28bit_pseudo_direct , except that the
   jumping instruction also stores the return address (the adress of the
   instruction following the delay slot) in $31.  The instruction encoding
   is the same, except that the opcode is 3 instead of 2.
     0c000000   # jal to the first address in the same region.
     00200825   # or $1, $1, $0 */
jitter_snippet jump_and_link_28bit_pseudo_direct,  \
           <.long 0xc000000>,                      \
           <JITTER_MIPS_NOP>
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
/* Again, this snippet is empty because the inline asm code is sufficient.  Its
   offset field is the only part to be patched. */
jitter_snippet branch_conditional_compact_18bit_offset,  \
        <>

/* Likewise, this is an empty snippet. */
jitter_snippet branch_conditional_compact_23bit_offset,  \
        <>
#else /* pre-r6 */
/* This snippet does nothing and takes zero bytes: all the complexity is in the
   inline asm and in the patch-in replacement code, which only replaces the
   branch offset without touching the rest of the instruction.  This solution is
   sensible on MIPS, where conditional branch instructions mention register
   numbers instead of flags, and the register numbers are easy to generate in
   inline asm -- but much less here in assembly, where we don't know about GCC's
   register assignment. */
jitter_snippet branch_conditional_18bit_offset,  \
        <>

#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
