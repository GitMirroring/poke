/* JitterLisp: interpreter: naïve C version header.

   Copyright (C) 2017, 2018 Luca Saiu
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
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTERLISP_EVAL_INTERPRETER_H_
#define JITTERLISP_EVAL_INTERPRETER_H_

#include "jitterlisp-sexpression.h"



/* Non-Jittery interpreter.
 * ************************************************************************** */

/* This is a naïf interpreter directly written in C not using a Jittery VM and
   intended as a baseline to compare against. */




/* Non-Jittery interpreter.
 * ************************************************************************** */

/* Macroexpand the given JitterLisp form, and return the result of its
   evaluation, using the global environment only.  Evaluate using the
   non-Jittery AST interpreter. */
jitterlisp_object
jitterlisp_eval_globally_interpreter (jitterlisp_object form);

/* Macroexpand the given JitterLisp form, and return the result of its
   evaluation, using the given non-global environment shaped as described in
   jitterlisp-utility.h . */
jitterlisp_object
jitterlisp_eval_interpreter (jitterlisp_object form, jitterlisp_object env);

/* Return the result of applying closure_value, which is assumed to be an
   already evaluated closure (not checked by the function) to the operands,
   assumed to be a list (not checked for) of already evaluated operands.
   Evaluate using the non-Jittery AST interpreter.  */
jitterlisp_object
jitterlisp_apply_interpreter (jitterlisp_object closure_value,
                              jitterlisp_object operands_as_list);




/* Non-Jittery AST interpreter.
 * ************************************************************************** */

/* Return the result of the evaluation of the given form (as an encoded AST),
   which must be already macroexpanded, in the given non-global environment.
   The environment is encoded according to the unspecified internal encoding
   used in the interpreter. */
jitterlisp_object
jitterlisp_eval_interpreter_ast (jitterlisp_object ast, jitterlisp_object env);




/* Call into interpreted code.
 * ************************************************************************** */

/* Call the pointed compiled closure using as arguments the pointed actuals in
   the given number, already evaluated; return the result or error out in case
   of problems at any point during the call extent.  Assume that the closure
   in-arity is correct, without checking.

   Rationale: this is useful to call an interpreted closure from compiled
   code.  Compiled code checks for in-arity mismatches before the call. */
jitterlisp_object
jitterlisp_call_interpreted (const struct jitterlisp_interpreted_closure *ic,
                             jitterlisp_object *actual_values,
                             jitter_uint actual_value_no);

#endif // #ifndef JITTERLISP_EVAL_INTERPRETER_H_
