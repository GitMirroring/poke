/* VM library: m68k definitions, to be included from both C and assembly.

   Copyright (C) 2017, 2018, 2019, 2020, 2021 Luca Saiu
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




/* How to introduce comments in assembly on m68k, at any point within the
   line. */
#define JITTER_ASM_COMMENT_PREFIX                               \
  /* "#" works in some contexts but has other uses as well. */  \
  "| "




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)       \
  /* The best way I could find on m68k takes two instructions. */  \
  "eoril #" integer_literal_as_string ", %%d0\n\t"                 \
  "eoril #" integer_literal_as_string ", %%d0\n\t"

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                             \
  /* The m68k has a canonical instruction for this case whose usage,  \
     despite the name, does not violate any law. */                   \
  "illegal"




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE  \
  "jmp %[_jitter_the_target]@"
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT                             \
  /* Unfortunately we cannot indirect-jump through data registers ("d"), and  \
     this may cause some superfluous moves. */                                \
  "a"




/* Reserved registers.
 * ************************************************************************** */

/* At least on my test m68k configuration (GNU/Linux, debian, as of late 2020)
   the following registers appear to be usable:
     %d2 %d3 %d4 %d5 %d6 %d7
     %a2 %a3 %a4
   Reserved registers:
     %a6 also called %fp, is the frame pointer; apparently not usable here;
     %a7 also called %sp, is the stack pointer; of course I cannot use it.

   This looks very promising.  However having many registers which are usable
   in theory does not mean that we should use them all.  The architecture is
   not particularly register-rich, and as many registers as possible should be
   left available for VM state structures.  Moreover the m68k has no problem
   generating residuals in memory, and the typical implementation should have
   relatively fast memory access, compared to the CPU [FIXME: is this still
   true on modern embeddded implementations?]
   Tweaking JITTER_RESIDUAL_REGISTER_NO is a good way to experiment. */

/* How many registers we can use to hold residual arguments.  The residual
   registers defined below can be not be more than the value of this. */
//#define JITTER_RESIDUAL_REGISTER_NO   6 // the current maximum
#define JITTER_RESIDUAL_REGISTER_NO   2
//#define JITTER_RESIDUAL_REGISTER_NO   0 // this is good for testing memory literals

/* Register pointing to The Array base.  This is an ideal candidate for an
   address register on m68k, as the register is almost always used as a base for
   memory operands.
   This version has no "%" prefix in the register name, for ease of use in
   assembly with Gas and CPP macros. */
#define JITTER_UNPREFIXED_BASE_REGISTER  a2

/* Names for registers holding residual arguments, with 0-based suffixes.  Only
   the first JITTER_RESIDUAL_REGISTER_NO registers are actually used; fewer for
   VMs not requiring every residual.
   These register names have no "%" prefix, for ease of use in assembly with Gas
   and CPP macros. */
#define JITTER_UNPREFIXED_RESIDUAL_REGISTER_0    d2
#define JITTER_UNPREFIXED_RESIDUAL_REGISTER_1    d3
#define JITTER_UNPREFIXED_RESIDUAL_REGISTER_2    d4
#define JITTER_UNPREFIXED_RESIDUAL_REGISTER_3    d5
#define JITTER_UNPREFIXED_RESIDUAL_REGISTER_4    d6
#define JITTER_UNPREFIXED_RESIDUAL_REGISTER_5    d7

/* The same as the unprefixed macro above, with a "%" prefix. */
#define JITTER_BASE_REGISTER          %JITTER_UNPREFIXED_BASE_REGISTER

/* The same as the unprefixed macros above, with a "%" prefix. */
#define JITTER_RESIDUAL_REGISTER_0    %JITTER_UNPREFIXED_RESIDUAL_REGISTER_0
#define JITTER_RESIDUAL_REGISTER_1    %JITTER_UNPREFIXED_RESIDUAL_REGISTER_1
#define JITTER_RESIDUAL_REGISTER_2    %JITTER_UNPREFIXED_RESIDUAL_REGISTER_2
#define JITTER_RESIDUAL_REGISTER_3    %JITTER_UNPREFIXED_RESIDUAL_REGISTER_3
#define JITTER_RESIDUAL_REGISTER_4    %JITTER_UNPREFIXED_RESIDUAL_REGISTER_4
#define JITTER_RESIDUAL_REGISTER_5    %JITTER_UNPREFIXED_RESIDUAL_REGISTER_5

/* The m68k can easily materialise word-sized constants without a scratch
   register, and does not need one for other purposes either. */

/* These data registers are not reserved, but only used as assembly operands for
   register local variables from a few macros here.  They do not even need to be
   caller-save; however they must not conflict with reserved registers.
   These identifiers are specific to m68k, and not part of the Jitter API. */
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY  %d0
#define JITTER_NON_RESERVED_REGISTER_REMAINDER  %d1




/* Patch-ins.
 * ************************************************************************** */

/* Patch-ins are supported on the the M68K architecture. */
#define JITTER_MACHINE_SUPPORTS_PATCH_IN  1

/* On m68k an instruction entirely made of 0xff bytes is invalid. */
#define JITTER_ASM_PATCH_IN_FILL_BYTE    "0x00"

/* The branch instructions we patch all have the same format:
   - 8 bits: for opcode, condition codes and the like. 
   - 8 bits: short displacement (not used by Jittet)
   - 32 bits: long displacement
   We patch the 32-bit operand (a signed displacement) which is in the last
   field, preceded by an 8-bit displacment which we set to 0xff to mean that
   the displacement is in fact 32-bit.
   Conditional branches use a zero-byte patch-in.  See below. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL    6
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK  6




/* VM low-level conditional branches.
 * ************************************************************************** */

/* Ignore this page if patch-ins have been disabled (for debugging) or the
   dispatching model does not support them. */
#if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)

/* Common code factoring conditional branches.  The second operand template can
   be an empty string when it is not needed; otherwise it should start with a
   comma. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_(opd0_constraint, opd0,      \
                                                   opd1_template,              \
                                                   opd1_constraint, opd1,      \
                                                   compare_insn, branch_insn,  \
                                                   tgt)                        \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                       \
            compare_insn " %[jitter_operand0]" opd1_template "\t\n"            \
            /* Emit the patch-in right before a correct conditional branch     \
               instruction, where the only field to patch is the branch        \
               offset.  All the rest is correctly generated                    \
               by the inline asm and needs no patching. */                     \
            JITTER_ASM_PATCH_IN_PLACEHOLDER                                    \
               (0 /* size in bytes */,                                         \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /*case*/,     \
                (tgt),                                                         \
                0, 0, 0 /* not used for this case */)                          \
            "1: " branch_insn " 1b\t\n"                                        \
            : /* outputs */                                                    \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                           \
              [jitter_operand0] opd0_constraint (opd0),                        \
              [jitter_operand1] opd1_constraint (opd1),                        \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */               \
            : /* clobbers */                                                   \
            : jitter_fake_target /* goto labels */)

/* Low-level fast branches on sign. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_(opd0, tgt)                \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("dam", (opd0), "", "i", 0, \
                                              "tstl", "beql", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_(opd0, tgt)             \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("dam", (opd0), "", "i", 0, \
                                              "tstl", "bnel", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_(opd0, tgt)            \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("dam", (opd0), "", "i", 0, \
                                              "tstl", "bltl", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONNEGATIVE_(opd0, tgt)         \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("dam", (opd0), "", "i", 0, \
                                              "tstl", "bgel", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_POSITIVE_(opd0, tgt)            \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("dam", (opd0), "", "i", 0, \
                                              "tstl", "bgtl", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONPOSITIVE_(opd0, tgt)         \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("dam", (opd0), "", "i", 0, \
                                              "tstl", "blel", (tgt))

/* Low-level fast branches on equality comparisons. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_EQUAL_(opd0, opd1, tgt)       \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),          \
                                              ", %[jitter_operand1]",  \
                                              "da", (opd1),            \
                                              "cmpl", "beql", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTEQUAL_(opd0, opd1, tgt)    \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),          \
                                              ", %[jitter_operand1]",  \
                                              "da", (opd1),            \
                                              "cmpl", "bnel", (tgt))

/* Low-level fast branches on magnitude comparisons. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),           \
                                              ", %[jitter_operand1]",   \
                                              "da", (opd1),             \
                                              "cmpl", "bgtl", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),              \
                                              ", %[jitter_operand1]",      \
                                              "da", (opd1),                \
                                              "cmpl", "bltl", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),               \
                                              ", %[jitter_operand1]",      \
                                              "da", (opd1),                \
                                              "cmpl", "blel", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),                 \
                                              ", %[jitter_operand1]",         \
                                              "da", (opd1),                   \
                                              "cmpl", "bgel", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),             \
                                              ", %[jitter_operand1]",     \
                                              "da", (opd1),               \
                                              "cmpl", "bhil", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),                \
                                              ", %[jitter_operand1]",        \
                                              "da", (opd1),                  \
                                              "cmpl", "blol", (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd0),                \
                                              ", %[jitter_operand1]",        \
                                              "da", (opd1),                  \
                                              "cmpl", "blsl", (tgt))
/* Is there a bcc variant symmetrical to blsl?  I found none.  However I
   can just use the solution from the previous macros and reverse the order
   of the operands. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idam", (opd1),                   \
                                              ", %[jitter_operand1]",           \
                                              "da", (opd0),                     \
                                              "cmpl", "blsl", (tgt))

/* Low-level fast branches on bitwise and.  This is slightly involved, but the
   m68k can efficiently support the case we are interested in, checking the low
   bits for a tag. */

/* Expand to a constant expression evaluating to non-false iff the argument is
   itself known to be a constant expression, suitable as a mask to be used with
   the bftst instruction on the m68k. */
#define JITTER_M68K_BFTST_CANDIDATE(non_side_effecting_expression)       \
  /* The only operands we can use are compile-time constants which are   \
     nonzero predecessors of  powers of two.  These describe contiguous  \
     bit masks at the low bits of a word, starting with the least        \
     significant.  These bits, which can hold tags, can be examined      \
     with bftst. */                                                      \
  (__builtin_constant_p (non_side_effecting_expression)                  \
   && (non_side_effecting_expression) != 0                               \
   && JITTER_IS_A_POWER_OF_TWO ((non_side_effecting_expression) + 1))

/* Expand to a low-level fast branch using bftst, assuming the mask is a
   compile-time constant of the right shape. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_BFTST_(opd, mask,          \
                                                    branch_insn, tgt)   \
  jitter_int _jitter_field_length = __builtin_popcount (mask);          \
  /* The bftst syntax requires the operand to check and two immediates  \
     the first one specifying at which bit the mask begins, counting    \
     zero-based from the most significant (!) bit, the second giving    \
     the mask width in bits.                                            \
     The template for arg1 here actually contains the two               \
     immediates. */                                                     \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                            \
     ("idm", (opd),                                                     \
      ", 32-%c[jitter_operand1], %c[jitter_operand1]",                  \
      "J", (_jitter_field_length),                                      \
      "bftst", branch_insn, (tgt))

/* Expand to either _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_BFTST_ , when
   applicable, or to a more general fallback solution.  This factors the
   two fast branches on and and notand. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_(opd0, opd1,        \
                                                        branch_insn, and,  \
                                                        tgt)               \
  jitter_uint _jitter_opd0 = (jitter_uint) (opd0);                         \
  jitter_uint _jitter_opd1 = (jitter_uint) (opd1);                         \
  if (JITTER_M68K_BFTST_CANDIDATE (_jitter_opd0))                          \
    {                                                                      \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_BFTST_ (_jitter_opd1,           \
                                                   _jitter_opd0,           \
                                                   branch_insn, (tgt));    \
    }                                                                      \
  else if (JITTER_M68K_BFTST_CANDIDATE (_jitter_opd1))                     \
    {                                                                      \
      _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_BFTST_ (_jitter_opd0,           \
                                                   _jitter_opd1,           \
                                                   branch_insn, (tgt));    \
    }                                                                      \
  else                                                                     \
    {                                                                      \
      if (and)                                                             \
        _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_ ((_jitter_opd0           \
                                                    & _jitter_opd1),       \
                                                   (tgt));                 \
      else                                                                 \
        _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_ ((_jitter_opd0              \
                                                 & _jitter_opd1),          \
                                                (tgt));                    \
    }

/* Branch-fast on bitwise and. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_(opd0, opd1, tgt)           \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_ ((opd0), (opd1),       \
                                                   "bnel", true, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTAND_(opd0, opd1, tgt)         \
  _JITTER_LOW_LEVEL_BRANCH_FAST_IF_AND_OR_NOTAND_ ((opd0), (opd1),        \
                                                   "beql", false, (tgt))

/* Overflow-checking operations. */

/* This macro factors the common code in every overflow-checking operation. */
#define _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_(                  \
            insn, branch_if_opd1_is_zero, is_remainder, middle_operand_tplt,   \
            res, opd0, opd1, tgt)                                              \
  /* This uses the same delicate trick as on x86_64, in which an inline asm    \
     operand is actually an output, but must be declared as input.  See the    \
     comment about similar macros in                                           \
     machine/x86_64/jitter/machine/jitter-machine.h .                          \
     Handling the remainder operation here is even more complicated.  The      \
     divll instruction, in the assembler syntax, can have two or three         \
     operands: the two-operand version is an alternative syntax for the        \
     three-operand version where the last two registers are the same: in       \
     that case the operation computes a quotient only.  When the last two      \
     operands are different the middle one holds the dividend and (as output)  \
     the remainder, and the last one (only used as output) the quotient.       \
     The variable _jitter_llop_remainder, when computing a remainder, is       \
     bound to the register holding the remainder result.  Inline asm           \
     statements with empty templates having it as an operand force the         \
     correct ordering.                                                         \
     Ordinary two-operand insteger instructions will take the empty string as  \
     the middle operand template. */                                           \
  jitter_int _jitter_llop_opd0_result = (opd0);                                \
  jitter_int _jitter_llop_opd1_result = (opd1);                                \
  /* The m68k traps on division by zero, but here I want to branch to the      \
     overflow label in that case.  Add an explicit check, using a *high-level* \
     branch in order to seize opportunities to optimise away the check         \
     [FIXME: can the check be optimised away?  Divisions by -1 are already     \
     subtractions, so do not use this macro; can there be overflow with other  \
     negative divisors?  Not for the quotient, but maybe for the remainder.    \
     I have to think of it.  Anyway the current check is correct.] */          \
  if (branch_if_opd1_is_zero)                                                  \
    JITTER_BRANCH_FAST_IF_ZERO (_jitter_llop_opd1_result, (tgt));              \
  register jitter_int _jitter_llop_temporary                                   \
     asm (JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_TEMPORARY))           \
    = _jitter_llop_opd0_result;                                                \
  asm ("" : "+d" (_jitter_llop_temporary));                                    \
  /* This expands to code actually changing _jitter_llop_temporary which is    \
     passed as if it were only an *input* argument, because of the GCC         \
     asm goto limitation.  The instruction we use to "compare" is in fact the  \
     one also updating _jitter_llop_temporary (and _jitter_llop_remainder      \
     when computing the remainder). */                                         \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("idm",                           \
                                              (_jitter_llop_opd1_result),      \
                                                  middle_operand_tplt          \
                                                  ", %[jitter_operand1]",      \
                                              "d", (_jitter_llop_temporary),   \
                                              insn, "bvsl", (tgt));            \
  /* If we have not branched then the computation did not overflow.  Copy the  \
     result into the destination. */                                           \
  if (is_remainder) /* Resolved at compile time, of course. */                 \
    {                                                                          \
      /* In the case of the remainder we are not actually interested in        \
         _jitter_llop_temporary any longer; the actual result is now in its    \
         intended register.  In order to force the register to be read only    \
         after the branching asm above pretend that _jitter_llop_remainder     \
         is updated (only if this statement is reached, which means after the  \
         non-taken branch) based on _jitter_llop_temporary .  At the end of    \
         this inline asm statement _jitter_llop_remainder is bound to the      \
         correct register, than we can read. */                                \
      asm (JITTER_ASM_COMMENT_UNIQUE ("pretend to update remainder")           \
           : [jitter_remainder] "+d" (_jitter_llop_remainder),                 \
             [jitter_unused_quotient] "+d" (_jitter_llop_temporary));          \
      (res) = _jitter_llop_remainder;                                          \
    }                                                                          \
  else                                                                         \
    {                                                                          \
      /* Pretend to update _jitter_llop_temporary here, after the non-taken    \
         branch.  We want to make sure that the current value is given as a    \
         result, and not a previous copy. */                                   \
      asm (JITTER_ASM_COMMENT_UNIQUE ("pretend to update result")              \
           : [jitter_temporary] "+d" (_jitter_llop_temporary));                \
      (res) = _jitter_llop_temporary;                                          \
    }

#define _JITTER_LOW_LEVEL_PLUS_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  jitter_int _jitter_llop_remainder __attribute__ ((unused));                  \
  /* In case of immediate operands Gas automatically rewrite addl into addq    \
     where possible.  This works well here, as addq sets the overflow flag     \
     just like addl (actualli addil) does. */                                  \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_ ("addl",                \
                                                        false, false, "",      \
                                                        res, opd0, opd1, tgt)
#define _JITTER_LOW_LEVEL_MINUS_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  jitter_int _jitter_llop_remainder __attribute__ ((unused));                   \
  /* The same remark about addl applies to subl. */                             \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_ ("subl",                 \
                                                        false, false, "",       \
                                                        res, opd0, opd1, tgt)
#define _JITTER_LOW_LEVEL_TIMES_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  jitter_int _jitter_llop_remainder __attribute__ ((unused));                   \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_ ("mulsl",                \
                                                        false, false, "",       \
                                                        res, opd0, opd1, tgt)
#define _JITTER_LOW_LEVEL_DIVIDED_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1,    \
                                                           tgt)                \
  jitter_int _jitter_llop_remainder __attribute__ ((unused));                  \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_ ("divsll",              \
                                                        true, false, "",       \
                                                        res, opd0, opd1, tgt)
#define _JITTER_LOW_LEVEL_REMAINDER_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1,  \
                                                             tgt)              \
  /* Keep _jitter_llop_remainder in the register that will actually hold its   \
     result when it is computed.  Pretend to give it a value immediately.  */  \
  register jitter_int _jitter_llop_remainder                                   \
     asm (JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_REMAINDER));          \
  asm ("" : "=d" (_jitter_llop_remainder));                                    \
  _JITTER_LOW_LEVEL_OPERATION_BRANCH_FAST_IF_OVERFLOW_                         \
      ("divsll",                                                               \
       true, true,                                                             \
       ", %" JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_REMAINDER),        \
       res, opd0, opd1, tgt)

#endif // #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)




/* VM procedures.
 * ************************************************************************** */

/* Procedures are supported on m68k ; however we conditionalise the definition
   on JITTER_MACHINE_SUPPORTS_PATCH_IN , for testing convenience. */
#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
# define JITTER_MACHINE_SUPPORTS_PROCEDURE    1
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN)   \
    && defined(JITTER_DISPATCH_NO_THREADING)       \
    && defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)

/* Code automatically emitted at the very beginning of callee instructions.
   This is executed in a fragile state, when the stack is not what GCC
   thinks. */
#define _JITTER_PROCEDURE_PROLOG(link_lvalue)                                 \
  do                                                                          \
    {                                                                         \
      /* Pop the return address from the stack and save it into a register    \
         variable.  It would be incorrect to accept a memory location, as     \
         it could end up on the stack, and GCC does not know that the stack   \
         has been pushed to.                                                  \
         The asm statement *must/ be volatile: the popq instruction must not  \
         be optimised away in any circumstance, even in bizarre cases where   \
         the user did not use the return value. */                            \
      void *jitter_link_temporary;                                            \
      asm volatile ("movel (%%a7)+, %[return_address]\n\t"                    \
                    : [return_address] "=da" /* No "m" constraint. */         \
                      (jitter_link_temporary) /* outputs */);                 \
      /* Copy the temporary to the location desired by the user.  If it is    \
         a register then the copy might be actually optimised away. */        \
      (link_lvalue).pointer = jitter_link_temporary;                          \
    }                                                                         \
  while (false)

/* Return from a procedure call, to the given destination.  This uses the
   hardware stack to return. */
#define _JITTER_RETURN(link_rvalue)                                            \
  do                                                                           \
    {                                                                          \
      const void * jitter_the_return_address = (const void*) (link_rvalue);    \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                   \
                /* Push the return address to the stack and return from        \
                   subroutine. */                                              \
                "movel %[return_addr], -(%%a7)\n\t"                            \
                "rts\n\t"                                                      \
                : /* outputs. */                                               \
                : [return_addr] "dam" (jitter_the_return_address) /* inputs. */ \
                : /* clobbers. */                                              \
                : jitter_fake_target /* gotolabels. */);                    \
      /* The rest of the VM instruction is unreachable. */                     \
      __builtin_unreachable ();                                                \
    }                                                                          \
  while (false)

/* Perform an indirect call. */
#define _JITTER_BRANCH_AND_LINK_NATIVE(callee_rvalue)                         \
  do                                                                          \
    {                                                                         \
      const void * const jitter_destination =                                 \
        (const void * const) (callee_rvalue);                                 \
      /* Jump to subroutine via register.  This pushes the return address on  \
         the stack, which will be popped by the callee through                \
         _JITTER_PROCEDURE_PROLOG . */                                        \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                  \
                "jsr %[destination]@\n\t"                                     \
                : /* outputs. */                                              \
                : [destination] "a" (jitter_destination) /* inputs. */        \
                : /* clobbers. */                                             \
                : jitter_fake_target /* gotolabels. */);                   \
      /* It would be incorrect to have __builtin_unreachable or               \
         JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END here: see the comment in  \
         the x86_64 version. */                                               \
    }                                                                         \
  while (false)

/* Perform an indirect call where the return address is not the next
   instruction, but the address given by the user. */
#define _JITTER_BRANCH_AND_LINK_WITH_NATIVE(_jitter_callee_rvalue,            \
                                            _jitter_new_link)                 \
  do                                                                          \
    {                                                                         \
      const void * const jitter_callee_rvalue =                               \
        (const void * const) (_jitter_callee_rvalue);                         \
      const void * const jitter_new_link =                                    \
        (const void * const) (_jitter_new_link);                              \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                  \
                /* Push the new return address to the stack, and jump         \
                   elsewhere; the jump destination will be a callee.          \
                   This solution, while semantically correct, will mess up    \
                   branch target prediction on return. */                     \
                "movel %[jitter_new_link], -(%%a7)\n\t"                       \
                "jmp %[jitter_callee_rvalue]@\n\t"                            \
                : /* outputs. */                                              \
                : [jitter_callee_rvalue] "a" (jitter_callee_rvalue),          \
                  [jitter_new_link] "dam" (jitter_new_link) /* inputs. */     \
                : /* clobbers. */                                             \
                : jitter_fake_target /* gotolabels. */);                   \
      /* The rest of the VM instruction is unreachable: this tail call is an  \
         unconditional jump. */                                               \
      __builtin_unreachable ();                                               \
    }                                                                         \
  while (false)

/* Perform a procedure call to a known destination, using a patch-in. */
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
    /* Materlialise a constant into a register. */
    jitter_snippet_load_zero_to_register_0,
    jitter_snippet_load_zero_to_register_1,
    jitter_snippet_load_zero_to_register_2,
    jitter_snippet_load_zero_to_register_3,
    jitter_snippet_load_zero_to_register_4,
    jitter_snippet_load_zero_to_register_5,
    jitter_snippet_load_8bit_sign_extended_to_register_0,
    jitter_snippet_load_8bit_sign_extended_to_register_1,
    jitter_snippet_load_8bit_sign_extended_to_register_2,
    jitter_snippet_load_8bit_sign_extended_to_register_3,
    jitter_snippet_load_8bit_sign_extended_to_register_4,
    jitter_snippet_load_8bit_sign_extended_to_register_5,
    jitter_snippet_load_32bit_to_register_0,
    jitter_snippet_load_32bit_to_register_1,
    jitter_snippet_load_32bit_to_register_2,
    jitter_snippet_load_32bit_to_register_3,
    jitter_snippet_load_32bit_to_register_4,
    jitter_snippet_load_32bit_to_register_5,

    /* Materlialise a constant into memory. */
    jitter_snippet_load_zero_to_memory,
    jitter_snippet_load_32bit_to_memory,

    /* Branches */
    jitter_snippet_branch_unconditional_32bit_offset,
    jitter_snippet_branch_and_link_32bit_offset,
    jitter_snippet_branch_conditional_32bit_offset,

    /* The number of snippets. */
    jitter_snippet_no
  };

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
