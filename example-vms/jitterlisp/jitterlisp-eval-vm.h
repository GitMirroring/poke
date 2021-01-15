/* JitterLisp: interpreter: naïve C version header.

   Copyright (C) 2017, 2018, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with Jitter under the same license.

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


#ifndef JITTERLISP_EVAL_VM_H_
#define JITTERLISP_EVAL_VM_H_

#include "jitterlisp-sexpression.h"
#include "jitterlispvm-vm.h"




/* Jittery engine.
 * ************************************************************************** */

/* This is the evaluation engine using the Jittery VM. */
// FIXME: update the comments, which may be wrong now.




/* Jittery engine.
 * ************************************************************************** */

// FIXME: remove this section.

/* Macroexpand the given JitterLisp form, and return the result of its
   evaluation, using the global environment only.  Evaluate using the Jittery
   VM. */
jitterlisp_object
jitterlisp_eval_globally_vm (jitterlisp_object form);

/* Macroexpand the given JitterLisp form, and return the result of its
   evaluation, using the given non-global environment shaped as described in
   jitterlisp-utility.h .  Evaluate using the Jittery VM. */
jitterlisp_object
jitterlisp_eval_vm (jitterlisp_object form, jitterlisp_object env);

/* Return the result of applying closure_value, which is assumed to be an
   already evaluated closure (not checked by the function) to the operands,
   assumed to be a list (not checked for) of already evaluated operands.
   Evaluate using the Jittery VM. */
jitterlisp_object
jitterlisp_apply_vm (jitterlisp_object closure_value,
                     jitterlisp_object operands_as_list);




/* State pool.
 * ************************************************************************** */

extern struct jitter_list_header
jitterlisp_unused_states;




/* Slow register counter.
 * ************************************************************************** */

/* This global variable keeps track of how many slow registers are needed
   for the currently existing compiled code. */
extern jitter_int
jitterlisp_slow_register_per_class_no; // FIXME: use this.




/* Call into VM code.
 * ************************************************************************** */

/* Call the given (tagged, already evaluated) compiled closure, first evaluating
   its actuals with the AST interpreter, and return the result.  Error out
   without evaluating anything in case of in-arity mismatch. */
jitterlisp_object
jitterlisp_call_compiled (jitterlisp_object rator_value,
                          const jitterlisp_object *rand_asts,
                          jitter_uint rand_ast_no,
                          jitterlisp_object env)
  __attribute__ ((nonnull (2)));

/* Call the given (tagged, already evaluated) compiled closure with the
   arguments, already evaluated, from the given list and return the result.
   Error out without evaluating anything in case of in-arity mismatch.

   This doesn't check that the operator is a compiled closure or that the
   operands are actually a list, but it *does* check in-arity.
   Rationale: this is what the caller in jitterlisp_apply_interpreter
   needs. */
jitterlisp_object
jitterlisp_apply_compiled (jitterlisp_object rator_value,
                           jitterlisp_object rand_value_list);





/* Not for the user: VM initialization and finalization.
 * ************************************************************************** */

/* These are called from the global initialization and finalization code. */

/* Initialize the VM substystem and the global VM state. */
void
jitterlisp_vm_initialize (void);

/* Finalize the VM substystem and the global VM state. */
void
jitterlisp_vm_finalize (void);

#endif // #ifndef JITTERLISP_EVAL_VM_H_
