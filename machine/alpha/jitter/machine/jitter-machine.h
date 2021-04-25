/* VM library: Alpha definitions, to be included from both C and assembly.

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




/* How to introduce comments in assembly, at any point within the line. */
#define JITTER_ASM_COMMENT_PREFIX   \
  "#"




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "lda $31, " integer_literal_as_string "($31)"

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                \
  /* Cause an exception from user mode. */                               \
  "bugchk"




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE  \
  "jmp (%[_jitter_the_target])"
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  \
  "r"




/* Execution-beginning and execution-end code.
 * ************************************************************************** */

/* On Alpha, at least with the GNU/Linux ABI, the global pointer $29 / $gp is
   *not* preserved across calls; this means that the register value must be
   restored after each C function call.  GCC of course provides for this, but
   unfortunately the techinque used to reconstruct $gp depends on the program
   counter, and breaks under replication.
   In order to work around the issue save $gp at the beginning of VM code
   execution, and restore it after each call to C. */

/* Save $gp to an automatic variable (in memory). */
#define JITTER_EXECUTION_BEGINNING_                                         \
  void *_jitter_saved_alpha_gp __attribute__ ((unused));                    \
  asm volatile ("stq $gp, %[_jitter_saved_alpha_gp]"                        \
                : [_jitter_saved_alpha_gp] "=m" (_jitter_saved_alpha_gp));

/* No need for JITTER_EXECUTION_END_. */

/* No need for JITTER_EXECUTION_PRE_C_CALL_. */

/* Restore $gp from the automatic variable (in memory). */
#define JITTER_EXECUTION_POST_C_CALL_                                      \
  asm volatile ("ldq $gp, %[_jitter_saved_alpha_gp]"                       \
                : /* outputs */                                            \
                : /* inputs */                                             \
                  [_jitter_saved_alpha_gp] "m" (_jitter_saved_alpha_gp));

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
