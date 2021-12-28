/* VM library: SPARC definitions, to be included from both C and assembly.
   This supports both 32- and 64-bit configurations.

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


#ifndef JITTER_NATIVE_MACHINE_H_ /* One macro suffices for all architectures. */
#define JITTER_NATIVE_MACHINE_H_


/* Assembler syntax.
 * ************************************************************************** */

/* On SPARC '#' is recognized as the beginning of a comment only when occurring
   *at the beginning* of a line.  On the other hand '!' is recognized anywhere,
   so we will always use that. */
#define JITTER_ASM_COMMENT_PREFIX "! "




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "add %%g0, " integer_literal_as_string ", %%g0"

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                  \
  /* Return from a trap in privileged mode.  This will cause an exception  \
     in user mode, of a kind not usually seen. */                          \
  "done"




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE  \
  "jmp %[_jitter_the_target]\n\t"          \
  " nop"  /* Delay slot. */
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  \
  "r"




/* Reserved registers.
 * ************************************************************************** */

/* SPARC configurations should have at least the following registers available
   as "callee-save" -- in the case of SPARC, either automatically saved/restored
   by the caller or not clobbered by the callee:
     %i0 %i1 %i2 %i3 %i4 %i5 %l0 %l1 %l2 %l3 %l4 %l5 %l6 %l7
   They are 14.
   The global registers %g1, %g2, %g3, %g4, %g5 are not preserved across calls,
   but usable as scratch.
    %g6 (??) and %g7 (thread pointer) are reserved for the system. */

/* Register pointing to The Array base. */
#define JITTER_BASE_REGISTER           %l7

/* How many registers we can use to hold residual arguments. */
#define JITTER_RESIDUAL_REGISTER_NO    12
//#define JITTER_RESIDUAL_REGISTER_NO   0 // this is good for testing memory literals

/* Registers holding residual arguments, with 0-based suffixes.  These have to
   be as many as JITTER_RESIDUAL_REGISTER_NO . */
#define JITTER_RESIDUAL_REGISTER_0     %i0
#define JITTER_RESIDUAL_REGISTER_1     %i1
#define JITTER_RESIDUAL_REGISTER_2     %i2
#define JITTER_RESIDUAL_REGISTER_3     %i3
#define JITTER_RESIDUAL_REGISTER_4     %i4
#define JITTER_RESIDUAL_REGISTER_5     %i5
#define JITTER_RESIDUAL_REGISTER_6     %l0
#define JITTER_RESIDUAL_REGISTER_7     %l1
#define JITTER_RESIDUAL_REGISTER_8     %l2
#define JITTER_RESIDUAL_REGISTER_9     %l3
#define JITTER_RESIDUAL_REGISTER_10    %l4
#define JITTER_RESIDUAL_REGISTER_11    %l5

/* The scratch register.  The same remark above applies. */
//#define JITTER_SCRATCH_REGISTER        %g1 // FIXME: this is call-clobbered, so I can't easily
                                             // reserve it, but it would work well as scratch.
#define JITTER_SCRATCH_REGISTER        %l6




/* Patch-ins.
 * ************************************************************************** */

// FIXME: my current unconditional branch patch-in locks up the Uninspired
// VM, for some reason I still have to investigate.
/* /\* Patch-ins are supported on the the SPARC architecture. *\/ */
#define JITTER_MACHINE_SUPPORTS_PATCH_IN 1 */

/* I'm not sure if this is a good way for generating invalid instructions on
   SPARC; however the pattern is easy to eyeball. */
#define JITTER_ASM_PATCH_IN_FILL_BYTE    "0xff"

/* For each patch-in case define its size in bytes, corresponding to the total
   size of the instructions to be patched in in bytes, including possible delay
   slots. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL                        8

/* /\* SPARC uses a single patch-in case for every conditional branch, and its size */
/*    is zero. *\/ */
/* #define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK                      8 */
/* /\* /\\* These should be extended in the future with floating-point conditionals. *\\/ *\/ */




/* /\* VM conditional branches. */
/*  * ************************************************************************** *\/ */

/* /\* Ignore this page if patch-ins have been disabled (for debugging) or the */
/*    dispatch does not support them. *\/ */
/* #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING) */

/* /\* The same patch-in works for any conditional branch on SPARC, and the */
/*    placeholder has size zero: the idea is emitting the patch-in right before a */
/*    correct conditional branch instruction, where the only field to patch is the */
/*    16-bit branch offset.  All the rest, including registers, is correctly */
/*    generated by inline asm and needs no patching. *\/ */
/* #define JITTER_ASM_SPARC_CONDITIONAL_BRANCH_PATCH_IN(target_index)  \ */
/*   JITTER_ASM_PATCH_IN_PLACEHOLDER(                                 \ */
/*      0 /\* size in bytes *\/,                                        \ */
/*      JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /\*case*\/,    \ */
/*      target_index,                                                 \ */
/*      0, 0, 0 /\* not used for this case *\/) */

/* /\* Emit a conditional branch instruction with one register operand. *\/ */
/* #define JITTER_SPARC_BRANCH_FAST_CONDITIONAL_R(instruction_string, operand,    \ */
/*                                               target_index)                   \ */
/*   do                                                                          \ */
/*     {                                                                         \ */
/*       asm                                                                     \ */
/*       goto (JITTER_ASM_SPARC_CONDITIONAL_BRANCH_PATCH_IN(target_index)         \ */
/*             "1:\n\t"                                                          \ */
/*             instruction_string " %[jitter_operand], 1b"                       \ */
/*             : /\* outputs *\/                                                   \ */
/*             : [jitter_operand] "r" (operand) /\* inputs *\/                     \ */
/*             : /\* clobbers *\/                                                  \ */
/*             : jitter_jump_anywhere_label,                                     \ */
/*               JITTER_SPECIALIZED_INSTRUCTION_BEGIN_LABEL /\* goto labels *\/);  \ */
/*     }                                                                         \ */
/*   while (false) */

/* /\* Emit a conditional branch instruction with two register operands. *\/ */
/* #define JITTER_SPARC_BRANCH_FAST_CONDITIONAL_R_R(instruction_string, operand0,  \ */
/*                                                 operand1, target_index)        \ */
/*   do                                                                           \ */
/*     {                                                                          \ */
/*       asm                                                                      \ */
/*       goto (JITTER_ASM_SPARC_CONDITIONAL_BRANCH_PATCH_IN(target_index)          \ */
/*             "1:\n\t"                                                           \ */
/*             instruction_string " %[jitter_operand0], %[jitter_operand1], 1b"   \ */
/*             : /\* outputs *\/                                                    \ */
/*             :   [jitter_operand0] "r" (operand0)                               \ */
/*               , [jitter_operand1] "r" (operand1) /\* inputs *\/                  \ */
/*             : /\* clobbers *\/                                                   \ */
/*             : jitter_jump_anywhere_label,                                      \ */
/*               JITTER_SPECIALIZED_INSTRUCTION_BEGIN_LABEL /\* goto labels *\/);   \ */
/*      }                                                                         \ */
/*   while (false) */

/* /\* Emit a conditional branch instruction with two register operands, where the */
/*    first is $0. *\/ */
/* #define JITTER_SPARC_BRANCH_FAST_CONDITIONAL_Z_R(instruction_string, operand1,  \ */
/*                                                 target_index)                  \ */
/*   do                                                                           \ */
/*     {                                                                          \ */
/*       asm                                                                      \ */
/*       goto (JITTER_ASM_SPARC_CONDITIONAL_BRANCH_PATCH_IN(target_index)          \ */
/*             "1:\n\t"                                                           \ */
/*             instruction_string " $0, %[jitter_operand1], 1b"                   \ */
/*             : /\* outputs *\/                                                    \ */
/*             : [jitter_operand1] "r" (operand1) /\* inputs *\/                    \ */
/*             : /\* clobbers *\/                                                   \ */
/*             : jitter_jump_anywhere_label,                                      \ */
/*               JITTER_SPECIALIZED_INSTRUCTION_BEGIN_LABEL /\* goto labels *\/);   \ */
/*     }                                                                          \ */
/*   while (false) */

/* /\* Emit a conditional branch instruction with two register operands, where the */
/*    second is $0. *\/ */
/* #define JITTER_SPARC_BRANCH_FAST_CONDITIONAL_R_Z(instruction_string, operand0,  \ */
/*                                                 target_index)                  \ */
/*   do                                                                           \ */
/*     {                                                                          \ */
/*       asm                                                                      \ */
/*       goto (JITTER_ASM_SPARC_CONDITIONAL_BRANCH_PATCH_IN(target_index)          \ */
/*             "1:\n\t"                                                           \ */
/*             instruction_string " %[jitter_operand0], $0, 1b"                   \ */
/*             : /\* outputs *\/                                                    \ */
/*             : [jitter_operand0] "r" (operand0) /\* inputs *\/                    \ */
/*             : /\* clobbers *\/                                                   \ */
/*             : jitter_jump_anywhere_label,                                      \ */
/*               JITTER_SPECIALIZED_INSTRUCTION_BEGIN_LABEL /\* goto labels *\/);   \ */
/*     }                                                                          \ */
/*   while (false) */

/* /\* Emit a conditional branch instruction with one register operand. *\/ */
/* #define JITTER_SPARC_BRANCH_FAST_CONDITIONAL_UNARY(insn_name, opd,           \ */
/*                                                   target_index)             \ */
/*   do                                                                        \ */
/*     {                                                                       \ */
/*       JITTER_SPARC_BRANCH_FAST_CONDITIONAL_R(insn_name, opd, target_index);  \ */
/*     }                                                                       \ */
/*   while (false) */

/* /\* Emit a conditional branch instruction with two register operands, using */
/*    JITTER_SPARC_BRANCH_FAST_CONDITIONAL_Z_R or */
/*    JITTER_SPARC_BRANCH_FAST_CONDITIONAL_R_Z where possible, falling back to */
/*    JITTER_SPARC_BRANCH_FAST_CONDITIONAL_R_R when neither argument is known to be */
/*    zero. *\/ */
/* #define JITTER_SPARC_BRANCH_FAST_CONDITIONAL_BINARY(insn_name, type,            \ */
/*                                                    opd0, opd1, target_index)   \ */
/*   do                                                                           \ */
/*     {                                                                          \ */
/*       type _jitter_operand_0 = (type) (opd0);                                  \ */
/*       type _jitter_operand_1 = (type) (opd1);                                  \ */
/*       if (JITTER_IS_KNOWN_CONSTANT_ZERO(_jitter_operand_0))                    \ */
/*         JITTER_SPARC_BRANCH_FAST_CONDITIONAL_Z_R(insn_name, _jitter_operand_1,  \ */
/*                                                 target_index);                 \ */
/*       else if (JITTER_IS_KNOWN_CONSTANT_ZERO(_jitter_operand_1))               \ */
/*         JITTER_SPARC_BRANCH_FAST_CONDITIONAL_R_Z(insn_name, _jitter_operand_0,  \ */
/*                                                 target_index);                 \ */
/*       else                                                                     \ */
/*         JITTER_SPARC_BRANCH_FAST_CONDITIONAL_R_R(insn_name, _jitter_operand_0,  \ */
/*                                                 _jitter_operand_1,             \ */
/*                                                 target_index);                 \ */
/*     }                                                                          \ */
/*   while (false) */


/* /\* Unary conditions.  SPARC can express branches conditional on a comparisons */
/*    with zero in one instruction. *\/ */
/* #define JITTER_BRANCH_FAST_IF_ZERO(opd, target_index)                   \ */
/*   JITTER_SPARC_BRANCH_FAST_CONDITIONAL_BINARY("beq", jitter_int, opd, 0, \ */
/*                                              target_index) */
/* #define JITTER_BRANCH_FAST_IF_NONZERO(opd, target_index)                \ */
/*   JITTER_SPARC_BRANCH_FAST_CONDITIONAL_BINARY("bne", jitter_int, opd, 0, \ */
/*                                              target_index) */
/* #define JITTER_BRANCH_FAST_IF_POSITIVE(opd, target_index)               \ */
/*   JITTER_SPARC_BRANCH_FAST_CONDITIONAL_UNARY("bgtz", opd, target_index) */
/* #define JITTER_BRANCH_FAST_IF_NONPOSITIVE(opd, target_index)            \ */
/*   JITTER_SPARC_BRANCH_FAST_CONDITIONAL_UNARY("blez", opd, target_index) */
/* #define JITTER_BRANCH_FAST_IF_NEGATIVE(opd, target_index)               \ */
/*   JITTER_SPARC_BRANCH_FAST_CONDITIONAL_UNARY("bltz", opd, target_index) */
/* #define JITTER_BRANCH_FAST_IF_NONNEGATIVE(opd, target_index)            \ */
/*   JITTER_SPARC_BRANCH_FAST_CONDITIONAL_UNARY("bgez", opd, target_index) */

/* /\* Simple binary conditions.  Before the SPARC{32,64}r6 revision SPARC can express */
/*    branches conditional on an "equal" or "different" condition in one */
/*    instruction, but not branches conditional on other binary comparisons, unless */
/*    one of the operands is zero. *\/ */
/* #define JITTER_BRANCH_FAST_IF_EQUAL(opd0, opd1, target_index)  \ */
/*   JITTER_SPARC_BRANCH_FAST_CONDITIONAL_BINARY("beq", jitter_uint, opd0, opd1, \ */
/*                                              target_index) */
/* #define JITTER_BRANCH_FAST_IF_NOTEQUAL(opd0, opd1, target_index)  \ */
/*   JITTER_SPARC_BRANCH_FAST_CONDITIONAL_BINARY("bne", jitter_uint, opd0, opd1, \ */
/*                                              target_index) */

/* /\* GCC can easily generate an instruction such as slt , slti , stlu or sltiu to */
/*    set a temporary register to nonzero if a condition is true; we can use C to */
/*    set such a register, then use a fast conditional branch on the register */
/*    checking if it's nonzero. */

/*    SPARC has no sgt instruction, so simply computing a condition which is true */
/*    when we need to jump may require one more instruction to reverse it; if I */
/*    force it to jump on a true condition GCC generates a "xori $FINAL_CONDITION, */
/*    $TEMPORARY_CONDITION, 0x1" instruction, which works; but that is suboptimal. */
/*    The right solution is computing the opposite condition when a xori would be */
/*    needed, and then branching on false instead of true.  The compute_opposite */
/*    parameter determines if the condition is to be reversed. [FIXME: this is what */
/*    I did naïvely: I have to check whether the relevant parameter is known, */
/*    and whether the relevant one is the first or the second depends on the */
/*    condition.] */

/*    This is good but not always optimal: we can do better if we know that one */
/*    of the operands being compared is zero.  The macros below use this one */
/*    when it is not possible to prove that either operand is zero. *\/ */
/* /\* FIXME: there is a xori generated in example-vms/uninspired/examples/fibo.vm . */
/*    The instruction in question is "ble %r0, 0x1, $L16", which uses */
/*    JITTER_BRANCH_FAST_IF_NOTGREATER_SIGNED , itslef defined in terms of */
/*    JITTER_BRANCH_FAST_IF_NOTLESS_SIGNED . */
/*    Despite being correct the code is suboptimal: there must be a mishandled */
/*    case here. */
/*    I have to sit down when I'm less tired and think hard about the logic in this */
/*    code, but I suppose the problem is which one of the two arguments is known: */
/*    slti and sltiu only take an immediate as their *second* comparand.  If I'm */
/*    not taking this into account then my code is not optimal for every case.  The */
/*    assignment to _jitter_condition below is supposed to generate exactly one */
/*    instruction. either "slt $d, $c1, c2" or "slt $d, $c1, $c2"; I don't think we */
/*    can ignore whether c2 (which, by the way, could be opd0 rather than opd1) is */
/*    known.  Or maybe it isn't possible to do it with just one instruction in */
/*    every case; but I'd be surprised that the SPARC designers didn't include */
/*    sgti/sgtiu instructions then. *\/ */
/* #define JITTER_BRANCH_FAST_IF_BINARY_GENERAL(infix, type,                  \ */
/*                                              opd0, opd1, target_index,     \ */
/*                                              compute_opposite)             \ */
/*   do                                                                       \ */
/*     {                                                                      \ */
/*       jitter_uint _jitter_condition;                                       \ */
/*       if (compute_opposite)                                                \ */
/*         {                                                                  \ */
/*           _jitter_condition = ! ((type)(opd0) infix (type)(opd1));         \ */
/*           JITTER_BRANCH_FAST_IF_ZERO(_jitter_condition, target_index);     \ */
/*         }                                                                  \ */
/*       else                                                                 \ */
/*         {                                                                  \ */
/*           _jitter_condition = ((type)(opd0) infix (type)(opd1));           \ */
/*           JITTER_BRANCH_FAST_IF_NONZERO(_jitter_condition, target_index);  \ */
/*         }                                                                  \ */
/*     }                                                                      \ */
/*   while (false) */

/* /\* Check if either opd0 or opd1 is known to be zero; in that case use a */
/*    conditional fast branch on the other, which takes just one instruction. */
/*    Otherwise use JITTER_BRANCH_FAST_IF_BINARY_GENERAL , which takes two */
/*    [FIXME: or three?  See the comment about xori] instructions. *\/ */
/* #define JITTER_BRANCH_FAST_IF_BINARY(infix, type, opd0, opd1, target_index, \ */
/*                                      condition_on_opd1_when_opd0_is_zero,   \ */
/*                                      condition_on_opd0_when_opd1_is_zero,   \ */
/*                                      compute_opposite)                      \ */
/*   do                                                                        \ */
/*     {                                                                       \ */
/*       type _jitter_operand_0 = (type) (opd0);                               \ */
/*       type _jitter_operand_1 = (type) (opd1);                               \ */
/*       if (JITTER_IS_KNOWN_CONSTANT_ZERO(_jitter_operand_0))                 \ */
/*         JITTER_CONCATENATE_TWO(JITTER_BRANCH_FAST,                          \ */
/*                                condition_on_opd1_when_opd0_is_zero)(        \ */
/*             _jitter_operand_1, target_index);                               \ */
/*       else if (JITTER_IS_KNOWN_CONSTANT_ZERO(_jitter_operand_1))            \ */
/*         JITTER_CONCATENATE_TWO(JITTER_BRANCH_FAST,                          \ */
/*                                condition_on_opd0_when_opd1_is_zero)(        \ */
/*             _jitter_operand_0, target_index);                               \ */
/*       else                                                                  \ */
/*         JITTER_BRANCH_FAST_IF_BINARY_GENERAL(infix, type,                   \ */
/*                                              _jitter_operand_0,             \ */
/*                                              _jitter_operand_1,             \ */
/*                                              target_index,                  \ */
/*                                              compute_opposite);             \ */
/*     }                                                                       \ */
/*   while (false) */

/* /\* Fast branches conditional on a binary comparison. *\/ */
/* #define JITTER_BRANCH_FAST_IF_LESS_SIGNED(opd0, opd1, target_index)      \ */
/*   JITTER_BRANCH_FAST_IF_BINARY(<, jitter_int, opd0, opd1, target_index,  \ */
/*                                _IF_POSITIVE, /\* 0 < opd1 *\/              \ */
/*                                _IF_NEGATIVE  /\* opd0 < 0 *\/,             \ */
/*                                false) */
/* #define JITTER_BRANCH_FAST_IF_LESS_UNSIGNED(opd0, opd1, target_index)     \ */
/*   JITTER_BRANCH_FAST_IF_BINARY(<, jitter_uint, opd0, opd1, target_index,  \ */
/*                                _IF_NONZERO,    /\* 0 <u opd1 *\/            \ */
/*                                _IF_NEVER_UNARY /\* opd0 <u 0 *\/,           \ */
/*                                false) */
/* #define JITTER_BRANCH_FAST_IF_NOTLESS_SIGNED(opd0, opd1, target_index)    \ */
/*   JITTER_BRANCH_FAST_IF_BINARY(>=, jitter_int, opd0, opd1, target_index,  \ */
/*                                _IF_NONPOSITIVE, /\* 0 >= opd1 *\/           \ */
/*                                _IF_NONNEGATIVE  /\* opd0 >= 0 *\/,          \ */
/*                                true) */
/* #define JITTER_BRANCH_FAST_IF_NOTLESS_UNSIGNED(opd0, opd1, target_index)   \ */
/*   JITTER_BRANCH_FAST_IF_BINARY(>=, jitter_uint, opd0, opd1, target_index,  \ */
/*                                _IF_ZERO,        /\* 0 >=u opd1 *\/           \ */
/*                                _IF_ALWAYS_UNARY /\* opd0 >=u 0 *\/,          \ */
/*                                true) */
/* #define JITTER_BRANCH_FAST_IF_GREATER_SIGNED(opd0, opd1, target_index)  \ */
/*   JITTER_BRANCH_FAST_IF_LESS_SIGNED(opd1, opd0, target_index) */
/* #define JITTER_BRANCH_FAST_IF_GREATER_UNSIGNED(opd0, opd1, target_index)  \ */
/*   JITTER_BRANCH_FAST_IF_LESS_UNSIGNED(opd1, opd0, target_index) */
/* #define JITTER_BRANCH_FAST_IF_NOTGREATER_SIGNED(opd0, opd1, target_index)  \ */
/*   JITTER_BRANCH_FAST_IF_NOTLESS_SIGNED(opd1, opd0, target_index) */
/* #define JITTER_BRANCH_FAST_IF_NOTGREATER_UNSIGNED(opd0, opd1, target_index)  \ */
/*   JITTER_BRANCH_FAST_IF_NOTLESS_UNSIGNED(opd1, opd0, target_index) */

/* #endif // #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING) */




/* /\* VM procedures. */
/*  * ************************************************************************** *\/ */

/* /\* Procedures are supported on SPARC ; but we conditionalize the definition */
/*    on JITTER_MACHINE_SUPPORTS_PATCH_IN , for testing convenience. *\/ */
/* #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN */
/* # define JITTER_MACHINE_SUPPORTS_PROCEDURE    1 */
/* #endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN */

/* #if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN)   \ */
/*     && defined(JITTER_DISPATCH_NO_THREADING)       \ */
/*     && defined(JITTER_MACHINE_SUPPORTS_PROCEDURE) */

// FIXME: this has now changed in the other architectures.  Do not simply uncomment.
/* /\* The return address is in $31 (and I never need to worry about saving its */
/*    initial value before starting, since the executor has already called C */
/*    functions at initialization, which means that before the executor */
/*    returns it will have to restore its original $31 from the stack anyway); */
/*    just copy $31 to link_lvalue . *\/ */
/* #define _JITTER_PROCEDURE_PROLOG(link_lvalue)                                   \ */
/*   do                                                                            \ */
/*     {                                                                           \ */
/*       register const void * jitter_the_return_address asm ("$31");              \ */
/*       /\* Let GCC believe we are initializing $31 in the inline asm code; *\/     \ */
/*       /\* in reality it's already set. *\/                                        \ */
/*       asm ("# Pretend to set %[return_address], even if it's already set."      \ */
/*            : [return_address] "=r" (jitter_the_return_address) /\* outputs *\/);  \ */
/*       link_lvalue = (const void *) (jitter_the_return_address);                 \ */
/*     }                                                                           \ */
/*   while (false) */

/* /\* The recommended way of returning from a procedure on SPARC is by jumping via */
/*    register to $31 ; the SPARC32 specification, while remaining abstract, at */
/*    least strongly implies that jumping to $31 may give the correct (pop) hint to */
/*    the CPU branch target predictor, differently from other registers; GCC */
/*    generates code consistent with this hypothesis.  So it's simple: place the */
/*    target address into $31 , and jump to it. *\/ */
/* #define JITTER_RETURN(link_rvalue)                                             \ */
/*   do                                                                           \ */
/*     {                                                                          \ */
/*       /\* By using a local register variable we can avoid a register copy *\/    \ */
/*       /\* to set $31 . *\/                                                       \ */
/*       register const void * const jitter_the_return_address asm ("$31")        \ */
/*         = (const void* const) (link_rvalue);                                   \ */
/*       /\* Gas seems to do a good job of using the delay slot, which is not *\/   \ */
/*       /\* always a nop.  This is why I don't use .set nomacro, noreorder . *\/   \ */
/*       asm goto ("jr %[return_addr]"                                            \ */
/*                 : /\* outputs. *\/                                               \ */
/*                 : [return_addr] "r" (jitter_the_return_address) /\* inputs. *\/  \ */
/*                 : /\* clobbers. *\/                                              \ */
/*                 : jitter_jump_anywhere_label /\* gotolabels. *\/);               \ */
/*       /\* The rest of the VM instruction is unreachable. *\/                     \ */
/*       __builtin_unreachable ();                                                \ */
/*     }                                                                          \ */
/*   while (false) */

/* /\* Easy: perform a jump-and-link via ragister, and the return address will be */
/*    in $31 . *\/ */
/* #define _JITTER_BRANCH_AND_LINK(callee_rvalue)                                \ */
/*   do                                                                          \ */
/*     {                                                                         \ */
/*       const void * const jitter_destination =                                 \ */
/*         (const void * const) (callee_rvalue);                                 \ */
/*       /\* Gas seems to do a good job of using the delay slot, which is not *\/  \ */
/*       /\* always a nop.  This is why I don't use .set nomacro, noreorder . *\/  \ */
/*       asm goto ("jalr %[destination]"                                         \ */
/*                 : /\* outputs. *\/                                              \ */
/*                 : [destination] "r" (jitter_destination) /\* inputs. *\/        \ */
/*                 : "$31" /\* clobbers. *\/                                       \ */
/*                 : jitter_jump_anywhere_label /\* gotolabels. *\/);              \ */
/*       /\* Skip the rest of the specialized instruction, for compatibility *\/   \ */
/*       /\* with more limited dispatches. *\/                                     \ */
/*       JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END;                             \ */
/*     }                                                                         \ */
/*   while (false) */

/* #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN */
/* /\* Perform a pseudo-direct jump-and-link, and the return address will be in $31 */
/*    .  Of course we need a patch-in for the pseudo-direct jump-and-link, since */
/*    the destination address is encoded in the jumping instruction. *\/ */
/* #define _JITTER_BRANCH_AND_LINK_FAST(target_index)                             \ */
/*   do                                                                           \ */
/*     {                                                                          \ */
/*       asm goto (JITTER_ASM_PATCH_IN_PLACEHOLDER(                               \ */
/*                    JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK /\*size_in_bytes*\/, \ */
/*                    JITTER_PATCH_IN_CASE_FAST_BRANCH_BRANCH_AND_LINK /\*case*\/,  \ */
/*                    target_index,                                               \ */
/*                    0, 0, 0 /\* not used for this case *\/)                       \ */
/*                 : /\* outputs. *\/                                               \ */
/*                 : /\* inputs. *\/                                                \ */
/*                 : /\* clobbers. *\/                                              \ */
/*                 :   /\* not actually used as a jump target *\/                   \ */
/*                     JITTER_SPECIALIZED_INSTRUCTION_BEGIN_LABEL                 \ */
/*                   , jitter_jump_anywhere_label /\* gotolabels. *\/);             \ */
/*       /\* Skip the rest of the specialized instruction, for compatibility *\/    \ */
/*       /\* with more limited dispatches. *\/                                      \ */
/*       JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END;                              \ */
/*     }                                                                          \ */
/*   while (false) */
/* #endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN */

/* #endif // #if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) ... */



/* C-only page, expanding to nothing if this header is included by assembly.
 * ************************************************************************** */

#ifndef __ASSEMBLER__

#include <jitter/jitter.h>

/* Notice that the order matters, and these shouldn't be rearranged without also
   changing the order of definitions in machine.S .  We also rely on the first
   case having value 0, since we use enum jitter_snippet_to_patch values as
   array indices. */
enum jitter_snippet_to_patch
  {
#define ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(idx)                   \
  JITTER_CONCATENATE_TWO(jitter_snippet_load_or_to_reg_, idx),        \
  JITTER_CONCATENATE_TWO(jitter_snippet_load_sethi_or_to_reg_, idx),  \
  JITTER_CONCATENATE_TWO(jitter_snippet_load_sethi_to_reg_, idx),     \
  JITTER_CONCATENATE_TWO(jitter_snippet_load_64bit_to_reg_, idx),

    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(0)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(1)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(2)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(3)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(4)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(5)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(6)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(7)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(8)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(9)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(10)
    ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER(11)
    /* FIXME: snippets for loading to residuals memory. */

#undef ROUTINES_TO_LOAD_RESIDUALS_TO_REGISTER

    jitter_snippet_branch_unconditional_18bits_offset,

    /* FIXME: this is broken for some reason I have to investigate, and I'm
       using the previous snippet despite its narrower range. */
    jitter_snippet_branch_unconditional_21bits_offset,

    /* The number of snippets. */
    jitter_snippet_no
  };

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
