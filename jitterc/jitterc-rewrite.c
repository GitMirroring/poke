/* Jitter: rewriting rules, only used at VM-generation time.

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


/* Include Gnulib functions. */
#include <config.h>

/* Include standard headers. */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/* We use Gnulib lists and (here arbitrarily: Jitter's version would work just
   as well) xmalloc and friends. */
#include <gl_xlist.h>
#include <gl_array_list.h>
#include <xalloc.h>

/* Include the Jitter main header, generation-time headers, and generation-time
   utilities. */
#include <jitter/jitter.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-hash.h>
#include "jitterc-rewrite.h"
#include "jitterc-vm.h"
#include "jitterc-utility.h"




/* Finding placeholder occurrences within rule patterns.
 * ************************************************************************** */

/* Add any placeholder occurring in the pointed argument pattern to the given
   unique string list. */
static void
jitterc_add_placeholders_from_argument_pattern
   (gl_list_t placeholders,
    const struct jitterc_argument_pattern *ap)
{
  if (ap->placeholder_or_NULL != NULL)
    jitterc_list_add_string_unique (placeholders, ap->placeholder_or_NULL);
}

/* Add any placeholder occurring in the pointed pattern to the given unique
   string list. */
static void
jitterc_add_placeholders_from_pattern
   (gl_list_t placeholders,
    const struct jitterc_instruction_pattern *ip)
{
  gl_list_t argument_patterns = ip->argument_patterns;
  size_t length = gl_list_size (argument_patterns);
  int i;
  for (i = 0; i < length; i ++)
    {
      const struct jitterc_argument_pattern *ap
        = gl_list_get_at (argument_patterns, i);
      jitterc_add_placeholders_from_argument_pattern (placeholders, ap);
    }
}

/* Add any placeholder occurring in the given pattern list to the given unique
   string list. */
static void
jitterc_add_placeholders_from_patterns (gl_list_t placeholders,
                                        gl_list_t in_instruction_patterns)
{
  size_t length = gl_list_size (in_instruction_patterns);
  int i;
  for (i = 0; i < length; i ++)
    {
      const struct jitterc_instruction_pattern *ip
        = gl_list_get_at (in_instruction_patterns, i);
      jitterc_add_placeholders_from_pattern (placeholders, ip);
    }
}




/* Template expression operators.
 * ************************************************************************** */

/* A few type arrays for to be used internally for operator in-types,
   referred in the definition of jitterc_expression_operators below. */
static const enum jitterc_expression_type
jitterc_expression_types_fixnum_fixnum []
  = { jitterc_expression_type_fixnum, jitterc_expression_type_fixnum };
static const enum jitterc_expression_type
jitterc_expression_types_boolean_boolean []
  = { jitterc_expression_type_boolean, jitterc_expression_type_boolean };
static const enum jitterc_expression_type
jitterc_expression_types_boolean []
  = { jitterc_expression_type_boolean };

const struct jitterc_expression_operator
jitterc_expression_operators []
  = {
      {
        "plus", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_fixnum
      },
      {
        "minus", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_fixnum
      },
      {
        "times", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_fixnum
      },
      {
        "divided", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_fixnum
      },
      {
        "remainder", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_fixnum
      },
      {
        "equal", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_boolean
      },
      {
        "notequal", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_boolean
      },
      {
        "less", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_boolean
      },
      {
        "lessorequal", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_boolean
      },
      {
        "greater", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_boolean
      },
      {
        "greaterorequal", 2, jitterc_expression_types_fixnum_fixnum,
        jitterc_expression_type_boolean
      },
      {
        "logicaland", 2, jitterc_expression_types_boolean_boolean,
        jitterc_expression_type_boolean
      },
      {
        "logicalor", 2, jitterc_expression_types_boolean_boolean,
        jitterc_expression_type_boolean
      },
      {
        "logicalnot", 1, jitterc_expression_types_boolean,
        jitterc_expression_type_boolean
      },
    };

/* How many operators there are.  The size is expressed in array elements, not
   in chars. */
const size_t
jitterc_expression_operator_no
  = (sizeof (jitterc_expression_operators)
     / sizeof (struct jitterc_expression_operator));

const struct jitterc_expression_operator*
jitterc_lookup_expression_operator (const char *name)
{
  /* Search sequentially in the array until a name matches.  This may be
     optimizable, but right now it's not at all clear that using an associative
     structure is worth the trouble. */
  int i;
  for (i = 0; i < jitterc_expression_operator_no; i ++)
    {
      const struct jitterc_expression_operator *res
        = jitterc_expression_operators + i;
      if (! strcmp (res->name, name))
        return res;
    }

  jitter_fatal ("unknown template expression operator %s", name);
}




/* Rewrite AST-construction API.
 * ************************************************************************** */

struct jitterc_rule*
jitterc_make_rule (gl_list_t in_instruction_patterns,
                   gl_list_t out_instruction_templates,
                   struct jitterc_template_expression *guard,
                   /* The list of placeholders is not given. */
                   char *name,
                   int line_no)
{
  struct jitterc_rule *res = xmalloc (sizeof (struct jitterc_rule));

  res->in_instruction_patterns = in_instruction_patterns;
  res->out_instruction_templates = out_instruction_templates;
  res->guard = guard;

  /* Compute the list of all the occurring placeholders in the pattern part.
     Those are all the placeholders in the rule, if the rule is well-written.
     If there are some placeholders in the guard or in instruction templates
     then it's a mistake, which will be discovered later in the semantic-check
     phase.  */
  res->placeholders = jitterc_make_empty_list ();
  jitterc_add_placeholders_from_patterns (res->placeholders,
                                          in_instruction_patterns);

  res->name = name;
  res->line_no = line_no;

  return res;
}

struct jitterc_instruction_pattern*
jitterc_make_instruction_pattern (char *instruction_name,
                                  gl_list_t argument_patterns,
                                  int line_no)
{
  struct jitterc_instruction_pattern *res
    = xmalloc (sizeof (struct jitterc_instruction_pattern));

  res->instruction_name = instruction_name;
  res->argument_patterns = argument_patterns;
  res->line_no = line_no;

  return res;
}

struct jitterc_argument_pattern*
jitterc_make_argument_pattern (enum jitterc_instruction_argument_kind kind,
                               bool has_literal,
                               union jitter_word literal,
                               char *placeholder_or_NULL,
                               int line_no)
{
  struct jitterc_argument_pattern *res
    = xmalloc (sizeof (struct jitterc_argument_pattern));

  res->kind = kind;
  res->has_literal = has_literal;
  res->literal = literal;
  res->placeholder_or_NULL = placeholder_or_NULL;
  res->line_no = line_no;

  return res;
}

struct jitterc_instruction_template*
jitterc_make_instruction_template (char *instruction_name,
                                   gl_list_t argument_expressions,
                                   int line_no)
{
  struct jitterc_instruction_template *res
    = xmalloc (sizeof (struct jitterc_instruction_template));

  res->instruction_name = instruction_name;
  res->argument_expressions = argument_expressions;
  res->line_no = line_no;

  return res;
}

/* Return a pointer to a new template expression with the given case, and the
   union part uninitialized.  Argument types match struct field types. */
static struct jitterc_template_expression*
jitterc_make_template_expression
   (enum jitterc_template_expression_case case_,
    int line_no)
{
  struct jitterc_template_expression *res
    = xmalloc (sizeof (struct jitterc_template_expression));

  /* The actually important fields are left uninitialized here. */
  res->case_ = case_;
  res->line_no = line_no;

  return res;
}

struct jitterc_template_expression*
jitterc_make_template_expression_boolean (bool bool_value,
                                          int line_no)
{
  struct jitterc_template_expression *res
    = jitterc_make_template_expression
         (jitterc_instruction_argument_expression_case_boolean_constant,
          line_no);

  res->constant.fixnum = bool_value;

  return res;
}

struct jitterc_template_expression*
jitterc_make_template_expression_fixnum (union jitter_word word,
                                         int line_no)
{
  struct jitterc_template_expression *res
    = jitterc_make_template_expression
         (jitterc_instruction_argument_expression_case_fixnum_constant,
          line_no);

  res->constant = word;

  return res;
}

struct jitterc_template_expression*
jitterc_make_template_expression_placeholder (char *placeholder,
                                              int line_no)
{
  struct jitterc_template_expression *res
    = jitterc_make_template_expression
         (jitterc_instruction_argument_expression_case_placeholder, line_no);

  res->placeholder = placeholder;

  return res;
}

struct jitterc_template_expression*
jitterc_make_template_expression_operation (const char *operator_name,
                                            gl_list_t operand_expressions,
                                            int line_no)
{
  struct jitterc_template_expression *res
    = jitterc_make_template_expression
         (jitterc_instruction_argument_expression_case_operation, line_no);

  res->operator = jitterc_lookup_expression_operator (operator_name);
  res->operand_expressions = operand_expressions;

  return res;
}




/* Adding rewrite rules to a VM.
 * ************************************************************************** */

void
jitterc_add_rule (struct jitterc_vm *vm,
                  struct jitterc_rule *rule)
{
  gl_list_add_last (vm->rewrite_rules, rule);
}




/* Fatal semantic errors.
 * ************************************************************************** */

/* Exit with a fatal message printable with a printf format string along with
   its arguments, specifying a source line.
   This relies on a struct jitterc_vm pointer named vm being in scope. */
#define JITTERC_SEMANTIC_ERROR(_jitter_line_no, ...)              \
  do                                                              \
    {                                                             \
      printf ("%s:%i: ", vm->source_file_name, _jitter_line_no);  \
      printf (__VA_ARGS__);                                       \
      printf ("\n");                                              \
      exit (EXIT_FAILURE);                                        \
    }                                                             \
  while (false)




/* Finding placeholder occurrences within rule templates and expressions.
 * ************************************************************************** */

/* Fail fatally if any placeholder occurring in the pointed template expression
   is not in the given list of strings.  This forward-declaration is needed
   because of mutual recursion. */
static void
jitterc_check_placeholders_in_expression
   (const struct jitterc_vm *vm,
    gl_list_t placeholders,
    const struct jitterc_template_expression *e);

/* Fail fatally if any placeholder occurring in the given template expressions
   is not in the given list of strings. */
static void
jitterc_check_placeholders_in_expressions (const struct jitterc_vm *vm,
                                           gl_list_t placeholders,
                                           gl_list_t template_expressions)
{
  size_t length = gl_list_size (template_expressions);
  int i;
  for (i = 0; i < length; i ++)
    {
      const struct jitterc_template_expression *te
        = gl_list_get_at (template_expressions, i);
      jitterc_check_placeholders_in_expression (vm, placeholders, te);
    }
}

static void
jitterc_check_placeholders_in_expression
   (const struct jitterc_vm *vm,
    gl_list_t placeholders,
    const struct jitterc_template_expression *e)
{
  switch (e->case_)
    {
    case jitterc_instruction_argument_expression_case_boolean_constant:
    case jitterc_instruction_argument_expression_case_fixnum_constant:
      return;

    case jitterc_instruction_argument_expression_case_placeholder:
      if (! jitterc_list_has_string (placeholders, e->placeholder))
        JITTERC_SEMANTIC_ERROR(e->line_no, "unbound placeholder %s",
                               e->placeholder);
      break;

    case jitterc_instruction_argument_expression_case_operation:
      jitterc_check_placeholders_in_expressions (vm,
                                                 placeholders,
                                                 e->operand_expressions);
      break;

    default:
      jitter_fatal ("invalid template expression case");
    }
}

/* Fail fatally if any placeholder occurring in the pointed instruction template
   is not in the given list of strings. */
static void
jitterc_check_placeholders_in_template
   (const struct jitterc_vm *vm,
    gl_list_t placeholders,
    const struct jitterc_instruction_template *it)
{
  jitterc_check_placeholders_in_expressions (vm,
                                             placeholders,
                                             it->argument_expressions);
}

/* Fail fatally if any placeholder occurring in the given instruction templates
   is not in the given list of strings. */
static void
jitterc_check_placeholders_in_templates (const struct jitterc_vm *vm,
                                         gl_list_t placeholders,
                                         gl_list_t out_instruction_templates)
{
  size_t length = gl_list_size (out_instruction_templates);
  int i;
  for (i = 0; i < length; i ++)
    {
      const struct jitterc_instruction_template *it
        = gl_list_get_at (out_instruction_templates, i);
      jitterc_check_placeholders_in_template (vm, placeholders, it);
    }
}

/* Check that the pointed rule in the pointed VM doesn't use undefined
   placeholders. */
static void
jitterc_check_rule_placeholders (const struct jitterc_vm *vm,
                                 const struct jitterc_rule *rule)
{
  /* Check that every placeholder in the template and guard occurs in the
     pattern.  Any placeholder not found in the pattern is incorrect. */
  jitterc_check_placeholders_in_expression (vm,
                                            rule->placeholders,
                                            rule->guard);
  jitterc_check_placeholders_in_templates (vm,
                                           rule->placeholders,
                                           rule->out_instruction_templates);
}

/* Check that the named instruction in the given VM exists and has the given
   arity.  If not, fail reporting the given line number. */
static void
jitterc_check_instruction_existence_and_arity
   (const struct jitterc_vm *vm,
    const char *instruction_name,
    const int provided_arity,
    int line_no)
{
  /* Fail if the hash doesn't have the instruction name as key. */
  if (! jitter_string_hash_table_has (& vm->name_to_instruction,
                                      instruction_name))
    JITTERC_SEMANTIC_ERROR(line_no, "unknown instruction %s", instruction_name);

  /* Now we can safely lookup the hash. */
  const struct jitterc_instruction *ins
    = jitter_string_hash_table_get (& vm->name_to_instruction,
                                    instruction_name).pointer_to_void;

  /* Compare the given arity with what is expected. */
  const int required_arity = gl_list_size (ins->arguments);
  if (provided_arity != required_arity)
    JITTERC_SEMANTIC_ERROR(line_no, "invalid arity for instruction %s: "
                           "%i instead of %i", instruction_name,
                           provided_arity, required_arity);
}

/* Check that the pointed rule of the pointed VM uses instructions correctly. */
static void
jitterc_check_instructions_in_rule (const struct jitterc_vm *vm,
                                    const struct jitterc_rule *rule)
{
  size_t length;
  int i;

  /* For every instruction pattern... */
  length = gl_list_size (rule->in_instruction_patterns);
  for (i = 0; i < length; i ++)
    {
      const struct jitterc_instruction_pattern *ip
        = gl_list_get_at (rule->in_instruction_patterns, i);

      /* ...Check that the instruction actually exists and is used with the
         correct arity. */
      jitterc_check_instruction_existence_and_arity
         (vm, ip->instruction_name,
          gl_list_size (ip->argument_patterns),
          ip->line_no);

      // FIXME: check pattern arguments.
    }

  /* For every instruction template... */
  length = gl_list_size (rule->out_instruction_templates);
  for (i = 0; i < length; i ++)
    {
      const struct jitterc_instruction_template *tp
        = gl_list_get_at (rule->out_instruction_templates, i);

      /* ...Check that the instruction actually exists and is used with the
         correct arity. */
      jitterc_check_instruction_existence_and_arity
         (vm, tp->instruction_name,
          gl_list_size (tp->argument_expressions),
          tp->line_no);

      // FIXME: check template arguments.
    }
}



/* Rule semantic checks.
 * ************************************************************************** */

/* Check that the pointed rule of the pointed VM doesn't violate semantic
   constraints. */
static void
jitterc_check_rule (const struct jitterc_vm *vm,
                    const struct jitterc_rule *rule)
{
  /* Check that the rule doesn't refer undefined placeholders. */
  jitterc_check_rule_placeholders (vm, rule);

  /* Check that the rule uses instructions correctly. */
  jitterc_check_instructions_in_rule (vm, rule);
}

void
jitterc_check_rules (const struct jitterc_vm *vm)
{
  /* Check every rule, one after the other. */
  gl_list_t rules = vm->rewrite_rules;
  size_t length = gl_list_size (rules);
  int i;
  for (i = 0; i < length; i ++)
    jitterc_check_rule (vm, gl_list_get_at (rules, i));
}
