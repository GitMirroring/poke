/* Jittery structured language example: stack code generator header.

   Copyright (C) 2017, 2019 Luca Saiu
   Written by Luca Saiu

   This file is part of the Jitter structured-language example, distributed
   along with GNU Jitter under the same license.

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


#ifndef STRUCTURED_CODE_GENERATOR_STACK_H_
#define STRUCTURED_CODE_GENERATOR_STACK_H_

#include "structuredvm-vm.h"
#include "structured-syntax.h"

/* Fill the pointed empty VM routine using stack-based instructions which are
   the translation of the pointed AST program. */
void
structured_translate_program_stack (struct structuredvm_mutable_routine *vmp,
                                    struct structured_program *p)
  __attribute__ ((nonnull (1, 2)));

#endif // #ifndef STRUCTURED_CODE_GENERATOR_STACK_H_
