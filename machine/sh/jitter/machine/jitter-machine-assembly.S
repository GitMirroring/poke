/* VM library: assembly support for SH .

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

/* SH uses '#' as an immediate prefix, but of course CPP messes with it.
   JITTER_HASHED(n) will expand to #n . */
#define JITTER_HASH_CHARACTER #
#define JITTER_EXPAND_ONCE(x) x
#define JITTER_HASHED(x) JITTER_EXPAND_ONCE(JITTER_HASH_CHARACTER)x

/* Handling '#' across both CPP and Gas macros is, unfortunately, very
   fragile. */
.macro jitter_op_immediate_to_register opcode, immediate, register_name
   \opcode JITTER_HASHED(\immediate)&, \register_name
/* If I keep this auxiliary macro jitter_mov_immediate_to_register then this
   alternative, within the macro, seems to work as well; but I don't
   completely understand why it does in the presence of CPP, so I keep
   this disabled.
     \opcode #\immediate&, \register_name */
.endm

/* Load a signed 8 bit value to a general register.  The value is always
   sign-extended.
      09 ea   ! mov #9, r10  (little-endian) */
.macro jitter_snippet_load_signed_8bit residual_register_index,  \
                                       residual_register_name
jitter_snippet load_signed_8bit_to_register_&\residual_register_index,  \
    <jitter_op_immediate_to_register mov 0 \residual_register_name>
.endm

/* Define a snippet per residual register.  This follows the order of enum
   jitter_snippet_to_patch . */
jitter_snippet_load_signed_8bit 0 JITTER_RESIDUAL_REGISTER_0
jitter_snippet_load_signed_8bit 1 JITTER_RESIDUAL_REGISTER_1
jitter_snippet_load_signed_8bit 2 JITTER_RESIDUAL_REGISTER_2

/* Load a sign-extended 16-bit value to a general register.  Since an or
   instruction with an immediate argument can only work on r0 I'd have to save
   and restore r0; I've tried, but it takes one instruction more and I don't see
   any advantages.  Instead here I use a mov #immediate, SCRATCH , which
   unfortunately sign-extends, and then clear all the scratch register bits
   except the ones in the low byte with an extu.b ; at that point, after
   shifting, I can or the scratch register into the destination.
   This version is to be used when the highest bit of the low byte of the
   constant is 1; othwerwise the extu.b instruction is useless, and for
   that case I have another set of snippets below.  Notice that this version
   is correct independently from the value of that bit; the variant below is
   faster, but its correctness relies on the bit being 0.
      30 e9   mov #48,r9      ! Sign-extend high byte into result's low byte
      39 ec   mov #57,r12     ! Move low byte to scratch, sign-extending
      18 49   shll8 r9        ! Logically left-shift target, clearing low byte
      cc 6c   extu.b r12,r12  ! Un-sign-extend scratch
      cb 29   or r12,r9       ! Or scratch into the result low byte.
   FIXME: is this version faster than the generic pcrel version below, using
   three instructions including a jump? */
.macro jitter_snippet_load_signed_16bit_8th_bit_1_to_register \
    residual_register_index,  \
    residual_register_name
jitter_snippet load_signed_16bit_8th_bit_1_to_register_&\residual_register_index,  \
    <jitter_op_immediate_to_register mov 0xff residual_register_name>, \
    <jitter_op_immediate_to_register mov 0xee JITTER_SCRATCH_REGISTER>, \
    <shll8 residual_register_name>, \
    <extu.b JITTER_SCRATCH_REGISTER, JITTER_SCRATCH_REGISTER>, \
    <or JITTER_SCRATCH_REGISTER, residual_register_name>
.endm

/* Old alternative version using r0. */
/* Load a sign-extended 16-bit value to a general register.  Since the or and
   and operations with an immediate only work on r0 I have to save it and
   restore it.
      03 6c   mov r0,r12  ! Save r0 to the scratch register
      fe e0   mov #-2,r0  ! Copy the high byte to r0's low byte, sign-extending
      18 40   shll8 r0    ! Left-shift r0 by one byte; the low byte is now zero
      0c cb   or #12,r0   ! Or the low byte in
      03 69   mov r0,r9   ! Copy r0 to the intended target register
      c3 60   mov r12,r0  ! Restore r0 from the scratch register */
 /*
.macro jitter_snippet_load_signed_16bit residual_register_index,  \
                                        residual_register_name
jitter_snippet load_signed_16bit_to_register_&\residual_register_index,  \
    <mov r0, JITTER_SCRATCH_REGISTER>, \
    <jitter_op_immediate_to_register mov 0xff r0>, \
    <shll8 r0>, \
    <jitter_op_immediate_to_register or 0x2 r0>, \
    <mov r0, \residual_register_name&>, \
    <mov JITTER_SCRATCH_REGISTER, r0>
.endm
 */

/* Define a snippet per residual register.  This follows the order of enum
   jitter_snippet_to_patch . */
jitter_snippet_load_signed_16bit_8th_bit_1_to_register  \
   0 JITTER_RESIDUAL_REGISTER_0
jitter_snippet_load_signed_16bit_8th_bit_1_to_register  \
   1 JITTER_RESIDUAL_REGISTER_1
jitter_snippet_load_signed_16bit_8th_bit_1_to_register  \
   2 JITTER_RESIDUAL_REGISTER_2

/* See the comment above about
   jitter_snippet_load_signed_16bit_8th_bit_1_to_register ; this is identical,
   except that it doesn't have the extu.b instruction.  This variant is only
   correct if the 8-th bit (the highest bit of the low byte) is zero.
   This version is not always optimal either: if the low byte were all 0s I
   could do everything in two instructions ( mov , shll8 ).  But I guess
   that case is not common enough.
   The immediates to be patched from C are at the same positions as in the
   jitter_snippet_load_signed_16bit_8th_bit_1_to_register snippets. */
.macro jitter_snippet_load_signed_16bit_8th_bit_0_to_register \
    residual_register_index,  \
    residual_register_name
jitter_snippet load_signed_16bit_8th_bit_0_to_register_&\residual_register_index,  \
    <jitter_op_immediate_to_register mov 0xff residual_register_name>, \
    <jitter_op_immediate_to_register mov 0xee JITTER_SCRATCH_REGISTER>, \
    <shll8 residual_register_name>, \
    <or JITTER_SCRATCH_REGISTER, residual_register_name>
.endm

/* Define a snippet per residual register.  This follows the order of enum
   jitter_snippet_to_patch . */
jitter_snippet_load_signed_16bit_8th_bit_0_to_register  \
   0 JITTER_RESIDUAL_REGISTER_0
jitter_snippet_load_signed_16bit_8th_bit_0_to_register  \
   1 JITTER_RESIDUAL_REGISTER_1
jitter_snippet_load_signed_16bit_8th_bit_0_to_register  \
   2 JITTER_RESIDUAL_REGISTER_2

/* This way of loading constants works for any 32-bit value and is not so
   terrible in the end, even with the branch: it costs three instructions
   including the delay slot, or 48 instructions bits plus 32 data bits, and
   possibly 16 bits more for padding.  Unfortunately I can't execute the
   PC-relative load in the branch delay slot according to the spec, even if QEmu
   accepts it.

   SH instructions are 16-bit wide and only aligned to 16 bits, but 32-bit
   constants embedded among instructions have to be aligned to 32 bits.
   Therefore there may be two possible situations, according to the alignment of
   the code coming right before one of these snippets: either the PC happens to
   be aligned to 32 bits, or not.  In one case I have to insert a 16-bit
   padding, and that is the *aligned* case, since the padding comes right before
   an odd number of instructions: mov.l , bra , nop .

   The PC-relative offset is encoded as the same in both cases even if the
   actual distance is different.  The spec says that "the 8-bit displacement is
   zero-extended and quadrupled [...] but the lowest two bits of the PC are
   corrected to B'00".  Moreover the GNU Binutils show the offset as bytes from
   the beginning of the current instruction (therefore 8), while the actual
   encoding computes the offset from PC+4, which would make it 4 or 6 --
   right-shifted to be encoded in the instruction immediate, 0x01.

   This is little-endian, but the big-endian case uses the exact same logic
   and the exact same offsets.
      01 d9        mov.l @(8, pc), r9  ! With Gas I could say "mov.l D, r9".
      03 a0        bra L
      09 00        nop
      00 00        .word <padding>
   D: 12 ef cd ab  .long <datum>
   L: ...                              ! Out of the snippet. */
.macro jitter_snippets_load_pcrel residual_register_index,  \
                                  residual_register_name
jitter_snippet load_pcrel_to_register_&\residual_register_index&_pc_aligned,  \
    <mov.l @(8, pc), \residual_register_name>, \
    <bra 1f>, \
    <nop>, \
    <.word 0x0000> /* Padding*/, \
    <.long 0xabcdef12>, \
    <1:>
jitter_snippet load_pcrel_to_register_&\residual_register_index&_pc_misaligned, \
    <mov.l @(8, pc), \residual_register_name>, \
    <bra 1f>, \
    <nop>, \
    <.ualong 0xabcdef12> /* Misaligned here but not in the copy. */, \
    <1:>
.endm

/* Define two snippets, one aligned and one mialigned, per residual register.
   This follows the order of enum jitter_snippet_to_patch . */
jitter_snippets_load_pcrel 0 JITTER_RESIDUAL_REGISTER_0
jitter_snippets_load_pcrel 1 JITTER_RESIDUAL_REGISTER_1
jitter_snippets_load_pcrel 2 JITTER_RESIDUAL_REGISTER_2


/* Branch to the instruction identified by a signed 13-bit displacement (aligned
   to 16 bits, so that only 12 bits are represented) from PC + 4; include a nop
   for the delay slot.  This example is little-endian.
   L: fe af   bra L ! 0xffe, or -2, is half the offset from the end of the nop.
      09 00   nop   ! From here to the branch target the offset is -4 bytes. */
jitter_snippet branch_unconditional_13bit_offset, \
   <1: bra 1b>, \
   <nop>

/* This has the same encoding as branch_unconditional_13bit_offset , the only
   difference being in the four bits coming before the 12-bit immediate. */
jitter_snippet branch_and_link_13bit_offset, \
   <1: bsr 1b>, \
   <nop>
