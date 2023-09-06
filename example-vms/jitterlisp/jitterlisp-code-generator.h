/* JitterLisp: Jittery VM code generation header.

   Copyright (C) 2018 Luca Saiu
   Updated in 2023 by Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with GNU Jitter under the same license.

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


#ifndef JITTERLISP_CODE_GENERATOR_H_
#define JITTERLISP_CODE_GENERATOR_H_

#include <jitter/jitter.h>
#include "jitterlisp-sexpression.h"




/* Code generator: C part.
 * ************************************************************************** */

/* Make the pointed closure compiled, if it's not compiled already.  In either
   case replace all of its fields, generating VM code from the given arguments.
   The function accepts in_arity arguments, and uses the nonlocals in the given
   list, in order; the code is encoded as an s-expression, meant to be obtained
   from Lisp code. */
void
jitterlisp_compile (struct jitterlisp_closure *c,
                    jitter_int in_arity,
                    jitterlisp_object nonlocals,
                    jitterlisp_object code_as_sexpression);




/* Code generation debugging.
 * ************************************************************************** */

/* Print the VM code in the given compiled closure in human-readable form. */
void
jitterlisp_print_compiled_closure (struct jitterlisp_compiled_closure *cc);

/* Disassemble native code from the given compiled closure. */
void
jitterlisp_disassemble_compiled_closure
   (struct jitterlisp_compiled_closure *cc);


#endif // #ifndef JITTERLISP_CODE_GENERATOR_H_
