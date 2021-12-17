/* VM library: assembly support for SPARC (either bitness).

   Copyright (C) 2017, 2019 Luca Saiu
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

/* Include configuration definitions: they are acceptable in an assembly file,
   since they do not expand to anything. */
#include <jitter/jitter-config.h>

/* Include the architecture-independent Gas macro definitions. */
#include <jitter/jitter-machine-common.S>

.text

/* Here come the actual snippet definitions, containing the code to be copied
   and patched.  Notice that the order matters, and the calls to jitter_snippet
   here must follow the same order as the enum jitter_snippet_to_patch cases in
   native.h . */
jitter_arrays

/* I need these redundant macro indirections to convince Gas that '%' is
   actually a register prefix, and not a binary operator.  Ugly, but in
   the end readable. */
.macro mybrz_g0 label
  brz,pt %g0, \label
.endm
.macro myor arg0, arg1, arg2
  or \arg0, \arg1, \arg2
.endm

/* Calling this macro will define a snippet to load a 13-bit sign-extended
   constant to \residual_register_name .
   The immediate to patch is conveniently at [12,0].  Notice that all
   arithmetic-logic instructions always sign-extend on SPARC, like on PowerPC
   and differently from MIPS.
     0000004000cd4000 b0 10 20 05   or %g0, 5, %i0  */
.macro jitter_snippet_load_or_to_reg  \
  residual_register_index,            \
  residual_register_name
jitter_snippet load_or_to_reg_&\residual_register_index,  \
   <myor <%g0>, <0>, <\residual_register_name>>
.endm

/* Calling this macro will define a snippet to load a 32-bit constant into
   \residual_register_name .
   The two immediates to be inserted are, convenientely, at [21,0] for sethi
   and at [12,0] for or.  The or instruction only needs the low 10 bits of its
   sign-extended 13-bit immediate here, and we must always zero-extend it from
   10 to 13 bits: the hardware will then sign-extend the 13-bit immediate
   producing zeroes at positions [31,10] of the target register, which is what
   we want in order to preserve the high 22 bits set by sethi.
     0000004000cd4000 31 00 02 af   sethi %hi(0xabc00), %i0 ! High 22 bits
     0000004000cd4004 b0 16 20 de   or %i0, 0xde, %i0       ! Low 10 bits, zero-
                                                            ! -extended to 13 */
.macro jitter_snippet_load_sethi_or_to_reg  \
  residual_register_index,                  \
  residual_register_name
jitter_snippet load_sethi_or_to_reg_&\residual_register_index,      \
  <sethi 0, \residual_register_name>,                               \
  <myor <\residual_register_name>, <0>, <\residual_register_name>>
.endm

/* Calling this macro will define a snippet to load a 32-bit constant whose
   low 10 bits are zero into \residual_register_name .
   An optimized special case of jitter_snippet_load_sethi_or_to_reg , for cases
   where the low part is zero.  This only contains the sethi instruction, to
   be patched just like above. */
// FIXME: this is not used yet.
.macro jitter_snippet_load_sethi_to_reg  \
  residual_register_index,               \
  residual_register_name
jitter_snippet load_sethi_to_reg_&\residual_register_index,  \
   <sethi 0xabc, \residual_register_name>
.endm

/* Calling this macro will define a snippet to load a 64-bit constant into
   \residual_register_name , clobbering the scratch register.
   The constants to be inserted are at the lowest bits of the first four
   instructions: [21,0] for the two sethi instructions and [12,0] for the two
   or instructions.  See the jitter_snippet_load_sethi_or_to_reg comment about
   sign-extension and zero-extension.
     2d 04 44 6a   sethi %hi(0x1111a800), %l6  ! %hi(high_half)
     31 08 88 ae   sethi %hi(0x2222b800), %i0  ! %hi(low_half)
     ac 15 a2 aa   or %l6, 0x2aa, %l6          ! %lo(high_half)
     b0 16 23 bb   or %i0, 0x3bb, %i0          ! %lo(low_half)
     ad 2d b0 20   sllx %l6, 0x20, %l6
     b0 16 00 16   or %i0, %l6, %i0  */
.macro jitter_snippet_load_64bit_to_reg  \
  residual_register_index,               \
  residual_register_name
jitter_snippet load_64bit_to_reg_&\residual_register_index,                    \
   <setx 0x1111aaaa2222bbbb, JITTER_SCRATCH_REGISTER, \residual_register_name>
.endm

/* Define all the residual-register loading snippets for the one given target
   residual register, in the right order. */
.macro jitter_snippets_load_to_reg  \
    residual_register_index,        \
    residual_register_name
jitter_snippet_load_or_to_reg <\residual_register_index>, \
  <\residual_register_name>
jitter_snippet_load_sethi_or_to_reg <\residual_register_index>, \
  <\residual_register_name>
jitter_snippet_load_sethi_to_reg <\residual_register_index>, \
  <\residual_register_name>
jitter_snippet_load_64bit_to_reg <\residual_register_index>, \
  <\residual_register_name>
.endm

/* Define all the residual-register loading snippets for every target residual
   register, in the right order. */
jitter_snippets_load_to_reg 0, <JITTER_RESIDUAL_REGISTER_0>
jitter_snippets_load_to_reg 1, <JITTER_RESIDUAL_REGISTER_1>
jitter_snippets_load_to_reg 2, <JITTER_RESIDUAL_REGISTER_2>
jitter_snippets_load_to_reg 3, <JITTER_RESIDUAL_REGISTER_3>
jitter_snippets_load_to_reg 4, <JITTER_RESIDUAL_REGISTER_4>
jitter_snippets_load_to_reg 5, <JITTER_RESIDUAL_REGISTER_5>
jitter_snippets_load_to_reg 6, <JITTER_RESIDUAL_REGISTER_6>
jitter_snippets_load_to_reg 7, <JITTER_RESIDUAL_REGISTER_7>
jitter_snippets_load_to_reg 8, <JITTER_RESIDUAL_REGISTER_8>
jitter_snippets_load_to_reg 9, <JITTER_RESIDUAL_REGISTER_9>
jitter_snippets_load_to_reg 10, <JITTER_RESIDUAL_REGISTER_10>
jitter_snippets_load_to_reg 11, <JITTER_RESIDUAL_REGISTER_11>

/* Branch unconditionally, to a location a singned 18-bit displacement
   away from the beginning of the jumping instruction.  The two least
   significant bits of the displacement are not represented, and the
   rest is split into two fields: 2 high bits at [21,20], and 14 low
   bits at [13,0].
   This is actually a conditional branch on %g0, branching when it is
   zero; the encoded register index doesn't need to be patched.
     0000004000cd400c 02 cc 00 04   brz %l0, 0x0000004000cd401c
     0000004000cd4010 01 00 00 00   nop   ! delay slot */
jitter_snippet branch_unconditional_18bits_offset,  \
   <mybrz_g0 1f>,                                   \
   <nop>,                                           \
   <1:>

/* Branch unconditionally, to a location a singned 21-bit displacement
   away from the beginning of the jumping instruction.  The two least
   significant bits of the displacement are not represented, and the
   rest is simply laid out at [18,0].
   This looks very easy, but my patching snippet is clearly incorrect.
   See the comment in jitter-machine-c.c .
     0000004000cd4014 10 bf ff fd   b 0x0000004000cd4008
     0000004000cd4018 01 00 00 00   nop   ! delay slot */
jitter_snippet branch_unconditional_21bits_offset,  \
   <ba 0>,                                          \
   <nop>
