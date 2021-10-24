/* Jittery structured language example: stack-based code generator.

   Copyright (C) 2017, 2018, 2019, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of the Jitter structured-language example, distributed
   along with Jitter under the same license.

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


#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <jitter/jitter.h>
#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>

#include "structuredvm-vm.h"
#include "structured-syntax.h"
#include "structured-code-generator.h"


/* Code generation by recursion over an AST.
 * ************************************************************************** */

/* Add code to translate the pointed expression AST to the pointed Jittery
   routine, using the pointed static environment to be looked up and updated.
   We need one forward-declaration here, as structured_translate_expression and
   structured_translate_primitive are mutually recursive. */
static void
structured_translate_expression (structuredvm_routine vmp,
                                 struct structured_expression *e,
                                 struct structured_static_environment *env);

/* Add code to translate a primitive-case expression AST to the pointed Jittery
   routine, using the pointed static environment to be looked up and updated.
   The primitive is given by its case and its two operands, of which the second
   is ignored if the primitive is unary. */
static void
structured_translate_primitive (structuredvm_routine vmp,
                                enum structured_primitive case_,
                                struct structured_expression *operand_0,
                                struct structured_expression *operand_1,
                                struct structured_static_environment *env)
{
  /* Emit nothing for nullary primitive, code to evaluate the first operand for
     unary primitives, or code to evaluate the first and then second operand for
     binary primitives. */
  switch (case_)
    {
    case structured_primitive_input:
      break;
    case structured_primitive_unary_minus:
    case structured_primitive_logical_not:
    case structured_primitive_is_nonzero:
      structured_translate_expression (vmp, operand_0, env);
      break;
    default: /* The other primitives are binary. */
      structured_translate_expression (vmp, operand_0, env);
      structured_translate_expression (vmp, operand_1, env);
    }

  /* Emit one VM instruction actually implementing the primitive, working over
     its two actual arguments already evaluated in the top one or two stack
     elements. */
  switch (case_)
    {
    case structured_primitive_plus:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, plus_mstack); break;
    case structured_primitive_minus:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, minus_mstack); break;
    case structured_primitive_times:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, times_mstack); break;
    case structured_primitive_divided:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, divided_mstack); break;
    case structured_primitive_remainder:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, remainder_mstack); break;
    case structured_primitive_unary_minus:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, uminus_mstack); break;
    case structured_primitive_equal:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, equal_mstack); break;
    case structured_primitive_different:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, different_mstack); break;
    case structured_primitive_less:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, less_mstack); break;
    case structured_primitive_less_or_equal:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, lessorequal_mstack); break;
    case structured_primitive_greater:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, greater_mstack); break;
    case structured_primitive_greater_or_equal:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, greaterorequal_mstack); break;
    case structured_primitive_logical_not:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, logicalnot_mstack); break;
    case structured_primitive_is_nonzero:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, isnonzero_mstack); break;
    case structured_primitive_input:
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, input_mstack); break;
    default:
      jitter_fatal ("structured_translate_primitive: unexpected primitive %i",
                    (int) case_);
    }
}

/* Emit instruction to save the given number of registers, starting from index
   0, consecutively. */
static void
structured_emit_save_registers (structuredvm_routine vmp,
                                size_t register_no)
{
  int i;
  for (i = 0; i < register_no; i ++)
    {
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, push_mstack);
      STRUCTUREDVM_ROUTINE_APPEND_REGISTER_PARAMETER(vmp, r, i);
    }
}

/* Emit instruction to restore the given number of registers, starting from
   index 0, consecutively; the generated code pops register values off the stack
   in reversed order compared to the code generated by
   structured_emit_save_registers .  Notice that the top is not popped, because
   that is supposed to contain the procedure result. */
static void
structured_emit_restore_registers (structuredvm_routine vmp,
                                   size_t register_no)
{
  int i;
  for (i = register_no - 1; i >= 0; i --)
    {
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, underpop_mstack);
      STRUCTUREDVM_ROUTINE_APPEND_REGISTER_PARAMETER(vmp, r, i);
    }
}

static void
structured_translate_call (structuredvm_routine vmp,
                           char *callee_name,
                           struct structured_expression **actuals,
                           size_t actual_no,
                           bool drop_result,
                           struct structured_static_environment *env)
{
  /* Check arity. */
  size_t formal_no
    = structured_static_environment_lookup_procedure_arity (env, callee_name);
  if (actual_no != formal_no)
    jitter_fatal ("%s called with %i arguments (expected %i)",
                  callee_name, (int) actual_no, (int) formal_no);

  size_t used_register_no
    = structured_static_environment_used_register_no (env);

  /* Push instructions to save used registers.  It is correct to do it now
     since in this language evaluating actuals for a call can not change
     the values of locals. */
  structured_emit_save_registers (vmp, used_register_no);

  /* Push actual values. */
  int i;
  for (i = 0; i < actual_no; i ++)
    structured_translate_expression (vmp, actuals [i], env);

  /* Call. */
  structuredvm_label procedure_label
    = structured_static_environment_lookup_procedure (env, callee_name);
  STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, call);
  structuredvm_routine_append_label_parameter (vmp, procedure_label);

  /* Restore registers. */
  structured_emit_restore_registers (vmp, used_register_no);

  /* Drop the procedure result off the stack, if that was requested.  This
     behaviour is correct for call *statements*, which ignore any result. */
  if (drop_result)
    STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, drop_mstack);
}

static void
structured_translate_expression (structuredvm_routine vmp,
                                 struct structured_expression *e,
                                 struct structured_static_environment *env)
{
  switch (e->case_)
    {
    case structured_expression_case_undefined:
      {
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, push_munspecified_mstack);
        break;
      }
    case structured_expression_case_literal:
      {
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, push_mstack);
        structuredvm_routine_append_signed_literal_parameter (vmp, e->literal);
        break;
      }
    case structured_expression_case_variable:
      {
        structured_register_index idx
          = structured_static_environment_lookup_variable (env, e->variable);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, push_mstack);
        STRUCTUREDVM_ROUTINE_APPEND_REGISTER_PARAMETER(vmp, r, idx);
        break;
      }
    case structured_expression_case_if_then_else:
      {
        structuredvm_label before_else = structuredvm_fresh_label (vmp);
        structuredvm_label after_else = structuredvm_fresh_label (vmp);
        structured_translate_expression (vmp, e->if_then_else_condition, env);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, bf_mstack);
        structuredvm_routine_append_label_parameter (vmp, before_else);
        structured_translate_expression (vmp, e->if_then_else_then_branch, env);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, b);
        structuredvm_routine_append_label_parameter (vmp, after_else);
        structuredvm_routine_append_label (vmp, before_else);
        structured_translate_expression (vmp, e->if_then_else_else_branch, env);
        structuredvm_routine_append_label (vmp, after_else);
        break;
      }
    case structured_expression_case_primitive:
      {
        structured_translate_primitive (vmp, e->primitive,
                                        e->primitive_operand_0,
                                        e->primitive_operand_1, env);
        break;
      }
    case structured_expression_case_call:
      {
        structured_translate_call (vmp, e->callee, e->actuals, e->actual_no,
                                   false, env);
        break;
      }
    default:
      jitter_fatal ("invalid expression case (bug): %i", (int) e->case_);
    }
}

/* Add code to translate the pointed statement AST to the pointed Jittery
   routine, using the pointed static environment to be looked up and updated. */
static void
structured_translate_statement (structuredvm_routine vmp,
                                struct structured_statement *s,
                                struct structured_static_environment *env)
{
  switch (s->case_)
    {
    case structured_statement_case_skip:
      {
        break;
      }
    case structured_statement_case_block:
      {
        structured_static_environment_bind_variable (env, s->block_variable);
        structured_translate_statement (vmp, s->block_body, env);
        structured_static_environment_unbind_variable (env, s->block_variable);
        break;
      }
    case structured_statement_case_assignment:
      {
        structured_register_index idx
          = structured_static_environment_lookup_variable
               (env, s->assignment_variable);
        structured_translate_expression (vmp, s->assignment_expression, env);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, pop_mstack);
        STRUCTUREDVM_ROUTINE_APPEND_REGISTER_PARAMETER(vmp, r, idx);
        break;
      }
    case structured_statement_case_print:
      {
        structured_translate_expression (vmp, s->print_expression, env);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, print_mstack);
        break;
      }
    case structured_statement_case_sequence:
      {
        structured_translate_statement (vmp, s->sequence_statement_0, env);
        structured_translate_statement (vmp, s->sequence_statement_1, env);
        break;
      }
    case structured_statement_case_if_then_else:
      {
        structuredvm_label before_else = structuredvm_fresh_label (vmp);
        structuredvm_label after_else = structuredvm_fresh_label (vmp);
        structured_translate_expression (vmp, s->if_then_else_condition, env);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, bf_mstack);
        structuredvm_routine_append_label_parameter (vmp, before_else);
        structured_translate_statement (vmp, s->if_then_else_then_branch, env);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, b);
        structuredvm_routine_append_label_parameter (vmp, after_else);
        structuredvm_routine_append_label (vmp, before_else);
        structured_translate_statement (vmp, s->if_then_else_else_branch, env);
        structuredvm_routine_append_label (vmp, after_else);
        break;
      }
    case structured_statement_case_repeat_until:
      {
        structuredvm_label before_body = structuredvm_fresh_label (vmp);
        structuredvm_routine_append_label (vmp, before_body);
        structured_translate_statement (vmp, s->repeat_until_body, env);
        structured_translate_expression (vmp, s->repeat_until_guard, env);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, bf_mstack);
        structuredvm_routine_append_label_parameter (vmp, before_body);
        break;
      }
    case structured_statement_case_return:
      {
        /* Compile the expression and generate a return instruction.  The return
           address will be at the undertop, right below the result. */
        structured_translate_expression (vmp, s->return_result, env);
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, return_mto_mundertop);
        break;
      }
    case structured_statement_case_call:
      {
        structured_translate_call (vmp, s->callee, s->actuals, s->actual_no,
                                   true, env);
        break;
      }
    default:
      jitter_fatal ("invalid statement case (bug): %i", (int) s->case_);
    }
}

/* Generate two labels, one for non-tail calls and another for tail calls,
   associate them to the procedure name in the environment and save them.
   These will be used when procedures are actually compiled, elsewhere.
   It is useful to be able to get these labels before compilation, so that
   procedures can call one another in any order without declarations. */
static void
structured_bind_procedure_labels (structuredvm_routine vmp,
                                  struct structured_procedure *p,
                                  struct structured_static_environment *env)
{
  structuredvm_label label = structuredvm_fresh_label (vmp);
  structuredvm_label label_tail_recursive = structuredvm_fresh_label (vmp);
  structured_static_environment_bind_procedure (env, p->procedure_name,
                                                label, label_tail_recursive,
                                                p->formal_no);

}

/* Emit code for the pointed procedure. */
static void
structured_translate_procedure (structuredvm_routine vmp,
                                struct structured_procedure *p,
                                struct structured_static_environment *env)
{
  /* Obtain the two labels, one for non-tail calls and another for tail calls,
     as generated in structured_bind_procedure_labels.  Emit the prolog
     including the two labels at their appropriate program points. */
  structuredvm_label label
    = structured_static_environment_lookup_procedure (env, p->procedure_name);
  structuredvm_label label_tail_recursive
    = structured_static_environment_lookup_procedure_tail (env,
                                                           p->procedure_name);
  structuredvm_routine_append_label (vmp, label);
  STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, procedure_mprolog);
  structuredvm_routine_append_label (vmp, label_tail_recursive);

  /* Pop actuals into registers, backwards. */
  int i;
  for (i = p->formal_no - 1; i >= 0; i --)
    {
      char *name = p->formals [i];
      structured_register_index register_index
        = structured_static_environment_bind_variable (env, name);
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, underpop_mstack);
      STRUCTUREDVM_ROUTINE_APPEND_REGISTER_PARAMETER(vmp, r, register_index);
    }

  structured_translate_statement (vmp, p->body, env);

  /* Add an implicit return, which may or may not be reachable. */
  STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, push_munspecified_mstack);
  STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, return_mto_mundertop);

  /* Unbind formals. */
  for (i = 0; i < p->formal_no; i ++)
    {
      char *name = p->formals [i];
      structured_static_environment_unbind_variable (env, name);
    }
}

/* Add code to translate the pointed program AST to the pointed Jittery
   routine. */
static void
structured_translate_program (structuredvm_routine vmp,
                              struct structured_program *p)
{
  struct structured_static_environment *env
    = structured_static_environment_make ();

  /* Jump to a label right right before the main statement. */
  structuredvm_label main_statement = structuredvm_fresh_label (vmp);
  STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, b);
  structuredvm_routine_append_label_parameter (vmp, main_statement);

  /* First make every procedure name globally visible, to allow for mutual call;
     then translate every procedure. */
  int i;
  for (i = 0; i < p->procedure_no; i ++)
    structured_bind_procedure_labels (vmp, p->procedures [i], env);
  for (i = 0; i < p->procedure_no; i ++)
    structured_translate_procedure (vmp, p->procedures [i], env);

  /* Translate the main statement. */
  structuredvm_routine_append_label (vmp, main_statement);
  structured_translate_statement (vmp, p->main_statement, env);

  structured_static_environment_destroy (env);
}




/* Entry point: translate an AST program to a VM routine.
 * ************************************************************************** */

void
structured_translate_program_stack (structuredvm_routine vmp,
                                    struct structured_program *p)
{
  /* Translate the AST pointed by p into *vmp.  This of course works by
     recursion. */
  structured_translate_program (vmp, p);
}
