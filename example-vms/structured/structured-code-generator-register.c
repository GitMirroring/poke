/* Jittery structured language example: register-based code generator.

   Copyright (C) 2017, 2018, 2019 Luca Saiu
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
                         const struct structured_location *l)
{
  switch (l->case_)
    {
    // FIXME: shall I support this case, and just decide the location late?
    case structured_location_case_anywhere:
      jitter_fatal ("invalid instruction operand: anywhere");

    // FIXME: shall I support this case, and just decide the location late?
    case structured_location_case_nonconstant:
      jitter_fatal ("invalid instruction operand: nonconstant");

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
      jitter_fatal ("unexpected variable expression result location: temporary");

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
                                 struct structured_static_environment *env);


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
    structured_translate_expression (vmp, &o0l, operand_0, env);
  if (operand_1 != NULL)
    {
      structured_translate_expression (vmp, &o1l, operand_1, env);
      structured_consume_location (env, & o1l);
    }
  if (operand_0 != NULL)
    structured_consume_location (env, & o0l);

  /* The result of the primitive will go to a temporary if no specific location
     was requested. */
  if (rl->case_ == structured_location_case_anywhere
      || rl->case_ == structured_location_case_nonconstant)
    {
      rl->case_ = structured_location_case_temporary;
      rl->temporary = structured_static_environment_fresh_temporary (env);
      rl->register_index
        = structured_static_environment_bind_temporary (env, rl->temporary);
    }

  /* Give the two operand results, or the one operand result, as operands of
     the primitive instruction. */
  structured_translate_non_conditional_primitive_opcode (vmp, case_);
  if (operand_0 != NULL)
    structured_emit_operand (vmp, & o0l);
  if (operand_1 != NULL)
    structured_emit_operand (vmp, & o1l);
  structured_emit_operand (vmp, rl);
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
  structured_emit_operand (vmp, rl);
  structuredvm_mutable_routine_append_instruction_name (vmp, "b");
  structuredvm_mutable_routine_append_label_parameter (vmp, after);
  structuredvm_mutable_routine_append_label (vmp, false_comparison);
  structuredvm_mutable_routine_append_instruction_name (vmp, "mov");
  structuredvm_mutable_routine_append_signed_literal_parameter (vmp, 0);
  structured_emit_operand (vmp, rl);
  structuredvm_mutable_routine_append_label (vmp, after);
}

/* Emit code translating the pointed expression AST to the pointed Jittery
   routine, using the pointed static environment to be looked up and updated.
   The result of the expression will be stored, in emitted code, in the required
   location, updated here if its case is "anywhere". */
static void
structured_translate_expression (struct structuredvm_mutable_routine *vmp,
                                 struct structured_location *rl,
                                 struct structured_expression *e,
                                 struct structured_static_environment *env)
{
  switch (e->case_)
    {
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

        structuredvm_label before_else = structuredvm_fresh_label (vmp);
        structuredvm_label after_else = structuredvm_fresh_label (vmp);
        structured_translate_conditional (vmp, e->if_then_else_condition,
                                          before_else,
                                          false,
                                          env);
        structured_translate_expression (vmp, rl, e->if_then_else_then_branch,
                                         env);
        structuredvm_mutable_routine_append_instruction_name (vmp, "b");
        structuredvm_mutable_routine_append_label_parameter (vmp, after_else);
        structuredvm_mutable_routine_append_label (vmp, before_else);
        structured_translate_expression (vmp, rl, e->if_then_else_else_branch,
                                         env);
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
        structured_translate_expression (vmp, &o0l, operand_0, env);
        struct structured_location o1l = STRUCTURED_LOCATION_ANYWHERE;
        structured_translate_expression (vmp, &o1l, operand_1, env);
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
        structured_emit_operand (vmp, & o0l);
        structured_emit_operand (vmp, & o1l);
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
        structured_emit_operand (vmp, & rl);
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

/* Emit code translating the pointed statement AST to the pointed Jittery
   routine, using the pointed static environment to be looked up and updated. */
static void
structured_translate_statement (struct structuredvm_mutable_routine *vmp,
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
        struct structured_location vl = STRUCTURED_LOCATION_REGISTER (idx);
        structured_translate_expression (vmp, & vl, s->assignment_expression,
                                         env);
        structured_consume_location (env, & vl);
        break;
      }
    case structured_statement_case_print:
      {
        struct structured_location l = STRUCTURED_LOCATION_ANYWHERE;
        structured_translate_expression (vmp, & l, s->print_expression, env);
        structured_consume_location (env, & l);
        structuredvm_mutable_routine_append_instruction_name (vmp, "print");
        structured_emit_operand (vmp, & l);
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
        /* Translate
             if C then T else E end
           into
               branch-unless C $BEFORE_ELSE
               T
               b $AFTER_ELSE
             $BEFORE_ELSE:
               E
             $AFTER_ELSE: */
        structuredvm_label before_else = structuredvm_fresh_label (vmp);
        structuredvm_label after_else = structuredvm_fresh_label (vmp);
        structured_translate_conditional (vmp, s->if_then_else_condition,
                                          before_else, false, env);
        structured_translate_statement (vmp, s->if_then_else_then_branch, env);
        structuredvm_mutable_routine_append_instruction_name (vmp, "b");
        structuredvm_mutable_routine_append_label_parameter (vmp, after_else);
        structuredvm_mutable_routine_append_label (vmp, before_else);
        structured_translate_statement (vmp, s->if_then_else_else_branch, env);
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
        structured_translate_statement (vmp, s->repeat_until_body, env);
        structured_translate_conditional (vmp, s->repeat_until_guard,
                                          before_body, false, env);
        break;
      }
    default:
      jitter_fatal ("invalid statement case (bug): %i", (int) s->case_);
    }
}

static void
structured_translate_program (struct structuredvm_mutable_routine *vmp,
                              struct structured_program *p)
{
  struct structured_static_environment *env
    = structured_static_environment_make ();
  structured_translate_statement (vmp, p->main_statement, env);
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
