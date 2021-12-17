/* VM library: assembly support for RISC-V .

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


/* Include the architecture-dependent CPP macro definitions. */
#include <jitter/machine/jitter-machine.h>

/* Include the architecture-independent Gas macro definitions. */
#include <jitter/jitter-machine-common.S>

.text

/* Do not replace instruction by their compressed variants, even when possible.
   I want to be able to patch instructions independently of what register they
   may be using.
   This might be suboptimal, or there may be no real performance difference;
   in any case it the right way to get things working at the beginning. */
.option norvc

/* Here come the actual snippet definitions, containing the code to be copied
   and patched.  Notice that the order matters, and the calls to jitter_snippet
   here must follow the same order as the enum jitter_snippet_to_patch cases in
   native.h . */
jitter_arrays

/* Load a sign-extended 12-bit literal into a XLEN-bit register.  This
   assembles to something like
     fff06993   # ori x19,x0,-1
   where the 12 leftmost bits, here all ones, are the literal. */
#define LOAD_SIGN_EXTENDED_12BIT_TO_REGISTER(register)  \
  <ori register, x0, -1>
jitter_snippet load_sign_extended_12bit_to_register_0,               \
  LOAD_SIGN_EXTENDED_12BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_sign_extended_12bit_to_register_1,               \
  LOAD_SIGN_EXTENDED_12BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_sign_extended_12bit_to_register_2,               \
  LOAD_SIGN_EXTENDED_12BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_sign_extended_12bit_to_register_3,               \
  LOAD_SIGN_EXTENDED_12BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_sign_extended_12bit_to_register_4,               \
  LOAD_SIGN_EXTENDED_12BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_sign_extended_12bit_to_register_5,               \
  LOAD_SIGN_EXTENDED_12BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)

/* Load a sign-extended 32-bit literal into a XLEN-bit.  This assembles to
   something like
     000019b7   lui x19,0x1
     1119e993   ori x19,x19,0x111
   Both instructions have the immediate in their most significant bits, written
   here on the left following the little-endian convention.
   Since ori uses a sign-extended immediate patching the routine may change it
   into a xori, updating the lui immediate to fit the new instruction. */
#define LOAD_SIGN_EXTENDED_32BIT_TO_REGISTER(register)  \
  <lui register, 0>,                                    \
  <ori register, register, -1>
jitter_snippet load_sign_extended_32bit_to_register_0,               \
  LOAD_SIGN_EXTENDED_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_sign_extended_32bit_to_register_1,               \
  LOAD_SIGN_EXTENDED_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_sign_extended_32bit_to_register_2,               \
  LOAD_SIGN_EXTENDED_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_sign_extended_32bit_to_register_3,               \
  LOAD_SIGN_EXTENDED_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_sign_extended_32bit_to_register_4,               \
  LOAD_SIGN_EXTENDED_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_sign_extended_32bit_to_register_5,               \
  LOAD_SIGN_EXTENDED_32BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)

/* Load a sign-extended 32-bit literal into a XLEN-bit, whose 12 low bits are
   zero.  This assembles to something like
     000019b7   lui x19,0x1  */
#define LOAD_LUI_ONLY_TO_REGISTER(register)  \
  <lui register, 0>
jitter_snippet load_lui_only_to_register_0,               \
  LOAD_LUI_ONLY_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_lui_only_to_register_1,               \
  LOAD_LUI_ONLY_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_lui_only_to_register_2,               \
  LOAD_LUI_ONLY_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_lui_only_to_register_3,               \
  LOAD_LUI_ONLY_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_lui_only_to_register_4,               \
  LOAD_LUI_ONLY_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_lui_only_to_register_5,               \
  LOAD_LUI_ONLY_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)

/* Load an XLEN-bit literal which can be expressed by adding a 32-bit literal to
   the program counter.  The code looks like this:
     00000997   auipc x19,0x0
     03a98993   addi x19,x19,0x03a  */
#define LOAD_PCREL_ADDRESS_TO_REGISTER(register)  \
  <auipc register, 0>,                            \
  <addi register, register, 0>
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

/* Load an XLEN-bit literal which can be expressed by adding a 20-bit literal
   left-shifted by 12 bits to the program counter.  The code looks like this:
     00000997   auipc x19,0x0  */
#define LOAD_PCREL_ADDRESS_NO_ADD_TO_REGISTER(register)  \
  <auipc register, 0>
jitter_snippet load_pcrel_address_no_add_to_register_0,               \
  LOAD_PCREL_ADDRESS_NO_ADD_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_pcrel_address_no_add_to_register_1,               \
  LOAD_PCREL_ADDRESS_NO_ADD_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_pcrel_address_no_add_to_register_2,               \
  LOAD_PCREL_ADDRESS_NO_ADD_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_pcrel_address_no_add_to_register_3,               \
  LOAD_PCREL_ADDRESS_NO_ADD_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_pcrel_address_no_add_to_register_4,               \
  LOAD_PCREL_ADDRESS_NO_ADD_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_pcrel_address_no_add_to_register_5,               \
  LOAD_PCREL_ADDRESS_NO_ADD_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)

#if JITTER_XLEN == 64
/* Load a 64-bit literal into a XLEN-bit register.  This assembles to something
   like (materialising 0x0011223344556677 into x19):
     44556937   lui x18,0x44556
     67796913   ori x18,x18,0x677
     001129b7   lui x19,0x112
     2339e993   ori x19,x19,0x233
     02099993   slli x19,x19,32
     0129e9b3   or x19,x19,x18
  . Notice that the two ori instructions and the final or instruction can be
  turned into xor and xori instructions, with other immediates accordingly
  changed, in order to keep immediate sign extensions into account. */
#define LOAD_64BIT_TO_REGISTER(register)                       \
  <lui JITTER_SCRATCH_REGISTER, 0>,                            \
  <ori JITTER_SCRATCH_REGISTER, JITTER_SCRATCH_REGISTER, -1>,  \
  <lui register, 0>,                                           \
  <ori register, register, -1>,                                \
  <slli register, register, 32>,                               \
  <or register, register, JITTER_SCRATCH_REGISTER>
jitter_snippet load_64bit_to_register_0,               \
  LOAD_64BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_0)
jitter_snippet load_64bit_to_register_1,               \
  LOAD_64BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_1)
jitter_snippet load_64bit_to_register_2,               \
  LOAD_64BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_2)
jitter_snippet load_64bit_to_register_3,               \
  LOAD_64BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_3)
jitter_snippet load_64bit_to_register_4,               \
  LOAD_64BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_4)
jitter_snippet load_64bit_to_register_5,               \
  LOAD_64BIT_TO_REGISTER (JITTER_RESIDUAL_REGISTER_5)
#endif // #if JITTER_XLEN == 64

/* FIXME: these, or similar snippets, are not implemented yet.  I wonder if
   I really need them on such a register-rich architecture. */
/* jitter_snippet load_zero_extended_16bit_to_memory,  \ */
/*            <nop> */
/* jitter_snippet load_sign_extended_16bit_to_memory,  \ */
/*            <nop> */
/* jitter_snippet load_32bit_to_memory,  \ */
/*            <nop> */

/* PC-relative jump to a 20-bit signed offset.  Apart from the bit ordering of
   the immediate field one unusual feature one should remark that the
   destination address is given by PC summed to the immediate, *with the least
   significant bit cleared*.  The least significant bit is not represented in
   the instruction.
   This assembles to something like:
     L: 0000006f   jal x0, L  */
jitter_snippet jump_unconditional_20bit_pcrel,  \
  <1: jal x0, 1b>

/* PC-relative jump-and-link to a 20-bit signed offset.  This is in fact
   identical to the jump_unconditional_20bit_pcrel snippet above, except for
   the return address register.
   This assembles to something like:
        004000ef   jal x1, L
     L:  */
jitter_snippet jump_and_link_20bit_pcrel,  \
  <1: jal x1, 1b>

/* See the comment in jitter-machine.h .  There is no need to copy a snippet
   from here, since the inline assembly code is already correct, and only
   requires patching the destination field.  It is always a single B-type
   instruction such as beq. */
jitter_snippet jump_conditional_13bit_pcrel,  \
  <>
