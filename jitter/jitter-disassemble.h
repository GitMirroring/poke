/* VM library: native code disassembler.

   Copyright (C) 2017, 2019, 2020 Luca Saiu
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


#ifndef JITTER_DISASSEMBLE_H_
#define JITTER_DISASSEMBLE_H_

#include <stdbool.h>
#include <stdio.h>

#include <jitter/jitter.h>
#include <jitter/jitter-print.h>
#include <jitter/jitter-specialize.h>

/* Disassemble the given routine to the given print context using a pipe to
   objdump, or falling back to a binary dump if such functionality is not
   available or fails at run time.  The internal implementation is quite
   inefficient, as it relies on one objdump invocation per specialised
   instruction.
   
   The output uses the following class names (see jitter/jitter-print.h), with
   "vmprefix" replaced by the lower-case name of the VM for the executable
   routine:
   - "vmprefix-comment";
   - "vmprefix-memory-address";
   - "vmprefix-native-instruction-hex";
   - "vmprefix-disassembly". */
void
jitter_executable_routine_disassemble (jitter_print_context out,
                                       const struct jitter_executable_routine
                                       *er, bool raw, const char *objdump_name,
                                       const char *objdump_options_or_NULL)
  __attribute__ ((nonnull(1, 2, 4)));


#endif // #ifndef JITTER_DISASSEMBLE_H_
