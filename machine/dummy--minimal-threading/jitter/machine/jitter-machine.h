/* VM library: ARCHITECTURENAME definitions, to be included from both C and assembly.

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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#ifndef JITTER_NATIVE_MACHINE_H_ /* One macro suffices for all architectures. */
#define JITTER_NATIVE_MACHINE_H_




/* How to introduce comments in assembly, at any point within the line. */
#define JITTER_ASM_COMMENT_PREFIX                               \
  "FIXME: define this.  For example:  #"




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)  \
  "FIXME: define this: for example:  add $0, $0, $" integer_literal_as_string

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                \
  /* Cause an exception from user mode. */                               \
  "FIXME: define this: for example:  eret"




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE  \
  "FIXME: define this: for example:  br %[_jitter_the_target]"
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  \
  "FIXME: define this: it will typically be:  r"




/* Execution-beginning and execution-end code.
 * ************************************************************************** */

/* Some architectures need some code to be executed at VM execution beginning,
   VM execution end, pre-C-function-call, post-C-function-call; a typical
   use case would be saving and restoring the value of a register.

   All of these macros can be left undefined for configurations not needing
   them.  These macros are not used in simple dispatches. */

/* A sequence of statements and variable declarations to be executed at
   execution entry.  This can define automatic variables, visible within the
   executor body. */
/*#define JITTER_EXECUTION_BEGINNING_*/                                 \
  /* Nothing here.  See the powerpc or the alpha version of jitter-machine.h for
     an example. */

/* Like JITTER_EXECUTION_BEGINNING_ , to be run at VM execution end. */
/*#define JITTER_EXECUTION_END_*/                                       \
  /* Nothing here.  See the powerpc version of jitter-machine.h for an
     example. */

/* A sequence of statements to be executed before every call to a wrapped C
   function. */
/*#define JITTER_EXECUTION_PRE_C_CALL_*/                                 \
  /* Nothing here.  See the alpha version of jitter-machine.h for an
     example. */

/* A sequence of statements to be executed after every call to a wrapped C
   function. */
/*#define JITTER_EXECUTION_POST_C_CALL_*/                                \
  /* Nothing here.  See the alpha version of jitter-machine.h for an
     example. */

#endif // #ifndef JITTER_NATIVE_MACHINE_H_
