/* VM library: x86_64 definitions, to be included from both C and assembly.

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


#ifndef JITTER_NATIVE_MACHINE_H_ /* One macro suffices for all architectures. */
#define JITTER_NATIVE_MACHINE_H_


/* Assembler syntax.
 * ************************************************************************** */

/* How to introduce comments in assembly on x86_64. */
#define JITTER_ASM_COMMENT_PREFIX "# "




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "nopl " integer_literal_as_string "(%%eax)"

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                   \
  /* Return from interrupt.  This will cause an exception in user mode, of  \
     a kind not usually seen. */                                            \
  "iretq"




/* Optional features.
 * ************************************************************************** */

/* If this macro is defined then implement branch-and-link differently, without
   using callq and retq: instead use %rip-relative address to compute the link
   address with a leaq instruction, load the return address into the scratch
   register, here used like a link register, and do a regular jump.
   This ended up being slower than the obvious solution on the machines I
   tested but is useful to keep as an example, possibly to be adapted to
   other architectures */
//#define JITTER_BRANCH_AND_LINK_NO_CALL 1




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE          "jmpq *%[_jitter_the_target]"
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  "rm"




/* Reserved registers.
 * ************************************************************************** */

/* On x86_64 the following registers should not be clobbered by calls, and
   therefore are suitable to reserve:
     %rbx %r12 %r13 %r14 %r15 .
   In theory I would like to reserve
     %rbp
   as well, but I've seen GCC complain
     ("error: bp cannot be used in asm here", and
      "error: frame pointer required, but reserved")
   in some cases if I reserve %rbp with vector code, so that is better avoided.
   The frame pointer seems to be required for vectorized code with some AVX or
   SSE variant, even if I don't understand the details. */

/* Register pointing to the base of The Array.  This is always used as a
   64-bit register, so no _32BIT version is needed. */
#define JITTER_BASE_REGISTER                %rbx

/* How many registers we can use to hold residual arguments.  In the
   (experimental) JITTER_BRANCH_AND_LINK_NO_CALL mode I reserve one register as
   scratch instead of as residual.  See the comment below.  It is harmless to
   still unconditionally define all the residual register names; only as many
   as the expansion of this macro will actually be reserved, at most. */
/* FIXME: redefine this as 4, but reserve fewer register by default on
   register-starved architectures like this one. */
#ifdef JITTER_BRANCH_AND_LINK_NO_CALL
# define JITTER_RESIDUAL_REGISTER_NO       0 //3
#else /* Ordinary procedures, based on callq and retq . */
# define JITTER_RESIDUAL_REGISTER_NO       1 // 4
#endif // #ifdef JITTER_BRANCH_AND_LINK_NO_CALL

/* Registers holding residual arguments, with 0-based suffixes.  These have to
   be as many as JITTER_RESIDUAL_REGISTER_NO .  On x86_64 I also need to use the low
   32 bits of the registers, which have different names; since I couldn't find
   a way of generating them programmatically from suffixes in all cases using
   either Gas or CPP macros due to the irregularity of the syntax (%eax<->%rax
   vs. %r12<->%r12d), the different definitions need to be kept synchronized by
   hand. */
#define JITTER_RESIDUAL_REGISTER_0          %r12
#define JITTER_RESIDUAL_REGISTER_0_32BIT    %r12d
#define JITTER_RESIDUAL_REGISTER_1          %r13
#define JITTER_RESIDUAL_REGISTER_1_32BIT    %r13d
#define JITTER_RESIDUAL_REGISTER_2          %r14
#define JITTER_RESIDUAL_REGISTER_2_32BIT    %r14d
/* This last register is only used when JITTER_BRANCH_AND_LINK_NO_CALL is
   undefined. */
#define JITTER_RESIDUAL_REGISTER_3          %r15
#define JITTER_RESIDUAL_REGISTER_3_32BIT    %r15d

/* This architecture does not need a scratch register to materialize immediates.
   (Search for "scratch register" in the jitter-machine.S comments); however
   a further register to reserve will be useful for the mostly experimental
   JITTER_BRANCH_AND_LINK_NO_CALL mode, as a link register.

   The ordinary mode relying on native callq/retq instructions and the hardware
   stack for procedures does not need a scratch register, which is quite
   important to reduce register pressure on an architecture like this. */
#ifdef JITTER_BRANCH_AND_LINK_NO_CALL
# define JITTER_SCRATCH_REGISTER            JITTER_RESIDUAL_REGISTER_3
# define JITTER_SCRATCH_REGISTER_32BIT      JITTER_RESIDUAL_REGISTER_3_32BIT
#endif // #ifdef JITTER_BRANCH_AND_LINK_NO_CALL




/* Patch-ins.
 * ************************************************************************** */

/* Patch-ins are supported on the x86_64 architecture. */
#define JITTER_MACHINE_SUPPORTS_PATCH_IN    1

/* Skip the rest of this page if JITTER_MACHINE_SUPPORTS_PATCH_IN has been
   disabled (presumably for testing), or if patch-ins are not used with the
   current dispatching mode. */
#if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN)  \
    && defined(JITTER_DISPATCH_NO_THREADING)

/* It's nice to have a byte always making up invalid instructions when repeated
   any number of times; this should always be invalid on x86_64 in long mode,
   even if the hexadecimal value is not particularly memorable or easy to
   recognize at a glance. */
#define JITTER_ASM_PATCH_IN_FILL_BYTE       "0xea"

/* For each patch-in case define its size in bytes, corresponding to the total
   size of the instructions to be patched in in bytes.
   On this archtiecture, in the case of conditioanl branches, I made the choice
   of using patch-ins to record the end of the conditional branch instruction.
   This means that the snippet to be inserted has size zero, and the code to
   be patched comes right *before* the pointer. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL        5
#define JITTER_PATCH_IN_SIZE_0_FAST_BRANCH_AFTER_CONDITIONAL  0 /* all cases. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK      5

/* This macro serves to factor the common code in every low-level conditional on
   x86_64, by emitting a comparing instruction followed by a conditional
   branching instruction. The patch-in case is not really used here: every
   sequence of a comparing instruction followed by a branching instruction is
   patched the same way (in the branch part only). */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_(compare_insn_template,       \
                                                   branch_insn_template,        \
                                                   opd0_constraints, opd0,      \
                                                   opd1_constraints, opd1,      \
                                                   tgt)                         \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                        \
            /* In AT&T syntax operand1 comes before operand0: this is not a     \
               mistake. */                                                      \
            compare_insn_template " %[jitter_operand1], %[jitter_operand0]\n\t" \
            branch_insn_template ".d32 .jitter_irrelevant_label_%=\n"           \
            ".jitter_irrelevant_label_%=:\n\t"                                  \
            /* This patch-in marks the end of the instruction to be patched,    \
               and has actually size zero: I already emitted the correct        \
               branching instruction with the correct prefixes and opcode; the  \
               only thing remaining to patch in is the destination displacement \
               at the very end of the instruction we just emitted, which the    \
               ".d32" above forced to always be 32-bit wide. */                 \
            JITTER_ASM_PATCH_IN_PLACEHOLDER(                                    \
               JITTER_PATCH_IN_SIZE_0_FAST_BRANCH_AFTER_CONDITIONAL /*size*/,   \
               JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /*case*/,       \
               tgt,                                                             \
               0, 0, 0 /* not used for this case */)                            \
            : /* outputs */                                                     \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                            \
              [jitter_operand0] opd0_constraints (opd0),                        \
              [jitter_operand1] opd1_constraints (opd1),                        \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */                \
            : "cc" /* clobbers */                                               \
            : jitter_fake_target /* goto labels */)

/* Low-level conditional fast-branches.  
   Implementation note: I could use testq in tests against zero and in tests for
   sign.  In fact testq would be one byte smaller than cmpq if the operand (in
   this case to be repeated twice) were in a register, but there is way of making
   sure it doesn't have to be loaded from memory just in order to satisfy the
   constraint. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_               \
     ("cmpq", "jz", "rm", (opd0), "e", (0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                  \
     ("cmpq", "jnz", "rm", (opd0), "e", (0), (tgt))

#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_POSITIVE_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                   \
     ("cmpq", "jg", "rm", (opd0), "e", (0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONPOSITIVE_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                      \
     ("cmpq", "jle", "rm", (opd0), "e", (0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                   \
     ("cmpq", "js", "rm", (opd0), "e", (0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONNEGATIVE_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                      \
     ("cmpq", "jns", "rm", (opd0), "e", (0), (tgt))

#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_EQUAL_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                      \
     ("cmpq", "je", "rm", (opd0), "er", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTEQUAL_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                         \
     ("cmpq", "jne", "rm", (opd0), "er", (opd1), (tgt))

#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                              \
     ("cmpq", "jb", "rm", (opd0), "er", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                            \
     ("cmpq", "jl", "rm", (opd0), "er", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                 \
     ("cmpq", "ja", "rm", (opd0), "er", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                               \
     ("cmpq", "jg", "rm", (opd0), "er", (opd1), (tgt))

#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                 \
     ("cmpq", "jae", "rm", (opd0), "er", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                               \
     ("cmpq", "jge", "rm", (opd0), "er", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                    \
     ("cmpq", "jbe", "rm", (opd0), "er", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                  \
     ("cmpq", "jle", "rm", (opd0), "er", (opd1), (tgt))

#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                    \
     ("testq", "jnz", "rm", (opd0), "er", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTAND_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                       \
     ("testq", "jz", "rm", (opd0), "er", (opd1), (tgt))

/* This factors the common code of low-level primitives for checking overflow.
   On x86_64 it is convenient to define as primitives the
   operate-and-branch-on-overflow operations, actually computing a result --
   based on those, the other branch-on-overflow primitives will be defined
   automatically in
      jitter/jitter-fast-branch-machine-generated.h
   , which however can also do the converse.
   On other architecture generating a correct result is not necessarily so
   obvious, so it may be better to follow a different route and only define
   branch-on-overflow primitives, with the automatic script generating
   operate-and-branch-on-overflow macros based on them. */
#define _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_(res, insn, opd0,   \
                                                             opd1, tgt)         \
  const jitter_int _jitter_opd0_value = (opd0);                                 \
  const jitter_int _jitter_opd1_value = (opd1);                                 \
  /* This is a dirty way of working around GCC's restriction on asm goto        \
     statements not supporting output operands.  GCC does not know that         \
     _jitter_tmp is changed by the asm goto statement, and it receives it as    \
     an *input* operand.  Still, the same local register variable is then used  \
     as an input/output for another (dummy) inline assembly statement, and      \
     then read.  This is enough, I suppose, for the register to remain          \
     assigned to the variable during its entire life time.  Nothing should      \
     clobber the register. */                                                   \
  register jitter_int _jitter_tmp asm ("%rax") = _jitter_opd0_value;            \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                    \
     (insn, "jo", "r", (_jitter_tmp), "er", (_jitter_opd1_value), (tgt));       \
  /* This is the critical point: see the comment above.  I want to make sure    \
     that _jitter_tmp remains in the same register, which is to say, that       \
     (res) gets assigned the updated value.  GCC cannot move this statement     \
     above the asm goto statement, because it is volatile -- asm goto           \
     statements being also implicitly volatile; so the most recent updated      \
     value of _jitter_tmp right before the assignment to (res) will be in the   \
     register I am expecting, and nowhere else. */                              \
  asm volatile ("": "+r" (_jitter_tmp));                                        \
  (res) = _jitter_tmp

/* The operate-and-branch-on-overflow primitives.  Division and remainder use
   the default definition. */
#define _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_(res, opd0,  \
                                                        opd1, tgt)  \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_              \
     (res, "addq", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_(res, opd0,  \
                                                         opd1, tgt)  \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_               \
     (res, "subq", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_TIMES_BRANCH_FAST_IF_OVERFLOW_(res, opd0,  \
                                                         opd1, tgt)  \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_               \
     (res, "imulq", (opd0), (opd1), (tgt))

/* FIXME: figure out how to handle known immediates in a clean way.  On this
   architecture immediates fit naturally on the right, as operand 1; therefore
   I should define low-level conditional branches with immediates on the right
   only, and let the machine-generated code define the missing cases with known
   literals on the left. */

#endif // #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(...




/* VM procedures.
 * ************************************************************************** */

/* Procedures are supported on x86_64 ; but we conditionalize the definition
   on JITTER_MACHINE_SUPPORTS_PATCH_IN , for testing convenience. */
#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
# define JITTER_MACHINE_SUPPORTS_PROCEDURE    1
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN)   \
    && defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)

#ifdef JITTER_BRANCH_AND_LINK_NO_CALL

/* In this configuration I don't use the hardware stack to hold return
   addresses; instead I will use simple jumps to transfer control, and
   the scratch register as a link register. */

/* Procedure prolog, the version not relying on callq / retq . */
#define _JITTER_PROCEDURE_PROLOG(link_union)                   \
  do                                                           \
    {                                                          \
      asm ("movq %" JITTER_STRINGIFY(JITTER_SCRATCH_REGISTER)  \
           ", %[the_link_pointer]"                             \
           : [the_link_pointer] "=rm"                          \
             ((link_union).pointer) /* outputs */              \
           : /* inputs */                                      \
           : /* clobbers */);                                  \
    }                                                          \
  while (false)

/* Procedure return, the version not relying on callq / retq . */
#define _JITTER_RETURN(link_rvalue)                                   \
  do                                                                  \
    {                                                                 \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                          \
                "jmpq *%[the_link_rvalue]\n\t"                        \
                : /* outputs. */                                      \
                : [the_link_rvalue] "rm" (link_rvalue) /* inputs. */  \
                : /* clobbers. */                                     \
                : jitter_fake_target /* gotolabels. */);           \
      /* The rest of the VM instruction is unreachable. */            \
      __builtin_unreachable ();                                       \
    }                                                                 \
  while (false)

/* Branch-and-link, the version not relying on callq / retq . */
#define _JITTER_BRANCH_AND_LINK_NATIVE(callee_rvalue)                        \
  do                                                                         \
    {                                                                        \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                 \
                "leaq jitter_return_address_%=(%%rip), %"                    \
                   JITTER_STRINGIFY(JITTER_SCRATCH_REGISTER) "\n\t"          \
                "jmpq *%[the_callee_rvalue]\n"                               \
                "jitter_return_address_%=:\n"                                \
                : /* outputs. */                                             \
                : [the_callee_rvalue] "rm" (callee_rvalue) /* inputs. */     \
                : /* clobbers. */                                            \
                : jitter_fake_target /* gotolabels. */);                  \
      /* See the comment in _JITTER_BRANCH_AND_LINK_NATIVE below. */         \
    }                                                                        \
  while (false)

#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

/* Branch-and-link to a fast label, the version not relying on callq / retq . */
#define _JITTER_BRANCH_FAST_AND_LINK_NATIVE(target_index)                       \
  do                                                                            \
    {                                                                           \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                    \
                "leaq jitter_return_address_%=(%%rip), %"                       \
                   JITTER_STRINGIFY(JITTER_SCRATCH_REGISTER) "\n\t"             \
                JITTER_ASM_PATCH_IN_PLACEHOLDER(                                \
                   JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL /*size_in_bytes*/, \
                   JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL /*case*/,     \
                   target_index,                                                \
                   0, 0, 0 /* not used for this case */)                        \
                "jitter_return_address_%=:\n"                                   \
                : /* outputs. */                                                \
                : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                        \
                  JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */            \
                : /* clobbers. */                                               \
                : jitter_fake_target /* gotolabels. */);                     \
      /* See the comment in _JITTER_BRANCH_AND_LINK_NATIVE below. */            \
    }                                                                           \
  while (false)

/* Branch-and-link-with, the version not relying on callq / retq -- or in this
   case, not relying on the hardware stack to hold the return address. */
#define _JITTER_BRANCH_AND_LINK_WITH_NATIVE(_jitter_callee_rvalue,              \
                                            _jitter_new_link)                   \
  do                                                                            \
    {                                                                           \
      const void *jitter_callee_rvalue = (_jitter_callee_rvalue);               \
      const void *jitter_new_link = (_jitter_new_link);                         \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                    \
                JITTER_ASM_COMMENT_UNIQUE("Branch-and-link-with, pretending "   \
                                          "to go to %l[jitter_fake_target]") \
                "movq %[jitter_new_link], %"                                    \
                   JITTER_STRINGIFY(JITTER_SCRATCH_REGISTER) "\n\t"             \
                "jmpq *%[jitter_target]\n"                                      \
                : /* outputs */                                                 \
                : [jitter_new_link] "g" (jitter_new_link),                      \
                  [jitter_target] "g" (jitter_callee_rvalue) /* inputs */       \
                : /* clobbers */                                                \
                : jitter_fake_target /* goto labels */);                     \
      /* This is a tail call: the next statement within this VM instruction is  \
         not reachable. */                                                      \
      __builtin_unreachable ();                                                 \
    }                                                                           \
  while (false)
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#else // ! defined(JITTER_BRANCH_AND_LINK_NO_CALL)

/* Code automatically emitted at the very beginning of callee instructions.
   This is executed in a fragile state, when the stack is not what GCC
   thinks.
   This is the version relying on callq / retq . */
#define _JITTER_PROCEDURE_PROLOG(link_union)                                 \
  do                                                                         \
    {                                                                        \
      /* Pop the return address from the stack and save it into a register   \
         variable.  It would be incorrect to accept a memory location, as    \
         it could end up on the stack, and GCC does not know that the stack  \
         has been pushed to.                                                 \
         The asm statement must be volatile: the popq instruction must not   \
         be optimised away in any circumstance, even in bizarre cases where  \
         the user did not use the return value. */                           \
      void *jitter_the_link_value;                                           \
      asm volatile                                                           \
         ("popq %[the_link_value]"                                           \
          : [the_link_value] "=r" (jitter_the_link_value) /* outputs */      \
          : /* inputs */                                                     \
          : /* clobbers */);                                                 \
      (link_union).pointer = jitter_the_link_value;                          \
    }                                                                        \
  while (false)

/* Procedure return, the version relying on callq / retq . */         \
#define _JITTER_RETURN(link_rvalue)                                   \
  do                                                                  \
    {                                                                 \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                          \
                "pushq %[the_link_rvalue]\n\t"                        \
                "retq"                                                \
                : /* outputs. */                                      \
                : [the_link_rvalue] "g" (link_rvalue) /* inputs. */   \
                : /* clobbers. */                                     \
                : jitter_fake_target /* gotolabels. */);           \
      /* The rest of the VM instruction is unreachable. */            \
      __builtin_unreachable ();                                       \
    }                                                                 \
  while (false)

/* Branch-and-link, the version relying on callq / retq . */
#define _JITTER_BRANCH_AND_LINK_NATIVE(callee_rvalue)                       \
  do                                                                        \
    {                                                                       \
      const void * restrict jitter_call_indirect_target = (callee_rvalue);  \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                \
                "# Do a real call, pretending to go to\n\t"                 \
                "# %l[jitter_fake_target]\n\t"                           \
                "callq *%[target]\n"                                        \
                : /* outputs */                                             \
                : [target] "rm" (jitter_call_indirect_target) /* inputs */  \
                  , "X"(jitter_ip)                                          \
                : /* clobbers */                                            \
                : jitter_fake_target /* goto labels */);                 \
      /* It is possible for control to return here.  This is why Jitter     \
         does not specify whether code in the same VM instruction after a   \
         branch-and-link is executed or not.  Adding __builtin_unreachable  \
         here would be wrong: GCC might move code here incorrectly assuming \
         that control cannot fall through.                                  \
         Adding JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END here, as an      \
         intra-instruction jump, can introduce undetectable defects.  This  \
         is the rationale for Jitter's ugly branch-and-link semantics. */   \
    }                                                                       \
  while (false)

#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
/* Branch-and-link to a fast label, the version relying on callq / retq . */
#define _JITTER_BRANCH_FAST_AND_LINK_NATIVE(target_index)                       \
  do                                                                            \
    {                                                                           \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                    \
                JITTER_ASM_PATCH_IN_PLACEHOLDER(                                \
                   JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK /*size_in_bytes*/, \
                   JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK /*case*/,   \
                   target_index,                                                \
                   0, 0, 0 /* not used for this case */)                        \
                : /* outputs */                                                 \
                : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                        \
                  JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */            \
                : /* clobbers */                                                \
                : jitter_fake_target /* goto labels */);                     \
      /* See the comment in _JITTER_BRANCH_AND_LINK_NATIVE above. */            \
    }                                                                           \
  while (false)
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

/* Branch-and-link-with, the version relying on callq / retq -- in this case,
   relying on the hardware stack to hold the return address. */
#define _JITTER_BRANCH_AND_LINK_WITH_NATIVE(_jitter_callee_rvalue,              \
                                            _jitter_new_link)                   \
  do                                                                            \
    {                                                                           \
      const void *jitter_callee_rvalue = (_jitter_callee_rvalue);               \
      const void *jitter_new_link = (_jitter_new_link);                         \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                    \
                JITTER_ASM_COMMENT_UNIQUE("Branch-and-link-with, pretending "   \
                                          "to go to %l[jitter_fake_target]") \
                "pushq %[jitter_new_link]\n\t"                                  \
                "jmpq *%[jitter_target]\n"                                      \
                : /* outputs */                                                 \
                : [jitter_new_link] "g" (jitter_new_link),                      \
                  [jitter_target] "r" (jitter_callee_rvalue) /* inputs */       \
                : /* clobbers */                                                \
                : jitter_fake_target /* goto labels */);                     \
      /* This is a tail call: the next statement within this VM instruction is  \
         not reachable. */                                                      \
      __builtin_unreachable ();                                                 \
    }                                                                           \
  while (false)


#endif // #ifdef JITTER_BRANCH_AND_LINK_NO_CALL

#endif // #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(...




/* C-only page, expanding to nothing if this header is included by assembly.
 * ************************************************************************** */

#ifndef __ASSEMBLER__

/* On x86_64 any 32-bit write to a 64-bit register (but of course not a 32-bit
   store) automatically sets the high 32 bits to zero -- this is a zero-extend,
   not a sign-extend.
   Moreover there is no direct way of loading a literal to a register without
   supplying an immediate smaller than 32 bits.  The choice, therefore, is
   limited to just 32-bit (non-negative and up to 1 << 31 included) or 64-bit
   (otherwise). */
/* FIXME: no, there's another useful case: I can load a 32-bit negative literal
   into a 64-bit register with a movq, and in this case it's sign-extended --
   with the instruction being much shorter than a movabsq:
     48 c7 c3 ff ff ff ff   # movq $-1, %rbx # the 64-bit register value is negative
   Which is different from, say,
     bb ff ff ff ff         # movl $-1, %ebx
   , where the result as a 64-bit integer is positive -- I already cover this.
   What shall I do about this with memory residuals?
   I could even use
     31 db         # xorl %ebx, %ebx
     83 c3 01      # addl $0x1, %ebx
   to load constants in [1, 127] into 64-bit registers.
   This:
     31 db         # xorl %ebx, %ebx
     48 83 c3 ff   # addq $-1, %rbx   # sign-extended
   works for constants in [-128, -1].
   Are these fast?  Is there a way not to have them affect flags?  I doubt it.
   This is good for loading 1, assuming it's fast (not so sure: incq modifies
   some flag, and /mnt/big/handbooks/assembly/intel/optimizing-subsnippets-in-assembly-language--fog--1996-2016.pdf §16.2 recommends using add/sub when optimizing for speed, vs. inc/dec when optimizing for size unless we expect no penalty from flag dependencies, which might or might not be my case):
     31 db         # xorl %ebx, %ebx
     48 ff c3      # incq %rbx

   FIXME: See /mnt/big/handbooks/assembly/intel/optimizing-subsnippets-in-assembly-language--fog--1996-2016.pdf , §10.2 "Using shorter constants and addresses".
*/

/* Notice that the order matters, and these shouldn't be rearranged without also
   changing the order of definitions in machine.S .  We also rely on the first
   case having value 0, since we use enum jitter_snippet_to_patch values as
   array indices. */
enum jitter_snippet_to_patch
  {
    jitter_snippet_load_0_to_64bit_residual_register_0,
    jitter_snippet_load_0_to_64bit_residual_register_1,
    jitter_snippet_load_0_to_64bit_residual_register_2,
    jitter_snippet_load_0_to_64bit_residual_register_3,
    jitter_snippet_load_minus_1_to_64bit_residual_register_0,
    jitter_snippet_load_minus_1_to_64bit_residual_register_1,
    jitter_snippet_load_minus_1_to_64bit_residual_register_2,
    jitter_snippet_load_minus_1_to_64bit_residual_register_3,
    jitter_snippet_set_64bit_residual_register_0,
    jitter_snippet_set_64bit_residual_register_1,
    jitter_snippet_set_64bit_residual_register_2,
    jitter_snippet_set_64bit_residual_register_3,
    jitter_snippet_set_32bit_residual_register_0,
    jitter_snippet_set_32bit_residual_register_1,
    jitter_snippet_set_32bit_residual_register_2,
    jitter_snippet_set_32bit_residual_register_3,
    jitter_snippet_set_32bit_sign_extended_residual_register_0,
    jitter_snippet_set_32bit_sign_extended_residual_register_1,
    jitter_snippet_set_32bit_sign_extended_residual_register_2,
    jitter_snippet_set_32bit_sign_extended_residual_register_3,
    jitter_snippet_set_pcrel_address_residual_register_0,
    jitter_snippet_set_pcrel_address_residual_register_1,
    jitter_snippet_set_pcrel_address_residual_register_2,
    jitter_snippet_set_pcrel_address_residual_register_3,
    jitter_snippet_set_64bit_residual_memory_two_32bit_stores,
    jitter_snippet_set_32bit_sign_extended_residual_memory,
    jitter_snippet_jump_unconditional_32bit_offset,
    jitter_snippet_empty_after_conditional_jump_32bit_offset,
    jitter_snippet_call_32bit_offset,

    /* The number of snippets. */
    jitter_snippet_no
  };

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
