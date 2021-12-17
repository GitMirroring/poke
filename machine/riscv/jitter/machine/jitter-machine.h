/* VM library: RISC-V definitions, to be included from both C and assembly.

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


/* Bitness.
 * ************************************************************************** */

/* This port covers the RISC-V architecture, with either word length.  The
   RISC-V specification defines "XLEN" as the general register size in bits,
   to be either 32 or 64.
   Define JITTER_XLEN as a literal constant, to be usable from assembly as well
   in any context, without the bother or parentheses which might be parsed as
   part of a memory operand. */
#include <jitter/jitter-config.h>
#if JITTER_SIZEOF_VOID_P == 8
# define JITTER_XLEN  64
#elif JITTER_SIZEOF_VOID_P == 4
# define JITTER_XLEN  32
#else
# error "JITTER_SIZEOF_VOID_P is not defined or has a bizarre value: this"
# error "should not happen."
#endif




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "addi x0, x0, " integer_literal_as_string

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                     \
  /* Return from M-mode trap.  This will cause an exception in user mode, of  \
     a kind not usually seen. */                                              \
  "mret"




/* Computed goto.
 * ************************************************************************** */

/* /\* Computed goto implemented with inline asm.  See the comments about */
/*    JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and */
/*    JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and */
/*    JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . *\/ */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE                                  \
  /* The explicit "jalr x0, 0(%[_jitter_the_target])" is functionally      \
     equivalent, but this pseudoinstruction can be compressed into a c.jr  \
     instruction if the configuration supports the C extension.  Gas does  \
     not seem to automatically rewrite jalr x0, 0(xR) into c.jr xR. */     \
  "jr %[_jitter_the_target]"
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  \
  "r"




/* Reserved registers.
 * ************************************************************************** */

/* RISC-V configurations should have at least the following registers available
   as callee-save:
     x9 x18 x19 x20 x21 x22 x23 x24 x25 x26 x27
   The frame pointer register may also be usable, since I do not use it when
   compiling VMs.

   Reserved registers, or registers which are reserved in practice, on RISC-V:
   - x0 hardwired to zero;
   - x1 holds the return address;
   - x2 stack pointer;
   - x5 alternate link register.

   By convention:
   - x8 saved register/frame pointer;
   - x9 saved register;
   - x10-x11 function arguments/function results;
   - x12-x17 function arguments;
   - x18-x27 saved registers;
   - x28-x31 temporaries. */

/* Register pointing to The Array base. */
#define JITTER_BASE_REGISTER          x9

/* How many registers we can use to hold residual arguments. */
#define JITTER_RESIDUAL_REGISTER_NO   6
//#define JITTER_RESIDUAL_REGISTER_NO   0 // this is good for testing memory literals

/* Registers holding residual arguments, with 0-based suffixes.  These have to
   be as many as JITTER_RESIDUAL_REGISTER_NO . */
#define JITTER_RESIDUAL_REGISTER_0    x19
#define JITTER_RESIDUAL_REGISTER_1    x20
#define JITTER_RESIDUAL_REGISTER_2    x21
#define JITTER_RESIDUAL_REGISTER_3    x22
#define JITTER_RESIDUAL_REGISTER_4    x23
#define JITTER_RESIDUAL_REGISTER_5    x24

/* The scratch register.  The same remark above applies. */
#if JITTER_XLEN == 64
  /* The scratch register is currently only used in the 64-bit case, for
     materialising 64-bits constants in the least efficient case. */
# define JITTER_SCRATCH_REGISTER      x18
#endif // #if JITTER_XLEN == 64

/* These registers are not reserved, but only used internally as assembly
   operands for register local variables or as clobbered assembly temporaries
   from a few macros here.  They do not even need to be caller-save; however 
   they must not conflict with reserved registers, other than the scratch
   register.
   These identifiers are specific to the RISC-V port and not part of the Jitter
   API. */
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_RESULT  x28
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1  x29
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2  x30

/* Same as above, as literal strings. */
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_RESULT_STRING  \
  JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_TEMPORARY_RESULT)
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING  \
  JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1)
#define JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING  \
  JITTER_STRINGIFY (JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2)




/* Patch-ins.
 * ************************************************************************** */

/* Patch-ins are supported on the the RISC-V architecture. */
#define JITTER_MACHINE_SUPPORTS_PATCH_IN  1

/* On RISC-V an instruction made of 0x00 bytes is invalid. */
#define JITTER_ASM_PATCH_IN_FILL_BYTE    "0x00"

/* For each patch-in case define its size in bytes, corresponding to the total
   size of the instructions to be patched in in bytes, including possible
   padding nops.  RISC-V has simple 4-byte instructions -- For code subject to
   being patched in we explicitly disable the C extension. */
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_UNCONDITIONAL    4
#define JITTER_PATCH_IN_SIZE_FAST_BRANCH_BRANCH_AND_LINK  4




/* VM low-level conditional branches.
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
               instruction, where the only field to patch is the (split)    \
               branch offset.  All the rest, including register numberss,   \
               is correctly generated by the inline asm and needs no        \
               patching. */                                                 \
            JITTER_ASM_PATCH_IN_PLACEHOLDER                                 \
               (0 /* size in bytes */,                                      \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /*case*/,  \
                (tgt),                                                      \
                0, 0, 0 /* not used for this case */)                       \
            "\n\t"                                                          \
            ".option push\n\t"                                              \
            ".option norvc\n"                                               \
            "1:\n\t"                                                        \
            insn " %[jitter_operand0]" opd1_tplt ", 1b\n\t"                 \
            ".option pop\n\t"                                               \
            : /* outputs */                                                 \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                        \
              [jitter_operand0] "r" (opd0),                                 \
              [jitter_operand1] "rJ" (opd1),                                \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */            \
            : /* clobbers */                                                \
            : jitter_fake_target /* goto labels */)

/* Low-level fast branches on sign. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_ZERO_(opd0, tgt)                       \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("beq", (opd0), ", x0", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONZERO_(opd0, tgt)                    \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("bne", (opd0), ", x0", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NEGATIVE_(opd0, tgt)                   \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("blt", (opd0), ", x0", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONNEGATIVE_(opd0, tgt)                \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("bge", (opd0), ", x0", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_POSITIVE_(opd0, tgt)                   \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("bgt", (opd0), ", x0", 0, (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NONPOSITIVE_(opd0, tgt)                \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_ ("ble", (opd0), ", x0", 0, (tgt))

/* Low-level fast branches on equal or not-equal. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_EQUAL_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                      \
     ("beq", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTEQUAL_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                         \
     ("bne", (opd0), ", %[jitter_operand1]", (opd1), (tgt))

/* Low-level fast branches on magnitude comparisons. */
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                         \
     ("blt", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                               \
     ("bgt", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                               \
     ("bge", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_SIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                  \
     ("ble", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_LESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                              \
     ("bltu", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_GREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                 \
     ("bgtu", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTLESS_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                 \
     ("bgeu", (opd0), ", %[jitter_operand1]", (opd1), (tgt))
#define _JITTER_LOW_LEVEL_BRANCH_FAST_IF_NOTGREATER_UNSIGNED_(opd0, opd1, tgt)  \
  _JITTER_LOW_LEVEL_BRANCH_FAST_CONDITIONAL_                                    \
     ("bleu", (opd0), ", %[jitter_operand1]", (opd1), (tgt))

/* Jitter's default solution for overflow-checking on sum and subtraction works
   well on RISC-V.  The solution for multiplication, right below, is essentially
   identical to the one in MIPS r6. */

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
            "mul %[jitter_product], %[jitter_operand0], %[jitter_operand1]\n\t" \
            "mulh " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING        \
               ", %[jitter_operand0], %[jitter_operand1]\n\t"                   \
            "srai " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING        \
               ", %[jitter_product], "                                          \
               "(" JITTER_STRINGIFY (JITTER_XLEN) "- 1)\n\t"                    \
            JITTER_ASM_PATCH_IN_PLACEHOLDER /* See patch-in comment above */    \
               (0 /* size in bytes */,                                          \
                JITTER_PATCH_IN_CASE_FAST_BRANCH_CONDITIONAL_ANY /* case */,    \
                (tgt), 0, 0, 0 /* not used for this case */)                    \
            "1: bne " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING      \
               ", " JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING        \
               ", 1b\n\t"                                                       \
            : /* outputs */                                                     \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                            \
              [jitter_product] "r" (_jitter_product), /* actually an output! */ \
              [jitter_operand0] "r" (opd0),                                     \
              [jitter_operand1] "r" (opd1),                                     \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */                \
            : JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER1_STRING,             \
              JITTER_NON_RESERVED_REGISTER_TEMPORARY_OTHER2_STRING              \
              /* clobbers */                                                    \
            : jitter_fake_target /* goto labels */);                         \
  /* Make sure to get the current value in the register as the result, and not  \
     a previous copy.  In order to force this, pretend to update the resiter    \
     here, only if the branch was not taken.  Inline asm will use the same      \
     register assignment for _jitter_product. */                                \
  asm ("" : "+r" (_jitter_product));                                            \
  (res) = _jitter_product


#endif // #if defined(JITTER_MACHINE_SUPPORTS_PATCH_IN) && defined(JITTER_DISPATCH_NO_THREADING)




/* VM procedures.
 * ************************************************************************** */

/* Procedures are supported on RISC-V ; however we conditionalise the definition
   on JITTER_MACHINE_SUPPORTS_PATCH_IN , for testing convenience. */
#ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
# define JITTER_MACHINE_SUPPORTS_PROCEDURE    1
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN

#if    defined(JITTER_MACHINE_SUPPORTS_PATCH_IN)   \
    && defined(JITTER_DISPATCH_NO_THREADING)       \
    && defined(JITTER_MACHINE_SUPPORTS_PROCEDURE)

/* The return address is in x1 (and I never need to worry about saving its
   initial value before starting, since the executor has already called C
   functions at initialization, which means that before the executor returns it
   will have to restore its original x1 from the stack anyway); just copy x1 to
   the union . */
#define _JITTER_PROCEDURE_PROLOG(link_union)                                    \
  do                                                                            \
    {                                                                           \
      register void * jitter_the_return_address asm ("x1");                     \
      /* Let GCC believe we are initializing x1 in the inline asm code; in      \
         reality it's already set. */                                           \
      asm ("# Pretend to set %[return_address], even if it's already set."      \
           : [return_address] "=r" (jitter_the_return_address) /* outputs */);  \
      (link_union).pointer = jitter_the_return_address;                         \
    }                                                                           \
  while (false)

/* The recommended way of returning from a procedure on RISC-V is by jumping via
   register to x1 ; this has the correct effect on the branch target prediction,
   which takes the used register as a pop hint to, differently from when other
   registers (other than x5) are used.  So it is simple: place the target
   address into x1 , and jump to it. */
#define _JITTER_RETURN(link_rvalue)                                            \
  do                                                                           \
    {                                                                          \
      /* By using a local register variable we can avoid a register copy */    \
      /* to set x1 . */                                                        \
      register const void * const jitter_the_return_address asm ("x1")         \
        = (const void* const) (link_rvalue);                                   \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                   \
                "jalr x0, 0(%[return_addr])\n\t"                               \
                : /* outputs. */                                               \
                : [return_addr] "r" (jitter_the_return_address) /* inputs. */  \
                : /* clobbers. */                                              \
                : jitter_fake_target /* gotolabels. */);                    \
      /* The rest of the VM instruction is unreachable. */                     \
      __builtin_unreachable ();                                                \
    }                                                                          \
  while (false)

/* Easy: perform a jalr with the return address set in x1 , which on RISC-V
   implies the correct branch prediction hint (push). */
#define _JITTER_BRANCH_AND_LINK_NATIVE(callee_rvalue)                         \
  do                                                                          \
    {                                                                         \
      const void * const jitter_destination =                                 \
        (const void * const) (callee_rvalue);                                 \
      asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                                  \
                "jalr x1, 0(%[destination])\n\t"                              \
                : /* outputs. */                                              \
                : [destination] "r" (jitter_destination) /* inputs. */        \
                : "x1" /* clobbers. */                                        \
                : jitter_fake_target /* gotolabels. */);                   \
      /* It would be incorrect to have __builtin_unreachable or               \
         JITTER_JUMP_TO_SPECIALIZED_INSTRUCTION_END here: see the comment in  \
         the x86_64 version. */                                               \
    }                                                                         \
  while (false)

/* Perform an ordinary jalr not saving the return register, after manually
   loading the given "return address" into x1. */
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
                "addi x1, %[jitter_new_link], 0\n\t"                          \
                "jalr x0, 0(%[jitter_callee_rvalue])\n\t"                     \
                : /* outputs. */                                              \
                : [jitter_callee_rvalue] "r" (jitter_callee_rvalue),          \
                  [jitter_new_link] "r" (jitter_new_link) /* inputs. */       \
                : "x1" /* clobbers. */                                        \
                : jitter_fake_target /* gotolabels. */);                   \
      /* The rest of the VM instruction is unreachable: this tail call is an  \
         unconditional jump. */                                               \
      __builtin_unreachable ();                                               \
    }                                                                         \
  while (false)

/* Perform a jal with return address in x1 .  Of course we need a patch-in,
   since the destination address is encoded in the jumping instruction. */
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
    jitter_snippet_load_sign_extended_12bit_to_register_0,
    jitter_snippet_load_sign_extended_12bit_to_register_1,
    jitter_snippet_load_sign_extended_12bit_to_register_2,
    jitter_snippet_load_sign_extended_12bit_to_register_3,
    jitter_snippet_load_sign_extended_12bit_to_register_4,
    jitter_snippet_load_sign_extended_12bit_to_register_5,

    jitter_snippet_load_sign_extended_32bit_to_register_0,
    jitter_snippet_load_sign_extended_32bit_to_register_1,
    jitter_snippet_load_sign_extended_32bit_to_register_2,
    jitter_snippet_load_sign_extended_32bit_to_register_3,
    jitter_snippet_load_sign_extended_32bit_to_register_4,
    jitter_snippet_load_sign_extended_32bit_to_register_5,

    jitter_snippet_load_lui_only_to_register_0,
    jitter_snippet_load_lui_only_to_register_1,
    jitter_snippet_load_lui_only_to_register_2,
    jitter_snippet_load_lui_only_to_register_3,
    jitter_snippet_load_lui_only_to_register_4,
    jitter_snippet_load_lui_only_to_register_5,

    jitter_snippet_load_pcrel_address_to_register_0,
    jitter_snippet_load_pcrel_address_to_register_1,
    jitter_snippet_load_pcrel_address_to_register_2,
    jitter_snippet_load_pcrel_address_to_register_3,
    jitter_snippet_load_pcrel_address_to_register_4,
    jitter_snippet_load_pcrel_address_to_register_5,

    jitter_snippet_load_pcrel_address_no_add_to_register_0,
    jitter_snippet_load_pcrel_address_no_add_to_register_1,
    jitter_snippet_load_pcrel_address_no_add_to_register_2,
    jitter_snippet_load_pcrel_address_no_add_to_register_3,
    jitter_snippet_load_pcrel_address_no_add_to_register_4,
    jitter_snippet_load_pcrel_address_no_add_to_register_5,

#if JITTER_XLEN == 64
    jitter_snippet_load_64bit_to_register_0,
    jitter_snippet_load_64bit_to_register_1,
    jitter_snippet_load_64bit_to_register_2,
    jitter_snippet_load_64bit_to_register_3,
    jitter_snippet_load_64bit_to_register_4,
    jitter_snippet_load_64bit_to_register_5,
#endif // #if JITTER_XLEN == 64
    /* FIXME: the next three, or similar snippets to materialise constants into
       memory, are not implemented yet.  I might not need them on such a
       register-rich architecture. */
    /* jitter_snippet_load_zero_extended_16bit_to_memory, */
    /* jitter_snippet_load_sign_extended_16bit_to_memory, */
    /* jitter_snippet_load_32bit_to_memory, */

    jitter_snippet_jump_unconditional_20bit_pcrel,
    jitter_snippet_jump_and_link_20bit_pcrel,

    /* The same snippet works for any conditional branch. */
    jitter_snippet_jump_conditional_13bit_pcrel,

    /* The number of snippets. */
    jitter_snippet_no
  };

#endif // #ifndef __ASSEMBLER__

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
