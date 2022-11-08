/* VM library: assembly support for x86_64 .

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

/* Load zero into a register by xor-ing it with itself.  This assembles to
   something like
     31 db   # xor %ebx, %ebx
   , but in this case the code doesn't actually need patching.  I can use
   a 32-bit xorl to load zero to the high half as well. */
jitter_snippet load_0_to_64bit_residual_register_0, \
           <xorl JITTER_RESIDUAL_REGISTER_0_32BIT, JITTER_RESIDUAL_REGISTER_0_32BIT>
jitter_snippet load_0_to_64bit_residual_register_1, \
           <xorl JITTER_RESIDUAL_REGISTER_1_32BIT, JITTER_RESIDUAL_REGISTER_1_32BIT>
jitter_snippet load_0_to_64bit_residual_register_2, \
           <xorl JITTER_RESIDUAL_REGISTER_2_32BIT, JITTER_RESIDUAL_REGISTER_2_32BIT>
jitter_snippet load_0_to_64bit_residual_register_3, \
           <xorl JITTER_RESIDUAL_REGISTER_3_32BIT, JITTER_RESIDUAL_REGISTER_3_32BIT>

/* Load -1 into a 64-bit register by or-ing it with a one-byte -1 literal.
   This assembles to something like
     48 83 cb ff   # orq $-1, %rbx
   , but in this case the code doesn't actually need patching. */
jitter_snippet load_minus_1_to_64bit_residual_register_0, \
           <orq $-1, JITTER_RESIDUAL_REGISTER_0>
jitter_snippet load_minus_1_to_64bit_residual_register_1, \
           <orq $-1, JITTER_RESIDUAL_REGISTER_1>
jitter_snippet load_minus_1_to_64bit_residual_register_2, \
           <orq $-1, JITTER_RESIDUAL_REGISTER_2>
jitter_snippet load_minus_1_to_64bit_residual_register_3, \
           <orq $-1, JITTER_RESIDUAL_REGISTER_3>

/* Load a 64-bit literal into a register.  These assemble to something like
     48 bb 00 00 00 00 00 00 00 00   # movabsq $0x0, %rbx
   , where the zeroes are the literal constant to patch over.  The literal
   to be patched over is always at the same offset from the end, independently
   from the destination register.
   It goes without saying that these instructions are huge.  Even if on modern
   CPUs they don't take long to execute, according to Agner Fog they can become
   a bottleneck at decoding time, so they are better avoided.  Whenever the
   literal fits in 32 bits, either zero-extended or sign-extended, there is
   a better solution. */
jitter_snippet set_64bit_residual_register_0,       \
           <movabsq $0, JITTER_RESIDUAL_REGISTER_0>
jitter_snippet set_64bit_residual_register_1,       \
           <movabsq $0, JITTER_RESIDUAL_REGISTER_1>
jitter_snippet set_64bit_residual_register_2,       \
           <movabsq $0, JITTER_RESIDUAL_REGISTER_2>
jitter_snippet set_64bit_residual_register_3,       \
           <movabsq $0, JITTER_RESIDUAL_REGISTER_3>

/* Load a 32-bit literal (or a 64-bit literal which fits zero-extended in
   32 bits -- when writing to the low half of a 64-bit register x86_64 zeroes
   its high half) into a register.
   These assemble to something like
     bb 00 00 00 00   # movl $0x0, %ebx
   , where the zeroes are the literal constant to patch over.  The literal
   to be patched over is always at the same offset *from the end*,
   independently from the destination register; however using the "new"
   registers requires a so-called prefix before the actual opcode, making
   the instruction longer.
   For example:
     41 bd 00 00 00 00   # movl $0x0, %r13d */
jitter_snippet set_32bit_residual_register_0,           \
           <movl $0, JITTER_RESIDUAL_REGISTER_0_32BIT>
jitter_snippet set_32bit_residual_register_1,           \
           <movl $0, JITTER_RESIDUAL_REGISTER_1_32BIT>
jitter_snippet set_32bit_residual_register_2,           \
           <movl $0, JITTER_RESIDUAL_REGISTER_2_32BIT>
jitter_snippet set_32bit_residual_register_3,           \
           <movl $0, JITTER_RESIDUAL_REGISTER_3_32BIT>

/* Sign-extend a 32-bit literal into a 64-bit register.
   These assemble to something like
     48 c7 c0 00 00 00 00  # movq $0x0,%rax
   , where the zeroes are the literal constant to patch over.  The literal
   to be patched over is always at the same offset *from the end*; see the
   comment above. */
jitter_snippet set_32bit_sign_extended_residual_register_0,  \
           <movq $0, JITTER_RESIDUAL_REGISTER_0>
jitter_snippet set_32bit_sign_extended_residual_register_1,  \
           <movq $0, JITTER_RESIDUAL_REGISTER_1>
jitter_snippet set_32bit_sign_extended_residual_register_2,  \
           <movq $0, JITTER_RESIDUAL_REGISTER_2>
jitter_snippet set_32bit_sign_extended_residual_register_3,  \
           <movq $0, JITTER_RESIDUAL_REGISTER_3>

/* Materialise a 64-bit constant expressible as %rip plus a signed 32-bit offset
   into a residual register.  This assembles to something like
     4c 8d 25 0d 00 00 00    # leaq 0xd(%rip), %r12  */
jitter_snippet set_pcrel_address_residual_register_0,         \
  <1: leaq (1b + 1048576)(%rip), JITTER_RESIDUAL_REGISTER_0>
jitter_snippet set_pcrel_address_residual_register_1,         \
  <1: leaq (1b + 1048576)(%rip), JITTER_RESIDUAL_REGISTER_1>
jitter_snippet set_pcrel_address_residual_register_2,         \
  <1: leaq (1b + 1048576)(%rip), JITTER_RESIDUAL_REGISTER_2>
jitter_snippet set_pcrel_address_residual_register_3,         \
  <1: leaq (1b + 1048576)(%rip), JITTER_RESIDUAL_REGISTER_3>

/* Load a 64-bit literal into residual argument memory at a fixed offset,
   using two separate 32-bit store instructions.  I decided to do this
   rather than loading a 64-bit immediate into a scratch register, which
   would need to be kept reserved just for this purpose, and then storing.
   This assembles to something like:
     c7 44 24 0c 00 00 00 00    # movl $0x0, 0xc(%rsp)
     c7 44 24 10 00 00 00 00    # movl $0x0, 0x10(%rsp)
   At 8 bits (signed) the offset from The Array base is small, but should
   always be more than enough for residuals.  Of course the low and high
   halves of the 64-bit datum will have offsets from the base differing by
   4 bytes -- in the example above, 0xc and 0x10.
   The immeditates to patch in are at the end of each instruction:
   four bytes at the end for each 32 bit word, preceded by one byte
   for the offset from the base.  I can ignore the instruction length,
   which as usual on x86_64 is complicated; however I can safely make
   the assumption that the two instructions will have the same length,
   since they take immediate operands of the same size and work with
   the same base register. */
jitter_snippet set_64bit_residual_memory_two_32bit_stores,  \
           <movl $0x0, 0xc(JITTER_BASE_REGISTER)>,          \
           <movl $0x0, 0x10(JITTER_BASE_REGISTER)>

/* Sign-extend a 32-bit constant into a 64-bit memory location at a given
   (small) offset from the base, using just one sign-extending movq
   instruction storing a sign-extended 32-bit literal.
   See the snippet above about the offset restriction.
   This assembles to something like:
     48 c7 43 08 ff ff ff ff    # movq $0xffffffffffffffff, 0x8(%rbx)
   The part to be patched in is at the end: four bytes for the value,
   preceded by one byte for the offset. */
jitter_snippet set_32bit_sign_extended_residual_memory,   \
           <movq $0x00aabbcc, 0xc(JITTER_BASE_REGISTER)>

/* Branch relative (to the end of the jmp instruction) with a 32-bit signed
   offset.  This assembles to something like:
     e9 00 00 00 00   #    jmp L
                      # L:
   An offset of zero, like in the example, would jump to the address
   immediately following the jmp instruction. */
jitter_snippet jump_unconditional_32bit_offset,                             \
           /* <jmp.d32 0> would be nicer, but this syntax is not supported  \
              by old binutils. */                                           \
           <.byte 0xe9, 0, 0, 0, 0>

/* The point after a conditional branch, relative (to the end of the branch
   instruction) with a 32-bit signed offset.  The previous part, *not*
   included in the empty snippet, assembles to something like:
     0f 8? 00 00 00 00   # j? L
   For example:
     0f 87 00 00 00 00   # ja L
     0f 8f 00 00 00 00   # jg L
   An offset of zero, like in the examples, would jump to the address
   immediately following the conditional branch instruction. */
jitter_snippet empty_after_conditional_jump_32bit_offset,                \
           /* No new code to insert.  This patch-in marks the end        \
              of an instruction, already generated, whose last argument  \
              nees to be patched. */                                     \
           <>

/* Call a procedure whose beginning is given relative (to the end of the call
   instruction) with a 32-bit signed offset.  This assembles to something like:
     e8 78 56 34 12   # callq  0x12345678
*/
jitter_snippet call_32bit_offset,    \
           <callq 0x12345678>
