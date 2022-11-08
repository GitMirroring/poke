/* JitterLisp: interpreter: naïve C version.

   Copyright (C) 2017, 2018, 2020 Luca Saiu
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


#include "jitterlisp-eval-interpreter.h"
#include "jitterlisp-eval-vm.h"

#include "jitterlisp.h"




/* Non-Jittery interpreter: AST evaluation helpers for primitives and closures.
 * ************************************************************************** */

/* Return the evaluation of the given primitive on the given (of course still
   unevaluated) operand ASTs.  Assume that the rator argument is an encoded
   primitive, and that rand_asts is a C array of operand_no elements.
   The noinline attribute is important here: this function invokes a primitive C
   function in what would syntactically look like a tail context, but passing it
   a pointer to local storage as argument; that prevents GCC from compiling the
   call as a sibling call optimization, which in itself is a very minor loss.
   However having the body of this function inlined in
   jitterlisp_eval_interpreter_ast , which in its turn also inlines
   jitterlisp_eval_interpreter_ast_call , would prevent sibling call compilation
   in the case of *closure* tail calls, thus leaking stack space for tail calls.
   Tested with a GCC 8 snapshot from early October 2017. */
__attribute__ ((noinline))
static jitterlisp_object
jitterlisp_eval_interpreter_ast_primitive (jitterlisp_object rator,
                                           const jitterlisp_object *rand_asts,
                                           size_t rand_no,
                                           jitterlisp_object env)
{
  /* FIXME: this, and likely this C function signature as well, will need to
     change with an exact-pointer-finding GC.
     Evaluate primitive actuals into a temporary array which is large enough for
     the actuals of any primitive.  Don't bother initializing the elements we
     don't actually use.  If the AST has been built correctly the primitive
     in-arity is correct, so we don't need to check it now at run time. */
  jitterlisp_object values [JITTERLISP_PRIMITIVE_MAX_IN_ARITY];
  int i;
  for (i = 0; i < rand_no; i ++)
    values [i] = jitterlisp_eval_interpreter_ast (rand_asts [i], env);

  /* FIXME: does GCC guarantee that this will not be compiled as a sibling
     call?  It is important that the current stack frame is not popped until
     the callee returns. */
  return JITTERLISP_PRIMITIVE_DECODE(rator)->function (values);
}

/* Return the result of the given call in the given environment.  The operator
   is an AST, still to evaluate, and the operands are tagged ASTs in the given
   number; the operator comes first in the array.  If the operator doesn't
   evaluate to a closure this function errors out cleanly. */
static inline jitterlisp_object
jitterlisp_eval_interpreter_ast_call
   (const jitterlisp_object *rator_and_rand_asts,
    size_t rator_and_rand_no,
    jitterlisp_object env)
{
  /* First evaluate the operator. */
  jitterlisp_object rator_value
    = jitterlisp_eval_interpreter_ast (rator_and_rand_asts [0], env);
  if (! JITTERLISP_IS_CLOSURE(rator_value))
    {
      jitterlisp_print_error_char_star ("About "); // FIXME: add to the error message
      jitterlisp_print_error (rator_value);
      jitterlisp_print_error_char_star (":\n");
      jitterlisp_error_cloned ("call: non-closure operator");
    }

  /* If we arrived here the operator is a closure.  Is it compiled or
     interpreted? */
  struct jitterlisp_closure *c = JITTERLISP_CLOSURE_DECODE(rator_value);

  // FIXME: shall I check the arity *before* evaluating actuals or after, as
  // the code does now?
  // In either case compiled code must have the same semantics.  Do whatever
  // is faster on compiled code.  There is another identical case above
  // and one more below.

  /* If the closure is compiled tail-call the helper function and ignore the
     rest of this. */
  if (__builtin_expect ((c->kind == jitterlisp_closure_type_compiled),
                        false))
    return jitterlisp_call_compiled (rator_value,
                                     rator_and_rand_asts + 1,
                                     rator_and_rand_no - 1,
                                     env);

  /* The closure is interpreted.  Evaluate actuals binding them to the closure
     formals, in order, starting from the closure environment.  Unfortunately we
     have to check the arity at run time, differently from the primitive
     case. */
  struct jitterlisp_interpreted_closure *ic = & c->interpreted;
  jitterlisp_object formals = ic->formals;
  jitterlisp_object body_env = ic->environment;
  int i;
  // FIXME: shall I check the arity *before* evaluating actuals or after, as
  // the code does now?
  // In either case compiled code must have the same semantics.  Do whatever
  // is faster on compiled code.  There are two other identical cases above.
  for (i = 1; i < rator_and_rand_no; i ++)
    {
      if (JITTERLISP_IS_EMPTY_LIST(formals))
        {
          jitterlisp_print_error_char_star ("About a call to "); // FIXME: add to the error message
          jitterlisp_print_error (rator_value);
          jitterlisp_print_error_char_star (":\n");
          jitterlisp_error_cloned ("call: too many actuals");
        }

      jitterlisp_object rand_value =
        jitterlisp_eval_interpreter_ast (rator_and_rand_asts [i], env);
      jitterlisp_object formal = JITTERLISP_EXP_C_A_CAR(formals);
      body_env = jitterlisp_environment_bind (body_env, formal, rand_value);

      formals = JITTERLISP_EXP_C_A_CDR(formals);
    }
  if (! JITTERLISP_IS_EMPTY_LIST(formals))
    {
      jitterlisp_print_error_char_star ("About a call to "); // FIXME: add to the error message
      jitterlisp_print_error (rator_value);
      jitterlisp_print_error_char_star (":\n");
      jitterlisp_error_cloned ("call: not enough actuals");
    }

  /* Return the evaluation of the closure body in the extended closure
     environment. */
  jitterlisp_object body_ast = ic->body;
  return jitterlisp_eval_interpreter_ast (body_ast, body_env);
}




/* Non-Jittery interpreter: AST evaluation.
 * ************************************************************************** */

/* This is the main function for AST interpretation. */
jitterlisp_object
jitterlisp_eval_interpreter_ast (jitterlisp_object o,
                                 jitterlisp_object env)
{
  /* No need to validate o: if it comes from macroexpansion it's definitely an
     encoded AST, and its subs are well-formed as well.  No need to validate
     env for the same reason. */
  const struct jitterlisp_ast *ast = JITTERLISP_AST_DECODE(o);
  const jitter_uint sub_no = ast->sub_no;
  const jitterlisp_object * const subs = ast->subs;
  switch (ast->case_)
    {
    case jitterlisp_ast_case_literal:
      return subs [0];

    case jitterlisp_ast_case_variable:
      return jitterlisp_environment_lookup (env, subs [0]);

    case jitterlisp_ast_case_define:
      {
        jitterlisp_object defined_value
          = jitterlisp_eval_interpreter_ast (subs [1], env);
        jitterlisp_define (subs [0], defined_value);
        return JITTERLISP_NOTHING;
      }

    case jitterlisp_ast_case_if:
      {
        jitterlisp_object condition_result
          = jitterlisp_eval_interpreter_ast (subs [0], env);
        jitterlisp_object branch
          = (JITTERLISP_IS_FALSE(condition_result)
             ? subs [2]
             : subs [1]);
        return jitterlisp_eval_interpreter_ast (branch, env);
      }

    case jitterlisp_ast_case_setb:
      {
        jitterlisp_object bound_value
          = jitterlisp_eval_interpreter_ast (subs [1], env);
        jitterlisp_environment_setb (env, subs [0], bound_value);
        return JITTERLISP_NOTHING;
      }

    case jitterlisp_ast_case_while:
      {
        const jitterlisp_object guard = subs [0];
        const jitterlisp_object body = subs [1];
        while (! JITTERLISP_IS_FALSE(jitterlisp_eval_interpreter_ast (guard,
                                                                      env)))
          jitterlisp_eval_interpreter_ast (body, env);
        return JITTERLISP_NOTHING;
      }

    case jitterlisp_ast_case_primitive:
      return jitterlisp_eval_interpreter_ast_primitive (subs [0],
                                                        subs + 1,
                                                        sub_no - 1,
                                                        env);

    case jitterlisp_ast_case_call:
      return jitterlisp_eval_interpreter_ast_call (subs, sub_no, env);

    case jitterlisp_ast_case_lambda:
      {
        /* Notice that the lambda formals are already stored as a list of
           symbols in the AST, differently from other AST cases; that is an
           optimization to make this closure initialization faster. */
        jitterlisp_object res;
        JITTERLISP_CLOSURE_(res, env, subs [0], subs [1]);
        return res;
      }

    case jitterlisp_ast_case_let:
      {
        /* Evaluate the bound form in env, then bind its result to the bound
           variable in the current environment. */
        jitterlisp_object bound_value
          = jitterlisp_eval_interpreter_ast (subs [1], env);
        env = jitterlisp_environment_bind (env, subs [0], bound_value);

        /* Evaluate the body in the extended environment. */
        return jitterlisp_eval_interpreter_ast (subs [2], env);
      }

    case jitterlisp_ast_case_sequence:
      jitterlisp_eval_interpreter_ast (subs [0], env);
      return jitterlisp_eval_interpreter_ast (subs [1], env);

    default:
      jitterlisp_print_error_char_star ("About "); // FIXME: add to the error message
      jitterlisp_print_error (o);
      jitterlisp_print_error_char_star (":\n");
      jitterlisp_error_cloned ("eval: invalid or unimplemented AST case");
    }
}




/* Non-Jittery interpreter: user API.
 * ************************************************************************** */

jitterlisp_object
jitterlisp_eval_globally_interpreter (jitterlisp_object unexpanded_form)
{
  return jitterlisp_eval_interpreter (unexpanded_form,
                                      jitterlisp_empty_environment);
}

jitterlisp_object
jitterlisp_eval_interpreter (jitterlisp_object unexpanded_form,
                             jitterlisp_object env)
{
  if (jitterlisp_settings.verbose)
    {
      jitterlisp_log_char_star ("Macroexpanding ");
      jitterlisp_log (unexpanded_form);
      jitterlisp_log_char_star ("...\n");
    }
  jitterlisp_object ast = jitterlisp_macroexpand (unexpanded_form, env);
  if (jitterlisp_settings.verbose)
    {
      jitterlisp_log_char_star ("...into ");
      jitterlisp_log (ast);
      jitterlisp_log_char_star ("\n");
    }
  return jitterlisp_eval_interpreter_ast (ast, env);
}




/* Non-Jittery interpreter: apply.
 * ************************************************************************** */

/* Differently from what happens in simple meta-circual interpreters here eval
   and apply are not mutually recursive: eval doesn't evaluate a procedure call
   operands into a temporary list, for efficiency reasons.  However this is
   convenient to have, particularly to be called from Lisp (with additional type
   checking done by the primimitive function), when the operands are already a
   list. */

/* Unfortunately this is difficult to factor with
   jitterlisp_eval_interpreter_ast_call without introducing unnecessary
   allocation, and here performance is important. */
jitterlisp_object
jitterlisp_apply_interpreter (jitterlisp_object closure_value,
                              jitterlisp_object operands_as_list)
{
  /* Decode the closure.  No need to check that it's actually a closure, but
     here we don't know if it's compiled or interpreted. */
  struct jitterlisp_closure *c = JITTERLISP_CLOSURE_DECODE(closure_value);

  /* If the closure is compiled tail-call another function which does the
     job of calling the VM, checking for in-arity mismatches. */
  if (__builtin_expect ((c->kind == jitterlisp_closure_type_compiled),
                        false))
    return jitterlisp_apply_compiled (closure_value, operands_as_list);

  /* The closure is interpreted, so it's the interpreter's job to evaluate
     the call.  Keep fields in automatic C variables. */
  struct jitterlisp_interpreted_closure *ic = & c->interpreted;
  jitterlisp_object formals = ic->formals;
  jitterlisp_object body_env = ic->environment;

  /* Bind operands to formals in the closure environment. */
  while (! JITTERLISP_IS_EMPTY_LIST (operands_as_list))
    {
      if (JITTERLISP_IS_EMPTY_LIST(formals))
        {
          jitterlisp_print_error_char_star ("About a call to "); // FIXME: add to the error message
          jitterlisp_print_error (closure_value);
          jitterlisp_print_error_char_star ("\n");
          jitterlisp_error_cloned ("apply: too many actuals");
        }
      /* If this were a safe C function I would check whether operands_as_list
         is a cons; but this has been already checked out of this function when
         we get here thru a primitive call. */

      /* Extend the environment with one formal/operand binding. */
      jitterlisp_object formal = JITTERLISP_EXP_C_A_CAR(formals);
      jitterlisp_object rand_value = JITTERLISP_EXP_C_A_CAR(operands_as_list);
      body_env = jitterlisp_environment_bind (body_env, formal, rand_value);

      /* Advance the two lists. */
      formals = JITTERLISP_EXP_C_A_CDR(formals);
      operands_as_list = JITTERLISP_EXP_C_A_CDR(operands_as_list);
    }
  if (! JITTERLISP_IS_EMPTY_LIST(formals))
    {
      jitterlisp_print_error_char_star ("About a call to "); // FIXME: add to the error message
      jitterlisp_print_error (closure_value);
      jitterlisp_print_error_char_star ("\n");
      jitterlisp_error_cloned ("apply: not enough actuals");
    }

  /* Return the evaluation of the closure body in the extended closure
     environment. */
  jitterlisp_object body_ast = ic->body;
  return jitterlisp_eval_interpreter_ast (body_ast, body_env);
}




/* Call into interpreted code.
 * ************************************************************************** */

jitterlisp_object
jitterlisp_call_interpreted (const struct jitterlisp_interpreted_closure *ic,
                             jitterlisp_object *actual_values,
                             jitter_uint actual_value_no)
{
  /* Keep fields in automatic C variables. */
  jitterlisp_object formals = ic->formals;
  jitterlisp_object body_env = ic->environment;

  /* Bind operands to formals in the closure environment.  No need for arity
     checking. */
  int i;
  for (i = 0; i < actual_value_no; i ++)
    {
      /* Extend the environment with one formal/operand binding. */
      jitterlisp_object formal = JITTERLISP_EXP_C_A_CAR(formals);
      jitterlisp_object rand_value = actual_values [i];
      body_env = jitterlisp_environment_bind (body_env, formal, rand_value);

      /* Advance the formal list. */
      formals = JITTERLISP_EXP_C_A_CDR(formals);
    }

  /* Return the evaluation of the closure body in the extended closure
     environment. */
  jitterlisp_object body_ast = ic->body;
  return jitterlisp_eval_interpreter_ast (body_ast, body_env);
}
