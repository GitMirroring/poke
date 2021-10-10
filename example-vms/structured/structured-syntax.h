/* Jittery structured language example: syntax.

   Copyright (C) 2017, 2019 Luca Saiu
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


#ifndef JITTER_STRUCTURED_SYNTAX_H_
#define JITTER_STRUCTURED_SYNTAX_H_

#include <jitter/jitter.h>


/* About AST data structures and heap-allocation.
 * ************************************************************************** */

/* This headers defines C data types representing a high-level AST data
   structure for a structured program.

   Unboxed AST data structures are all heap-allocated with malloc .  There is no
   sharing within an AST (no two parents ever point to the same children) and in
   particular every text string is allocated independently, even when the text
   it contains is identical to the text of another string occurring elsewhere in
   the same AST.

   All the allocation, right now, occurs within the parser rules.  There is no
   explicit facility to free ASTs, but that would be trivial to add if needed in
   the future. */




/* Structured-language ASTs.
 * ************************************************************************** */

/* The case of an AST expression. */
enum structured_expression_case
  {
    structured_expression_case_undefined,
    structured_expression_case_literal,
    structured_expression_case_variable,
    structured_expression_case_if_then_else,
    structured_expression_case_primitive,
    structured_expression_case_call
  };

/* An identifier for a structured-language primitive.  Primitives always work on
   values (one or two), and always produce one result.  In other words a
   primitive call is always an expression taking other expressions as arguments.
   There are no statement-like primitives. */
enum structured_primitive
  {
    structured_primitive_plus,
    structured_primitive_minus,
    structured_primitive_times,
    structured_primitive_divided,
    structured_primitive_remainder,
    structured_primitive_unary_minus,
    structured_primitive_equal,
    structured_primitive_different,
    structured_primitive_less,
    structured_primitive_less_or_equal,
    structured_primitive_greater,
    structured_primitive_greater_or_equal,
    structured_primitive_logical_not,
    structured_primitive_is_nonzero,
    structured_primitive_input
  };

/* A variable is represented as a pointer to a malloc-allocated C string holding
   the variable name.  There is no sharing: each instance of the same variable
   is allocated separately. */
typedef char* structured_variable;

/* A structured-language expression AST.  Whenever an expression is contained
   within a statement or a larger super-expresison the parent points to a struct
   of this type. */
struct structured_expression
{
  /* The expression case. */
  enum structured_expression_case case_;

  /* Expression fields, as an anonymous union.  Some fields of the anonymous
     union are anonymous structs. */
  union
  {
    /* An integer. */
    jitter_int literal;

    /* A variable. */
    structured_variable variable;

    /* If-then-else fields. */
    struct
    {
      /* A pointer to the condition expression, as a malloc-allocated struct. */
      struct structured_expression *if_then_else_condition;

      /* A pointer to the then-branch expression, as a malloc-allocated
         struct. */
      struct structured_expression *if_then_else_then_branch;

      /* A pointer to the else-branch expression, as a malloc-allocated
         struct. */
      struct structured_expression *if_then_else_else_branch;
    };

    /* Primitive fields. */
    struct
    {
      /* Primitive identifier. */
      enum structured_primitive primitive;

      /* Pointer to a malloc-allocated first operand structure as an expression;
         NULL if there is no first operand. */
      struct structured_expression *primitive_operand_0;

      /* Pointer to a malloc-allocated second operand structure as an
         expression; NULL if there is no second operand. */
      struct structured_expression *primitive_operand_1;
    };

    /* Call fields. */
    struct
    {
      structured_variable callee;
      struct structured_expression **actuals;
      size_t actual_no;
    };
  }; /* end of the anonymous union. */
};

/* The case of an AST statement. */
enum structured_statement_case
  {
    structured_statement_case_skip,
    structured_statement_case_block,
    structured_statement_case_assignment,
    structured_statement_case_print,
    structured_statement_case_sequence,
    structured_statement_case_if_then_else,
    structured_statement_case_repeat_until,
    structured_statement_case_return,
    structured_statement_case_call
  };




/* Structured-language statements.
 * ************************************************************************** */

/* A structured-language statement AST.  Whenever a statement is contained
   within a larger super-statement or directly within the programa AST, the
   parent points to a struct of this type. */
struct structured_statement
{
  /* The statement case. */
  enum structured_statement_case case_;

  /* Statement fields, as an anonymous union.  Some fields of the anonymous
     union are anonymous structs. */
  union
  {
    /* There are no fields for the skip case. */

    /* Block fields. */
    struct
    {
      /* The variable being declared. */
      structured_variable block_variable;
      struct structured_statement *block_body;
    };

    /* Assignmenet fields. */
    struct
    {
      /* The set variable. */
      structured_variable assignment_variable;

      /* A pointer to the expression whose value will be set into the
         variable, as a malloc-allocated struct. */
      struct structured_expression *assignment_expression;
    };

    /* A pointer to the expression to be printed, as a malloc-allocated
       struct. */
    struct structured_expression *print_expression;

    /* Sequence fields. */
    struct
    {
      /* A pointer to the first statement in the sequence, as a malloc-allocated
         struct.  The parser will nest sequences on the right, but there is no
         deep reason why the pointed first statement could not be a sequence as
         well. */
      struct structured_statement *sequence_statement_0;

      /* A pointer to the second statement in the sequence, as a malloc-allocated
         struct.  The second statement may be another sequence. */
      struct structured_statement *sequence_statement_1;
    };

    /* If-then-else fields. */
    struct
    {
      /* A pointer to the condition expression, as a malloc-allocated struct. */
      struct structured_expression *if_then_else_condition;

      /* A pointer to the then-branch statement, as a malloc-allocated struct. */
      struct structured_statement *if_then_else_then_branch;

      /* A pointer to the else-branch statement, as a malloc-allocated struct. */
      struct structured_statement *if_then_else_else_branch;
    };

    /* If-then fields. */
    struct
    {
      /* A pointer to the condition expression, as a malloc-allocated struct. */
      struct structured_expression *if_then_condition;

      /* A pointer to the then-branch statement, as a malloc-allocated struct. */
      struct structured_statement *if_then_then_branch;
    };

    /* While-do fields. */
    struct
    {
      /* A pointer to the guard expression, as a malloc-allocated struct. */
      struct structured_expression *while_do_guard;

      /* A pointer to the body statement, as a malloc-allocated struct. */
      struct structured_statement *while_do_body;
    };

    /* Repeat-until fields. */
    struct
    {
      /* A pointer to the body statement, as a malloc-allocated struct. */
      struct structured_statement *repeat_until_body;

      /* A pointer to the guard expression, as a malloc-allocated struct. */
      struct structured_expression *repeat_until_guard;
    };

    /* Return fields. */
    struct
    {
      /* A pointer to the return expression, as a malloc-allocated struct. */
      struct structured_expression *return_result;
    };

    /* Call fields. */
    struct
    {
      structured_variable callee;
      struct structured_expression **actuals;
      size_t actual_no;
    };
  }; /* end of the anonymous union. */
};

struct structured_procedure
{
  /* A pointer to the procedure name as a malloc-allocated C string. */
  char *procedure_name;

  /* A malloc-allocated array of malloc-allocated C strings containing formal
     parameter names. */
  char **formals;

  /* The number of formal parameters. */
  size_t formal_no;

  /* The procedure body. */
  struct structured_statement *body;
};

/* A structured program AST.  Right now a program consists of a single
   statement. */
struct structured_program
{
  /* A pointer to the source file pathname as a malloc-allocated C string. */
  char *source_file_name;

  /* A malloc-allocated array of pointers to malloc-allocated procedures. */
  struct structured_procedure **procedures;

  /* The number of procedures. */
  size_t procedure_no;

  /* A pointer to the main statement, as a malloc-allocated struct. */
  struct structured_statement *main_statement;
};




/* Boolean primitives.
 * ************************************************************************** */

/* Given the case for a primitive, return non-false iff the primitive is a
   comparison primitive. */
bool
structured_is_comparison_primitive (enum structured_primitive p);

/* Given the case for a comparison primitive returning a boolean, return the
   case of the opposite primitive.  For example, the reverse of less is
   greater_or_equal.
   Fail if reversing is not defined on the given primitive. */
enum structured_primitive
structured_reverse_comparison_primitive (enum structured_primitive p);


#endif // #ifndef JITTER_STRUCTURED_SYNTAX_H_
