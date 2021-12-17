/* JitterLisp: Lisp macros.

   Copyright (C) 2017, 2018, 2019, 2020 Luca Saiu
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


#include "jitterlisp-macros.h"

#include "jitterlisp.h"
#include "jitterlisp-eval-interpreter.h" // FIXME: use the generic interpreter instead?


/* Macroexpansion utility.
 * ************************************************************************** */

/* Return the list of the macroexpansions of the s-expressions in the given
   list. */
static jitterlisp_object
jitterlisp_macroexpand_multiple (jitterlisp_object os,
                                 jitterlisp_object env)
{
  /* This logic is slightly more complex than the obvious recursive alternative,
     but uses constant stack space and doesn't use any temporary heap data
     structure. */
  jitterlisp_object res = JITTERLISP_UNDEFINED; /* Invalid, to catch bugs. */
  jitterlisp_object *res_restp = & res;
  while (! JITTERLISP_IS_EMPTY_LIST(os))
    {
      if (! JITTERLISP_IS_CONS(os))
        {
          jitterlisp_print_error_char_star ("About "); // FIXME: integrate with the error function
          jitterlisp_print_error (os);
          jitterlisp_print_error_char_star (":\n");
          jitterlisp_error_cloned ("jitterlisp_macroexpand_multiple: non-list "
                                   "argument");
        }
      jitterlisp_object car = JITTERLISP_EXP_C_A_CAR(os);
      jitterlisp_object macroexpanded_car = jitterlisp_macroexpand (car, env);
      * res_restp = jitterlisp_cons (macroexpanded_car, JITTERLISP_UNDEFINED);
      res_restp = & JITTERLISP_EXP_C_A_CDR(* res_restp);

      os = JITTERLISP_EXP_C_A_CDR(os);
    }
  * res_restp = JITTERLISP_EMPTY_LIST;
  return res;
}

/* Return the expansion of the given list of forms as a sequence.  This is what
   the primitive macro begin does and the same code is used whenever a form
   sequence is allowed, for example in a lambda body. */
static jitterlisp_object
jitterlisp_macroexpand_begin (jitterlisp_object forms,
                              jitterlisp_object env)
{
  /* Zero-form body. */
  if (JITTERLISP_IS_EMPTY_LIST(forms))
    return jitterlisp_ast_make_literal (JITTERLISP_NOTHING);

  /* Ill-formed body. */
  if (! JITTERLISP_IS_CONS(forms))
    jitterlisp_error_cloned ("begin: non-list body");

  /* At this point we have to look inside the cons. */
  jitterlisp_object first = JITTERLISP_EXP_C_A_CAR(forms);
  jitterlisp_object rest = JITTERLISP_EXP_C_A_CDR(forms);

  /* One-form body. */
  if (JITTERLISP_IS_EMPTY_LIST(rest))
    return jitterlisp_macroexpand (first, env);

  /* Multiple-form body. */
  return jitterlisp_ast_make_sequence
            (jitterlisp_macroexpand (first, env),
             jitterlisp_macroexpand_begin (rest, env));
}

static jitterlisp_object
jitterlisp_macroexpand_macro_call (jitterlisp_object macro,
                                   jitterlisp_object cdr,
                                   jitterlisp_object env)
{
  if (JITTERLISP_IS_PRIMITIVE_MACRO(macro))
    {
      struct jitterlisp_primitive *p = JITTERLISP_PRIMITIVE_MACRO_DECODE(macro);
      /* FIXME: This plays well with tail calls but is not reentrant. */
      static jitterlisp_object primitive_args [2];
      primitive_args [0] = cdr; primitive_args [1] = env;
      return p->function (primitive_args);
    }
  else if (JITTERLISP_IS_NON_PRIMITIVE_MACRO(macro))
    {
      struct jitterlisp_interpreted_closure *macro_closure
        = JITTERLISP_NON_PRIMITIVE_MACRO_DECODE(macro);
      jitterlisp_object expansion_env = macro_closure->environment;
      /* We can ignore macro formals: low-level macros only have one formal
         with the fixed name low-level-macro-args . */
      jitterlisp_object macro_ast = macro_closure->body;

      /* Bind the macro call cdr to the one formal.  Notice that the cdr is not
         expanded or evaluated in any way: it's the macro's job to do that as
         needed. */
      expansion_env
        = jitterlisp_environment_bind (expansion_env,
                                       jitterlisp_low_level_macro_args,
                                       cdr);
      /* Evaluate the macro body in the extended environment. */
      jitterlisp_object first_result
        = jitterlisp_eval_interpreter_ast (macro_ast, expansion_env);

      /* The result can be any Lisp object, which may need to be macroexpanded
         in its turn. */
      return jitterlisp_macroexpand (first_result, env);
    }
  else
    jitterlisp_error_cloned ("macroexpanding macro call to non-macro");
}

static jitterlisp_object
jitterlisp_macroexpand_cons (jitterlisp_object o, jitterlisp_object env)
{
  jitterlisp_object car = JITTERLISP_EXP_C_A_CAR(o);
  jitterlisp_object cdr = JITTERLISP_EXP_C_A_CDR(o);

  /* If the car is a symbol bound to a macro then the cons encodes a macro call.
     Macroexpand it as such. */
  if (JITTERLISP_IS_SYMBOL (car) &&
      jitterlisp_environment_has (env, car))
    {
      jitterlisp_object car_value = jitterlisp_environment_lookup (env, car);
      if (JITTERLISP_IS_MACRO (car_value))
        return jitterlisp_macroexpand_macro_call (car_value, cdr, env);
    }

  /* The cons doesn't encode a macro call.  Macroexpand it into a procedure
     call. */
  return jitterlisp_primitive_macro_function_call (o, env);
}




/* Macroexpansion main function.
 * ************************************************************************** */

jitterlisp_object
jitterlisp_macroexpand (jitterlisp_object o,
                        jitterlisp_object env)
{
  if (JITTERLISP_IS_AST(o))
    /* This is already an AST: there is nothing more to expand. */
    return o;
  else if (JITTERLISP_IS_SYMBOL(o))
    return jitterlisp_ast_make_variable (o);
  else if (JITTERLISP_IS_CONS(o))
    return jitterlisp_macroexpand_cons (o, env);
  else
    return jitterlisp_ast_make_literal (o);
}

jitterlisp_object
jitterlisp_macroexpand_globally (jitterlisp_object o)
{
  return jitterlisp_macroexpand (o, jitterlisp_empty_environment);
}




/* AST primitive macro functions.
 * ************************************************************************** */

/* Expand to a definition possibly followed by making the defined symbol
   constant, according to the boolean. */
static jitterlisp_object
jitterlisp_primitive_macro_function_define_internal (jitterlisp_object cdr,
                                                     jitterlisp_object env,
                                                     bool constant)
{
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("define: invalid cdr");

  jitterlisp_object defined_thing = JITTERLISP_EXP_C_A_CAR (cdr);
  if (JITTERLISP_IS_SYMBOL(defined_thing))
    {
      jitterlisp_object body
        = jitterlisp_macroexpand_begin (JITTERLISP_EXP_C_A_CDR (cdr), env);
      jitterlisp_object res = jitterlisp_ast_make_define (defined_thing, body);
      if (constant)
        {
          /* Wrap the definition AST inside a sequence, also making the
             defined symbol constant. */
          jitterlisp_object primitive
            = JITTERLISP_SYMBOL_DECODE(jitterlisp_primitive_make_constantb)
                 ->global_value;
          jitterlisp_object make_constantb
            = jitterlisp_ast_make_primitive
                 (primitive,
                  jitterlisp_list_1 (jitterlisp_ast_make_literal
                                        (defined_thing)));
          res = jitterlisp_ast_make_sequence (res, make_constantb);
        }
      return res;
    }
  else if (JITTERLISP_IS_CONS(defined_thing))
    {
      jitterlisp_object defined_name = JITTERLISP_EXP_C_A_CAR (defined_thing);
      if (! JITTERLISP_IS_SYMBOL (defined_name))
        jitterlisp_error_cloned ("define: non-symbol procedure name");
      jitterlisp_object formals = jitterlisp_cdr (defined_thing);
      jitterlisp_object lambda
        = jitterlisp_primitive_macro_function_lambda
             (jitterlisp_cons (formals, jitterlisp_cdr (cdr)),
              env);
      jitterlisp_object args = jitterlisp_list_2 (defined_name, lambda);
      return jitterlisp_primitive_macro_function_define_internal (args,
                                                                  env,
                                                                  constant);
    }
  else
    jitterlisp_error_cloned ("define: invalid cadr");
}

jitterlisp_object
jitterlisp_primitive_macro_function_define (jitterlisp_object cdr,
                                            jitterlisp_object env)
{
  return jitterlisp_primitive_macro_function_define_internal (cdr, env, false);
}

jitterlisp_object
jitterlisp_primitive_macro_function_define_constant (jitterlisp_object cdr,
                                                     jitterlisp_object env)
{
  return jitterlisp_primitive_macro_function_define_internal (cdr, env, true);
}

jitterlisp_object
jitterlisp_primitive_macro_function_if (jitterlisp_object cdr,
                                        jitterlisp_object env)
{
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("if: invalid cdr");
  if (! JITTERLISP_IS_CONS(JITTERLISP_EXP_C_A_CDR (cdr)))
    jitterlisp_error_cloned ("if: invalid cddr");

  jitterlisp_object condition = JITTERLISP_EXP_C_A_CAR (cdr);
  jitterlisp_object then_branch = jitterlisp_cadr (cdr);
  jitterlisp_object else_branch_forms = jitterlisp_cddr (cdr);

  return jitterlisp_ast_make_if
            (jitterlisp_macroexpand (condition, env),
             jitterlisp_macroexpand (then_branch, env),
             jitterlisp_macroexpand_begin (else_branch_forms, env));
}

jitterlisp_object
jitterlisp_primitive_macro_function_cond (jitterlisp_object cdr,
                                          jitterlisp_object env)
{
  jitterlisp_object clauses = cdr;

  /* A no-clause cons expands to a no-form begin. */
  if (JITTERLISP_IS_EMPTY_LIST(clauses))
    return jitterlisp_macroexpand_begin (clauses, env);

  if (! JITTERLISP_IS_CONS(clauses))
    jitterlisp_error_cloned ("cond: non-list clauses");

  /* There is at least one clause.  Expand to an if with another cond in the
     else branch. */
  jitterlisp_object clause = JITTERLISP_EXP_C_A_CAR (cdr);
  if (! JITTERLISP_IS_CONS(clause))
    jitterlisp_error_cloned ("cond: non-list clause");
  jitterlisp_object condition = JITTERLISP_EXP_C_A_CAR (clause);
  jitterlisp_object clause_body = JITTERLISP_EXP_C_A_CDR (clause);
  jitterlisp_object more_clauses = JITTERLISP_EXP_C_A_CDR (clauses);

  /* I support a Scheme-style else condition in the last clause. */
  if (condition == jitterlisp_else)
    {
      /* An else-condition clause is only acceptable if there are no more clauses
         following it. */
      if (! JITTERLISP_IS_EMPTY_LIST(more_clauses))
        jitterlisp_error_cloned ("cond: else-condition clause not the last one");

      /* Treat the else condition as if it were #t. */
      condition = JITTERLISP_TRUE;
    }

  /* Expand to an if conditional, with the expansion of another cond with the
     remaining clauses in the else branch. */
  return jitterlisp_ast_make_if
            (jitterlisp_macroexpand (condition, env),
             jitterlisp_macroexpand_begin (clause_body, env),
             jitterlisp_primitive_macro_function_cond (more_clauses,
                                                       env));
}

jitterlisp_object
jitterlisp_primitive_macro_function_setb (jitterlisp_object cdr,
                                          jitterlisp_object env)
{
  // FIXME: move validation to AST construction?
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("set!: invalid cdr");
  jitterlisp_object variable = JITTERLISP_EXP_C_A_CAR (cdr);
  if (! JITTERLISP_IS_SYMBOL (variable))
    {
      jitterlisp_print_error_char_star ("About "); // FIXME: integrate with the error function
      jitterlisp_print_error (variable); printf (":\n");
      jitterlisp_error_cloned ("set!: non-symbol variable name");
    }
  jitterlisp_object forms = JITTERLISP_EXP_C_A_CDR (cdr);

  return jitterlisp_ast_make_setb
            (variable,
             jitterlisp_macroexpand_begin (forms, env));
}

jitterlisp_object
jitterlisp_primitive_macro_function_while (jitterlisp_object cdr,
                                           jitterlisp_object env)
{
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("while: invalid cdr");

  jitterlisp_object guard = JITTERLISP_EXP_C_A_CAR (cdr);
  jitterlisp_object body_forms = JITTERLISP_EXP_C_A_CDR (cdr);

  return jitterlisp_ast_make_while
            (jitterlisp_macroexpand (guard, env),
             jitterlisp_macroexpand_begin (body_forms, env));
}

jitterlisp_object
jitterlisp_primitive_macro_function_primitive (jitterlisp_object cdr,
                                               jitterlisp_object env)
{
  // FIXME: move validation to AST construction?
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("primitive: invalid cdr");
  jitterlisp_object primitive_name = jitterlisp_car (cdr);
  if (! JITTERLISP_IS_SYMBOL (primitive_name))
    {
      jitterlisp_print_error_char_star ("About "); // FIXME: integrate with the error function
      jitterlisp_print_error (primitive_name);
      jitterlisp_print_error_char_star (":\n");
      jitterlisp_error_cloned ("primitive: non-symbol primitive name");
    }
  jitterlisp_object primitive
    = JITTERLISP_SYMBOL_DECODE(primitive_name)->global_value;
  if (! JITTERLISP_IS_PRIMITIVE (primitive))
    {
      jitterlisp_print_error_char_star ("About "); // FIXME: integrate with the error function
      jitterlisp_print_error (primitive_name);
      jitterlisp_print_error_char_star (":\n");
      jitterlisp_error_cloned ("primitive: non primitive name");
    }
  jitterlisp_object primitive_actuals = jitterlisp_cdr (cdr);
  size_t actual_arity = jitterlisp_length (primitive_actuals);
  if (actual_arity != JITTERLISP_PRIMITIVE_DECODE(primitive)->in_arity)
    jitterlisp_error_cloned ("primitive: invalid arity");

  return jitterlisp_ast_make_primitive
            (primitive,
             jitterlisp_macroexpand_multiple (primitive_actuals, env));
}

jitterlisp_object
jitterlisp_primitive_macro_function_call (jitterlisp_object cdr,
                                          jitterlisp_object env)
{
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("call: invalid cdr");
  jitterlisp_object operator = JITTERLISP_EXP_C_A_CAR (cdr);
  jitterlisp_object operands = JITTERLISP_EXP_C_A_CDR (cdr);
  return jitterlisp_ast_make_call (jitterlisp_macroexpand (operator, env),
                                   jitterlisp_macroexpand_multiple (operands,
                                                                    env));
}

jitterlisp_object
jitterlisp_primitive_macro_function_lambda (jitterlisp_object cdr,
                                            jitterlisp_object env)
{
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("lambda: invalid cdr");
  jitterlisp_object formals = JITTERLISP_EXP_C_A_CAR(cdr);
  jitterlisp_object body_forms = JITTERLISP_EXP_C_A_CDR(cdr);
  return jitterlisp_ast_make_lambda
            (formals,
             jitterlisp_macroexpand_begin (body_forms, env));
}

/* Given a list encoding let bindings return the list of bound symbols in order,
   or error out if the binding list is not well-formed.
   Rationale: this is a slightly inefficient, non-tail recursive way of scanning
   let bindings, but is simpler to handle than building a list front-to-back
   with pointers in code that is supposed to support exact pointer finding in
   the future: the alternative would require pointers to list elements to be
   filled, or reversing lists. */
static jitterlisp_object
jitterlisp_bindings_get_bound_variables (jitterlisp_object bindings)
{
  if (JITTERLISP_IS_EMPTY_LIST(bindings))
    return JITTERLISP_EMPTY_LIST;
  else if (! JITTERLISP_IS_CONS(bindings))
    jitterlisp_error_cloned ("let: non-list bindings");

  jitterlisp_object binding = JITTERLISP_EXP_C_A_CAR(bindings);
  if (! JITTERLISP_IS_CONS(binding))
    {
      jitterlisp_print_error_char_star ("About "); // FIXME: integrate with the error function
      jitterlisp_print_error (binding);
      jitterlisp_error_cloned ("let: non-list binding");
    }
  jitterlisp_object bound_symbol = JITTERLISP_EXP_C_A_CAR(binding);
  if (! JITTERLISP_IS_SYMBOL(bound_symbol))
    {
      jitterlisp_print_error_char_star ("About "); // FIXME: integrate with the error function
      jitterlisp_print_error (bound_symbol);
      jitterlisp_error_cloned ("let: non-symbol bound variable");
    }

  jitterlisp_object cdr_bindings = JITTERLISP_EXP_C_A_CDR (bindings);
  jitterlisp_object cdr_bound_variables
    = jitterlisp_bindings_get_bound_variables (cdr_bindings);
  return jitterlisp_cons (bound_symbol, cdr_bound_variables);
}

/* Given a list encoding let bindings and a non-global environment return the
   list of bound ASTs in order, already expanded.  This assumes that
   jitterlisp_bindings_get_bound_variables has been called before on the same
   bindings, so doesn't need to check that the bindings are well-formed.
   Rationale: see the comment for jitterlisp_bindings_get_bound_variables . */
static jitterlisp_object
jitterlisp_bindings_get_bound_asts (jitterlisp_object bindings,
                                    jitterlisp_object env)
{
  if (JITTERLISP_IS_EMPTY_LIST(bindings))
    return JITTERLISP_EMPTY_LIST;

  jitterlisp_object binding = JITTERLISP_EXP_C_A_CAR(bindings);
  jitterlisp_object bound_forms = JITTERLISP_EXP_C_A_CDR(binding);
  jitterlisp_object bound_ast = jitterlisp_macroexpand_begin (bound_forms, env);

  jitterlisp_object cdr_bindings = JITTERLISP_EXP_C_A_CDR (bindings);
  jitterlisp_object cdr_bound_asts
    = jitterlisp_bindings_get_bound_asts (cdr_bindings, env);
  return jitterlisp_cons (bound_ast, cdr_bound_asts);
}

/* Return a tagged AST encoding a let* having the given bound symbols with their
   respective bound_asts (assumed to be lists of the right type of elements, of
   the same length) with the given body. */
static jitterlisp_object
jitterlisp_primitive_macro_function_let_star_helper
   (jitterlisp_object bound_symbols,
    jitterlisp_object bound_asts,
    jitterlisp_object body_ast)
{
  if (JITTERLISP_IS_EMPTY_LIST(bound_symbols))
    return body_ast;
  else
    {
      jitterlisp_object inner_ast
        = jitterlisp_primitive_macro_function_let_star_helper
             (JITTERLISP_EXP_C_A_CDR(bound_symbols),
              JITTERLISP_EXP_C_A_CDR(bound_asts),
              body_ast);
      return jitterlisp_ast_make_let (JITTERLISP_EXP_C_A_CAR(bound_symbols),
                                      JITTERLISP_EXP_C_A_CAR(bound_asts),
                                      inner_ast);
    }
}

jitterlisp_object
jitterlisp_primitive_macro_function_let_star (jitterlisp_object cdr,
                                              jitterlisp_object env)
{
  /* Start by holding the binging list and the body form list in automatic C
     variables, everything unexpanded. */
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("let*: invalid cdr");
  jitterlisp_object bindings = JITTERLISP_EXP_C_A_CAR(cdr);
  jitterlisp_object body_forms = JITTERLISP_EXP_C_A_CDR(cdr);

  /* Make two separate lists, one for bound variables and another for their
     bound ASTs, by walking the binding list in order. */
  jitterlisp_object bound_symbols
    = jitterlisp_bindings_get_bound_variables (bindings);
  jitterlisp_object bound_asts
    = jitterlisp_bindings_get_bound_asts (bindings, env);

  /* Expand the body as well, and I have every component for the let AST. */
  jitterlisp_object body_ast = jitterlisp_macroexpand_begin (body_forms, env);

  /* Build the AST recursively, with one form per binding. */
  return jitterlisp_primitive_macro_function_let_star_helper (bound_symbols,
                                                              bound_asts,
                                                              body_ast);
}

jitterlisp_object
jitterlisp_primitive_macro_function_begin (jitterlisp_object cdr,
                                           jitterlisp_object env)
{
  return jitterlisp_macroexpand_begin (cdr, env);
}

jitterlisp_object
jitterlisp_primitive_macro_function_quote (jitterlisp_object cdr,
                                           jitterlisp_object env)
{
  if (! JITTERLISP_IS_CONS(cdr))
    jitterlisp_error_cloned ("quote: invalid cdr");
  if (! JITTERLISP_IS_EMPTY_LIST(JITTERLISP_EXP_C_A_CDR(cdr)))
    jitterlisp_error_cloned ("quote: invalid cddr");

  jitterlisp_object literal = JITTERLISP_EXP_C_A_CAR(cdr);
  return jitterlisp_ast_make_literal (literal);
}

jitterlisp_object
jitterlisp_primitive_macro_function_undefined (jitterlisp_object cdr,
                                               jitterlisp_object env)
{
  if (! JITTERLISP_IS_EMPTY_LIST(cdr))
    jitterlisp_error_cloned ("undefined: non-null cdr");

  return jitterlisp_ast_make_literal (JITTERLISP_UNDEFINED);
}

/* Return a new macro closure for the given body in the given environment.
   This returns a macro object, not an AST. */
static jitterlisp_object
jitterlisp_make_macro (jitterlisp_object body_forms, jitterlisp_object env)
{
  /* Expand tehe body in the (expansion time) environment. */
  jitterlisp_object body_ast = jitterlisp_macroexpand_begin (body_forms, env);

  /* Make a macro object.  Notice that the formals are not actually used
     (non-primitive macros always have exactly one formal with the fixed name
     low_level_macro_args , therefore there is no need to store it explicity):
     to avoid the formals being used by mistake I define them as a non-list. */
  jitterlisp_object macro_formals = JITTERLISP_NOTHING;
  jitterlisp_object macro;
  JITTERLISP_NON_PRIMITIVE_MACRO_(macro, env, macro_formals, body_ast);

  /* Return the macro. */
  return macro;
}

jitterlisp_object
jitterlisp_primitive_macro_function_low_level_macro (jitterlisp_object cdr,
                                                     jitterlisp_object env)
{
  /* Return am AST evaluating to the macro.  Since the macro object is built
     now, at expansion time, the AST will simply be a literal. */
  return jitterlisp_ast_make_literal (jitterlisp_make_macro (cdr, env));
}
