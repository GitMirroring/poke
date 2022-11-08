/* VM-independent routine code frontend: Bison parser.

   Copyright (C) 2016, 2017, 2019, 2020, 2022 Luca Saiu
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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


/* This code is included in the generated .c file, but not in the header. */
%{
  #include <stdio.h>
  #include <limits.h>
  #include <jitter/jitter.h>
  #include <jitter/jitter-instruction.h>
  #include <jitter/jitter-mutable-routine.h>

  #include <jitter/jitter-routine-parser.h>
  #include <jitter/jitter-routine-scanner.h>
  #include <jitter/jitter-malloc.h>
  #include <jitter/jitter-string.h>
  #include <jitter/jitter-fatal.h>

  /* This is required for Bison parsers, called in case of parse errors for
     "reporting"; instead of reporting I use it to record the error position. */
  static void // FIXME: I think that this is required for Bison parsers.
  jitter_error (YYLTYPE *locp,
                struct parser_arg *parser_arg,
                yyscan_t scanner, const char *message);

/* JITTER_HANDLE_STATUS: expand to a statement that, according to the
   status (of type enum jitter_routine_edit_status) either does nothing or
   registers the current parse error details in parser_arg->parse_status .  This
   can be called from both actions and the jitter_error function.  In the case
   of actions we would need to execute YYABORT right after this, but that would
   not work in jitter_error ; hence the need for
   JITTER_HANDLE_STATUS_WITHOUT_YYABORT. */
#define JITTER_HANDLE_STATUS_WITHOUT_YYABORT(the_status)                        \
  do                                                                            \
    {                                                                           \
      /* jitter_scanner is visible from this, since the expansion is in         \
         actions. */                                                            \
      enum jitter_routine_edit_status                                           \
        _jitter_handle_edit_status_status = (the_status);                       \
      switch (_jitter_handle_edit_status_status)                                \
        {                                                                       \
        case jitter_routine_edit_status_success:                                \
          /* Do nothing: there is no error and parsing should continue. */      \
          break;                                                                \
        case jitter_routine_edit_status_label_defined_twice:                    \
        case jitter_routine_edit_status_too_many_parameters:                    \
        case jitter_routine_edit_status_last_instruction_incomplete:            \
        case jitter_routine_edit_status_invalid_instruction:                    \
        case jitter_routine_edit_status_invalid_register:                       \
        case jitter_routine_edit_status_register_class_mismatch:                \
        case jitter_routine_edit_status_nonexisting_register_class:             \
        case jitter_routine_edit_status_invalid_parameter_kind:                 \
        case jitter_routine_edit_status_other_parse_error:                      \
          /* Update the parse status and location so that at parser exit the    \
             caller will find precise information. */                           \
          parser_arg->parse_status.status = _jitter_handle_edit_status_status;  \
          parser_arg->parse_status.error_line_no                                \
            = jitter_get_lineno (jitter_scanner);                               \
          parser_arg->parse_status.error_token_text                             \
            = jitter_clone_string (jitter_get_text (jitter_scanner));           \
          /* Here we would call YYABORT. */                                     \
          break;                                                                \
        default:                                                                \
          jitter_fatal ("unimplemented or invalid case %i",                     \
                        _jitter_handle_edit_status_status);                     \
        };                                                                      \
    }                                                                           \
  while (false)
#define JITTER_HANDLE_STATUS(the_status)                     \
  do                                                         \
    {                                                        \
      /* First do what JITTER_HANDLE_STATUS_WITHOUT_YYABORT  \
         does... */                                          \
      enum jitter_routine_edit_status                        \
        _jitter_handle_edit_status_status_y = (the_status);  \
      JITTER_HANDLE_STATUS_WITHOUT_YYABORT                   \
         (_jitter_handle_edit_status_status_y);              \
      /* ...Then use YYABORT in case of error. */            \
      if (_jitter_handle_edit_status_status_y                \
          != jitter_routine_edit_status_success)             \
        YYABORT;                                             \
    }                                                        \
  while (false)

/* Set the given lvalue, with the %type whose definition is union
   jitter_literal, converting jitter_get_text (jitter_scanner) using the given
   function.  These are useful to convert to a specified base, in the case of
   signed and unsigned literals.  The lvalues will be either
   $$.fixnum or $$.ufixnum , and the functions to
   call will be jitter_strtoll or jitter_strtoull . */
#define JITTER_SET_BINARY(lvalue, function)           \
  do                                                  \
    {                                                 \
      char *text = jitter_get_text (jitter_scanner);  \
      * strchr (text, 'b') = '0';                     \
      lvalue = function (text, NULL, 2);              \
    }                                                 \
  while (false)
#define JITTER_SET_OCTAL(lvalue, function)            \
  do                                                  \
    {                                                 \
      char *text = jitter_get_text (jitter_scanner);  \
      char *oindex = strchr (text, 'o');              \
      if (oindex != NULL)                             \
        * oindex = '0';                               \
      lvalue = function (text, NULL, 8);              \
    }                                                 \
  while (false)
#define JITTER_SET_DECIMAL(lvalue, function)          \
  do                                                  \
    {                                                 \
      char *text = jitter_get_text (jitter_scanner);  \
      lvalue = function (text, NULL, 10);             \
    }                                                 \
  while (false)
#define JITTER_SET_HEXADECIMAL(lvalue, function)      \
  do                                                  \
    {                                                 \
      char *text = jitter_get_text (jitter_scanner);  \
      lvalue = function (text, NULL, 16);             \
    }                                                 \
  while (false)

/* Expand to an assignment of the given field of the given result as an
   operation involving the given infix operator, and the fields with the same
   names as the field of the result of the two given operands.  This is intended
   for uses such as JITTER_SET_OPERATION(fixnum, $$, $1, +, $3)
   . */
#define JITTER_SET_OPERATION(field, result, operand1, operator, operand2)  \
  do                                                                       \
    {                                                                      \
      result.field = operand1.field operator operand2.field;               \
    }                                                                      \
  while (false)


static jitter_int
jitter_power (jitter_int base, jitter_int exponent)
{
  if (exponent < 0)
    return 1;
  jitter_int res = 1;
  for (; exponent > 0; exponent --)
    res *= base;
  return res;
}
static jitter_uint
jitter_upower (jitter_uint base, jitter_uint exponent)
{
  jitter_uint res = 1;
  for (; exponent > 0; exponent --)
    res *= base;
  return res;
}
%}

/* We need a recent enough version of GNU Bison. */
%require "3.0" /* 2.3b was the first version supporting %define api.pure ,
                  but such old versions have not been tested in a long time,
                  and now Bison (as of 3.5) refuses to accept the "b" suffix
                  in requirements.  It appears that 2.3b was not an official
                  release. */

/* Use a prefix different from the default "yy" for the API. */
%define api.prefix {jitter_}

/* Generate a header file. */
%defines

/* This is a reentrant parser. */
%define api.pure full

/* We need to receive location information from the scanner, Bison-style. */
%locations

/* The parser and scanner functions both have additional parameters. */
%lex-param { jitter_scan_t jitter_scanner }
%parse-param { struct parser_arg *parser_arg }
%parse-param { void* jitter_scanner }

/* We don't need a %initial-action block, because the parser receives an already
   initialized routine; see the definition of
   jitter_parse_mutable_routine_from_file_star . */

/* This goes to the header file. */
%code requires {
  #include <stdio.h>

  #include <jitter/jitter.h>
  #include <jitter/jitter-instruction.h>
  #include <jitter/jitter-mutable-routine.h>
  #include <jitter/jitter-vm.h>

  /* A structure whose pointer is passed to the parser function. */
  struct parser_arg
  {
    /* In case of parse error we will need to return a pointer to a copy of this
       to the user; this is useful to store error details. */
    struct jitter_routine_parse_error parse_status;

    /* VM-dependent data for the routine's VM.  Not modified. */
    const struct jitter_vm *vm;

    /* The routine which is being parsed, allowed to contain no instructions at
       the beginning of parsing. */
// FIXME: in case of parse errors restore the routine to its initial state. /////////////////////////////
    struct jitter_mutable_routine *routine;
  };

  /* Parse a routine for the pointed VM from a file or a string in memory,
     adding code to the pointed VM routine.
     These functions work of course on any VM, but are slightly inconvenient for
     the user to call directly.  For this reason they are wrapped in the vm1.c
     template into VM-specific functions not requiring a VM struct pointer.

     The result is NULL on success, and on error a pointer to a fresh struct of
     type struct jitter_routine_parse_error containing details about the error,
     to be destroyed by the user via jitter_routine_parse_error_destroy .

     In case of parse error the routine is *not* modified. */
  struct jitter_routine_parse_error*
  jitter_parse_mutable_routine_from_file_star (FILE *input_file,
                                               struct jitter_mutable_routine *p,
                                               const struct jitter_vm *vm)
    __attribute__ ((nonnull (1, 2, 3)));
  struct jitter_routine_parse_error*
  jitter_parse_mutable_routine_from_file (const char *input_file_name,
                                          struct jitter_mutable_routine *p,
                                          const struct jitter_vm *vm)
    __attribute__ ((nonnull (1, 2, 3)));
  struct jitter_routine_parse_error*
  jitter_parse_mutable_routine_from_string (const char *string,
                                            struct jitter_mutable_routine *p,
                                            const struct jitter_vm *vm)
    __attribute__ ((nonnull (1, 2, 3)));
}

%union {
  union jitter_word literal;
}

%token INSTRUCTION_NAME REGISTER LABEL_LITERAL LABEL COMMA

%token OPEN_PARENS CLOSE_PARENS
%token SIGNED_BINARY_LITERAL SIGNED_OCTAL_LITERAL SIGNED_DECIMAL_LITERAL
%token SIGNED_HEXADECIMAL_LITERAL
%token UNSIGNED_BINARY_LITERAL UNSIGNED_OCTAL_LITERAL UNSIGNED_DECIMAL_LITERAL
%token UNSIGNED_HEXADECIMAL_LITERAL
%token BYTESPERWORD LGBYTESPERWORD BITSPERWORD
%token JITTER_INT_MIN_ JITTER_INT_MAX_ JITTER_UINT_MAX_

/* Lowest precedence... */
%left UNSIGNED_MINUS UNSIGNED_NOT UNSIGNED_PLUS UNSIGNED_OR UNSIGNED_XOR
%left UNSIGNED_AND TIMES UNSIGNED_TIMES DIV UNSIGNED_DIV MOD UNSIGNED_MOD
%precedence PREFIX_NEGATION
%right POWER UNSIGNED_POWER
/* ...Highest precedence */

/* This of course never occurs in the grammar and therefore causes a parse error
   when encountered; the token is returned by the scanner when if fails to
   recognise a token.  This trick allows me to unify scan errors and parse
   errors. */
%token INVALID_TOKEN

%expect 0
%type <literal> int_expression

%%

routine :
  /* nothing */
| routine instruction_or_label
;

instruction_or_label :
  instruction
| label
;

instruction :
  instruction_name arguments
;

label :
  LABEL { char *label = jitter_get_text (jitter_scanner);
          label [strlen (label) - 1] = '\0';  /* Remove the trailing colon. */
          JITTER_HANDLE_STATUS
             (jitter_mutable_routine_append_symbolic_label_safe
                 (NULL,
                  parser_arg->routine,
                  label)); }
;

instruction_name :
  INSTRUCTION_NAME { char *name = jitter_get_text (jitter_scanner);
                     JITTER_HANDLE_STATUS
                        (jitter_mutable_routine_append_instruction_name_safe
                            (parser_arg->routine, name)); }
;

arguments :
  /* nothing */
| argument after_one_argument
;

after_one_argument :
  /* nothing*/
| COMMA argument after_one_argument
;

int_expression :
  SIGNED_BINARY_LITERAL       { JITTER_SET_BINARY($$.fixnum,
                                                  jitter_strtoll); }
| SIGNED_OCTAL_LITERAL        { JITTER_SET_OCTAL($$.fixnum,
                                                 jitter_strtoll); }
| SIGNED_DECIMAL_LITERAL      { JITTER_SET_DECIMAL($$.fixnum,
                                                   jitter_strtoll); }
| SIGNED_HEXADECIMAL_LITERAL  { JITTER_SET_HEXADECIMAL($$.fixnum,
                                                       jitter_strtoll); }
| UNSIGNED_BINARY_LITERAL     { JITTER_SET_BINARY($$.ufixnum,
                                                  jitter_strtoull); }
| UNSIGNED_OCTAL_LITERAL      { JITTER_SET_OCTAL($$.ufixnum,
                                                 jitter_strtoull); }
| UNSIGNED_DECIMAL_LITERAL    { JITTER_SET_DECIMAL($$.ufixnum,
                                                   jitter_strtoull); }
| UNSIGNED_HEXADECIMAL_LITERAL{ JITTER_SET_HEXADECIMAL($$.ufixnum,
                                                       jitter_strtoull); }
| BYTESPERWORD                { $$.ufixnum = JITTER_SIZEOF_VOID_P; }
| LGBYTESPERWORD              { $$.ufixnum = JITTER_LG_BYTES_PER_WORD; }
| BITSPERWORD                 { $$.ufixnum = JITTER_SIZEOF_VOID_P * CHAR_BIT; }
| JITTER_INT_MIN_             { $$.fixnum = JITTER_INT_MIN; }
| JITTER_INT_MAX_             { $$.fixnum = JITTER_INT_MAX; }
| JITTER_UINT_MAX_            { $$.ufixnum = JITTER_UINT_MAX; }
| OPEN_PARENS int_expression CLOSE_PARENS { $$ = $2; }
| UNSIGNED_MINUS int_expression %prec PREFIX_NEGATION
     { union jitter_word zero = {.ufixnum = 0};
       JITTER_SET_OPERATION(ufixnum, $$, zero, -, $2); }
| int_expression TIMES int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, *, $3); }
| int_expression DIV int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, /, $3); }
| int_expression MOD int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, %, $3); }
| int_expression UNSIGNED_PLUS int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, +, $3); }
| int_expression UNSIGNED_MINUS int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, -, $3); }
| int_expression UNSIGNED_TIMES int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, *, $3); }
| int_expression UNSIGNED_DIV int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, /, $3); }
| int_expression UNSIGNED_MOD int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, %, $3); }
| UNSIGNED_NOT int_expression %prec PREFIX_NEGATION
     { union jitter_word minus_one = {.fixnum = -1L};
       JITTER_SET_OPERATION(ufixnum, $$, $2, ^, minus_one); }
| int_expression UNSIGNED_AND int_expression
     { JITTER_SET_OPERATION(ufixnum, $$, $1, &, $3); }
| int_expression UNSIGNED_OR int_expression
     { JITTER_SET_OPERATION(ufixnum, $$, $1, |, $3); }
| int_expression UNSIGNED_XOR int_expression
     { JITTER_SET_OPERATION(ufixnum, $$, $1, ^, $3); }
| int_expression POWER int_expression
     { $$.fixnum = jitter_power ($1.fixnum, $3.fixnum); }
| int_expression UNSIGNED_POWER int_expression
     { $$.ufixnum = jitter_upower ($1.ufixnum, $3.ufixnum); }
;

argument :
  int_expression { JITTER_HANDLE_STATUS
                      (jitter_mutable_routine_append_literal_parameter_safe
                         (parser_arg->routine, $1)); }
| LABEL_LITERAL  { char *text = jitter_get_text (jitter_scanner);
                   JITTER_HANDLE_STATUS
                      (jitter_mutable_routine_append_symbolic_label_parameter_safe
                         (NULL, parser_arg->routine, text)); }
| REGISTER       { char *text = jitter_get_text (jitter_scanner);
                   char register_class_character = text [1];
                   int register_id = strtol (text + 2, NULL, 10);
                   JITTER_HANDLE_STATUS
                      (jitter_mutable_routine_append_symbolic_register_parameter_safe
                         (parser_arg->routine, register_class_character,
                          register_id)); }
;

%%

static void
jitter_error (YYLTYPE *locp,
              struct parser_arg *parser_arg,
              yyscan_t jitter_scanner, const char *message)
{
  JITTER_HANDLE_STATUS_WITHOUT_YYABORT
     (jitter_routine_edit_status_other_parse_error);
}

/* This function could be implemented elsewhere, but in practice I suppose it is
   only used when the parser is also used: therefore having it in this
   compilation unit will make the code a little smaller. */
const char*
jitter_routine_edit_status_to_string (enum jitter_routine_edit_status s)
{
  switch (s)
    {
    case jitter_routine_edit_status_success:
      return "success";
    case jitter_routine_edit_status_label_defined_twice:
      return "label defined twice";
    case jitter_routine_edit_status_too_many_parameters:
      return "too many parameters";
    case jitter_routine_edit_status_last_instruction_incomplete:
      return "last instruction incomplete";
    case jitter_routine_edit_status_invalid_instruction:
      return "invalid instruction";
    case jitter_routine_edit_status_register_class_mismatch:
      return "register class mismatch";
    case jitter_routine_edit_status_nonexisting_register_class:
      return "nonexisting register class";
    case jitter_routine_edit_status_invalid_register:
      return "invalid register";
    case jitter_routine_edit_status_invalid_parameter_kind:
      return "invalid parameter kind";
    case jitter_routine_edit_status_other_parse_error:
      return "parse error";
    default:
      jitter_fatal ("jitter_routine_edit_status_to_string: invalid argument %i",
                    (int) s);
    };
}

/* The comment about code size before jitter_routine_edit_status_to_string
   applies here as well. */
__attribute__ ((nonnull (1)))
static void
jitter_routine_parse_error_finalize (struct jitter_routine_parse_error *e)
{
  free (e->file_name);
  free (e->error_token_text);
}

void
jitter_routine_parse_error_destroy (struct jitter_routine_parse_error *e)
{
  jitter_routine_parse_error_finalize (e);
  free (e);
}

/* This is the main parser function doing all the work.  The other parsing
   functions, meant as part of a more convenient API for the user, all rely on
   this.
   The pointed scanner must be already initialized when this is called, and
   it's the caller's responsibility to finalize it. */
static struct jitter_routine_parse_error *
jitter_parse_core (const char *file_name_to_print,
                   yyscan_t scanner, struct jitter_mutable_routine *p,
                   const struct jitter_vm *vm)
{
  struct parser_arg pa;
  /* Initialise the parse status to a no-error value. */
  pa.parse_status.status = jitter_routine_edit_status_success;
  pa.parse_status.file_name = jitter_clone_string (file_name_to_print);
  pa.parse_status.error_line_no = -1;
  pa.parse_status.error_token_text = NULL;

  /* Use pa to let the parser access the mutable routine it needs to add to, and
     its VM. */
  pa.routine = p;
  pa.vm = (struct jitter_vm *) vm;

  /* Notice that the lexer state will need to be destroyed, in case of both
     success and failure; but it will be destroyed by the caller, not here. */
  if (jitter_parse (& pa, scanner))
    {
      /* Parse error.  Make a copy of the parse status, a pointer of which will
         be returned to the user.  The user will have to destroy it with
         jitter_routine_parse_error_destroy .  It simply works by keeping each
         string field in the struct, and the struct itself, malloc-allocated. */
      struct jitter_routine_parse_error *res
        = jitter_xmalloc (sizeof (struct jitter_routine_parse_error));
      * res = pa.parse_status;
      return res;
    }
  else
    {
      /* Parse success.  Destroy the parse status malloc-allocated part, and we
         are done. */
      jitter_routine_parse_error_finalize (& pa.parse_status);
      return NULL;
    }
}

static struct jitter_routine_parse_error *
jitter_parse_mutable_routine_from_named_file_star (const char *input_file_name,
                                                   FILE *input_file,
                                                   struct jitter_mutable_routine
                                                   *p,
                                                   const struct jitter_vm *vm)
{
  yyscan_t scanner;
  jitter_lex_init (& scanner);
  jitter_set_in (input_file, scanner);

  struct jitter_routine_parse_error *res
    = jitter_parse_core (input_file_name, scanner, p, vm);

  jitter_set_in (NULL, scanner);
  jitter_lex_destroy (scanner);
  return res;
}

struct jitter_routine_parse_error *
jitter_parse_mutable_routine_from_file_star (FILE *input_file,
                                             struct jitter_mutable_routine *p,
                                             const struct jitter_vm *vm)
{
  return jitter_parse_mutable_routine_from_named_file_star ("<input>",
                                                            input_file,
                                                            p,
                                                            vm);
}

struct jitter_routine_parse_error *
jitter_parse_mutable_routine_from_file (const char *input_file_name,
                                        struct jitter_mutable_routine *p,
                                        const struct jitter_vm *vm)
{
  FILE *f;
  if ((f = fopen (input_file_name, "r")) == NULL)
    jitter_fatal ("could not open %s for reading", input_file_name);

  struct jitter_routine_parse_error *res
    = jitter_parse_mutable_routine_from_named_file_star (input_file_name,
                                                         f, p, vm);
  fclose (f);
  return res;
}

struct jitter_routine_parse_error *
jitter_parse_mutable_routine_from_string (const char *string,
                                          struct jitter_mutable_routine *p,
                                          const struct jitter_vm *vm)
{
  yyscan_t scanner;
  jitter_lex_init (& scanner);
  jitter__scan_string (string, scanner);
  struct jitter_routine_parse_error *res
    = jitter_parse_core ("<string>", scanner, p, vm);
  jitter_lex_destroy (scanner);
  return res;
}
