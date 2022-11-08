/* VM library: assembly support for PowerPC .

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

.text

/* Here come the actual snippet definitions, containing the code to be copied
   and patched.  Notice that the order matters, and the calls to jitter_snippet
   here must follow the same order as the enum jitter_snippet_to_patch cases in
   native.h . */
jitter_arrays

/* Load a sign-extended 16-bit literal into a 32-bit register.  This
   assembles to something like
     3b e0 00 42   # addi %r31, %r0, 0x42
   where the rightmost 16-bits, here 0x0042, are the literal.  In the case
   of addi (but not, unfortunately, for ori) %r0 is taken to mean zero, and
   not the content of %r0.

   Using bodies such as
     <addi JITTER_RESIDUAL_REGISTER_0, %r0, 0>
   used to work fine with older versions of Gas, but now I'm seeing warnings
   (but still the same correct code).
   I suppose this counterintuitive use or %r0 has been deprecated, even if I
   find nothing specific in the documentation at a quick look.
   According to the PowerPC user instruction set architecture manual
     li Rx, value
   is just an extended mnemonic for
     addi Rx, 0, value.
   I will now use
     <addi JITTER_RESIDUAL_REGISTER_0, 0, 0>
   .  I prefer never to use extended mnemonic or pseudo-instructions, out
   of fear that their encoding may change in the future, breaking my patch
   mechanism. */
jitter_snippet load_sign_extended_16bit_to_register_0, \
           <addi JITTER_RESIDUAL_REGISTER_0, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_1, \
           <addi JITTER_RESIDUAL_REGISTER_1, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_2, \
           <addi JITTER_RESIDUAL_REGISTER_2, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_3, \
           <addi JITTER_RESIDUAL_REGISTER_3, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_4, \
           <addi JITTER_RESIDUAL_REGISTER_4, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_5, \
           <addi JITTER_RESIDUAL_REGISTER_5, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_6, \
           <addi JITTER_RESIDUAL_REGISTER_6, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_7, \
           <addi JITTER_RESIDUAL_REGISTER_7, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_8, \
           <addi JITTER_RESIDUAL_REGISTER_8, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_9, \
           <addi JITTER_RESIDUAL_REGISTER_9, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_10, \
           <addi JITTER_RESIDUAL_REGISTER_10, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_11, \
           <addi JITTER_RESIDUAL_REGISTER_11, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_12, \
           <addi JITTER_RESIDUAL_REGISTER_12, 0, 0>
jitter_snippet load_sign_extended_16bit_to_register_13, \
           <addi JITTER_RESIDUAL_REGISTER_13, 0, 0>


/* Load a 32-bit literal into a 32-bit register.  This assembles to
   something like
     3c a0 12 34   # lis %r5,      0x1234
     60 a5 56 78   # ori %r5, %r5, 0x5678
   where the first instruction loads the high half and the second the low
   half.  Both halves are encoded in the rightmost 16 bits of each
   instruction.  The 32-bit literal in this example is 0x12345678 . */
jitter_snippet load_32bit_to_register_0,                               \
           <lis JITTER_RESIDUAL_REGISTER_0, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_0, JITTER_RESIDUAL_REGISTER_0, 0>
jitter_snippet load_32bit_to_register_1,                               \
           <lis JITTER_RESIDUAL_REGISTER_1, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_1, JITTER_RESIDUAL_REGISTER_1, 0>
jitter_snippet load_32bit_to_register_2,                               \
           <lis JITTER_RESIDUAL_REGISTER_2, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_2, JITTER_RESIDUAL_REGISTER_2, 0>
jitter_snippet load_32bit_to_register_3,                               \
           <lis JITTER_RESIDUAL_REGISTER_3, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_3, JITTER_RESIDUAL_REGISTER_3, 0>
jitter_snippet load_32bit_to_register_4,                               \
           <lis JITTER_RESIDUAL_REGISTER_4, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_4, JITTER_RESIDUAL_REGISTER_4, 0>
jitter_snippet load_32bit_to_register_5,                               \
           <lis JITTER_RESIDUAL_REGISTER_5, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_5, JITTER_RESIDUAL_REGISTER_5, 0>
jitter_snippet load_32bit_to_register_6,                               \
           <lis JITTER_RESIDUAL_REGISTER_6, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_6, JITTER_RESIDUAL_REGISTER_6, 0>
jitter_snippet load_32bit_to_register_7,                               \
           <lis JITTER_RESIDUAL_REGISTER_7, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_7, JITTER_RESIDUAL_REGISTER_7, 0>
jitter_snippet load_32bit_to_register_8,                               \
           <lis JITTER_RESIDUAL_REGISTER_8, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_8, JITTER_RESIDUAL_REGISTER_8, 0>
jitter_snippet load_32bit_to_register_9,                               \
           <lis JITTER_RESIDUAL_REGISTER_9, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_9, JITTER_RESIDUAL_REGISTER_9, 0>
jitter_snippet load_32bit_to_register_10,                               \
           <lis JITTER_RESIDUAL_REGISTER_10, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_10, JITTER_RESIDUAL_REGISTER_10, 0>
jitter_snippet load_32bit_to_register_11,                               \
           <lis JITTER_RESIDUAL_REGISTER_11, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_11, JITTER_RESIDUAL_REGISTER_11, 0>
jitter_snippet load_32bit_to_register_12,                               \
           <lis JITTER_RESIDUAL_REGISTER_12, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_12, JITTER_RESIDUAL_REGISTER_12, 0>
jitter_snippet load_32bit_to_register_13,                               \
           <lis JITTER_RESIDUAL_REGISTER_13, 0>,                        \
           <ori JITTER_RESIDUAL_REGISTER_13, JITTER_RESIDUAL_REGISTER_13, 0>

/* FIXME: actually implement. */
jitter_snippet load_sign_extended_16bit_to_memory,  \
           <nop>

/* FIXME: actually implement. */
jitter_snippet load_32bit_to_memory,  \
           <nop>

/* Unconditionally branch to an instruction, with a signed 26-bit offset
   relative to the beginning of the branch instruction.
   The opcode takes the six most significant bits, and is set to 16
   decimal.
   The last two bits of the 26-bit offset are zero, not encoded; the two
   least significant bits of the instruction are the AA and LK flags, which
   I set to zero for a relative non-link branch.
      48 00 00 00   # L: b L  */
jitter_snippet jump_unconditional_26bit_offset_no_link,  \
  <1: b 1b>

/* This is an easy variant of the previous snippet, the only difference being
   the least significant bit set to 1 for the LK flag. */
jitter_snippet jump_and_link_26bit_offset,  \
  <1: bl 1b>

/* A dummy 0-byte snippet: the code generated from C with inline assembly is
   already correct, and only the displacement needs to be patched. */
jitter_snippet jump_conditional_16bit_offset,  \
  <> /* no actual snippet: the snippet size is zero bytes. */
