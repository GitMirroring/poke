/* Jitter: rewriting rule header, only used at VM-generation time.

   Copyright (C) 2017 Luca Saiu
   Written by Luca Saiu

   This file is part of GNU Jitter.

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


#ifndef JITTERC_REWRITE_H_
#define JITTERC_REWRITE_H_

/* Include Gnulib functions. */
#include <config.h>

/* Include standard headers. */
#include <stdlib.h>
#include <stdbool.h>

/* Include the Jitter main header, and generation-time headers. */
#include <jitter/jitter.h>
#include "jitterc-rewrite.h"
#include "jitterc-vm.h"




/* Rewrite rule introduction.
 * ************************************************************************** */

/* A rule is a <guard, pattern, template> triple.  It means that when the
   trailing part of unspecialized VM routine which is being built matches the
   pattern, it is replaced by the template instantiated by replacing template
   placeholders with their matched arguments, if the guard is true.  The
   replacement is allowed to trigger other rules, and it's the user's
   responsibility to ensure that the process eventually terminates.

   A rewrite rule pattern consists of a sequence of one or more instruction
   patterns.  The pattern matches one or more unspecialized strictly sequential
   (with no labels or caller instructions in between) instructions.  A pattern
   is not required to be linear, which means that the same placeholder may occur
   more than once: for a pattern to match, equal arguments must match equal
   placeholders.

   A rewrite rule template is a list of instructions with template expressions
   in place of their arguments.  Template expressions may use constants and the
   placeholders matched in the pattern part, connected with a fixed set of
   statically-typed operators.  A template expression has a fully strict
   evaluation semantics, with no side effects.  Its result can be a word-sized
   integer, a boolean, or an actual parameter.

   A guard is a template expression whose result is a boolean.  If after
   matching the guard evaluates to false then the rule does not fire.

   We say that a rule matches when a substitution can be found for every
   instruction in the pattern, which means that the opcodes must match
   and every argument must match the corresponding argument pattern, and
   the guard is true. */




/* Rewrite rule AST definitions.
 * ************************************************************************** */

/* A rewrite rule contains one or more instruction patterns, zero or more
   instruction templates, and a guard.  The guard has the same structure (and
   type) of a template expression, as it involves operations on constants and
   placeholders.  */
struct jitterc_rule
{
  /* A list of one or more instruction patterns.  Each element has type struct
     jitterc_instruction_pattern * . */
  gl_list_t in_instruction_patterns;

  /* A list of zero or more instruction templates.  Each element has type struct
     jitterc_instruction_template * . */
  gl_list_t out_instruction_templates;

  /* A pointer to the rule guard.  This is always non-NULL, but may be a trivial
     expression such as "true". */
  struct jitterc_template_expression *guard;

  /* An unsorted list containing every placeholder occurring in the rule, with
     no duplicates.  As the number of placeholders will almost always be very
     small it makes no sense to use an associative data structure.
     Strings are shared with the rule sub-ASTs, and not cloned heap-to-heap. */
  gl_list_t placeholders;

  /* The rule name.  This is always a valid C string, even if no explicit name
     was given in the input. */
  char *name;

  /* The source line where the rule began. */
  int line_no;
};

/* An instruction pattern contains an instruction name followed by one argument
   pattern per instruction argument. */
struct jitterc_instruction_pattern
{
  /* The unspecialized instruction name, not mangled. */
  char *instruction_name;

  /* Instruction argument patterns; each element has type struct
     jitterc_argument_pattern * .  The number of elements needs to
     be checked against the instruction specification. */
  gl_list_t argument_patterns;

  /* The line number where the instruction pattern occurred in the VM
     specification. */
  int line_no;
};

/* An argument pattern contains a kind specification (a subset of the kinds
   allowed for a parameter in that position in the instruction), possibly a
   non-label literal, and possibly a placeholder name. */
struct jitterc_argument_pattern
{
  /* The accepted argument kind, which may be a disjunction of cases expressed
     as a bitmask, or as the unspecified kind if a kind was not explicitly
     specified in the pattern.

     An unspecified kind matches any argument accepted by the instruction
     [FIXME: can I do this in a simple way without type polymorphism?]. */
  enum jitterc_instruction_argument_kind kind;

  /* True iff there is a specified non-label literal. */
  bool has_literal;

  /* The literal value, only meaningful if has_literal is true. */
  union jitter_word literal;

  /* Pointer to a the malloc-allocated placeholder name, or NULL if there
     is no placeholder.  The placeholder prefix is stripped away, and the
     pointed string, if any, contains a valid C identifier usable as a
     suffix. */
  char *placeholder_or_NULL;

  /* The line number where the argument pattern occurred in the VM
     specification. */
  int line_no;
};

/* An instruction template contains an instruction name followed by one argument
   template per argument. */
struct jitterc_instruction_template
{
  /* The unspecialized instruction name, not mangled. */
  char *instruction_name;

  /* Instruction argument templates; each element has type struct
     jitterc_template_expression *.  The number of elements needs to be checked
     against the instruction specification. */
  gl_list_t argument_expressions;

  /* The line number where the instruction template occurred in the VM
     specification. */
  int line_no;
};

/* The possible syntactic cases of a template expression.  See the comment
   before struct jitterc_template_expression . */
enum jitterc_template_expression_case
  {
    jitterc_instruction_argument_expression_case_boolean_constant,
    jitterc_instruction_argument_expression_case_fixnum_constant,
    jitterc_instruction_argument_expression_case_placeholder,
    jitterc_instruction_argument_expression_case_operation
  };

/* A template expression can be:
   - a boolean constant;
   - a fixnum (actually, any word-sized type) constant;
   - a placeholder;
   - an operation, containing:
     *  an operator;
     *  a list of operands, as other template expressions. */
struct jitterc_template_expression
{
  /* What expression case this is. */
  enum jitterc_template_expression_case case_;

  /* Which union element is used depends on the case. */
  union
  {
    /* The word constant, boolean or word-sized. */
    union jitter_word constant;

    /* The placeholder name as a malloc-allocated C string. */
    char *placeholder;

    /* An operation. */
    struct
    {
      /* A pointer to the operator data structure. */
      const struct jitterc_expression_operator *operator;

      /* The actual arguments for the operand above.  Each element is
         a pointer to another struct jitterc_template_expression . */
      gl_list_t operand_expressions;
    };
  };

  /* The line number where the instruction template occurred in the VM
     specification. */
  int line_no;
};




/* Rewrite AST-construction API.
 * ************************************************************************** */

/* These functions are meant to be called from the parser, after it has already
   allocated data structures for the sub-ASTs.  For this reason it makes no
   sense to clone pointed data heap-to-heap: the existing sub-ASTs are just
   shared in the new ASTs, and space allocated only for the new part.

   Some "AST" data structure, such as struct jitterc_rule, actually contain more
   information than just an AST, which is useful for validating rules against
   the specified instructions.  Since rules and instructions can occur in any
   order the validation part of the data structure is initialized to be correct
   at structure initialization, but validation is not performed here.  It will
   occur in a separate pass after all the information about the VM has been
   parsed. */

/* Return a pointer to a new rule with the given fields.  Argument types match
   struct field types. */
struct jitterc_rule*
jitterc_make_rule (gl_list_t in_instruction_patterns,
                   gl_list_t out_instruction_templates,
                   struct jitterc_template_expression *guard,
                   /* The list of placeholders is computed automatically. */
                   char *name,
                   int line_no)
  __attribute__ ((returns_nonnull, nonnull (3, 4)));

/* Return a pointer to a new instruction pattern with the given fields.
   Argument types match struct field types. */
struct jitterc_instruction_pattern*
jitterc_make_instruction_pattern (char *instruction_name,
                                  gl_list_t argument_patterns,
                                  int line_no)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Return a pointer to a new argument pattern with the given fields.
   Argument types match struct field types. */
struct jitterc_argument_pattern*
jitterc_make_argument_pattern (enum jitterc_instruction_argument_kind kind,
                               bool has_literal,
                               union jitter_word literal,
                               char *placeholder_or_NULL,
                               int line_no)
  __attribute__ ((returns_nonnull, nonnull (4)));

struct jitterc_instruction_template*
jitterc_make_instruction_template (char *instruction_name,
                                   gl_list_t argument_expressions,
                                   int line_no)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Return a pointer to a new template boolean expression, with the case and all
   the case-dependent union fields initialized.  Argument types match struct
   field types. */
struct jitterc_template_expression*
jitterc_make_template_expression_boolean (bool bool_value,
                                          int line_no)
  __attribute__ ((returns_nonnull));

/* Return a pointer to a new template fixnum-literal expression, with the case
   and all the case-dependent union fields initialized.  Argument types match
   struct field types. */
struct jitterc_template_expression*
jitterc_make_template_expression_fixnum (union jitter_word word,
                                         int line_no)
  __attribute__ ((returns_nonnull));

/* Return a pointer to a new template placeholder expression, with the case
   and all the case-dependent union fields initialized.  Argument types match
   struct field types. */
struct jitterc_template_expression*
jitterc_make_template_expression_placeholder (char *placeholder,
                                              int line_no)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Return a pointer to a new template operation expression, with the case
   and all the case-dependent union fields initialized.  Argument types match
   struct field types except for operator_name, which is a textual operator
   name to be looked up in jitterc_expression_operators. */
struct jitterc_template_expression*
jitterc_make_template_expression_operation (const char *operator_name,
                                            gl_list_t operand_expressions,
                                            int line_no)
  __attribute__ ((returns_nonnull, nonnull (1)));




/* Rewrite type system.
 * ************************************************************************** */

/* Rewrite rules employ a very simple static type system to prevent mistakes
   with kinds at C-generation time.  This has nothing to do with typing of VM
   programs: it is only used to ensure that rewrite rules make sense.

   Each placeholder has a *set* of kinds it can match, and expression operators
   accept operands of specified types, and evaluate to one result of a specified
   type.

   Only a finite number of types exists.  There is no polymorphism.
   The types are:
   - boolean;
   - fixnum;
   - label (no distinction between fast and slow);
   - register of the first class;
   - register of the second class...;
   - ...register of the last class. */

/* The type of an expression.  Notice that the type is open: there are as many
   distinct register types as there are register classes in the VM. */
enum jitterc_expression_type
  {
    /* An invalid case, used to catch errors. */
    jitterc_expression_type_uninitialized = 0,

    /* False or true. */
    jitterc_expression_type_boolean = 1,

    /* A literal fixnum, suitable as an instruction argument. */
    jitterc_expression_type_fixnum = 2,

    /* A VM label literal; fast and slow labels are interchangeable here as the
       difference only emerges at specialization time, after rewriting. */
    jitterc_expression_type_label = 3,

    /* The base register type.  This case is never actually used by itself: a
       populated register type is made by summin the value of this case to a
       register class character.  There is one distinct register type per
       register class. */
    jitterc_expression_type_register_base = 4,
  };




/* Template expression operators.
 * ************************************************************************** */

/* A template expression operator has a name, a (fixed) input arity, an array of
   input types and and output type.  Output arity is always one.  Operators are
   all statically allocated. */
struct jitterc_expression_operator
{
  /* The name of this operator. */
  char *name;

  /* How many arguments the operator takes. */
  size_t in_arity;

  /* The operator argument types, in order.  There are exactly in_arity
     elements. */
  const enum jitterc_expression_type *in_types;

  /* The single result type. */
  enum jitterc_expression_type out_type;
};

/* An array containing every template expression operator in existence. */
extern const struct jitterc_expression_operator
jitterc_expression_operators [];

/* How many operators there are. */
extern const size_t
jitterc_expression_operator_no;

/* Return a pointer-to-constant to the template expression operator with the
   given name, or fail fatally. */
const struct jitterc_expression_operator*
jitterc_lookup_expression_operator (const char *name)
  __attribute__ ((returns_nonnull, nonnull (1)));




/* Adding rewrite rules to a VM.
 * ************************************************************************** */

/* Append the given rule to the given VM.  The rule is not cloned
   heap-to-heap. */
void
jitterc_add_rule (struct jitterc_vm *vm,
                  struct jitterc_rule *rule)
  __attribute__ ((nonnull (1, 2)));




/* Rule semantic checks.
 * ************************************************************************** */

/* Check that no rule in the given VM violates semantic constraints.  Do nothing
   on success, fail with an error message on any problem.

   This must be called after every instruction and and every rule has been
   added, and after the vm->name_to_instruction hash is initialized. */
void
jitterc_check_rules (const struct jitterc_vm *vm)
  __attribute__ ((nonnull (1)));


#endif // #ifndef JITTERC_REWRITE_H_
