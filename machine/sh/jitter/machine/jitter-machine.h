/* VM library: SH definitions, to be included from both C and assembly.

   Copyright (C) 2017, 2018, 2019 Luca Saiu
   Updated in 2020 by Luca Saiu
   Written by Luca Saiu

   This file is part of Jitter.

   Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTER_NATIVE_MACHINE_H_ /* One macro suffices for all architectures. */
#define JITTER_NATIVE_MACHINE_H_


/* Assembler syntax.
 * ************************************************************************** */

/* How to introduce comments in assembly on SH. */
#define JITTER_ASM_COMMENT_PREFIX "! "




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "nop\n\t"                                                   \
  "xor #" integer_literal_as_string ", r0\n\t"                \
  "xor #" integer_literal_as_string ", r0\n\t"               \
  "nop"

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                   \
  /* Return from exception handling.  This will cause an exception in user  \
     mode, of a kind not usually seen. */                                   \
  "rte\n\t"                                                                 \
  "nop"




/* Assembly comment syntax.
 * ************************************************************************** */

/* On SH '#' introduces immediate constants, so it is dangerous to use the
   character as a comment.  A '#' character occurring *at the beginning* of a
   line works as a comment on SH, at least with Gas, but it's better to use
   a prefix which can occur anywhere. */
# define JITTER_ASM_COMMENT_PREFIX  \
    "! "




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE  \
  "jmp @%[_jitter_the_target]\n\t"         \
  "nop"  /* Delay slot. */
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  \
  "r"




/* Reserved registers.
 * ************************************************************************** */

/* On SH I seem to be able to reserve the following callee-save registers:
     r8 r9 r10 r11 r13 r14
   I don't want to reserve r0, and I'm not even sure if I could, as it is
   the only valid target for some operations and the only register usable
   for certain addressing modes.
   r12 is the GOT context pointer.
   r14 is supposed to be the frame pointer, but I think I could use it.
   r15 is the stack pointer. */

/* Register pointing to The Array base. */
#define JITTER_BASE_REGISTER          r8

/* How many registers we can use to hold residual arguments. */
#define JITTER_RESIDUAL_REGISTER_NO   3

/* Registers holding residual arguments, with 0-based suffixes.  These have to
   be as many as JITTER_RESIDUAL_REGISTER_NO . */
#define JITTER_RESIDUAL_REGISTER_0    r9
#define JITTER_RESIDUAL_REGISTER_1    r10
#define JITTER_RESIDUAL_REGISTER_2    r11

/* The scratch register. */
#define JITTER_SCRATCH_REGISTER       r13

/* FIXME: I'm probably reserving way too many registers.  I should still be able
   to declare every usable register as a candidate to be reserved, but use only
   a few by default on architectures not providing so many registers, such as
   this one. */




/* Patch-ins.
 * ************************************************************************** */

/* Patch-ins are supported on the the SH architecture. */
#define JITTER_MACHINE_SUPPORTS_PATCH_IN 1

/* I have not checked extensively but an 0xffff instruction seems to always
   trap (and is not recognized by objdump), so this is a convenient choice. */
#define JITTER_ASM_PATCH_IN_FILL_BYTE    "0xff"

/* For each patch-in case define its size in bytes, corresponding to the total
   size of the instructions to be patched in in bytes, including possible
   padding nops. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL    4
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK  4



/* VM conditional branches.
 * ************************************************************************** */

/* FIXME: not implemented yet for SH. */




/* VM branch-and-link support implemented with patch-ins.
 * ************************************************************************** */

/* Procedures are supported on SH ; but we conditionalize the definition
   on JITTER_MACHINE_SUPPORTS_PATCH_IN , for testing convenience. */
#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
# define JITTER_MACHINE_SUPPORTS_PROCEDURE    1
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN)   \
    && defined(JITTER_DISPATCH_NO_THREADING)       \
    && defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)

/* Save the return address from the system register PR to the given destination
   (this is called "store system register" on SH).  We can do that in one
   instruction, as long as the destination is a general register; if not this
   macro will also generate one final store from the general register. */
#define _JITTER_PROCEDURE_PROLOG(link_union)                                   \
  do                                                                           \
    {                                                                          \
      void * jitter_the_return_address;                                        \
      /* This inline asm statement must be volatile because it has no explicit \
         inputs; the actual input is pr , which is not visible from C.         \
         If this is not volatile GCC can move it somewhere else where it is    \
         exectued only once, with the result saved on the stack.  Of course I  \
         don't want that.  VM instructions begin and and with volatile asm     \
         statements, so this cannot be moved across them. */                   \
      asm volatile ("sts pr, %[return_address]"                                \
                    : [return_address] "=r" (jitter_the_return_address)        \
                      /* outputs */);                                          \
      (link_union).pointer = jitter_the_return_address;                        \
    }                                                                          \
  while (false)

/* Restore the return address from a general register to the system register PR
   (this is called "load to system register" on SH); then use the return
   instruction, with its delay slot. */
#define _JITTER_RETURN(link_rvalue)                                            \
  do                                                                           \
    {                                                                          \
      const void * jitter_the_return_address = (const void*) (link_rvalue);    \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                   \
                "lds %[return_addr], pr\n\t"                                   \
                "rts\n\t"                                                      \
                "nop"                                                          \
                : /* outputs. */                                               \
                : [return_addr] "r" (jitter_the_return_address) /* inputs. */  \
                : "pr" /* clobbers. */                                         \
                : jitter_dispatch_label /* gotolabels. */);                    \
      /* The rest of the VM instruction is unreachable. */                     \
      __builtin_unreachable ();                                                \
    }                                                                          \
  while (false)

/* Easy: perform a "jump to subsnippet" (a branch-and-link via register, with an
   absolute target -- the bsrf instruction has a PC-relative target), and the
   return address will be in the system register PR , accessible from the
   procedure prolog.  The branch to subsnippet far instruction has a delay
   slot. */
#define _JITTER_BRANCH_AND_LINK_NATIVE(callee_rvalue)                        \
  do                                                                         \
    {                                                                        \
      const void * const jitter_destination =                                \
        (const void * const) (callee_rvalue);                                \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                 \
                "jsr @%[destination]\n\t"                                    \
                "nop"                                                        \
                : /* outputs. */                                             \
                : [destination] "r" (jitter_destination) /* inputs. */       \
                : "pr" /* clobbers. */                                       \
                : jitter_dispatch_label /* gotolabels. */);                  \
      /* It would be incorrect to have __builtin_unreachable or              \
         JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END here: see the comment in \
         the x86_64 version. */                                              \
    }                                                                        \
  while (false)

/* Perform an ordinary jump thru register, and load PR in the delay slot. */
#define _JITTER_BRANCH_AND_LINK_WITH_NATIVE(_jitter_callee_rvalue,            \
                                            _jitter_new_link)                 \
  do                                                                          \
    {                                                                         \
      const void * const jitter_callee_rvalue =                               \
        (const void * const) (_jitter_callee_rvalue);                         \
      const void * const jitter_new_link =                                    \
        (const void * const) (_jitter_new_link);                              \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                  \
                JITTER_ASM_COMMENT_UNIQUE("Branch-and-link-with, pretending"  \
                                          "to go to "                         \
                                          "%l[jitter_dispatch_label]")        \
                "jmp @%[jitter_callee_rvalue]\n\t"                            \
                "lds %[jitter_new_link], pr"                                  \
                : /* outputs. */                                              \
                : [jitter_callee_rvalue] "r" (jitter_callee_rvalue),          \
                  [jitter_new_link] "r" (jitter_new_link) /* inputs. */       \
                : "pr" /* clobbers. */                                        \
                : jitter_dispatch_label /* gotolabels. */);                   \
      /* The rest of the VM instruction is unreachable: this tail call is an  \
         unconditional jump. */                                               \
      __builtin_unreachable ();                                               \
    }                                                                         \
  while (false)

/* The patch-in has a simple two-instruction snippet ( bsr and nop ), of which
   the first instruction will be patched.  No other code is necessary. */
#define _JITTER_BRANCH_FAST_AND_LINK_NATIVE(target_index)                      \
  do                                                                           \
    {                                                                          \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                   \
                JITTER_ASM_PATCH_IN_PLACEHOLDER(                               \
                   JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK /*size_in_bytes*/, \
                   JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK /*case*/,  \
                   target_index,                                               \
                   0, 0, 0 /* not used for this case */)                       \
                : /* outputs. */                                               \
                : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                       \
                  JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs. */          \
                : /* clobbers. */                                              \
                : jitter_dispatch_label /* gotolabels. */);                    \
      /* It would be incorrect to have __builtin_unreachable or                \
         JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END here: see the comment in   \
         the x86_64 version. */                                                \
    }                                                                          \
  while (false)
#endif // #if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) ...




/* C-only page, expanding to nothing if this header is included by assembly.
 * ************************************************************************** */

#ifndef __ASSEMBLER__

/* Notice that the order matters, and these shouldn't be rearranged without also
   changing the order of definitions in machine.S .  We also rely on the first
   case having value 0, since we use enum jitter_snippet_to_patch values as
   array indices. */
enum jitter_snippet_to_patch
  {
    jitter_snippet_load_signed_8bit_to_register_0,
    jitter_snippet_load_signed_8bit_to_register_1,
    jitter_snippet_load_signed_8bit_to_register_2,

    /* These look inefficient.  I could very easily do zero-extended versions as
       well, but I wonder whether it's worth the trouble.  The SH really suffers
       under simple code generation systems like this where PC-relative loads
       are difficult to exploit. */
    jitter_snippet_load_signed_16bit_8th_bit_1_to_register_0,
    jitter_snippet_load_signed_16bit_8th_bit_1_to_register_1,
    jitter_snippet_load_signed_16bit_8th_bit_1_to_register_2,
    jitter_snippet_load_signed_16bit_8th_bit_0_to_register_0,
    jitter_snippet_load_signed_16bit_8th_bit_0_to_register_1,
    jitter_snippet_load_signed_16bit_8th_bit_0_to_register_2,

    /* Here I use PC-relative loads, but I have to skip over the datum word, and
       worry about alignment. */
    jitter_snippet_load_pcrel_to_register_0_pc_aligned,
    jitter_snippet_load_pcrel_to_register_0_pc_misaligned,
    jitter_snippet_load_pcrel_to_register_1_pc_aligned,
    jitter_snippet_load_pcrel_to_register_1_pc_misaligned,
    jitter_snippet_load_pcrel_to_register_2_pc_aligned,
    jitter_snippet_load_pcrel_to_register_2_pc_misaligned,

    jitter_snippet_branch_unconditional_13bit_offset,
    jitter_snippet_branch_and_link_13bit_offset,

    /* The number of snippets. */
    jitter_snippet_no
  };

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
