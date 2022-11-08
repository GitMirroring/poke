/* VM library: assembly support for m68k .

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


/* Include the architecture-dependent CPP macro definitions. */
#include <jitter/machine/jitter-machine.h>

/* Include the architecture-independent Gas macro definitions. */
#include <jitter/jitter-machine-common.S>


/* Macros working around preprocessor issues.
 * ************************************************************************** */

/* Handling '#' and '%' across both CPP and Gas macros is, unfortunately, very
   fragile.  Register numbers have a mandatory '%' prefix, which has a special
   meaning with Gas' alternative macros and is not easy or possible to
   escape. */

/* Expand to a two-operand instruction where the first argument is the given
   immediate, provided by the caller without the "#" prefix, and the second
   is a register, provided by the caller without the "%" prefix. */
.macro jitter_instruction_immediate_register opcode,               \
                                             immediate, register1
  \opcode #\immediate&, %\register1&
.endm

/* Expand to a one-operand instruction having as its argument the given
   register, provided by the caller without the "%" prefix. */
.macro jitter_instruction_register opcode,    \
                                   register1
  \opcode %\register1&
.endm

/* Expand to a one-operand instruction having as its argument a
   register-plus-offset memory address.  The caller supplies all the arguments
   without prefixes. */
.macro jitter_instruction_memory opcode,            \
                                 address_register,  \
                                 offset
  \opcode %\address_register@(\offset)
.endm

/* Expand to a two-operand instruction having as its first argument the
   given register, and a register-plus-offset memory address as its second
   argument.  The caller supplies all the arguments without prefixes. */
.macro jitter_instruction_immediate_memory opcode,            \
                                           immediate,         \
                                           address_register,  \
                                           offset
  \opcode #\immediate&, %\address_register@(\offset)
.endm

/* Main part.
 * ************************************************************************** */

.text


/* Snippets.
 * ************************************************************************** */

/* Here come the actual snippet definitions, containing the code to be copied
   and patched.  Notice that the order matters, and the calls to jitter_snippet
   here must follow the same order as the enum jitter_snippet_to_patch cases in
   native.h . */
jitter_arrays

/* Set the given register to zero.  This assembles to something like
     42 82   clrl %d2   */
#define LOAD_ZERO_TO_REGISTER(unprefixed)         \
  <jitter_instruction_register clrl, unprefixed>
jitter_snippet load_zero_to_register_0,                          \
  LOAD_ZERO_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_0)
jitter_snippet load_zero_to_register_1,                          \
  LOAD_ZERO_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_1)
jitter_snippet load_zero_to_register_2,                          \
  LOAD_ZERO_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_2)
jitter_snippet load_zero_to_register_3,                          \
  LOAD_ZERO_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_3)
jitter_snippet load_zero_to_register_4,                          \
  LOAD_ZERO_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_4)
jitter_snippet load_zero_to_register_5,                          \
  LOAD_ZERO_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_5)

/* Set the given register to an 8-bit literal sign-extended to 32 bits.  This
   assembles to something like
     74 01   moveq #1,%d2
   , where the literal is in the low byte.  */
#define LOAD_8BIT_SIGN_EXTENDED_TO_REGISTER(unprefixed)         \
  <jitter_instruction_immediate_register moveq, 1, unprefixed>
jitter_snippet load_8bit_sign_extended_to_register_0,                          \
  LOAD_8BIT_SIGN_EXTENDED_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_0)
jitter_snippet load_8bit_sign_extended_to_register_1,                          \
  LOAD_8BIT_SIGN_EXTENDED_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_1)
jitter_snippet load_8bit_sign_extended_to_register_2,                          \
  LOAD_8BIT_SIGN_EXTENDED_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_2)
jitter_snippet load_8bit_sign_extended_to_register_3,                          \
  LOAD_8BIT_SIGN_EXTENDED_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_3)
jitter_snippet load_8bit_sign_extended_to_register_4,                          \
  LOAD_8BIT_SIGN_EXTENDED_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_4)
jitter_snippet load_8bit_sign_extended_to_register_5,                          \
  LOAD_8BIT_SIGN_EXTENDED_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_5)

/* Set the given register to a 32-bit literal.  This assembles to something
   like
     24 3c 12 34 56 78   movel #0x12345678,%d2
   , where the literal is in the last four bytes.  */
#define LOAD_32BIT_TO_REGISTER(unprefixed)                               \
  <jitter_instruction_immediate_register movel, 0x12345678, unprefixed>
jitter_snippet load_32bit_to_register_0,                          \
  LOAD_32BIT_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_0)
jitter_snippet load_32bit_to_register_1,                          \
  LOAD_32BIT_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_1)
jitter_snippet load_32bit_to_register_2,                          \
  LOAD_32BIT_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_2)
jitter_snippet load_32bit_to_register_3,                          \
  LOAD_32BIT_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_3)
jitter_snippet load_32bit_to_register_4,                          \
  LOAD_32BIT_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_4)
jitter_snippet load_32bit_to_register_5,                          \
  LOAD_32BIT_TO_REGISTER (JITTER_UNPREFIXED_RESIDUAL_REGISTER_5)

/* Set a memory location to zero, at a fixed offset from the base register.  The
   instruction assemblies to something like this:
     42 aa 12 34   clrl %a2@(0x1234)
   The parts to patch are the 32-bit immediate and the 16-bit offset. */
jitter_snippet load_zero_to_memory,                                          \
  <jitter_instruction_memory clrl, JITTER_UNPREFIXED_BASE_REGISTER, 0x1234>

/* Materialise a 32-bit constant to memory, at a fixed offset from the base
   register.  The instruction assemblies to something like this:
     25 7c aa bb cc dd 12 34   movel #0xaabbccdd, %a2@(0x1234)
   The parts to patch are the 32-bit immediate and the 16-bit offset. */
jitter_snippet load_32bit_to_memory,                                        \
  <jitter_instruction_immediate_memory movel, 0xaabbccdd,                   \
                                          JITTER_UNPREFIXED_BASE_REGISTER,  \
                                          0x1234>

/* Branch unconditionally, using a 32-bit signed displacement.  The
   displacement is at the end of the instruction, and is preceded by
   an 8-bit field which must be set to 0xff.
   The instruction assembles to something like
     60 ff ff ff ff fe   .L: bral .L
   (that there would be a shorter variant for such a near branch, but
    this is the variant we patch.)*/
jitter_snippet branch_unconditional_32bit_offset,  \
  <1: bral 1b>

/* Call a subroutine, pushing the return address to the stack.  The destination
   is given as a 32-bit signed displacement along with an 8-bit displacement to
   ignore set to 0xff, exactly like in branch_unconditional_32bit_offset .
   The instruction assembles to something like
     61 ff 00 00 00 20   bsrl <label, past the branching instruction>
   (that there would be a shorter variant for such a near branch, but
    this is the variant we patch.)*/
jitter_snippet branch_and_link_32bit_offset,  \
  <1: bsrl 1b>

/* Branch conditionally according to the condition code register, using a
   32-bit signed displacement.  The encoding is the same as
   branch_unconditional_32bit_offset, but in this case the inline assembly
   code is already correct, with only the displacement to patch. */
jitter_snippet branch_conditional_32bit_offset,  \
  <>
