/* VM library: PowerPC definitions, to be included from both C and assembly.

   Copyright (C) 2017, 2019, 2020, 2021 Luca Saiu
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


#if ! defined (__ASSEMBLER__)
# include <jitter/jitter-cpp.h>
# include <jitter/jitter-arithmetic.h>
#endif // #if ! defined (__ASSEMBLER__)


/* Assembler syntax.
 * ************************************************************************** */

/* How to introduce comments in assembly on PowerPC. */
#define JITTER_ASM_COMMENT_PREFIX "# "




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "xori %%r0, %%r0, " integer_literal_as_string "\n\t"        \
  "xori %%r0, %%r0, " integer_literal_as_string "\n\t"

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                   \
  /* Return from interrupt.  This will cause an exception in user mode, of  \
     a kind not usually seen. */                                            \
  "rfi"




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE  \
  "mtctr %[_jitter_the_target]\n\t"        \
  "bctr"
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  \
  "r"
#define JITTER_ASM_COMPUTED_GOTO_CLOBBERS \
  "ctr"




/* Reserved registers.
 * ************************************************************************** */

/* PowerPC configurations should have at least the following registers available
   as callee-save:
     %r14 %r15 %r16 %r17 %r18 %r19 %r20 %r21 %r22
     %r23 %r24 %r25 %r26 %r27 %r28 %r29 .
   This is already excellent, but I suspect a few others might work as well.  For
   example GCC says that %r31 is used as a frame pointer, but stops complaining if
   I use -fomit-frame-pointer -- which I certainly do for VMs.
   I have seen this error message attempting to use %r30 on 32-bit PowerPC
   GNU/Linux:
     error: 30 cannot be used in asm here
   Not using it seems to solve the problem, so I deduce that %r30 is not usable,
   even I read somewhere that it was supposed to be callee-save.  I doubt PowerPC
   has instructions not accepting specific registers as arguments. */

/* Register pointing to The Array base. */
#define JITTER_BASE_REGISTER          %r14

/* How many registers we can use to hold residual arguments. */
#define JITTER_RESIDUAL_REGISTER_NO   14
//#define JITTER_RESIDUAL_REGISTER_NO   0 // this is good for testing memory literals

/* Registers holding residual arguments, with 0-based suffixes.  These have to
   be as many as JITTER_RESIDUAL_REGISTER_NO . */
#define JITTER_RESIDUAL_REGISTER_0    %r15
#define JITTER_RESIDUAL_REGISTER_1    %r16
#define JITTER_RESIDUAL_REGISTER_2    %r17
#define JITTER_RESIDUAL_REGISTER_3    %r18
#define JITTER_RESIDUAL_REGISTER_4    %r19
#define JITTER_RESIDUAL_REGISTER_5    %r20
#define JITTER_RESIDUAL_REGISTER_6    %r21
#define JITTER_RESIDUAL_REGISTER_7    %r22
#define JITTER_RESIDUAL_REGISTER_8    %r23
#define JITTER_RESIDUAL_REGISTER_9    %r24
#define JITTER_RESIDUAL_REGISTER_10   %r25
#define JITTER_RESIDUAL_REGISTER_11   %r26
#define JITTER_RESIDUAL_REGISTER_12   %r27
#define JITTER_RESIDUAL_REGISTER_13   %r28

/* The scratch register. */
#define JITTER_SCRATCH_REGISTER       %r29




/* Execution-beginning and execution-end code.
 * ************************************************************************** */

/* Save the initial state of the XER register at entry, clearing its overflow
   and summary overflow bits.  This will be restored before executing
   instructions checking for overflow.
   Hopefully the XER copy will be kept in a register; however correctness does
   not depend on this. */
#define JITTER_EXECUTION_BEGINNING_                                          \
  /* Read XER as it is now, in order to get a correct copy of the reserved   \
     bits. */                                                                \
  jitter_uint jitter_xer_at_entry;                                           \
  asm ("mfxer %[xer_at_entry]"                                               \
       : [xer_at_entry] "=r" (jitter_xer_at_entry) /* outputs */);           \
  /* Explicitly clear the summary overflow and overflow bits in the copy.    \
     This is the XER state we will set from inline assembly. */              \
  const jitter_uint jitter_xer_no_overflow                                   \
    = jitter_xer_at_entry & ~ (jitter_uint) 3;                               \
  /* Set the actual XER register to the value we saved, just to make sure    \
     that overflow and summary overflow are cleared at the beginning of the  \
     VM program execution. */                                                \
  asm ("mtxer %[xer_no_overflow]"                                            \
       : /* outputs */                                                       \
       : [xer_no_overflow] "r" (jitter_xer_no_overflow) /* inputs */);

/* Restore xer before exiting. */
#define JITTER_EXECUTION_END_                                        \
  /* Set the XER register to the original value it had at entry. */  \
  asm ("mtxer %[xer_at_entry]"                                       \
       : /* outputs */                                               \
       : [xer_at_entry] "r" (jitter_xer_at_entry) /* inputs */);




/* Patch-ins.
 * ************************************************************************** */

/* Patch-ins are supported on the the PowerPC architecture. */
#define JITTER_MACHINE_SUPPORTS_PATCH_IN 1

/* An instruction entirely made of 0x0 bytes is invalid on the PowerPC, and also
   very easy to recognize at a glance. */
#define JITTER_ASM_PATCH_IN_FILL_BYTE    "0x00"

/* For each patch-in case define its size in bytes, corresponding to the total
   size of the instructions to be patched in in bytes, including possible
   padding nops. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL    4
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK  4
/* There is no need for conditional branch snippets; the inline asm code is
   already correct and needs patching only in the offset.  Conditional branch
   snippets on PowerPC have size zero.  */




/* VM low-level conditional branches.
 * ************************************************************************** */

/* Ignore this page if patch-ins have been disabled (for debugging) or the
   dispatching model does not support them. */
#if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)

/* This factors the common logic of low-level conditional fast branches.
   opd1_tplt is a string literal template for the second register operand,
   including an initial comma; it can be passed as an empty string when there is
   no second register operand. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_UNARY_(cmp_insn, b_insn,  \
                                                         opd0, tgt)         \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                    \
            /* Emit the patch-in right before a correct conditional branch  \
               instruction, where the only field to patch is the (split)    \
               branch offset.  All the rest, including register numberss,   \
               is correctly generated by the inline asm and needs no        \
               patching. */                                                 \
            cmp_insn " cr7, %[jitter_operand0], 0\n"                        \
            JITTER_ASM_PATCH_IN_PLACEHOLDER                                 \
               (0 /* size in bytes */,                                      \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /*case*/,  \
                (tgt),                                                      \
                0, 0, 0 /* not used for this case */)                       \
            "1: " b_insn " cr7, 1b\n\t"                                     \
            : /* outputs */                                                 \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                        \
              [jitter_operand0] "r" (opd0),                                 \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */            \
            : "cr7" /* clobbers */                                          \
            : jitter_fake_target /* goto labels */)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_(                  \
           cmp_insn, b_insn, opd0, opd1, tgt)                               \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                    \
            /* See the comment above within in                              \
               _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_UNARY_ . */        \
            cmp_insn " cr7, %[jitter_operand0], %[jitter_operand1]\n"       \
            JITTER_ASM_PATCH_IN_PLACEHOLDER                                 \
               (0 /* size in bytes */,                                      \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /*case*/,  \
                (tgt),                                                      \
                0, 0, 0 /* not used for this case */)                       \
            "1: " b_insn " cr7, 1b\n\t"                                     \
            : /* outputs */                                                 \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                        \
              [jitter_operand0] "r" (opd0),                                 \
              [jitter_operand1] "r" (opd1),                                 \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */            \
            : "cr7" /* clobbers */                                          \
            : jitter_fake_target /* goto labels */)

/* Low-level fast branches on sign. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_UNARY_         \
     ("cmpi", "beq", (opd0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_UNARY_            \
     ("cmpi", "bne", (opd0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_UNARY_             \
     ("cmpi", "blt", (opd0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONNEGATIVE_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_UNARY_                \
     ("cmpi", "bge", (opd0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_POSITIVE_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_UNARY_             \
     ("cmpi", "bgt", (opd0), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONPOSITIVE_(opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_UNARY_                \
     ("cmpi", "ble", (opd0), (tgt))

/* Low-level fast branches on equality comparisons. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_EQUAL_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_               \
     ("cmp", "beq", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTEQUAL_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_               \
     ("cmp", "bne", (opd0), (opd1), (tgt))

/* Low-level fast branches on magnitude comparisons. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_                     \
     ("cmp", "blt", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_                        \
     ("cmp", "bgt", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_                        \
     ("cmp", "bge", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_                           \
     ("cmp", "ble", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_                       \
     ("cmpl", "blt", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_                          \
     ("cmpl", "bgt", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_                          \
     ("cmpl", "bge", (opd0), (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_BINARY_                             \
     ("cmpl", "ble", (opd0), (opd1), (tgt))

/* This factors the common code of overflow-checking operations. */
#define _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_(insn,              \
                                                             res, opd0, opd1,   \
                                                             tgt)               \
  /* This uses the same delicate trick as on x86_64, in which an inline asm     \
     operand is actually an output, but must be declared as input.  See the     \
     comment about similar macros in                                            \
     machine/x86_64/jitter/machine/jitter-machine.h . */                        \
  register jitter_int _jitter_operation_result                                  \
     asm (JITTER_STRINGIFY(JITTER_SCRATCH_REGISTER));                           \
  asm ("" : "=r" (_jitter_operation_result));                                   \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                        \
            /* Reset XER to a state with summary overflow and overflow both     \
               clear.  This is unfortunately necessary, as the operations       \
               the overflow bit also update XER and copy the current state of   \
               XER into CR.  XER keeps an incrmental overflow state, and its    \
               summary overflow and overflow bits are reset only on demand. */  \
            "mtxer %[jitter_xer_no_overflow]\n\t"                               \
            /* Perform the operation.  This updates XER and then cr0 with a     \
               copy of the overflow bit from XER. */                            \
            insn " %[jitter_operation_result], "                                \
               "%[jitter_operand0], %[jitter_operand1]\n\t"                     \
            /* Branch on overflow.  This checks CR: it is impractical to check  \
               XER. */                                                          \
            JITTER_ASM_PATCH_IN_PLACEHOLDER /* See patch-in comments above */   \
               (0 /* size in bytes */,                                          \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /* case */,    \
                (tgt), 0, 0, 0 /* not used for this case */)                    \
            "1: bso cr0, 1b\n\t"                                                \
            /* On overflow the branch is taken, and in that case the XER state  \
               keeps its summary overflow and overflow bits set.  This should   \
               not be a problem; if it is this code can be changed, at some     \
               cost in performance because of an added branch.  */              \
            : /* outputs */                                                     \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                            \
              [jitter_operation_result] "r" (_jitter_operation_result)          \
                                                    /* Actually an output! */,  \
              [jitter_operand0] "r" ((jitter_int) (opd0)),                      \
              [jitter_operand1] "r" ((jitter_int) (opd1)),                      \
              [jitter_xer_no_overflow] "r" (jitter_xer_no_overflow),            \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */                \
            : "xer", "cr0" /* clobbers */                                       \
            : jitter_fake_target /* goto labels */);                         \
  /* Make sure to get the current value in the register as the result, and not  \
     a previous copy.  In order to force this pretend to update the register    \
     here, only if the branch was not taken.  Inline asm will use the same      \
     register assignment for _jitter_operation_result. */                       \
  asm ("" : "+r" (_jitter_operation_result));                                   \
  (res) = _jitter_operation_result

/* Overflow-checking operations. */
#define _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_("addo.",                \
                                                       (res), (opd0), (opd1),  \
                                                       (tgt))
#define _JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_("subo.",                 \
                                                       (res), (opd0), (opd1),   \
                                                       (tgt))
#define _JITTER_LOW_LEVEL_TIMES_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_("mullwo.",               \
                                                       (res), (opd0), (opd1),   \
                                                       (tgt))
#define _JITTER_LOW_LEVEL_DIVIDED_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1,    \
                                                           tgt)                \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_("divwo.",               \
                                                       (res), (opd0), (opd1),  \
                                                       (tgt))
#define _JITTER_LOW_LEVEL_REMAINDER_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1,  \
                                                             tgt)              \
  /* PowerPC has no remainder instruction but can check overflow on            \
     division. */                                                              \
  jitter_int _jitter_quotient;                                                 \
  jitter_int _jitter_opd0_value = (jitter_int) (opd0);                         \
  jitter_int _jitter_opd1_value = (jitter_int) (opd1);                         \
  _JITTER_LOW_LEVEL_DIVIDED_BRANCH_FAST_IF_OVERFLOW_(_jitter_quotient,         \
                                                     _jitter_opd0_value,       \
                                                     _jitter_opd1_value,       \
                                                     (tgt));                   \
  /* If we arrived here the division operation did not overflow. */            \
  (res) = _jitter_opd0_value - _jitter_quotient * _jitter_opd1_value

/* The following macros serve to implement _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_
   in an optimal way, using andi. where possible instead of and. .  Some of this
   logic could be made machine-independent, but for the time being I am
   accepting to have this complexity here.
   This feature is critical to tag-checking performance. */

/* Expand to an expression evaluating to non-false iff the given argument
   evaluates to a compile-time constant expression which fits in 16 bits,
   zero-extended.  This is used to decide whether the expression can be passed
   as an immediate to an andi. instruction. */
#define JITTER_POWERPC_ANDI_IMMEDIATE_CANDIDATE(non_side_effecting_expression)  \
  (__builtin_constant_p (non_side_effecting_expression)                         \
   && JITTER_FITS_IN_BITS_ZERO_EXTENDED (non_side_effecting_expression, 16))

/* Expand to an inline asm statement computing a bitwise and and branching when
   the result is zero or nonzero, according to branch_insn.  This factors three
   possible argument patterns:
   - register, immediate;
   - immediate, register (exchanged by the caller);
   - register, register. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_ASM_(and_insn,           \
                                                            opd0,               \
                                                            opd0_constraint,    \
                                                            opd1,               \
                                                            opd1_constraint,    \
                                                            branch_insn,        \
                                                            tgt)                \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                        \
            and_insn " %" JITTER_STRINGIFY (JITTER_SCRATCH_REGISTER)            \
               ", %[jitter_operand0], %[jitter_operand1]\n\t"                   \
            JITTER_ASM_PATCH_IN_PLACEHOLDER /* See patch-in comments above */   \
               (0 /* size in bytes */,                                          \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /* case */,    \
                (tgt), 0, 0, 0 /* not used for this case */)                    \
            "1: " branch_insn " cr0, 1b\n\t"                                    \
            : /* outputs */                                                     \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                            \
              [jitter_operand0] opd0_constraint (opd0),                         \
              [jitter_operand1] opd1_constraint (opd1),                         \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */                \
            : JITTER_STRINGIFY (JITTER_SCRATCH_REGISTER), "cr0" /* clobbers */  \
            : jitter_fake_target /* goto labels */)

/* This uses the macro above and itself factors two use cases: branch on and,
   branch on not and. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_(branch_insn,            \
                                                        opd0, opd1,             \
                                                        tgt)                    \
  jitter_int _jitter_opd0_value = (jitter_int) (opd0);                          \
  jitter_int _jitter_opd1_value = (jitter_int) (opd1);                          \
  if (JITTER_POWERPC_ANDI_IMMEDIATE_CANDIDATE (_jitter_opd0_value))             \
    {                                                                           \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_ASM_ ("andi.",             \
                                                           _jitter_opd1_value,  \
                                                           "r",                 \
                                                           _jitter_opd0_value,  \
                                                           "i",                 \
                                                           branch_insn,         \
                                                           (tgt));              \
    }                                                                           \
  else if (JITTER_POWERPC_ANDI_IMMEDIATE_CANDIDATE (_jitter_opd1_value))        \
    {                                                                           \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_ASM_ ("andi.",             \
                                                           _jitter_opd0_value,  \
                                                           "r",                 \
                                                           _jitter_opd1_value,  \
                                                           "i",                 \
                                                           branch_insn,         \
                                                           (tgt));              \
    }                                                                           \
  else                                                                          \
    {                                                                           \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_ASM_ ("and.",              \
                                                           _jitter_opd0_value,  \
                                                           "r",                 \
                                                           _jitter_opd1_value,  \
                                                           "r",                 \
                                                           branch_insn,         \
                                                           (tgt));              \
    }

/* Fast-branch to the target if opd0 & opd0 gives a nonzero result. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_(opd0, opd1, tgt)      \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_ ("bne",           \
                                                   (opd0), (opd1),  \
                                                   (tgt))           \

/* Fast-branch to the target if opd0 & opd0 gives a zero result. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTAND_(opd0, opd1, tgt)   \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_ ("beq",           \
                                                   (opd0), (opd1),  \
                                                   (tgt))

#endif // #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)




/* VM procedures.
 * ************************************************************************** */

/* Procedures are supported on PowerPC ; however we conditionalise the definition
   on JITTER_MACHINE_SUPPORTS_PATCH_IN , for testing convenience. */
#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
# define JITTER_MACHINE_SUPPORTS_PROCEDURE  1
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN)   \
    && defined(JITTER_DISPATCH_NO_THREADING)       \
    && defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)

/* Load the link register into the given union.  This is executed at the
   very beginning of callee instructions. */
#define _JITTER_PROCEDURE_PROLOG(link_union)                                    \
  do                                                                            \
    {                                                                           \
      void * jitter_the_return_address;                                         \
      /* This inline asm statement must be volatile because it has no explicit  \
         inputs; the actual input is lr , which is not visible from C.          \
         If this is not volatile GCC can move it somewhere else where it is     \
         exectued only once, with the result saved on the stack.  Of course I   \
         don't want that.  VM instructions begin and and with volatile asm      \
         statements, so this cannot be moved across them. */                    \
      asm volatile ("mflr %[return_address]"                                    \
                    : [return_address] "=r" (jitter_the_return_address)         \
                      /* outputs */);                                           \
      (link_union).pointer = jitter_the_return_address;                         \
    }                                                                           \
  while (false)

/* Return using native machine language facilities, when the return address in
   in the given rvalue. */
#define _JITTER_RETURN(link_rvalue)                                            \
  do                                                                           \
    {                                                                          \
      const void * jitter_the_return_address = (const void*) (link_rvalue);    \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                   \
                "mtlr %[return_addr]\n\t"                                      \
                "blr\n\t"                                                      \
                : /* outputs. */                                               \
                : [return_addr] "r" (jitter_the_return_address) /* inputs. */  \
                : "lr" /* clobbers. */                                         \
                : jitter_fake_target /* gotolabels. */);                    \
      /* The rest of the VM instruction is unreachable. */                     \
      __builtin_unreachable ();                                                \
    }                                                                          \
  while (false)

/* Perform a branch-and-link to the pointed callee instruction using specific
   machine language features rather than generic indirect branches. */
#define _JITTER_BRANCH_AND_LINK_NATIVE(callee_rvalue)                         \
  do                                                                          \
    {                                                                         \
      const void * const jitter_destination =                                 \
        (const void * const) (callee_rvalue);                                 \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                  \
                "mtctr %[destination]\n\t"                                    \
                "bctrl\n\t"                                                   \
                : /* outputs. */                                              \
                : [destination] "r" (jitter_destination) /* inputs. */        \
                : "ctr", "lr" /* clobbers. */                                 \
                : jitter_fake_target /* gotolabels. */);                   \
      /* It would be incorrect to have __builtin_unreachable or               \
         JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END here: see the comment in  \
         the x86_64 version. */                                               \
    }                                                                         \
  while (false)

/* Branch to the pointed caleee destination, but set the link register to a
   return address different from the natural one. */
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
                                          "%l[jitter_fake_target]")        \
                /* This PowerPC version is particularly pretty and natural,   \
                   and does not negatively affect branch target prediction    \
                   for once. */                                               \
                "mtctr %[jitter_callee_rvalue]\n\t"                           \
                "mtlr %[jitter_new_link]\n\t"                                 \
                "bctr\n\t"                                                    \
                : /* outputs. */                                              \
                : [jitter_callee_rvalue] "r" (jitter_callee_rvalue),          \
                  [jitter_new_link] "r" (jitter_new_link) /* inputs. */       \
                : "ctr", "lr" /* clobbers. */                                 \
                : jitter_fake_target /* gotolabels. */);                   \
      /* The rest of the VM instruction is unreachable: this tail call is an  \
         unconditional jump. */                                               \
      __builtin_unreachable ();                                               \
    }                                                                         \
  while (false)

/* Perform an ordinary bl, writing the return address into the link register.
   Of course we need a patch-in, since the destination address is encoded in
   the jumping instruction. */
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
                  JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */           \
                : /* clobbers. */                                              \
                : jitter_fake_target /* gotolabels. */);                    \
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
    jitter_snippet_load_sign_extended_16bit_to_register_0,
    jitter_snippet_load_sign_extended_16bit_to_register_1,
    jitter_snippet_load_sign_extended_16bit_to_register_2,
    jitter_snippet_load_sign_extended_16bit_to_register_3,
    jitter_snippet_load_sign_extended_16bit_to_register_4,
    jitter_snippet_load_sign_extended_16bit_to_register_5,
    jitter_snippet_load_sign_extended_16bit_to_register_6,
    jitter_snippet_load_sign_extended_16bit_to_register_7,
    jitter_snippet_load_sign_extended_16bit_to_register_8,
    jitter_snippet_load_sign_extended_16bit_to_register_9,
    jitter_snippet_load_sign_extended_16bit_to_register_10,
    jitter_snippet_load_sign_extended_16bit_to_register_11,
    jitter_snippet_load_sign_extended_16bit_to_register_12,
    jitter_snippet_load_sign_extended_16bit_to_register_13,
    jitter_snippet_load_32bit_to_register_0,
    jitter_snippet_load_32bit_to_register_1,
    jitter_snippet_load_32bit_to_register_2,
    jitter_snippet_load_32bit_to_register_3,
    jitter_snippet_load_32bit_to_register_4,
    jitter_snippet_load_32bit_to_register_5,
    jitter_snippet_load_32bit_to_register_6,
    jitter_snippet_load_32bit_to_register_7,
    jitter_snippet_load_32bit_to_register_8,
    jitter_snippet_load_32bit_to_register_9,
    jitter_snippet_load_32bit_to_register_10,
    jitter_snippet_load_32bit_to_register_11,
    jitter_snippet_load_32bit_to_register_12,
    jitter_snippet_load_32bit_to_register_13,
    /* FIXME: the next two snippets are not implemented yet. */
    jitter_snippet_load_sign_extended_16bit_to_memory,
    jitter_snippet_load_32bit_to_memory,

    jitter_snippet_jump_unconditional_26bit_offset_no_link,
    jitter_snippet_jump_and_link_26bit_offset,
    jitter_snippet_jump_conditional_16bit_offset,

    /* The number of snippets. */
    jitter_snippet_no
  };

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
