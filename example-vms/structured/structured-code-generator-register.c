#include <assert.h>
/* Jittery structured language example: register-based code generator.

   Copyright (C) 2017, 2018, 2019, 2021 Luca Saiu
   Copyright (C) 2021 pEp Foundation
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


/* Locations.
 * ************************************************************************** */

/* The kind of place where a datum is, as known at compile time. */
enum structured_location_case
  {
    /* The object can go anywhere.  This case is used when supplying a location
       to the code generator for compiling an expression, but is never given back
       as the location of code which has been compiled.  The location of compiled
       code will be specific, and use one of the other cases. */
    structured_location_case_anywhere,

    /* The object can be anywhere, except that it cannot be a constant.  This is
       needed to compile branches of conditional epxressions, which must resolve
       to the same location which cannot be, in general, *the same* constant. */
    structured_location_case_nonconstant,

    /* The object is in a temporary. */
    structured_location_case_temporary,

    /* The object is in a register. */
    structured_location_case_register,

    /* The object is a literal, known at compile time. */
    structured_location_case_constant
  };

/* Where a datum is, as known at compile time. */
struct structured_location
{
  /* The case for this location.  See the comments within the definition of enum
     structured_location_case. */
  enum structured_location_case case_;

  /* Other data complementing the location, as needed for some cases. */
  /*
  union // Conceptually, this should be a union...
  {
    struct // ...Containing an anonymous struct.  However old compilers will
           // not support initialisers setting anonymous union and struct fields.
    {
    */
      /* A temporary identifier, used when the case is temporary. */
      structured_temporary temporary;

      /* A register index, used when the case is register or temporary. */
      structured_register_index register_index;
    /*};*/

    /* The value of the literal, only used when the case is literal. */
    jitter_int constant_value;
  /*
  };
  */
};

/* A C constant expression suitable for initializing a struct
   structured_location object to be a location with an anywhere case. */
#define STRUCTURED_LOCATION_ANYWHERE     \
  { .case_ = structured_location_case_anywhere }

/* A C constant expression suitable for initializing a struct
   structured_location object to be a location with a non-constant case. */
#define STRUCTURED_LOCATION_NONCONSTANT     \
  { .case_ = structured_location_case_nonconstant }

/* A C constant expression suitable for initializing a struct
   structured_location object to be a register, with the given index. */
#define STRUCTURED_LOCATION_REGISTER(register_idx)                         \
  { .case_ = structured_location_case_register, .register_index = (register_idx) }

/* If the pointed location, active on the pointed environment, is anywhere
   or nonconstant, change it to be a temporary.  This serves to be sure that
   the location can be used as an operand after the call. */
static void
structured_conctetize_location (struct structured_static_environment *env,
                                struct structured_location *l)
{
  if (l->case_ == structured_location_case_anywhere
      || l->case_ == structured_location_case_nonconstant)
    {
      l->case_ = structured_location_case_temporary;
      l->temporary = structured_static_environment_fresh_temporary (env);
      l->register_index
        = structured_static_environment_bind_temporary (env, l->temporary);
    }
}

/* Mark the fact that the pointed location has been used.  This does nothing if
   the location is anything but a temporary.  If the location is a temporary,
   unbind it.
   This must be called to ensure that temporaries are freed in the correct
   order, as per the constraints explaind in structured-code-generator.h . */
static void
structured_consume_location (struct structured_static_environment *env,
                             struct structured_location *l)
{
  if (l->case_ == structured_location_case_temporary)
    {
#ifdef DEBUG
      fprintf (stderr, "? Consuming a location: temporary %i at %%r%i\n",
               (int) l->temporary, (int) l->register_index);
#endif // #ifdef DEBUG
      structured_static_environment_unbind_temporary (env, l->temporary);
    }
}

/* Append the content of the pointed location as an instruction parameter, in
   the pointed VM routine. */
static void
structured_emit_operand (struct structuredvm_mutable_routine *vmp,
                         struct structured_static_environment *env,
                         struct structured_location *l)
{
  structured_conctetize_location (env, l);

  switch (l->case_)
    {
    /* The following two cases are impossible if structured_conctetize_location
       has done its job. */
    case structured_location_case_anywhere:
      jitter_fatal ("invalid instruction operand: anywhere (impossible)");
    case structured_location_case_nonconstant:
      jitter_fatal ("invalid instruction operand: nonconstant (impossible)");

    case structured_location_case_register:
    case structured_location_case_temporary:
      STRUCTUREDVM_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER (vmp, r, l->register_index);
      break;

    case structured_location_case_constant:
      structuredvm_mutable_routine_append_signed_literal_parameter (vmp, l->constant_value);
      break;

    default:
      jitter_fatal ("invalid instruction operand location: unexpected (bug): %i",
                    (int) l->case_);
    };
}

/* Emit code translating a literal expression with the given literal value.  The
   result of the expression will be stored, in emitted code, in the required
   location, updated here if its case is "anywhere". */
static void
structured_translate_expression_literal (struct structuredvm_mutable_routine *vmp,
                                         struct structured_location *rl,
                                         jitter_int literal,
                                         struct structured_static_environment
                                         *env)
{
  switch (rl->case_)
    {
    case structured_location_case_anywhere:
      rl->case_ = structured_location_case_constant;
      rl->constant_value = literal;
      break;

    case structured_location_case_nonconstant:
      rl->case_ = structured_location_case_temporary;
      rl->temporary = structured_static_environment_fresh_temporary (env);
      rl->register_index
        = structured_static_environment_bind_temporary (env, rl->temporary);
      /* Fall thru: at this point the literal literal needs to be copied into
         the register rl->register_index , just like for the following two
         cases. */

    case structured_location_case_register:
    case structured_location_case_temporary:
      structuredvm_mutable_routine_append_instruction_name (vmp, "mov");
      structuredvm_mutable_routine_append_signed_literal_parameter (vmp, literal);
      STRUCTUREDVM_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER (vmp, r, rl->register_index);
      break;

    case structured_location_case_constant:
      jitter_fatal ("unexpected expression result location: constant");

    default:
      jitter_fatal ("invalid expression result location: unexpected (bug): %i",
                    (int) rl->case_);
    };
}

/* Emit code translating a variable expression whose value is held in a
   register with the given index.  The result of the expression will be stored,
   in emitted code, in the pointed location, updated here if needed to become
   specific when it is structured_location_case_anywhere or
   structured_location_case_nonconstant at entry. */
static void
structured_translate_expression_variable (struct structuredvm_mutable_routine *vmp,
                                          struct structured_location *rl,
                                          structured_register_index ri)
{
  switch (rl->case_)
    {
    case structured_location_case_anywhere:
    case structured_location_case_nonconstant:
      rl->case_ = structured_location_case_register;
      rl->register_index = ri;
      break;

    case structured_location_case_register:
      /* Generate nothing if the assignment is from a variable to itself. */
      if (ri != rl->register_index)
        {
          structuredvm_mutable_routine_append_instruction_name (vmp, "mov");
          STRUCTUREDVM_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER (vmp, r, ri);
          STRUCTUREDVM_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER (vmp, r, rl->register_index);
        }
      break;

    case structured_location_case_constant:
      jitter_fatal ("unexpected variable expression result location: constant");

    case structured_location_case_temporary:
      {
        /* This can happen in expressions such as
             if EXP then 3 else x end
           where either a non-variable or a variable is to be stored
           in a temporary. */
        structuredvm_mutable_routine_append_instruction_name (vmp, "mov");
        STRUCTUREDVM_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER (vmp, r, ri);
        STRUCTUREDVM_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER (vmp, r, rl->register_index);
        break;
      }

    default:
      jitter_fatal ("unexpected variable expression result location: unexpected (bug): %i",
                    (int) rl->case_);
    };
}

/* Forward-declaration. */
static void
structured_translate_expression (struct structuredvm_mutable_routine *vmp,
                                 struct structured_location *rl,
                                 struct structured_expression *e,
                                 struct structured_static_environment *env,
                                 bool tail);


/* Emit one opcode, without the operands, for the given non-conditional
   primitive. */
static void
structured_translate_non_conditional_primitive_opcode
   (struct structuredvm_mutable_routine *vmp,
    enum structured_primitive case_)
{
  switch (case_)
    {
    case structured_primitive_plus:
      structuredvm_mutable_routine_append_instruction_name(vmp, "plus"); break;
    case structured_primitive_minus:
      structuredvm_mutable_routine_append_instruction_name(vmp, "minus"); break;
    case structured_primitive_times:
      structuredvm_mutable_routine_append_instruction_name(vmp, "times"); break;
    case structured_primitive_divided:
      structuredvm_mutable_routine_append_instruction_name(vmp, "divided"); break;
    case structured_primitive_remainder:
      structuredvm_mutable_routine_append_instruction_name(vmp, "remainder"); break;
    case structured_primitive_unary_minus:
      structuredvm_mutable_routine_append_instruction_name(vmp, "uminus"); break;
    case structured_primitive_input:
      structuredvm_mutable_routine_append_instruction_name(vmp, "input"); break;
    default:
      jitter_fatal ("invalid primitive case: unexpected (bug): %i",
                    (int) case_);
    }
}

/* Forward-declaration. */
static void
structured_translate_conditional (struct structuredvm_mutable_routine *vmp,
                                  struct structured_expression *e,
                                  structuredvm_label label,
                                  bool branch_on_true,
                                  struct structured_static_environment *env);

/* Forward-declaration. */
static void
structured_translate_conditional_primitive
   (struct structuredvm_mutable_routine *vmp,
    enum structured_primitive case_,
    struct structured_expression *operand_0,
    struct structured_expression *operand_1,
    structuredvm_label label,
    bool branch_on_true,
    struct structured_static_environment *env);


/* Emit code translating a primitive expression with the given case and
   operands, using the pointed static environment to be looked up and updated.
   The result of the expression will be stored, in emitted code, in the required
   location, updated here if its case is "anywhere".
   Here the primitive must be non-conditional: conditional primitives are
   compiled differently, by
   structured_translate_expression_conditional_primitive when they have to
   materialize a result, and directly by structured_translate_conditional when
   they are used for branching. */
static void
structured_translate_expression_non_conditional_primitive
   (struct structuredvm_mutable_routine *vmp,
    struct structured_location *rl,
    enum structured_primitive case_,
    struct structured_expression *operand_0,
    struct structured_expression *operand_1,
    struct structured_static_environment *env)
{
  /* Translate primitive operands into locations.  Consume the locations in an
     order opposite to their initialization, to respect the LIFO constraint
     explained in structured-code-generator.h . */
  struct structured_location o0l = STRUCTURED_LOCATION_ANYWHERE;
  struct structured_location o1l = STRUCTURED_LOCATION_ANYWHERE;
  if (operand_0 != NULL)
    structured_translate_expression (vmp, &o0l, operand_0, env, false);
  if (operand_1 != NULL)
    {
      structured_translate_expression (vmp, &o1l, operand_1, env, false);
      structured_consume_location (env, & o1l);
    }
  if (operand_0 != NULL)
    structured_consume_location (env, & o0l);

  /* The result of the primitive will go to a temporary if no specific location
     was requested. */
  structured_conctetize_location (env, rl);

  /* Give the two operand results, or the one operand result, as operands of
     the primitive instruction. */
  structured_translate_non_conditional_primitive_opcode (vmp, case_);
  if (operand_0 != NULL)
    structured_emit_operand (vmp, env, & o0l);
  if (operand_1 != NULL)
    structured_emit_operand (vmp, env, & o1l);
  structured_emit_operand (vmp, env, rl);
}

/* Translate a conditional primitive used to materialize a result rather than
   for branching.  The given expression must be a primitive, with a conditional
   case. */
static void
structured_translate_expression_conditional_primitive
   (struct structuredvm_mutable_routine *vmp,
    struct structured_location *rl,
    struct structured_expression *e,
    struct structured_static_environment *env)
{
  /* Comparison primitives are not directly implemented by one VM
     instruction.  Translate this into a conditional:
       branch-unless-P o0l, o1l, $FALSE_COMPARISON
       mov 1, rl
       b $AFTER
     $FALSE_COMPARISON:
       mov 0, rl
     $AFTER: */

  /* In case the result location is still generic, we have to restrict it: here
     it is not possible (in general) to compile either of the two branch to a
     constant, without restricting the other branch to the same constant. */
  if (rl->case_ == structured_location_case_constant)
    jitter_fatal ("invalid conditional primitive location: constant (bug)");
  else if (rl->case_ == structured_location_case_anywhere
           || rl->case_ == structured_location_case_nonconstant)
    {
      rl->case_ = structured_location_case_temporary;
      rl->temporary = structured_static_environment_fresh_temporary (env);
      rl->register_index
        = structured_static_environment_bind_temporary (env, rl->temporary);
    }
  /* There is no need to update the result location in the variable case. */

  structuredvm_label false_comparison = structuredvm_fresh_label (vmp);
  structuredvm_label after = structuredvm_fresh_label (vmp);
  structured_translate_conditional (vmp, e, false_comparison, false, env);
  structuredvm_mutable_routine_append_instruction_name (vmp, "mov");
  structuredvm_mutable_routine_append_signed_literal_parameter (vmp, 1);
  structured_emit_operand (vmp, env, rl);
  structuredvm_mutable_routine_append_instruction_name (vmp, "b");
  structuredvm_mutable_routine_append_label_parameter (vmp, after);
  structuredvm_mutable_routine_append_label (vmp, false_comparison);
  structuredvm_mutable_routine_append_instruction_name (vmp, "mov");
  structuredvm_mutable_routine_append_signed_literal_parameter (vmp, 0);
  structured_emit_operand (vmp, env, rl);
  structuredvm_mutable_routine_append_label (vmp, after);
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
                           struct structured_location *rl,
                           char *callee_name,
                           struct structured_expression **actuals,
                           size_t actual_no,
                           bool drop_result,
                           struct structured_static_environment *env,
                           bool tail)
{
  /* Check arity. */
  size_t formal_no
    = structured_static_environment_lookup_procedure_arity (env, callee_name);
  if (actual_no != formal_no)
    jitter_fatal ("%s called with %i arguments (expected %i)",
                  callee_name, (int) actual_no, (int) formal_no);

  size_t used_register_no
    = structured_static_environment_used_register_no (env);

  /* If the call is non-tail generate push instructions to save used registers.
     It is correct to do it now since in this language evaluating actuals for a
     call can not change the values of locals.
     A tail-called callee will never return, so in that case we do not need to
     save or restore anything. */
  if (! tail)
    structured_emit_save_registers (vmp, used_register_no);

  /* Push actual values. */
  int i;
  for (i = 0; i < actual_no; i ++)
    {
      struct structured_location al = STRUCTURED_LOCATION_ANYWHERE;
      structured_translate_expression (vmp, & al, actuals [i], env, false);
      structured_consume_location (env, & al);
      if (tail)
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, underpush_mstack);
      else
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, push_mstack);
      structured_emit_operand (vmp, env, & al);
    }

  /* If this is a non-tail call emit a call instruction to the callee
     instruction; otherwise emit a branch instruction to the appropriate
     instruction which, with our calling conventions, is the one immediately
     following the callee instruction. */
  structuredvm_label procedure_label;
  if (tail)
    {
      procedure_label
        = structured_static_environment_lookup_procedure_tail (env, callee_name);
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, b);
      structuredvm_routine_append_label_parameter (vmp, procedure_label);
    }
  else
    {
      procedure_label
        = structured_static_environment_lookup_procedure (env, callee_name);
      STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, call);
      structuredvm_routine_append_label_parameter (vmp, procedure_label);
    }

  /* Restore registers, unless this is a tail call. */
  if (! tail)
    structured_emit_restore_registers (vmp, used_register_no);

  /* Drop the procedure result off the stack, if that was requested.  This
     behaviour is correct for call *statements*, which ignore any result. */
  if (drop_result)
    {
      /* This is a call statement, where we need to drop the result. */
      if (! tail)
        STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, drop_mstack);
    }
  else
    {
      /* This is a call expression, where we do not need to drop the result. */
      structured_conctetize_location (env, rl);
      if (! tail)
        {
          STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, pop_mstack);
          structured_emit_operand (vmp, env, rl);
        }
    }
}

/* Emit code translating the pointed expression AST to the pointed Jittery
   routine, using the pointed static environment to be looked up and updated.
   The result of the expression will be stored, in emitted code, in the required
   location, updated here if its case is "anywhere". */
static void
structured_translate_expression (struct structuredvm_mutable_routine *vmp,
                                 struct structured_location *rl,
                                 struct structured_expression *e,
                                 struct structured_static_environment *env,
                                 bool tail)
{
  switch (e->case_)
    {
    case structured_expression_case_undefined:
      /* If a location has not been decided yet then use register 0, even if it
         is already used for something else; otherwise keep whatever location
         has been decided.  Anyway do not generate any instruction to set a
         value: the value of an "undefined" expression does not matter. */
      if (rl->case_ == structured_location_case_anywhere
          || rl->case_ == structured_location_case_nonconstant)
        {
          struct structured_location good_location
            = STRUCTURED_LOCATION_REGISTER (0);
          *rl = good_location;
        }
      break;
    case structured_expression_case_literal:
      structured_translate_expression_literal (vmp, rl, e->literal, env);
      break;
    case structured_expression_case_variable:
      {
        structured_register_index idx
          = structured_static_environment_lookup_variable (env, e->variable);
        structured_translate_expression_variable (vmp, rl, idx);
        break;
      }
    case structured_expression_case_if_then_else:
      {
        /* If * rl is an anywhere location, change it to a different case: here
           * rl cannot be (in the general case) a constant.  But it can be a
           variable.  The problem is that if one of the two branches compiled to
           a constant location then if would only be possible to compile the
           other branch to the *same* constant location, which will not happen
           in any non-trivial conditional.  The result must go to *one*
           non-constant location.  It is not a problem if it goes to a variable
           or to a temporary. */
        if (rl->case_ == structured_location_case_anywhere)
          rl->case_ = structured_location_case_nonconstant;

        /* About tailness and labels, the conditional case comment in
           structured_translate_statement does not apply here. */
        structuredvm_label before_else = structuredvm_fresh_label (vmp);
        structuredvm_label after_else = structuredvm_fresh_label (vmp);
        structured_translate_conditional (vmp, e->if_then_else_condition,
                                          before_else,
                                          false,
                                          env);
        structured_translate_expression (vmp, rl, e->if_then_else_then_branch,
                                         env, tail);
        if (! tail)
          {
            structuredvm_mutable_routine_append_instruction_name (vmp, "b");
            structuredvm_mutable_routine_append_label_parameter (vmp,
                                                                 after_else);
          }
        structuredvm_mutable_routine_append_label (vmp, before_else);
        structured_translate_expression (vmp, rl, e->if_then_else_else_branch,
                                         env, tail);
        structuredvm_mutable_routine_append_label (vmp, after_else);
        break;
      }
    case structured_expression_case_primitive:
      {
        if (structured_is_comparison_primitive (e->primitive))
          structured_translate_expression_conditional_primitive (vmp, rl, e,
                                                                 env);
        else
          structured_translate_expression_non_conditional_primitive
             (vmp, rl, e->primitive, e->primitive_operand_0,
              e->primitive_operand_1, env);
        break;
      }
    case structured_expression_case_call:
      {
        structured_translate_call (vmp, rl, e->callee, e->actuals, e->actual_no,
                                   false, env, tail);
        break;
      }
    default:
      jitter_fatal ("invalid expression case (bug): %i", (int) e->case_);
    }
}

/* Given a boolean primitive case, return the name of the VM instruction
   implementing it as a conditional branch.
   Fail if conditional branching is not defined on the given primitive. */
static const char *
structured_comparison_primitive_to_instruction (enum structured_primitive p)
{
  switch (p)
    {
    case structured_primitive_equal:
      return "be";
    case structured_primitive_different:
      return "bne";
    case structured_primitive_less:
      return "bl";
    case structured_primitive_less_or_equal:
      return "ble";
    case structured_primitive_greater:
      return "bg";
    case structured_primitive_greater_or_equal:
      return "bge";
    default:
      jitter_fatal ("comparison (?) primitive not supporting branching: %i",
                    (int) p);
    }
}

/* A helper for structured_translate_conditional, defined below.  Emit code for
   a conditioanl primitive with the given case and operands, to conditionally
   branch to the pointed label according the the result of the primitive.
   Generate a branch-on-non-zero if branch_on_true is non-false; generate a
   branch-on-zero if branch_on_true is false. */
static void
structured_translate_conditional_primitive
   (struct structuredvm_mutable_routine *vmp,
    enum structured_primitive case_,
    struct structured_expression *operand_0,
    struct structured_expression *operand_1,
    structuredvm_label label,
    bool branch_on_true,
    struct structured_static_environment *env)
{
  switch (case_)
    {
    case structured_primitive_equal:
    case structured_primitive_different:
    case structured_primitive_less:
    case structured_primitive_less_or_equal:
    case structured_primitive_greater:
    case structured_primitive_greater_or_equal:
      {
        /* Translate the two operands, the ordinary way. */
        struct structured_location o0l = STRUCTURED_LOCATION_ANYWHERE;
        structured_translate_expression (vmp, &o0l, operand_0, env, false);
        struct structured_location o1l = STRUCTURED_LOCATION_ANYWHERE;
        structured_translate_expression (vmp, &o1l, operand_1, env, false);
        structured_consume_location (env, & o1l);
        structured_consume_location (env, & o0l);
          /* Generate a conditional branch, with the appropriate opcode for the
           primitive case, reversed if we need to branch on false. */
        enum structured_primitive actual_case;
        if (branch_on_true)
          actual_case = case_;
        else
          actual_case = structured_reverse_comparison_primitive (case_);
        const char *opcode
          = structured_comparison_primitive_to_instruction (actual_case);
        structuredvm_mutable_routine_append_instruction_name (vmp, opcode);
        structured_emit_operand (vmp, env, & o0l);
        structured_emit_operand (vmp, env, & o1l);
        structuredvm_mutable_routine_append_label_parameter (vmp, label);
        break;
      }
    case structured_primitive_logical_not:
      /* Translate the not subexpression as an ordinary contitional,
         simply flipping the branch_on_true condition flag. */
      structured_translate_conditional (vmp, operand_0, label,
                                        ! branch_on_true, env);
      break;
    default:
      {
        /* The primitive is not a comparison primitive producing a boolean from
           non-booleans, or a boolean-composition primitive producing a boolean
           from other booleans.  We have to translate the primitive as an
           ordinary expression, materializing the result, and then conditionally
           branch according to its value. */
        struct structured_location rl = STRUCTURED_LOCATION_ANYWHERE;
        structured_translate_expression_non_conditional_primitive
           (vmp, & rl, case_, operand_0, operand_1, env);
        structured_consume_location (env, & rl);
        if (branch_on_true)
          structuredvm_mutable_routine_append_instruction_name (vmp, "bne");
        else
          structuredvm_mutable_routine_append_instruction_name (vmp, "be");
        structured_emit_operand (vmp, env, & rl);
        structuredvm_mutable_routine_append_signed_literal_parameter (vmp, 0);
        structuredvm_mutable_routine_append_label_parameter (vmp, label);
      }
    }
}

/* Emit code for a conditional branch, generating code to find the truth value
   of the given expression and then possibly jumping the given label.  If
   branch_on_true is non-false then branch when the given condition (as an
   expression) would evaluate to a non-zero value; if branch_on_true is false
   branch when the expression would evaluate to zero.
   When the generated code does not branch, it simply falls thru. */
static void
structured_translate_conditional (struct structuredvm_mutable_routine *vmp,
                                  struct structured_expression *e,
                                  structuredvm_label label,
                                  bool branch_on_true,
                                  struct structured_static_environment *env)
{
  switch (e->case_)
    {
    case structured_expression_case_undefined:
      /* The condition is undefined, so it is correct to either branch or not;
         we arbitrarily choose not to.  We cannot really tell if this is more
         efficient than the alternative without checking the rest of the code,
         but at least there will be no runtime conditional. */
      break;
    case structured_expression_case_literal:
      {
        /* The condition is constant: generate an unconditional branch, or
           nothing. */
        if ((branch_on_true && e->literal != 0)
            || (! branch_on_true && e->literal == 0))
          {
            structuredvm_mutable_routine_append_instruction_name (vmp, "b");
            structuredvm_mutable_routine_append_label_parameter (vmp, label);
          }
        break;
      }
    case structured_expression_case_variable:
      {
        structured_register_index idx
          = structured_static_environment_lookup_variable (env, e->variable);
        if (branch_on_true)
          structuredvm_mutable_routine_append_instruction_name (vmp, "bne");
        else
          structuredvm_mutable_routine_append_instruction_name (vmp, "be");
        STRUCTUREDVM_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER (vmp, r, idx);
        structuredvm_mutable_routine_append_signed_literal_parameter (vmp, 0);
        structuredvm_mutable_routine_append_label_parameter (vmp, label);
        break;
      }
    case structured_expression_case_primitive:
      {
        structured_translate_conditional_primitive (vmp, e->primitive,
                                                    e->primitive_operand_0,
                                                    e->primitive_operand_1,
                                                    label, branch_on_true,
                                                    env);
        break;
      }
    case structured_expression_case_if_then_else:
      {
        /* Translate
              branch[-unless] (if C then T else E) $L
           into
                branch-unless C $BEFORE_ELSE
                branch[-unless] T $L
                b $AFTER_ELSE
              $BEFORE_ELSE:
                branch[-unless] E $L
              $AFTER_ELSE: */
        struct structured_expression *c = e->if_then_else_condition;
        struct structured_expression *tb = e->if_then_else_then_branch;
        struct structured_expression *eb = e->if_then_else_else_branch;
        structuredvm_label before_else = structuredvm_fresh_label (vmp);
        structuredvm_label after_else = structuredvm_fresh_label (vmp);
        structured_translate_conditional (vmp, c, before_else, false, env);
        structured_translate_conditional (vmp, tb, label, branch_on_true, env);
        structuredvm_mutable_routine_append_instruction_name (vmp, "b");
        structuredvm_mutable_routine_append_label_parameter (vmp, after_else);
        structuredvm_mutable_routine_append_label (vmp, before_else);
        structured_translate_conditional (vmp, eb, label, branch_on_true, env);
        structuredvm_mutable_routine_append_label (vmp, after_else);
        break;
      }
    default:
      jitter_fatal ("invalid (conditional) expression case: %i",
                    (int) e->case_);
    }
}

/* Emit instructions returning an undefined value.  This is meant to be used
   at the end of a tail statement when there is no explicit return. */
static void
structured_generate_return_statement (struct structuredvm_mutable_routine *vmp)
{
  STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, push_munspecified_mstack);
  STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, return_mto_mundertop);
}

/* Emit code translating the pointed statement AST to the pointed Jittery
   routine, using the pointed static environment to be looked up and updated. */
static void
structured_translate_statement (struct structuredvm_mutable_routine *vmp,
                                struct structured_statement *s,
                                struct structured_static_environment *env,
                                bool tail)
{
  switch (s->case_)
    {
    case structured_statement_case_skip:
      {
        if (tail)
          structured_generate_return_statement (vmp);
        break;
      }
    case structured_statement_case_block:
      {
        structured_static_environment_bind_variable (env, s->block_variable);
        structured_translate_statement (vmp, s->block_body, env, tail);
        structured_static_environment_unbind_variable (env, s->block_variable);
        break;
      }
    case structured_statement_case_assignment:
      {
        structured_register_index idx
          = structured_static_environment_lookup_variable
               (env, s->assignment_variable);
        struct structured_location vl = STRUCTURED_LOCATION_REGISTER (idx);
        structured_translate_expression (vmp, & vl, s->assignment_expression,
                                         env, false);
        structured_consume_location (env, & vl);
        if (tail)
          structured_generate_return_statement (vmp);
        break;
      }
    case structured_statement_case_print:
      {
        struct structured_location l = STRUCTURED_LOCATION_ANYWHERE;
        structured_translate_expression (vmp, & l, s->print_expression, env,
                                         false);
        structured_consume_location (env, & l);
        structuredvm_mutable_routine_append_instruction_name (vmp, "print");
        structured_emit_operand (vmp, env, & l);
        if (tail)
          structured_generate_return_statement (vmp);
        break;
      }
    case structured_statement_case_sequence:
      {
        /* Avoid at least some tailness anonmalies due to parsing, where there
           can be trivial skip statements.  This does not really count as an
           optimisation to me.
           FIXME: factor with the other code generator; better: move to parsing
           or to a pass right after parsing. */
        if (s->sequence_statement_0->case_ == structured_statement_case_skip)
          structured_translate_statement (vmp, s->sequence_statement_1, env,
                                          tail);
        else if (s->sequence_statement_1->case_ == structured_statement_case_skip)
          structured_translate_statement (vmp, s->sequence_statement_0, env,
                                          tail);
        else
          {
            structured_translate_statement (vmp, s->sequence_statement_0, env,
                                            false);
            structured_translate_statement (vmp, s->sequence_statement_1, env,
                                            tail);
          }
        break;
      }
    case structured_statement_case_if_then_else:
      {
        /* Translate
             if C then T else E end
           into
               branch-unless C $BEFORE_ELSE
               T
               b $AFTER_ELSE
             $BEFORE_ELSE:
               E
             $AFTER_ELSE:
           In a tail context omit $AFTER_ELSE and the branch to it. */
        structuredvm_label before_else = structuredvm_fresh_label (vmp);
        structuredvm_label after_else = structuredvm_fresh_label (vmp);
        structured_translate_conditional (vmp, s->if_then_else_condition,
                                          before_else, false, env);
        structured_translate_statement (vmp, s->if_then_else_then_branch, env, tail);
        if (! tail)
          {
            structuredvm_mutable_routine_append_instruction_name (vmp, "b");
            structuredvm_mutable_routine_append_label_parameter (vmp,
                                                                 after_else);
          }
        structuredvm_mutable_routine_append_label (vmp, before_else);
        structured_translate_statement (vmp, s->if_then_else_else_branch, env, tail);
        structuredvm_mutable_routine_append_label (vmp, after_else);
        break;
      }
    case structured_statement_case_repeat_until:
      {
        /* Translate
             repeat B until G
           into
             $BEFORE_BODY:
               B
               branch-unless G $BEFORE_BODY */
        structuredvm_label before_body = structuredvm_fresh_label (vmp);
        structuredvm_mutable_routine_append_label (vmp, before_body);
        structured_translate_statement (vmp, s->repeat_until_body, env, false);
        structured_translate_conditional (vmp, s->repeat_until_guard,
                                          before_body, false, env);
        if (tail)
          structured_generate_return_statement (vmp);
        break;
      }
    case structured_statement_case_return:
      {
        struct structured_expression *e = s->return_result;
        /* In every case but one, compile the expression and generate a return
           instruction.  The return address will be at the undertop, right below
           the result.
           We can do better in one case, which is a return statement in tail
           position whose return result is a procedure call.  In that case we
           can compile the statement as a tail call, without any additional
           return instruction. */
        if (tail && e->case_ == structured_expression_case_call)
          {
            struct structured_location l = STRUCTURED_LOCATION_ANYWHERE;
            structured_translate_call (vmp, & l,
                                       e->callee, e->actuals, e->actual_no,
                                       false, env, tail);
            structured_consume_location (env, & l);
          }
        else
          {
            struct structured_location l = STRUCTURED_LOCATION_ANYWHERE;
            structured_translate_expression (vmp, & l, e, env, tail);
            structured_consume_location (env, & l);
            STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, push_mstack);
            structured_emit_operand (vmp, env, & l);
            STRUCTUREDVM_ROUTINE_APPEND_INSTRUCTION(vmp, return_mto_mundertop);
          }
        break;
      }
    case structured_statement_case_call:
      {
        struct structured_location l = STRUCTURED_LOCATION_ANYWHERE;
        structured_translate_call (vmp, & l, s->callee, s->actuals, s->actual_no,
                                   true, env, tail);
        structured_consume_location (env, & l);
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

  /* Translate the body. */
  structured_translate_statement (vmp, p->body, env, true);

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
  structured_translate_statement (vmp, p->main_statement, env,
                                  false /* no implicit return to generate */);

  structured_static_environment_destroy (env);
}




/* Entry point: translate an AST program to a VM routine.
 * ************************************************************************** */

void
structured_translate_program_register (struct structuredvm_mutable_routine *vmp,
                                       struct structured_program *p)
{
  /* Translate the AST pointed by p into *vmp.  This of course works by
     recursion. */
  structured_translate_program (vmp, p);
}
