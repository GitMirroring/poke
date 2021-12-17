/* VM-independent routine code frontend: Bison parser.

   Copyright (C) 2016, 2017, 2019, 2020 Luca Saiu
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


/* This code is included in the generated .c file, but not in the header. */
%{
  #include <stdio.h>
  #include <limits.h>
  #include <jitter/jitter.h>
  #include <jitter/jitter-instruction.h>
  #include <jitter/jitter-mutable-routine.h>

  #include <jitter/jitter-routine-parser.h>
  #include <jitter/jitter-routine-scanner.h>
  #include <jitter/jitter-fatal.h>

  /* This is currently a fatal error.  I could longjmp away instead. */
  static void
  jitter_error (YYLTYPE *locp,
                struct parser_arg *parser_arg, /*struct jitter_mutable_routine *routine,*/
                yyscan_t scanner, const char *message)
    __attribute__ ((noreturn));

  /* Just a forward-declaration.  FIXME: the error-reporting code in the jitterc
     parser is derived from this, but more advanced; I should integrate it. */
  static void
  jitter_simple_error (void *jitter_scanner, const char *message)
    __attribute__ ((noreturn));

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
/*%define api.pure full*/ /* FIXME: I'd need to %require "3.0" for this.  Do I
                             care about the difference?  Probably not. */
%define api.pure

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

  /* The structure whose pointer is passed to the parser function.  FIXME:
     revert to having just the routine as a parser argument: the VM is reachable
     from the routine. */
  struct parser_arg
  {
    /* The routine to be parsed, allowed but not required to be empty on
       input. */
    struct jitter_mutable_routine *routine;

    /* VM-dependent data.  Not modified. */
    const struct jitter_vm *vm;
  };

  /* Simplified error-reporting facilities calling vmprefix_error, suitable to be
     called from the scanner and the parser without the complicated and
     irrelevant parameters needed by vmprefix_error . */
  void
  jitter_scan_error (void *jitter_scanner) __attribute__ ((noreturn));
  void
  jitter_parse_error (void *jitter_scanner) __attribute__ ((noreturn));

  /* Parse a routine for the pointed VM from a file or a string in memory, adding
     code to the pointed VM routine.
     These functions work of course on any VM, but are slightly inconvenient for
     the user to call directly.  For this reason they are wrapped in the vm1.c
     template into VM-specific functions not requiring a VM struct pointer. */
  void
  jitter_parse_mutable_routine_from_file_star (FILE *input_file,
                                               struct jitter_mutable_routine *p,
                                               const struct jitter_vm *vm)
    __attribute__ ((nonnull (1, 2, 3)));
  void
  jitter_parse_mutable_routine_from_file (const char *input_file_name,
                                          struct jitter_mutable_routine *p,
                                          const struct jitter_vm *vm)
    __attribute__ ((nonnull (1, 2, 3)));
  void
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
%token PLUS MINUS TIMES DIV MOD
%token UNSIGNED_PLUS UNSIGNED_MINUS UNSIGNED_TIMES UNSIGNED_DIV UNSIGNED_MOD
%expect 100 /* FIXME: handle precedence and associativity.  Those shift/reduce
               conflicts should only come from expressions, unless something
               quite big escaped my attention. */
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
          /* Add one to skip the prefix. */
          jitter_mutable_routine_append_symbolic_label (parser_arg->routine,
                                                        label + 1); }
;

instruction_name :
  INSTRUCTION_NAME { char *name = jitter_get_text (jitter_scanner);
                     jitter_mutable_routine_append_instruction_name
                        (parser_arg->routine, name); }
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
| OPEN_PARENS int_expression CLOSE_PARENS { $$ = $2; }
| int_expression PLUS int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, +, $3); }
| int_expression MINUS int_expression
     { JITTER_SET_OPERATION(fixnum, $$, $1, -, $3); }
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
;

argument :
  int_expression { jitter_mutable_routine_append_literal_parameter
                      (parser_arg->routine, $1); }
| LABEL_LITERAL  { char *text = jitter_get_text (jitter_scanner) + 1; /* Skip the prefix. */
                   jitter_mutable_routine_append_symbolic_label_parameter
                      (parser_arg->routine, text); }
| REGISTER       { char *text = jitter_get_text (jitter_scanner);
                   char register_class_character = text [1];
                   const struct jitter_register_class *register_class
                     = parser_arg->routine->vm->register_class_character_to_register_class
                          (register_class_character);
                   if (register_class == NULL)
                     jitter_simple_error (jitter_scanner, "invalid register class");
                   int register_id = strtol (text + 2, NULL, 10);
                   jitter_mutable_routine_append_register_parameter
                      (parser_arg->routine, register_class, register_id); }
;

%%

/* FIXME: the error-reporting facility in generator/generator.y is derived
   from this, but improved. */
void
jitter_error (YYLTYPE *locp,
              struct parser_arg *parser_arg,
              yyscan_t jitter_scanner, const char *message)
{
  printf ("<INPUT>:%i: %s near \"%s\".\n",
          jitter_get_lineno (jitter_scanner), message,
          jitter_get_text (jitter_scanner));
  exit (EXIT_FAILURE);
}

__attribute__ ((noreturn)) static void
jitter_simple_error (void *jitter_scanner, const char *message)
{
  jitter_error (jitter_get_lloc (jitter_scanner),
            NULL, /* We have no routine here, but it's not important. */
            jitter_scanner,
            message);
}

void
jitter_scan_error (void *jitter_scanner)
{
  jitter_simple_error (jitter_scanner, "scan error");
}

void
jitter_parse_error (void *jitter_scanner)
{
  jitter_simple_error (jitter_scanner, "parse error");
}

/* This is the main parser function doing all the work.  The other parsing
   functions, meant as part of a more convenient API for the user, all rely on
   this.
   The pointed scanner must be already initialized when this is called, and
   it's the caller's responsibility to finalize it. */
static void
jitter_parse_core (yyscan_t scanner, struct jitter_mutable_routine *p,
                   const struct jitter_vm *vm)
{
  struct parser_arg pa;
  pa.vm = (struct jitter_vm *) vm;
  pa.routine = p;
  /* FIXME: if I ever make parsing errors non-fatal, call jitter_lex_destroy before
     returning. */
  if (jitter_parse (& pa, scanner))
    jitter_error (jitter_get_lloc (scanner),
                  & pa,
                  scanner, "parse error");
}

void
jitter_parse_mutable_routine_from_file_star (FILE *input_file,
                                             struct jitter_mutable_routine *p,
                                             const struct jitter_vm *vm)
{
  yyscan_t scanner;
  jitter_lex_init (& scanner);
  jitter_set_in (input_file, scanner);

  jitter_parse_core (scanner, p, vm);

  jitter_set_in (NULL, scanner);
  jitter_lex_destroy (scanner);
}

void
jitter_parse_mutable_routine_from_file (const char *input_file_name,
                                        struct jitter_mutable_routine *p,
                                        const struct jitter_vm *vm)
{
  FILE *f;
  if ((f = fopen (input_file_name, "r")) == NULL)
    {
      printf ("failed opening file");
      exit (EXIT_FAILURE);
    }

  /* FIXME: if I ever make parse errors non-fatal, I'll need to close the file
     before returning. */
  jitter_parse_mutable_routine_from_file_star (f, p, vm);
  fclose (f);
}

void
jitter_parse_mutable_routine_from_string (const char *string,
                                          struct jitter_mutable_routine *p,
                                          const struct jitter_vm *vm)
{
  yyscan_t scanner;
  jitter_lex_init (& scanner);
  jitter__scan_string (string, scanner);
  jitter_parse_core (scanner, p, vm);
  jitter_lex_destroy (scanner);
}
