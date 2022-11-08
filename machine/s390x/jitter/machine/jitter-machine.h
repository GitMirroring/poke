/* VM library: s390x definitions, to be included from both C and assembly.

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
#define JITTER_ASM_COMMENT_PREFIX   \
  "#"




/* Debugging.
 * ************************************************************************** */

/* Expand to an inline assembly template generating a nop instruction containing
   the given literal as an argument. */
#define _JITTER_ASM_DEBUGGING_NOP(integer_literal_as_string)                  \
  /* Exclusive or with immediate on the lower half, twice.  Maybe there is a  \
     more compact way, but I hardly know this architecture. */                \
  "xilf %%r1, " integer_literal_as_string "\n\t"                              \
  "xilf %%r1, " integer_literal_as_string "\n\t"

/* Expand to a native machine code snippet causing a trap, as a string literal
   in a syntax suitable for extended inline asm. */
#define _JITTER_ASM_CRASH                                                \
  /* Cause an exception from user mode. */                               \
  "trap2"




/* Computed goto.
 * ************************************************************************** */

/* Computed goto implemented with inline asm.  See the comments about
   JITTER_ASM_COMPUTED_GOTO_TEMPLATE, and
   JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT and
   JITTER_ASM_COMPUTED_GOTO_CLOBBERS in jitter/jitter-executor.h . */
#define JITTER_ASM_COMPUTED_GOTO_TEMPLATE                                   \
  /* This appears to be a pseudo-instruction; I know hardly anything about  \
     this architecture.  However, pseudo-instruction or not, this seems to  \
     branch to the address in a register. */                                \
  "br %[_jitter_the_target]"
#define JITTER_ASM_COMPUTED_GOTO_INPUT_CONSTRAINT  \
  "r"


#endif // #ifndef JITTER_NATIVE_MACHINE_H_
