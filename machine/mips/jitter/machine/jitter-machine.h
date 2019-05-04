/* VM library: MIPS definitions, to be included from both C and assembly.

   Copyright (C) 2017, 2018, 2019 Luca Saiu
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

/* How to introduce comments in assembly on MIPS. */
#define JITTER_ASM_COMMENT_PREFIX "# "




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "addi $0, $0, " integer_literal_as_string

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
   In the case of MIPS we can afford to keep .reorder on as per the default
   setting, and *not* to specify a delay slot.  Gas will be able to fill the
   delay slot itself with something useful, when we are lucky. */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE          "jr %[_jitter_the_target]"
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  "r"




/* Reserved registers.
 * ************************************************************************** */

/* MIPS configurations should have at least the following registers available
   as callee-save:
     $16 $17 $18 $19 $20 $21 $22 $23
   This is already very good, but I suspect a few others might work as well.
   I can almost certainly use $1/$at as scratch within an uninterrupted
   sequence of instructions written by me; I'm not so sure $30/$fp is actually
   used as a frame pointer by GCC, or if I should prevent it; and I've never
   seriously investigated $26-$27/$k0-k1 , but as far as I understand they
   are clobbered by the operating system at unpredictable times. */

/* Register pointing to The Array base. */
#define JITTER_BASE_REGISTER          $16

/* How many registers we can use to hold residual arguments. */
#define JITTER_RESIDUAL_REGISTER_NO   6
//#define JITTER_RESIDUAL_REGISTER_NO   0 // this is good for testing memory literals

/* Registers holding residual arguments, with 0-based suffixes.  These have to
   be as many as JITTER_RESIDUAL_REGISTER_NO . */
#define JITTER_RESIDUAL_REGISTER_0    $17
#define JITTER_RESIDUAL_REGISTER_1    $18
#define JITTER_RESIDUAL_REGISTER_2    $19
#define JITTER_RESIDUAL_REGISTER_3    $20
#define JITTER_RESIDUAL_REGISTER_4    $21
#define JITTER_RESIDUAL_REGISTER_5    $22

/* The scratch register.  The same remark above applies. */
#define JITTER_SCRATCH_REGISTER       $23




/* MIPS CPU-specific features.
 * ************************************************************************** */

/* At least some revisions of the Loongson2F processor have nop bugs which can
   be circumvented by generating
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

/* FIXME: This section will need a lot more conditionalization in the future, if
   I support the new MIPS32r6 and MIPS64r6, which define several nice but
   incompatible changes.  I will need to set up r6 cross-GCCs.  I have no very
   recent hardware, but QEmu seems to support MIPS well.

   These macros, defined by GCC, should be useful:
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
  memory to catch bugs. */
#define JITTER_ASM_PATCH_IN_FILL_BYTE    "0xff"

/* For each patch-in case define its size in bytes, corresponding to the total
   size of the instructions to be patched in in bytes, including possible
   padding nops. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL                        8

/* MIPS uses a single patch-in case for every conditional branch, and its size
   is zero. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK                      8
/* /\* These should be extended in the future with floating-point conditionals. *\/ */




/* VM conditional branches.
 * ************************************************************************** */

/* Ignore this page if patch-ins have been disabled (for debugging) or the
   dispatching model does not support them. */
#if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)

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
   twice. */
#define _JITTER_LOW_LEVEL_TIMES_BRANCH_FAST_IF_OVERFLOW_(res, opd0, opd1, tgt)  \
  /* This uses the same very fragile trick as on x86_64, in which an inline     \
     asm operand is actually an output, but must be declared as input.  See     \
     the comment about a macro named like this in                               \
     machine/x86_64/jitter/machine/jitter-machine.h . */                        \
  /* FIXME: generalize this machinery.  It will be useful for other ports as    \
     well. */                                                                   \
  /* Notice that this solution does not work on mips64, nor on                  \
     mips32r6/mips64r6 where HI and LO have been removed.  An essentially       \
     identical idea using dmult, or mul/muh, or dmul/dmulh will work, but this  \
     code will need to be either repeated or conditionalized in an intrusive    \
     way. */                                                                    \
  register jitter_int _jitter_product asm ("$4");                               \
  asm volatile ("" : "=r" (_jitter_product));                                   \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                        \
            "mult %[jitter_operand0], %[jitter_operand1]\n\t"                   \
            "mflo %[jitter_product]\n\t"                                        \
            "mfhi $10\n\t"                                                      \
            "sra $2, %[jitter_product], 31\n\t"                                 \
            JITTER_ASM_PATCH_IN_PLACEHOLDER /* See patch-in comment above */    \
               (0 /* size in bytes */,                                          \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /* case */,    \
                (tgt), 0, 0, 0 /* not used for this case */)                    \
            "\n.set noreorder\n\t"                                              \
            "\n.set nomacro\n\t"                                                \
            "\n.set noat\n\t"                                                   \
            "1:\n\t"                                                            \
            "bne $2, $10, 1b\n\t"                                               \
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
            : "hi", "lo", "$2", "$10" /* clobbers */                            \
            : jitter_dispatch_label /* goto labels */);                         \
  (res) = _jitter_product

#endif // #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)




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
   just copy $31 to link_lvalue . */
#define _JITTER_PROCEDURE_PROLOG(link_lvalue)                                   \
  do                                                                            \
    {                                                                           \
      register const void * jitter_the_return_address asm ("$31");              \
      /* Let GCC believe we are initializing $31 in the inline asm code; */     \
      /* in reality it's already set. */                                        \
      asm ("# Pretend to set %[return_address], even if it's already set."      \
           : [return_address] "=r" (jitter_the_return_address) /* outputs */);  \
      link_lvalue = (const void *) (jitter_the_return_address);                 \
    }                                                                           \
  while (false)

/* The recommended way of returning from a procedure on MIPS is by jumping via
   register to $31 ; the MIPS32 specification, while remaining abstract, at
   least strongly implies that jumping to $31 may give the correct (pop) hint to
   the CPU branch target predictor, differently from other registers; GCC
   generates code consistent with this hypothesis.  So it's simple: place the
   target address into $31 , and jump to it. */
#define JITTER_RETURN(link_rvalue)                                             \
  do                                                                           \
    {                                                                          \
      /* By using a local register variable we can avoid a register copy */    \
      /* to set $31 . */                                                       \
      register const void * const jitter_the_return_address asm ("$31")        \
        = (const void* const) (link_rvalue);                                   \
      /* Gas seems to do a good job of using the delay slot, which is not */   \
      /* always a nop.  This is why I don't use .set nomacro, noreorder . */   \
      asm goto ("jr %[return_addr]\n\t"                                        \
                /* Putting the defect descriptor before jr prevents GCC from   \
                   using the delay slot intelligently.  It's harmless to have  \
                   it here after the return instruction. */                    \
                JITTER_ASM_DEFECT_DESCRIPTOR                                   \
                : /* outputs. */                                               \
                : [return_addr] "r" (jitter_the_return_address) /* inputs. */  \
                : /* clobbers. */                                              \
                : jitter_dispatch_label /* gotolabels. */);                    \
      /* The rest of the VM instruction is unreachable. */                     \
      __builtin_unreachable ();                                                \
    }                                                                          \
  while (false)

/* Easy: perform a jump-and-link via ragister, and the return address will be
   in $31 . */
#define JITTER_BRANCH_AND_LINK_INTERNAL(callee_rvalue)                        \
  do                                                                          \
    {                                                                         \
      const void * const jitter_destination =                                 \
        (const void * const) (callee_rvalue);                                 \
      /* Gas seems to do a good job of using the delay slot, which is not */  \
      /* always a nop.  This is why I am not using                            \
            .set nomacro, noreorder                                           \
         . */                                                                 \
      asm goto ("jalr %[destination]"                                         \
                /* See the comment in JITTER_RETURN. */                       \
                JITTER_ASM_DEFECT_DESCRIPTOR                                  \
                : /* outputs. */                                              \
                : [destination] "r" (jitter_destination) /* inputs. */        \
                : "$31" /* clobbers. */                                       \
                : jitter_dispatch_label /* gotolabels. */);                   \
      /* Skip the rest of the specialized instruction, for compatibility */   \
      /* with more limited dispatches. */                                     \
      JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END;                             \
    }                                                                         \
  while (false)

/* Perform an ordinary jump thru register, and load the given return address
   in $31, exploiting the delay slot. */
#define JITTER_BRANCH_AND_LINK_WITH(_jitter_callee_rvalue, _jitter_new_link)  \
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
                "\n.set noreorder\n\t"                                        \
                "\n.set nomacro\n\t"                                          \
                "jr %[jitter_callee_rvalue]\n\t"                              \
                "or $31, %[jitter_new_link], $0\n\t"                          \
                "\n.set macro\n\t"                                            \
                "\n.set reorder\n\t"                                          \
                : /* outputs. */                                              \
                : [jitter_callee_rvalue] "r" (jitter_callee_rvalue),          \
                  [jitter_new_link] "r" (jitter_new_link) /* inputs. */       \
                : "$31" /* clobbers. */                                       \
                : jitter_dispatch_label /* gotolabels. */);                   \
      /* The rest of the VM instruction is unreachable: this is an            \
         unconditional jump. */                                               \
      __builtin_unreachable ();                                               \
    }                                                                         \
  while (false)

#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
/* Perform a pseudo-direct jump-and-link, and the return address will be in $31
   .  Of course we need a patch-in for the pseudo-direct jump-and-link, since
   the destination address is encoded in the jumping instruction. */
#define _JITTER_BRANCH_FAST_AND_LINK_INTERNAL(target_index)                    \
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
      /* The rest of this specialized instruction is unreachable.  This        \
         implementation is not based on hardware call and return, so there     \
         is no need to generate a hardware jump either. */                     \
      __builtin_unreachable ();                                                \
    }                                                                          \
  while (false)
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#endif // #if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) ...



/* C-only page, expanding to nothing if this header is included by assembly.
 * ************************************************************************** */

#ifndef __ASSEMBLER__

/* Notice that the order matters, and these shouldn't be rearranged without also
   changing the order of definitions in machine.S .  We also rely on the first
   case having value 0, since we use enum jitter_routine_to_patch values as
   array indices. */
enum jitter_routine_to_patch
  {
    jitter_routine_load_zero_extended_16bit_to_register_0,
    jitter_routine_load_zero_extended_16bit_to_register_1,
    jitter_routine_load_zero_extended_16bit_to_register_2,
    jitter_routine_load_zero_extended_16bit_to_register_3,
    jitter_routine_load_zero_extended_16bit_to_register_4,
    jitter_routine_load_zero_extended_16bit_to_register_5,
    jitter_routine_load_sign_extended_16bit_to_register_0,
    jitter_routine_load_sign_extended_16bit_to_register_1,
    jitter_routine_load_sign_extended_16bit_to_register_2,
    jitter_routine_load_sign_extended_16bit_to_register_3,
    jitter_routine_load_sign_extended_16bit_to_register_4,
    jitter_routine_load_sign_extended_16bit_to_register_5,
    jitter_routine_load_32bit_to_register_0,
    jitter_routine_load_32bit_to_register_1,
    jitter_routine_load_32bit_to_register_2,
    jitter_routine_load_32bit_to_register_3,
    jitter_routine_load_32bit_to_register_4,
    jitter_routine_load_32bit_to_register_5,
    /* FIXME: the next three are not implemented yet. */
    jitter_routine_load_zero_extended_16bit_to_memory,
    jitter_routine_load_sign_extended_16bit_to_memory,
    jitter_routine_load_32bit_to_memory,

    jitter_routine_jump_unconditional_28bit_pseudo_direct,
    jitter_routine_jump_and_link_28bit_pseudo_direct,

    /* The same routine works for any conditional branch. */
    jitter_routine_branch_conditional_18bit_offset,

    /* The number of routines. */
    jitter_routine_no
  };

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
