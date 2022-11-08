/* JitterLisp: ASTs.

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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#include "jitterlisp-ast.h"

#include "jitterlisp-sexpression.h"
#include "jitterlisp-allocator.h"

#include "jitterlisp.h"




/* Utility.
 * ************************************************************************** */





/* AST allocation utility.
 * ************************************************************************** */

/* Expand to an unprotected sequence of C declarations and statements declaring
   an automatic variable of type struct jitterlisp_ast * named unencoded_res,
   its encoded counterpart res, and a subs pointer pointer to the subs field
   of unencoded res; heap-allocating unencoded_res, initializing its case_ and
   sub_no fields to the given values.  The flexible array member content is not
   initialized. */
#define JITTERLISP_MAKE_LOCALS_(_jitterlisp_case, _jitterlisp_sub_no)  \
  const size_t sub_no = (_jitterlisp_sub_no);                          \
  struct jitterlisp_ast *unencoded_res                                 \
    = JITTERLISP_AST_MAKE_UNINITIALIZED_UNENCODED(sub_no);             \
  jitterlisp_object res = JITTERLISP_AST_ENCODE(unencoded_res);        \
  unencoded_res->case_ = (_jitterlisp_case);                           \
  unencoded_res->sub_no = (sub_no);                                    \
  jitterlisp_object * const subs __attribute__ ((unused))              \
    = unencoded_res->subs




/* AST high-level allocation.
 * ************************************************************************** */

jitterlisp_object
jitterlisp_ast_make_literal (jitterlisp_object value)
{
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_literal, 1);
  subs [0] = value;
  return res;
}

jitterlisp_object
jitterlisp_ast_make_variable (jitterlisp_object symbol)
{
  jitterlisp_validate_symbol (symbol);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_variable, 1);
  subs [0] = symbol;
  return res;
}

jitterlisp_object
jitterlisp_ast_make_define (jitterlisp_object symbol, jitterlisp_object ast)
{
  jitterlisp_validate_symbol (symbol);
  jitterlisp_validate_ast (ast);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_define, 2);
  subs [0] = symbol;
  subs [1] = ast;
  return res;
}

jitterlisp_object
jitterlisp_ast_make_if (jitterlisp_object condition_ast,
                        jitterlisp_object then_ast,
                        jitterlisp_object else_ast)
{
  jitterlisp_validate_ast (condition_ast);
  jitterlisp_validate_ast (then_ast);
  jitterlisp_validate_ast (else_ast);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_if, 3);
  subs [0] = condition_ast;
  subs [1] = then_ast;
  subs [2] = else_ast;
  return res;
}

jitterlisp_object
jitterlisp_ast_make_setb (jitterlisp_object symbol, jitterlisp_object ast)
{
  jitterlisp_validate_symbol (symbol);
  jitterlisp_validate_ast (ast);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_setb, 2);
  subs [0] = symbol;
  subs [1] = ast;
  return res;
}

jitterlisp_object
jitterlisp_ast_make_while (jitterlisp_object condition_ast,
                           jitterlisp_object body_ast)
{
  jitterlisp_validate_ast (condition_ast);
  jitterlisp_validate_ast (body_ast);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_while, 2);
  subs [0] = condition_ast;
  subs [1] = body_ast;
  return res;
}

jitterlisp_object
jitterlisp_ast_make_primitive (jitterlisp_object primitive_object,
                               jitterlisp_object actual_asts)
{
  jitterlisp_validate_primitive (primitive_object);
  jitterlisp_validate_asts (actual_asts);
  size_t actual_no = jitterlisp_length (actual_asts);
  struct jitterlisp_primitive *p
    = JITTERLISP_PRIMITIVE_DECODE(primitive_object);
  if (p->in_arity != actual_no)
    jitterlisp_error_cloned ("jitterlisp_ast_make_primitive: invalid in-arity");
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_primitive, actual_no + 1);
  subs [0] = primitive_object;
  int i;
  for (i = 0; i < actual_no; i ++)
    {
      subs [1 + i] = JITTERLISP_EXP_C_A_CAR(actual_asts);
      actual_asts = JITTERLISP_EXP_C_A_CDR(actual_asts);
    }
  return res;
}

jitterlisp_object
jitterlisp_ast_make_call (jitterlisp_object operator_ast,
                          jitterlisp_object actual_asts)
{
  jitterlisp_validate_ast (operator_ast);
  jitterlisp_validate_asts (actual_asts);
  size_t actual_no = jitterlisp_length (actual_asts);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_call, actual_no + 1);
  subs [0] = operator_ast;
  int i;
  for (i = 0; i < actual_no; i ++)
    {
      subs [1 + i] = JITTERLISP_EXP_C_A_CAR(actual_asts);
      actual_asts = JITTERLISP_EXP_C_A_CDR(actual_asts);
    }
  return res;
}

jitterlisp_object
jitterlisp_ast_make_lambda (jitterlisp_object formal_symbols,
                            jitterlisp_object body_ast)
{
  jitterlisp_validate_distinct_symbols (formal_symbols);
  jitterlisp_validate_ast (body_ast);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_lambda, 2);
  subs [0] = formal_symbols;
  subs [1] = body_ast;
  return res;
}

jitterlisp_object
jitterlisp_ast_make_let (jitterlisp_object bound_symbol,
                         jitterlisp_object bound_ast,
                         jitterlisp_object body_ast)
{
  jitterlisp_validate_symbol (bound_symbol);
  jitterlisp_validate_ast (bound_ast);
  jitterlisp_validate_ast (body_ast);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_let, 3);
  subs [0] = bound_symbol;
  subs [1] = bound_ast;
  subs [2] = body_ast;
  return res;
}

jitterlisp_object
jitterlisp_ast_make_sequence (jitterlisp_object ast_0,
                              jitterlisp_object ast_1)
{
  jitterlisp_validate_ast (ast_0);
  jitterlisp_validate_ast (ast_1);
  JITTERLISP_MAKE_LOCALS_(jitterlisp_ast_case_sequence, 2);
  subs [0] = ast_0;
  subs [1] = ast_1;
  return res;
}




/* AST accessors.
 * ************************************************************************** */

jitterlisp_object
jitterlisp_ast_operands (jitterlisp_object ast)
{
  jitterlisp_validate_ast (ast);
  struct jitterlisp_ast *unencoded_ast = JITTERLISP_AST_DECODE(ast);
  if (unencoded_ast->case_ != jitterlisp_ast_case_call
      && unencoded_ast->case_ != jitterlisp_ast_case_primitive)
    jitterlisp_error_cloned ("jitterlisp_ast_operands: non-primitive non-call "
                             "case");

  /* Build the result list, starting the last element. */
  jitterlisp_object res = JITTERLISP_EMPTY_LIST;
  int i;
  for (i = unencoded_ast->sub_no - 2; i >= 0; i --)
    res = jitterlisp_cons (unencoded_ast->subs [1 + i], res);

  return res;
}
