/* VM library: MIPS definitions, to be included from both C and assembly.

   Copyright (C) 2017, 2018, 2019, 2020, 2021 Luca Saiu
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


/* Include headers.
 * ************************************************************************** */

/* These are also safe for assembly. */
# include <jitter/jitter-cpp.h>
# include <jitter/jitter-config.h>




/* Assembler syntax.
 * ************************************************************************** */

/* How to introduce comments in assembly on MIPS. */
#define JITTER_ASM_COMMENT_PREFIX "# "




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "addiu $0, $0, " integer_literal_as_string

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                   \
  /* Return from exception.  This will cause an exception in user mode, of  \
     a kind not usually seen. */                                            \
  "rfe"




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h .
   In the case of (pre-r6) MIPS we can afford to keep .reorder on as per the
   default setting, and *not* to specify a delay slot; Gas will be able to fill
   the delay slot itself with something useful, when we are lucky.  Where r6
   instructions are available, use a compact jump with no delay slot. */
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
# define JITTER_ASM_COMPUTED_GOTO_TEMPLATE   "jic %[_jitter_the_target], 0"
#else /* pre-r6 */
# define JITTER_ASM_COMPUTED_GOTO_TEMPLATE   "jr %[_jitter_the_target]"
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  "r"




/* Reserved registers.
 * ************************************************************************** */

/* MIPS configurations should have at least the following registers available
   as callee-save:
     $16 $17 $18 $19 $20 $21 $22 $23
   This is already very good, but I suspect a few others might work as well.
   I can almost certainly use $1/$at as scratch within an uninterrupted
   sequence of instructions written by me; I'm not so sure $30/$fp is actually
   used as a frame pointer by GCC, or if I should force GCC not to use a frame
   pointer.
   Other reserved registers, or register with a special ABI role:
     $1/$at   assembler register, used to synthesize some pseudo-instructions;
     $26,$27  clobbered by interrupt handlers at unpredictable times;
     $29      stack pointer;
     $30/$fp  frame pointer;
     $31      return address.  */

/* Register pointing to The Array base. */
#define JITTER_BASE_REGISTER          $16

/* How many registers we can use to hold residual arguments.  The reserved
   residual registers specified below may be at most as many as this value.  If
   JITTER_RESIDUAL_REGISTER_NO specifies fewer residual registers then the
   excess ones are not reserved. */
#define JITTER_RESIDUAL_REGISTER_NO   6
//#define JITTER_RESIDUAL_REGISTER_NO   0 // this is good for testing memory literals

/* Registers holding residual arguments, with 0-based suffixes. */
#define JITTER_RESIDUAL_REGISTER_0    $17
#define JITTER_RESIDUAL_REGISTER_1    $18
#define JITTER_RESIDUAL_REGISTER_2    $19
#define JITTER_RESIDUAL_REGISTER_3    $20
#define JITTER_RESIDUAL_REGISTER_4    $21
#define JITTER_RESIDUAL_REGISTER_5    $22

/* The scratch register.  The same remark above applies. */
#define JITTER_SCRATCH_REGISTER       $1//$23

/* These registers are not reserved, but only used internally as assembly
   operands for register local variables or as clobbered assembly temporaries
   from a few macros here.  They do not even need to be caller-save; however 
   they must not conflict with reserved registers, other than the scratch
   register.
   These identifiers are specific to the MIPS port and not part of the Jitter
   API. */
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_RESULT  JITTER_SCRATCH_REGISTER
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1  $2
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2  $3

/* Same as above, as literal strings. */
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_RESULT_STRING  \
  JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_TEMPORARY_RESULT)
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING  \
  JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1)
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING  \
  JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2)




/* MIPS CPU-specific features.
 * ************************************************************************** */

/* Early revisions of the Loongson2F processor have branch prediction bugs
   (https://sourceware.org/legacy-ml/binutils/2009-11/msg00387.html ) which
   can be prevented by generating
      or $1, $1, $0
   intead of the canonical MIPS nop
      sll $0, $0, 0
   , which is encoded as a word of all zeroes.  Since GCC defines processor
   names as macros it is pretty easy to do this automatically.
   Like elsewhere in Jitter, macros whose name starts with JITTER_ASM expand to
   a literal C string suitable for inline asm; the other macro defined here is
   meant to be expanded directly in assembly files. */
#if (defined (_MIPS_ARCH_LOONGSON2F)      \
     || defined (_MIPS_TUNE_LOONGSON2F))
# define JITTER_ASM_MIPS_NOP  \
    "or $1, $1, $0"
# define JITTER_MIPS_NOP  \
    or $1, $1, $0
#else /* Not Loongson2F. */
# define JITTER_ASM_MIPS_NOP  \
    "sll $0, $0, 0"
# define JITTER_MIPS_NOP  \
    sll $0, $0, 0
#endif // Loongson2F.

/* Define the number of bits in a word in a way useful to Gas, for inline
   assembly. */
#if ! defined (__ASSEMBLER__)
# if JITTER_SIZEOF_VOID_P == 4
#   define JITTER_BITNESS   32
# elif JITTER_SIZEOF_VOID_P == 8
#   define JITTER_BITNESS   64
# else
#   error "this should never happen"
# endif
# define JITTER_BITNESS_STRING   JITTER_STRINGIFY (JITTER_BITNESS)
# define JITTER_REVISION_STRING  JITTER_STRINGIFY (__mips_isa_rev)
#endif // #if ! defined (__ASSEMBLER__)

/* These macros, defined by GCC, might be useful:
   - __mips : apparently defined as the highest supported architecture revision;
              for example 3 for MIPS III and 32 for MIPS32;
   - _MIPS_SIM : defined with values such as _ABIO32 and _ABI64, distinguishing
                 incompatible ABIs. */




/* Patch-ins.
 * ************************************************************************** */

/* Patch-ins are supported on the the MIPS architecture. */
#define JITTER_MACHINE_SUPPORTS_PATCH_IN 1

/* 0xffffffff is not technically an invalid instruction (objdump disassembles it
   as "sd $31, -1($31)"), but it makes programs crash very easily; this makes
   0xff a good choice for the default byte to write on uninitialized instruction
   memory for catching bugs. */
#define JITTER_ASM_PATCH_IN_FILL_BYTE    "0xff"

/* For each patch-in case define its size in bytes, corresponding to the total
   size of the instructions to be patched in in bytes, including delay slot
   nops. */
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
# define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL     4
# define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK   4
  /* On MIPS r6 we have two different kinds of conditional branches: one, on
     zero, with a 23-bit displacement, and another on equality or magnitude with
     an 18-bit displacement.  The patch-in size is still zero for both, but they
     need to be patched differently. */
# define JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_ZERO    1001
# define JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_OTHER   1002
#else
# define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL     8
# define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK   8
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

/* MIPS uses a single patch-in case for every conditional branch, and its size
   is zero.  There is no need for a macro in that case. */

/* These should be extended in the future with floating-point conditionals. */




/* VM conditional branches.
 * ************************************************************************** */

/* Ignore this page if patch-ins have been disabled (for debugging) or the
   dispatching model does not support them. */
#if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

/* MIPS r6 conditional compact branches are easy, except for the constraint of
   binary comparisons which must be between two distinct registers.  Compact
   conditionals all take two registers to compare, or one in the case of zero
   comparisons (with a wider displacement).  The left and right operands are
   perfectly symmetrical. */

/* Common code factoring _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_ZERO_R6,
   _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_OTHER_R6_ and
   _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_R6_ . */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_R6_(                       \
           prelude,                                                          \
           insn, fast_branch_case,                                           \
           opd0_tplt, opd0_cnst, opd0,                                       \
           opd1_tplt, opd1_cnst, opd1,                                       \
           tgt,                                                              \
           postlude)                                                         \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                     \
            /* Start by emitting the prelude, which may contain an assembly  \
               conditional to completely replace the conditional branch.     \
               Some r6 instructions require two *distinct* input registers,  \
               and such a constraint seems impossible to express in GNU C    \
               without introducing superfluous moves.  But when the          \
               registers happen to be equal I can do even better: accept     \
               any two registers including the same one twice, and then if   \
               the registers are repeated resolve the conditional before     \
               time time.  The prelude will involve a string comparison in   \
               Gas like                                                      \
                 .ifnc "%[jitter_operand0]","%[jitter_operand1]" ...  */     \
            "\n" prelude "\n"                                                \
            /* Emit the patch-in right before a correct conditional branch   \
               instruction, where the only field to patch is the branch      \
               offset.  All the rest, including registers, is correctly      \
               generated by inline asm and needs no patching. */             \
            JITTER_ASM_PATCH_IN_PLACEHOLDER                                  \
               (0 /* size in bytes */,                                       \
                fast_branch_case /* case */,                                 \
                (tgt),                                                       \
                0, 0, 0 /* not used for this case */)                        \
            "1: " insn " " opd0_tplt " " opd1_tplt ", 1b\n\t"                \
            /* Emit the postlude.  This may just close the Gas conditional   \
               or contain a ".else" alternative as well. */                  \
            "\n" postlude "\n"                                               \
            : /* outputs */                                                  \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                         \
              [jitter_operand0] opd0_cnst (opd0),                            \
              [jitter_operand1] opd1_cnst (opd1),                            \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */             \
            : /* clobbers */                                                 \
            : jitter_dispatch_label /* goto labels */)

/* Compare and conditionally fast-branch to the given target, one argument;
   wider branch only applicable to zero tests. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_ZERO_R6_(insn, opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_R6_                           \
     ("", insn, JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_ZERO,     \
      "%[jitter_operand0]", "r", (opd0),                                  \
      "", "X", 0,                                                         \
      (tgt), "")

/* Like _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_ZERO_R6_ , with the narrower
   branch applicable to every test except the zero tests. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_OTHER_R6_(insn, opd0, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_R6_                            \
     ("", insn, JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_OTHER,     \
      "%[jitter_operand0]", "r", (opd0),                                   \
      "", "X", 0,                                                          \
      (tgt), "")

/* Compare and conditionally fast-branch to the given target, two arguments.
   See the comments inside _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_R6_
   about the prelude and postlude.  */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_R6_(prelude,          \
                                                    insn,             \
                                                    opd0, opd1, tgt,  \
                                                    postlude)         \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_R6_                       \
     (prelude,                                                        \
      insn, JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_OTHER,    \
      "%[jitter_operand0]",   "r", (opd0),                            \
      ", %[jitter_operand1]", "r", (opd1),                            \
      (tgt),                                                          \
      postlude)

/* The variant of _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_R6_ which branches
   unconditionally when the two operands are in the same register -- "BS" stands
   for "branch-on-same". */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_BS_R6_(insn, opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_R6_                                  \
     ("# If the two registers are not the same (no superfluous whitespace\n"   \
      "# or comments in the next line!)...\n"                                  \
      ".ifnc \"%[jitter_operand0]\",\"%[jitter_operand1]\"\n",                 \
      insn, (opd0), (opd1), tgt,                                               \
      "#...Otherwise branch unconditionally.\n"                                \
      ".else\n"                                                                \
      JITTER_ASM_PATCH_IN_PLACEHOLDER                                          \
         (JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL /* size */,           \
          JITTER_PATCH_IN_CASE_FAST_BRANCH_UNCONDITIONAL /* case */,           \
          (tgt),                                                               \
          0, 0, 0 /* not used for this case */)                                \
      ".endif\n")

/* The variant of _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_R6_ which does nothing
   when the two operands are in the same register; "NS" stands for
   "nothing-on-same". */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_NS_R6_(insn, opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_R6_                                  \
     ("# If the two registers are not the same (no superfluous whitespace\n"   \
      "# or comments in the next line)...\n"                                   \
      ".ifnc \"%[jitter_operand0]\",\"%[jitter_operand1]\"\n",                 \
      insn, (opd0), (opd1), tgt,                                               \
      "#...Otherwise do nothing.\n"                                            \
      ".endif\n")

/* Zero conditional fast branches. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_(opd0, tgt)                 \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_ZERO_R6_ ("beqzc", (opd0), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_(opd0, tgt)              \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_ZERO_R6_ ("bnezc", (opd0), tgt)

/* Sign-based conditional fast branches. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_(opd0, tgt)              \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_OTHER_R6_ ("bltzc", (opd0), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONNEGATIVE_(opd0, tgt)           \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_OTHER_R6_ ("bgezc", (opd0), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_POSITIVE_(opd0, tgt)              \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_OTHER_R6_ ("bgtzc", (opd0), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONPOSITIVE_(opd0, tgt)           \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_UNARY_OTHER_R6_ ("blezc", (opd0), tgt)

/* Equality conditional fast branches. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_EQUAL_(opd0, opd1, tgt)                \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_BS_R6_ ("beqc", (opd0), (opd1), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTEQUAL_(opd0, opd1, tgt)             \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_NS_R6_ ("bnec", (opd0), (opd1), tgt)

/* Magnitude conditional fast branches. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_SIGNED_(opd0, opd1, tgt)          \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_NS_R6_ ("bltc", (opd0), (opd1), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_SIGNED_(opd0, opd1, tgt)       \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_NS_R6_ ("bltc", (opd1), (opd0), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_SIGNED_(opd0, opd1, tgt)       \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_BS_R6_ ("bgec", (opd0), (opd1), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_SIGNED_(opd0, opd1, tgt)    \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_BS_R6_ ("bgec", (opd1), (opd0), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_UNSIGNED_(opd0, opd1, tgt)        \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_NS_R6_ ("bltuc", (opd0), (opd1), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_UNSIGNED_(opd0, opd1, tgt)     \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_NS_R6_ ("bltuc", (opd1), (opd0), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_UNSIGNED_(opd0, opd1, tgt)     \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_BS_R6_ ("bgeuc", (opd0), (opd1), tgt)
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_BINARY_BS_R6_ ("bgeuc", (opd1), (opd0), tgt)

/* MIPS r6 introduces the bovc and bnvc instructions, very convenient for
   checking integer addition overflow.  The 18-bit displacement immediate is
   encoded in 16 bits like in the other non-zero compact branches, and can use
   the same patch-in case.
   Unfortunately this only covers 32-bit operands: there is no bdovc or bdnvc
   instruction. */
#define _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)   \
  /* This does *not* use the same fragile trick as x86_64 and other ports, in   \
     which what asm constraints claim to be an inline asm operand is actually   \
     an output: in this case the sum can be performed out of assembly, and the  \
     conditional branch-on-overflow only branches on overflow.  This yields     \
     better code, avoiding a move of the result from a temporary to (res). */   \
  jitter_int _jitter_opd0_result = (opd0); \
  jitter_int _jitter_opd1_result = (opd1); \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                        \
            JITTER_ASM_PATCH_IN_PLACEHOLDER /* See patch-in comment above */    \
               (0 /* size in bytes */,                                          \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_OTHER /*case*/, \
                (tgt), 0, 0, 0 /* not used for this case */)                    \
            "1: bovc %[jitter_operand0], %[jitter_operand1], 1b\n\t"            \
            /* If we arrived here we did not branch away and we can perform     \
               the sum from C without fear of overflowing.  Notice that addu    \
               (or addiu) is permitted at this point in the r6 forbidden        \
               slot. */                                                         \
            : /* outputs */                                                     \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                            \
              [jitter_operand0] "r" (_jitter_opd0_result),                      \
              [jitter_operand1] "r" (_jitter_opd1_result),                      \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */                \
            : /* clobbers */                                                    \
            : jitter_dispatch_label /* goto labels */);                         \
  /* Perform the sum in C, which will not overflow. */                          \
  (res) = _jitter_opd0_result + _jitter_opd1_result

/* There is no instruction similar to bovc for subtraction, and adding opd1
   with its sign changed might itself overflow, when opd1 is
   JITTER_MOST_NEGATIVE_SIGNED. */
#define _JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  /* Rewrite into _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_ when          \
     changing the sign of opd1 is known not to overflow.  This happens in some  \
     important cases such as when subtracting one, or indeed when subtracting   \
     any other literal different from JITTER_MOST_NEGATIVE_SIGNED.              \
     In the other cases use the fallback solution, which seems better than two  \
     overflow checks (one for reversing opd1's sign then another for plus)      \
     surviving into run time. */                                                \
  jitter_int _jitter_opd0_result_for_minus = (opd0);                            \
  jitter_int _jitter_opd1_result_for_minus = (opd1);                            \
  if (JITTER_KNOWN_NOT_TO_BE (_jitter_opd1_result_for_minus,                    \
                              JITTER_MOST_NEGATIVE_SIGNED))                     \
    {                                                                           \
      _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_                           \
         ((res),                                                                \
          _jitter_opd0_result_for_minus,                                        \
          (- _jitter_opd1_result_for_minus),                                    \
          (tgt));                                                               \
    }                                                                           \
  else                                                                          \
    {                                                                           \
      /* This is what the fallback check for minus overflow checking would do:  \
         See _JITTER_LOW_LEVEL_BRANCH_FAST_IF_MINUS_OVERFLOWS_ as defined in    \
         jitter/jitter-fast-branch.h .  The idea is using C to compute an       \
         expression, and then assembly to branch on its sign.  Not              \
         materialising a Boolean result from C avoids one further               \
         conditional. */                                                        \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_                                \
         (JITTER_WOULD_MINUS_OVERFLOW_SIGNED_WORD                               \
             (jitter_uint,                                                      \
              jitter_int,                                                       \
              _jitter_opd0_result_for_minus,                                    \
              _jitter_opd1_result_for_minus,                                    \
              JITTER_BITS_PER_WORD),                                            \
          (tgt));                                                               \
      (res) = _jitter_opd0_result_for_minus - _jitter_opd1_result_for_minus;    \
    }

/* There is no instruction similar to bovc for subtraction, and adding opd1
   with its sign changed might itself overflow, when opd1 is
   JITTER_MOST_NEGATIVE_SIGNED. */
#define _JITTER_LOW_LEVEL_TIMES_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  /* This uses the same fragile trick as on x86_64, in which an inline asm      \
     operand is actually an output, but must be declared as input.  See the     \
     comment about a macro named like this in                                   \
     machine/x86_64/jitter/machine/jitter-machine.h . */                        \
  register jitter_int _jitter_product                                           \
     asm (JITTER_NON_RESERVED_REGISTER_TEMPORARY_RESULT_STRING);                \
  asm volatile ("" : "=r" (_jitter_product));                                   \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                        \
            ".set noat\n"                                                       \
            "mul %[jitter_product], %[jitter_operand0], %[jitter_operand1]\n\t" \
            "muh " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING         \
               ", %[jitter_operand0], %[jitter_operand1]\n\t"                   \
            "sra " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING         \
               ", %[jitter_product], 31\n\t"                                    \
            JITTER_ASM_PATCH_IN_PLACEHOLDER /* See patch-in comment above */    \
               (0 /* size in bytes */,                                          \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_R6_CONDITIONAL_OTHER/*case*/,  \
                (tgt), 0, 0, 0 /* not used for this case */)                    \
            "1: bnec " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING     \
               ", " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING        \
               ", 1b\n\t"                                                       \
            /* The next instruction will be a move, either or or addu, allowed  \
               even in an r6 forbidden slot. */                                 \
            ".set at\n"                                                         \
            : /* outputs */                                                     \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                            \
              [jitter_product] "r" (_jitter_product), /* actually an output! */ \
              [jitter_operand0] "r" (opd0),                                     \
              [jitter_operand1] "r" (opd1),                                     \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */                \
            : JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING,             \
              JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING              \
              /* clobbers */                                                    \
            : jitter_dispatch_label /* goto labels */);                         \
  /* Make sure to get the current value in the register as the result, and not  \
     a previous copy.  In order to force this, pretend to update the resiter    \
     here, only if the branch was not taken.  Inline asm will use the same      \
     register assignment for _jitter_product. */                                \
  asm ("" : "+r" (_jitter_product));                                            \
  (res) = _jitter_product

#else /* pre-r6 */

/* This factors the common logic of low-level conditional fast branches.
   opd1_tplt is a string literal template for the second register operand,
   including an initial comma; it can be passed as an empty string when there is
   no second register operand. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_(insn, opd0, opd1_tplt,   \
                                                   opd1, tgt)               \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                    \
            /* Emit the patch-in right before a correct conditional branch  \
               instruction, where the only field to patch is the 16-bit     \
               branch offset.  All the rest, including registers, is        \
               correctly generated by inline asm and needs no patching. */  \
            JITTER_ASM_PATCH_IN_PLACEHOLDER                                 \
               (0 /* size in bytes */,                                      \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /*case*/,  \
                (tgt),                                                      \
                0, 0, 0 /* not used for this case */)                       \
            "\n.set noreorder\n\t"                                          \
            "\n.set nomacro\n\t"                                            \
            "\n.set noat\n\t"                                               \
            "1:\n\t"                                                        \
            insn " %[jitter_operand0]" opd1_tplt ", 1b\n\t"                 \
            JITTER_ASM_MIPS_NOP "\n\t"                                      \
            "\n.set at\n\t"                                                 \
            "\n.set macro\n\t"                                              \
            "\n.set reorder\n\t"                                            \
            : /* outputs */                                                 \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                        \
              [jitter_operand0] "r" (opd0),                                 \
              [jitter_operand1] "Jr" (opd1),                                \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */            \
            : /* clobbers */                                                \
            : jitter_dispatch_label /* goto labels */)

/* MIPS can do a conditional branch (no separate comparison instruction needed)
   on a register *sign*, and in this case every ordering predicate can be
   managed in one instruction.  The same does not hold for comparisons between
   two registers (see below), so these primitives are important: high-level fast
   branches will rewrite into these when possible.  */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_(opd0, tgt)                    \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("beqz", (opd0), "", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_(opd0, tgt)                 \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("bnez", (opd0), "", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_(opd0, tgt)                \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("bltz", (opd0), "", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONNEGATIVE_(opd0, tgt)             \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("bgez", (opd0), "", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_POSITIVE_(opd0, tgt)                \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("bgtz", (opd0), "", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONPOSITIVE_(opd0, tgt)             \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("blez", (opd0), "", 0, (tgt))

/* Low-level fast branches on equal or not-equal. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_EQUAL_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                      \
     ("beq", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTEQUAL_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                         \
     ("bne", (opd0), ", %[jitter_operand1]", (opd1), (tgt))

/* MIPS has a slti/sltiu (set register to 1 or 0 according to a less-than
   comparison, possibly unsigned, of another register against an immediate)
   instruction but no equivalent operation for greater, not-less or not-greater.
   So, implement every conditional as a less-than test followed by a conditional
   branch on either zero or non-zero: two instructions total, without counting
   the delay slot.

   What follows is embarrassingly simple but seems to use slti/sltiu the right
   way without ever generating unneeded xori instructions to negate a
   comparison: the trick is letting GCC do it, using inline assembly for the
   conditional branch on zero/nonzero but not the comparison itself.  As long as
   I do not have patch-in support for literals (which I might eventually want to
   have -- use case: comparison against a constant in the guard of a VM for
   loop, counting up) this might in fact be optimal, thanks to the logic of low-
   versus high-level conditional fast branches, which now takes care of
   rewriting conditionals into simpler forms.
   Someday I will have to carefully look at the disassembly and see if there is
   any case which I would compile differently, but I have not found any yet. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_                             \
     (((jitter_int) (opd0) < (jitter_int) (opd1)), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_                                \
     (((jitter_int) (opd1) < (jitter_int) (opd0)), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_                                   \
     (((jitter_int) (opd0) < (jitter_int) (opd1)), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_                                      \
     (((jitter_int) (opd1) < (jitter_int) (opd0)), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_                               \
     (((jitter_uint) (opd0) < (jitter_uint) (opd1)), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_                                  \
     (((jitter_uint) (opd1) < (jitter_uint) (opd0)), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_                                     \
     (((jitter_uint) (opd0) < (jitter_uint) (opd1)), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_                                        \
     (((jitter_uint) (opd1) < (jitter_uint) (opd0)), (tgt))

/* Jitter's default solution for overflow-checking on sum and subtraction
   appears to be optimal on MIPS.  For best efficiency it relies on a
   machine-specific version of _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_ ,
   which is defined above. */

/* I can do better than just using the GCC builtin as an operand from inline
   asm, in this case: in particular I can reuse the multiplication result which
   gets computed when deciding whether overflow occurs, and avoid multiplying
   twice.
   Notice that this solution only makes sense on pre-r6 MIPS, as it relies on
   the hi and lo registers, which have been deprecated and then removed.  I
   might want to write similar code for r6, if there is need. */
#define _JITTER_LOW_LEVEL_TIMES_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  /* This uses the same very fragile trick as on x86_64, in which an inline     \
     asm operand is actually an output, but must be declared as input.  See     \
     the comment about a macro named like this in                               \
     machine/x86_64/jitter/machine/jitter-machine.h . */                        \
  /* FIXME: generalize this machinery.  It will be useful for other ports as    \
     well. */                                                                   \
  /* Notice that this solution does not work on pre-r6 MIPS64, nor on r6 where  \
     hi and ho have been removed.  An essentially identical idea using dmult    \
     or mul/muh or dmul/dmulh will work, but this code will need to be either   \
     repeated or conditionalized in an intrusive way. */                        \
  register jitter_int _jitter_product                                           \
     asm (JITTER_NON_RESERVED_REGISTER_TEMPORARY_RESULT_STRING);                \
  asm volatile ("" : "=r" (_jitter_product));                                   \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                        \
            "mult %[jitter_operand0], %[jitter_operand1]\n\t"                   \
            "mflo %[jitter_product]\n\t"                                        \
            "mfhi " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING "\n\t" \
            "sra " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING         \
               ", %[jitter_product], 31\n\t"                                    \
            JITTER_ASM_PATCH_IN_PLACEHOLDER /* See patch-in comment above */    \
               (0 /* size in bytes */,                                          \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /* case */,    \
                (tgt), 0, 0, 0 /* not used for this case */)                    \
            "\n.set noreorder\n\t"                                              \
            "\n.set nomacro\n\t"                                                \
            "\n.set noat\n\t"                                                   \
            "1:\n\t"                                                            \
            "bne " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING         \
               ", " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING        \
               ", 1b\n\t"                                                       \
            JITTER_ASM_MIPS_NOP "\n\t"                                          \
            "\n.set at\n\t"                                                     \
            "\n.set macro\n\t"                                                  \
            "\n.set reorder\n\t"                                                \
            : /* outputs */                                                     \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                            \
              [jitter_product] "r" (_jitter_product), /* actually an output! */ \
              [jitter_operand0] "r" (opd0),                                     \
              [jitter_operand1] "r" (opd1),                                     \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */                \
            : "hi", "lo",                                                       \
              JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING,             \
              JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING              \
              /* clobbers */                                                    \
            : jitter_dispatch_label /* goto labels */);                         \
  /* Make sure to get the current value in the register as the result, and not  \
     a previous copy.  In order to force this, pretend to update the resiter    \
     here, only if the branch was not taken.  Inline asm will use the same      \
     register assignment for _jitter_product. */                                \
  asm ("" : "+r" (_jitter_product));                                            \
  (res) = _jitter_product

#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

#endif // #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)

/* There is no need to explicitly define conditional fast branches on and and
   not-and.  The code we get from the existing fast branches, first computing
   the bitwise and in C and then branching on a zero condition on it from a
   patch in is already optimal on both r6 and pre-r6. */




/* VM procedures.
 * ************************************************************************** */

/* Procedures are supported on MIPS ; but we conditionalize the definition
   on JITTER_MACHINE_SUPPORTS_PATCH_IN , for testing convenience. */
#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
# define JITTER_MACHINE_SUPPORTS_PROCEDURE    1
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN)   \
    && defined(JITTER_DISPATCH_NO_THREADING)       \
    && defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)

/* The return address is in $31 (and I never need to worry about saving its
   initial value before starting, since the executor has already called C
   functions at initialization, which means that before the executor
   returns it will have to restore its original $31 from the stack anyway);
   just copy $31 to the union . */
#define _JITTER_PROCEDURE_PROLOG(link_union)                                    \
  do                                                                            \
    {                                                                           \
      register void * jitter_the_return_address asm ("$31");                    \
      /* Let GCC believe we are initializing $31 in the inline asm code; */     \
      /* in reality it's already set. */                                        \
      asm ("# Pretend to change %[return_address], even if it's already set."   \
           : [return_address] "+r" (jitter_the_return_address) /* outputs */);  \
      (link_union).pointer = jitter_the_return_address;                         \
    }                                                                           \
  while (false)

/* The assembly template returning to the address in $31, used in _JITTER_RETURN
   .  This can assume that the operand jitter_return_addr is always in $31. */
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
# define JITTER_ASM_RETURN          \
    "jic %[jitter_return_addr], 0"
#else
# define JITTER_ASM_RETURN                                         \
    /* No .set noreorder: Gas might find something useful to fill  \
       the delay slot with. */                                     \
    "jalr $0, %[jitter_return_addr]"
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

/* The recommended way of returning from a procedure on MIPS is by jumping via
   register to $31 ; the MIPS32 specification, while remaining abstract, at
   least strongly implies that jumping to $31 may give the correct (pop) hint to
   the CPU branch target predictor, differently from other registers; GCC
   generates code consistent with this hypothesis.  So it's simple: place the
   target address into $31 , and jump to it. */
#define _JITTER_RETURN(link_rvalue)                                            \
  do                                                                           \
    {                                                                          \
      /* By using a local register variable we can avoid a register copy */    \
      /* to set $31 . */                                                       \
      register const void * const jitter_the_return_address asm ("$31")        \
        = (const void* const) (link_rvalue);                                   \
      asm goto (JITTER_ASM_RETURN                                              \
                /* Putting the defect descriptor before jr (pre-r6) prevents   \
                   Gas from using the delay slot intelligently.  It is         \
                   harmless to have it here after the return instruction. */   \
                JITTER_ASM_DEFECT_DESCRIPTOR                                   \
                : /* outputs. */                                               \
                : [jitter_return_addr]                                         \
                  "r" (jitter_the_return_address) /* inputs. */                \
                : /* clobbers. */                                              \
                : jitter_dispatch_label /* gotolabels. */);                    \
      /* The rest of the VM instruction is unreachable. */                     \
      __builtin_unreachable ();                                                \
    }                                                                          \
  while (false)

/* The assembly template branching-and-linking via register, used in
   JITTER_BRANCH_AND_LINK_INTERNAL . */
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
# define JITTER_ASM_BRANCH_AND_LINK_VIA_REGISTER_TEMPLATE               \
    /* The return address always goes to $31 with this instruction. */  \
    "jialc %[_jitter_the_target], 0"
#else
# define JITTER_ASM_BRANCH_AND_LINK_VIA_REGISTER_TEMPLATE          \
    /* No .set noreorder: Gas might find something useful to fill  \
       the delay slot with. */                                     \
    "jalr $31, %[_jitter_the_target]"
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

/* Easy: perform a jump-and-link via ragister, and the return address will be
   in $31 . */
#define _JITTER_BRANCH_AND_LINK_NATIVE(callee_rvalue)                         \
  do                                                                          \
    {                                                                         \
      const void * const jitter_destination =                                 \
        (const void * const) (callee_rvalue);                                 \
      /* Applicable to pre-r6: Gas seems to do a good job of using the delay  \
         slot, which is not always a nop.  This is why I am not using         \
            .set nomacro, noreorder                                           \
         . */                                                                 \
      asm goto (JITTER_ASM_BRANCH_AND_LINK_VIA_REGISTER_TEMPLATE              \
                /* See the comment in JITTER_RETURN. */                       \
                JITTER_ASM_DEFECT_DESCRIPTOR                                  \
                : /* outputs. */                                              \
                : [_jitter_the_target] "r" (jitter_destination) /* inputs. */ \
                : "$31" /* clobbers. */                                       \
                : jitter_dispatch_label /* gotolabels. */);                   \
      /* It would be incorrect to have __builtin_unreachable or               \
         JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END here: see the comment in  \
         the x86_64 version. */                                               \
    }                                                                         \
  while (false)

/* The assembly template branching-and-linking-with via register, used in
   _JITTER_BRANCH_AND_LINK_WITH_NATIVE . */
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
# define JITTER_ASM_BRANCH_AND_LINK_WITH_TEMPLATE                   \
    /* The pre-r6 solution would work here as well, and the delay   \
       slot would not be wasted.  However I am assuming that in     \
       some new out-of-order implementations compact branches may   \
       be more efficient. */                                        \
    "or $31, %[jitter_new_link], $0\n\t"                            \
    "jic %[jitter_callee_rvalue], 0\n\t"
#else
# define JITTER_ASM_BRANCH_AND_LINK_WITH_TEMPLATE  \
    "\n.set noreorder\n\t"                         \
    "\n.set nomacro\n\t"                           \
    "jr %[jitter_callee_rvalue]\n\t"               \
    "or $31, %[jitter_new_link], $0\n\t"           \
    "\n.set macro\n\t"                             \
    "\n.set reorder\n\t"
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

/* Perform an ordinary jump thru register, and load the given return address
   in $31, exploiting the delay slot. */
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
                JITTER_ASM_BRANCH_AND_LINK_WITH_TEMPLATE                      \
                : /* outputs. */                                              \
                : [jitter_callee_rvalue] "r" (jitter_callee_rvalue),          \
                  [jitter_new_link] "r" (jitter_new_link) /* inputs. */       \
                : "$31" /* clobbers. */                                       \
                : jitter_dispatch_label /* gotolabels. */);                   \
      /* The rest of the VM instruction is unreachable: this tail call is an  \
         unconditional jump. */                                               \
      __builtin_unreachable ();                                               \
    }                                                                         \
  while (false)

#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
/* Perform (pre-r6) a pseudo-direct jump-and-link or (>= r6) a branch-and-link
   compact, with the return address in $31 .
   Of course we need a patch-in for the jump or branch instruction, since
   the destination address is encoded in the jumping instruction. */
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
                : jitter_dispatch_label /* gotolabels. */);                    \
      /* It would be incorrect to have __builtin_unreachable or                \
         JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END here: see the comment in   \
         the x86_64 version. */                                                \
    }                                                                          \
  while (false)
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#endif // #if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) ...




/* C-only page, expanding to nothing if this header is included by assembly.
 * ************************************************************************** */

#if ! defined (__ASSEMBLER__)

/* Notice that the order matters, and these shouldn't be rearranged without also
   changing the order of definitions in machine.S .  We also rely on the first
   case having value 0, since we use enum jitter_snippet_to_patch values as
   array indices. */
enum jitter_snippet_to_patch
  {
    jitter_snippet_load_zero_extended_16bit_to_register_0,
    jitter_snippet_load_zero_extended_16bit_to_register_1,
    jitter_snippet_load_zero_extended_16bit_to_register_2,
    jitter_snippet_load_zero_extended_16bit_to_register_3,
    jitter_snippet_load_zero_extended_16bit_to_register_4,
    jitter_snippet_load_zero_extended_16bit_to_register_5,
    jitter_snippet_load_sign_extended_16bit_to_register_0,
    jitter_snippet_load_sign_extended_16bit_to_register_1,
    jitter_snippet_load_sign_extended_16bit_to_register_2,
    jitter_snippet_load_sign_extended_16bit_to_register_3,
    jitter_snippet_load_sign_extended_16bit_to_register_4,
    jitter_snippet_load_sign_extended_16bit_to_register_5,
    jitter_snippet_load_32bit_to_register_0,
    jitter_snippet_load_32bit_to_register_1,
    jitter_snippet_load_32bit_to_register_2,
    jitter_snippet_load_32bit_to_register_3,
    jitter_snippet_load_32bit_to_register_4,
    jitter_snippet_load_32bit_to_register_5,
#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    jitter_snippet_load_pcrel_address_to_register_0,
    jitter_snippet_load_pcrel_address_to_register_1,
    jitter_snippet_load_pcrel_address_to_register_2,
    jitter_snippet_load_pcrel_address_to_register_3,
    jitter_snippet_load_pcrel_address_to_register_4,
    jitter_snippet_load_pcrel_address_to_register_5,
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    /* FIXME: the next three are not implemented yet. */
    jitter_snippet_load_zero_extended_16bit_to_memory,
    jitter_snippet_load_sign_extended_16bit_to_memory,
    jitter_snippet_load_32bit_to_memory,

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    jitter_snippet_branch_unconditional_28bit_compact,
    jitter_snippet_branch_and_link_28bit_compact,
#else
    jitter_snippet_jump_unconditional_28bit_pseudo_direct,
    jitter_snippet_jump_and_link_28bit_pseudo_direct,
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

#if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)
    /* Compact conditional branches, introduced in r6, allow for a wider
       displacement in tests for zero than for comparisons on equality or
       magnitude. */
    jitter_snippet_branch_conditional_compact_18bit_offset,
    jitter_snippet_branch_conditional_compact_23bit_offset,
#else
    /* The same snippet works for any conditional branch (pre-r6). */
    jitter_snippet_branch_conditional_18bit_offset,
#endif // #if defined (JITTER_HOST_CPU_IS_MIPS_R6_OR_LATER)

    /* The number of snippets. */
    jitter_snippet_no
  };

#endif // #if ! defined (__ASSEMBLER__)

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
