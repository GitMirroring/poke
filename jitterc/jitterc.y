/* Jitter: Bison parser.

   Copyright (C) 2016, 2017, 2018, 2020, 2021 Luca Saiu
   Updated in 2019 by Luca Saiu
   Written by Luca Saiu

   This file is part of Jitter.

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


// FIXME: the #line support is really really ugly.  I should rewrite it.


/* This code does not go to the generated header. */
%{
/* Include the Gnulib header. */
#include <config.h>

#include <stdio.h>
#include <ctype.h>
#include <c-ctype.h>
#include <jitter/jitter-bitwise.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-parse-int.h>
#include <jitter/jitter-string.h>
#include <gl_xlist.h>
#include <gl_array_list.h>

#include "jitterc-vm.h"
#include "jitterc-mangle.h"
#include "jitterc-rewrite.h"
#include "jitterc-utility.h"
#include "jitterc-parser.h"
#include "jitterc-scanner.h"

/* This is currently a fatal error.  I could longjmp away instead. */
static void
jitterc_error (YYLTYPE *locp, struct jitterc_vm *vm,
               yyscan_t scanner, char *message)
  __attribute__ ((noreturn));

#define JITTERC_PARSE_ERROR(message)                      \
  do                                                      \
    {                                                     \
      jitterc_error (jitterc_get_lloc (jitterc_scanner),  \
                       vm, jitterc_scanner, message);     \
    }                                                     \
  while (false)

/* Set the given property of the last instruction to the given enum case,
   checking that each property is not set more than once.  This is useful
   for enumerate-valued properties such as hotness and relocatability. */
#define JITTERC_SET_PROPERTY(property, value)                  \
  do                                                           \
    {                                                          \
      enum jitterc_ ## property *property                      \
        = & jitterc_vm_last_instruction (vm)->property;        \
      if (* property != jitterc_ ## property ## _unspecified)  \
        JITTERC_PARSE_ERROR("duplicate " # property);          \
      * property = jitterc_ ## property ## _ ## value;         \
    }                                                          \
  while (false)

/* What would be yytext in a non-reentrant scanner. */
#define JITTERC_TEXT \
  (jitterc_get_text (jitterc_scanner))

 /* What would be yylineno in a non-reentrant scanner. */
#define JITTERC_LINENO \
  (jitterc_get_lineno (jitterc_scanner))

/* A copy of what would be yytext in a non-reentrant scanner. */
#define JITTERC_TEXT_COPY \
  (jitter_clone_string (JITTERC_TEXT))

/* Assign the given lvalue with a string concatenation of its current value and
   the new string from the code block, preceded by a #line CPP directive unless
   #line-generation was disabled.  Free both strings (but not the pointed
   struct, which normally comes from internal Bison data structures). */
#define JITTERC_APPEND_CODE(lvalue, code_block_pointerq)                        \
  do                                                                            \
    {                                                                           \
       struct jitterc_code_block *code_block_pointer = code_block_pointerq;     \
       int line_number = code_block_pointer->line_number;                       \
       char *new_code = code_block_pointer->code;                               \
       char *line_line = xmalloc (strlen (vm->source_file_name) + 100);         \
       if (vm->generate_line)                                                   \
         sprintf (line_line, "#line %i \"%s\"\n",                               \
                  line_number,                                                  \
                  vm->source_file_name);                                        \
       else                                                                     \
         line_line [0] = '\0';                                                  \
       size_t line_line_length = strlen (line_line);                            \
       size_t lvalue_length = strlen (lvalue);                                  \
       char *concatenation                                                      \
         = xrealloc (lvalue,                                                    \
                     lvalue_length + line_line_length                           \
                     + strlen (new_code) + 1);                                  \
       strcpy (concatenation + lvalue_length,                                   \
               line_line);                                                      \
       strcpy (concatenation + lvalue_length + line_line_length,                \
               new_code);                                                       \
       free (line_line);                                                        \
       free (new_code);                                                         \
       /* Poison the pointer still in the struct, just for defensiveness. */    \
       code_block_pointer->code = NULL;                                         \
       lvalue = concatenation;                                                  \
    }                                                                           \
  while (false)                                                                 \

/* Return a struct jitterc_code_block containing a copy of the pointed text,
   starting at the given line number. */
struct jitterc_code_block
jitterc_make_code_struct (struct jitterc_vm *vm,
                          const char *text, int line_number)
{
  /* Count newline characters. */
  int newline_no = 0;
  int i;
  for (i = 0; text [i] != '\0'; i++)
    if (text [i] == '\n')
      newline_no ++;

  /* Make the struct to be returned.  Its initial content must be
     malloc-allocated, here with jitter_clone_string, because of 
     JITTERC_APPEND_CODE which frees its parameters. */
  struct jitterc_code_block res;
  char *s = xmalloc (1);
  strcpy (s, "");
  res.line_number = line_number - newline_no - 1;
  res.code = jitter_clone_string (text);
  JITTERC_APPEND_CODE(s, & res);
  res.code = s;
  return res;
}

/* FIXME: unfactor this code back into the only rule which should need it. */
#define KIND_CASE(character, suffix)                      \
  case character:                                         \
    if (k & jitterc_instruction_argument_kind_ ## suffix) \
      JITTERC_PARSE_ERROR("duplicate " #suffix " kind");  \
    k |= jitterc_instruction_argument_kind_ ## suffix;    \
  break;
#define KIND_CASE_DEFAULT(out, character)                   \
  default:                                                  \
    if (c_isupper (character))                                \
      {                                                     \
        if (k & jitterc_instruction_argument_kind_register) \
          JITTERC_PARSE_ERROR("duplicate register kind");   \
        k |= jitterc_instruction_argument_kind_register;    \
        out.register_class_letter = tolower (character);    \
      }                                                     \
    else                                                    \
      JITTERC_PARSE_ERROR("invalid kind letter");

%}

/* We need a recent enough version of GNU Bison. */
%require "3.0" /* 2.3b was the first version supporting %define api.pure ,
                  but such old versions have not been tested in a long time,
                  and now Bison (as of 3.5) refuses to accept the "b" suffix
                  in requirements.  It appears that 2.3b was not an official
                  release. */

/* Use a prefix different from the default "yy" for the API. */
%define api.prefix {jitterc_}

/* Generate a header file. */
%defines

/* This is a reentrant parser. */
/*%define api.pure full*/ /* FIXME: I'd need to %require "3.0" for this.  Do I
                             care about the difference?  Probably not. */
%define api.pure

/* We need to receive location information from the scanner, Bison-style. */
%locations

/* The parser and scanner functions both have additional parameters. */
%lex-param { jitterc_scan_t jitterc_scanner }
%parse-param { struct jitterc_vm *vm }
%parse-param { void* jitterc_scanner }

/* We don't need a %initial-action block, because the parser receives an already
   initialized data structure; see the definition of jitterc_parse_file_star . */

/* This goes to the parser header file. */
%code requires {
/* The value associated to a bare_argument nonterminal -- which is to say, an
   argument without a mode.  This is only used within the parser, but needs go
   the header as well as one of the %type cases. */
struct jitterc_bare_argument
{
  /* The argument kind. */
  enum jitterc_instruction_argument_kind kind;

  /* The register letter, lower-case.  Only meaningful if the kind contains the
     register case. */
  char register_class_letter;
};

/* A code block to copy in the output.  This is only used within the parser, but
   needs go the header as well as one of the %type cases. */
struct jitterc_code_block
{
  /* The line number where the code block begins, in the Jitter VM specification
     file.  This is useful for friendlier error reporting thru the #line CPP
     feature. */
  int line_number;

  /* A malloc-allocated string. */
  char *code;
};

/* Simplified error-reporting facilities calling jitterc_error, suitable to be
   called from the scanner and the parser without the complicated and
   irrelevant parameters needed by jitterc_error . */
void
jitterc_scan_error (void *jitterc_scanner) __attribute__ ((noreturn));

/* Return a pointer to a fresh VM data structure parsed from the pointed stream,
   or fail fatally.  Don't generate #line directives iff generate_line is false.
   Rationale: unfortunately some C code generation already happens in the
   parser, so generate_line must be supplied early. */
struct jitterc_vm *
jitterc_parse_file_star (FILE *input_file, bool generate_line);

/* Like jitterc_parse_file_star, but parsing from a file whose pathname is
   given. */
struct jitterc_vm *
jitterc_parse_file (const char *input_file_name, bool generate_line);
}

%union
{
  char character;
  char* string;
  gl_list_t string_list;
  enum jitterc_instruction_argument_mode mode;
  struct jitterc_bare_argument bare_argument;
  jitter_int fixnum;
  bool boolean;
  struct jitterc_code_block code_block;
  struct jitterc_code_block legal_notice;

  struct jitterc_argument_pattern *argument_pattern;
  struct jitterc_template_expression *template_expression;
  struct jitterc_instruction_pattern *instruction_pattern;
  struct jitterc_instruction_template *instruction_template;

  /* List elements are pointers to struct jitterc_argument_pattern . */
  gl_list_t argument_patterns;

  /* List elements are pointers to struct jitterc_template_expression . */
  gl_list_t template_expressions;

  /* List elements are pointers to struct jitterc_instruction_pattern . */
  gl_list_t instruction_patterns;

  /* List elements are pointers to struct jitterc_instruction_template . */
  gl_list_t instruction_templates;

  /* Register-class section contents and stack section contents consist in
     pointers to VM structs holding all the data. */
  struct jitterc_register_class *register_class;
  struct jitterc_stack *stack;
}

%token VM LEGAL_NOTICE END CODE /*END_CODE*/ STRING
%token SET
%token INITIAL_HEADER_C INITIAL_VM1_C INITIAL_VM2_C INITIAL_VM_MAIN_C
%token EARLY_HEADER_C LATE_HEADER_C
%token PRINTER_C REWRITER_C
%token EARLY_C LATE_C INITIALIZATION_C FINALIZATION_C
%token STATE_EARLY_C
%token STATE_BACKING_STRUCT_C STATE_RUNTIME_STRUCT_C
%token STATE_INITIALIZATION_C STATE_RESET_C STATE_FINALIZATION_C
%token INSTRUCTION_BEGINNING_C INSTRUCTION_END_C
%token BARE_ARGUMENT IDENTIFIER WRAPPED_FUNCTIONS WRAPPED_GLOBALS
%token INSTRUCTION OPEN_PAREN CLOSE_PAREN COMMA SEMICOLON IN OUT
/* Some obscure configurations predefine TRUE and FALSE as CPP macros, which is
   why I have to append an underscore here.  Yes, these also turn into CPP
   macros, which is questionable but probably required for Yacc
   compatibility. */
%token RULE WHEN REWRITE INTO TRUE_ FALSE_ RULE_PLACEHOLDER
%token HOT COLD RELOCATABLE NON_RELOCATABLE
%token NON_BRANCHING BRANCHING CALLER CALLEE RETURNING
%token COMMUTATIVE NON_COMMUTATIVE TWO_OPERANDS
%token REGISTER_CLASS FAST_REGISTER_NO REGISTER_OR_STACK_LETTER
%token SLOW_REGISTERS NO_SLOW_REGISTERS
%token STACK /*LETTER*/ C_TYPE C_INITIAL_VALUE C_ELEMENT_TYPE LONG_NAME ELEMENT_NO
%token NON_TOS_OPTIMIZED TOS_OPTIMIZED
%token NO_GUARD_OVERFLOW NO_GUARD_UNDERFLOW GUARD_OVERFLOW GUARD_UNDERFLOW

%token FIXNUM BITSPERWORD BYTESPERWORD LGBYTESPERWORD

%type <string_list> identifiers;
%type <string> identifier placeholder string;
%type <string> optional_identifier optional_placeholder; /* either a heap-allocated string or NULL */
%type <character> register_or_stack_letter;
%type <code_block> code;
%type <code_block> legal_notice;
%type <mode> modes mode_character modes_rest;
%type <bare_argument> bare_argument;
%type <fixnum> literal;
%type <boolean> literals; /* This is true iff there is at least one literal. */
%type <string> optional_printer_name; /* NULL if there is no printer. */
%type <argument_pattern> rule_argument_pattern
%type <argument_patterns> rule_argument_patterns_zero_or_more
                          rule_argument_patterns_one_or_more
%type <instruction_pattern> rule_instruction_pattern
%type <instruction_patterns> rule_instruction_patterns_zero_or_more
                             rule_instruction_patterns_one_or_more
%type <template_expression> rule_guard rule_expression rule_operation;
%type <template_expressions> rule_expressions_zero_or_more
                             rule_expressions_one_or_more;
%type <instruction_template> rule_instruction_template
%type <instruction_templates> rule_instruction_templates_zero_or_more

%type <register_class> register_class_section_contents
%type <stack>          stack_section_contents

%%

vm:
  sections
;

sections:
  /* nothing */
| section sections
;

section:
  legal_notice
| vm_section
| c_section
| wrapped_functions_section
| wrapped_globals_section
| register_class_section
| stack_section
| instruction_section
| rule_section
;

vm_section:
  VM
    vm_section_contents
  END /*VM*/
;

/* This is not a section with contents: the legal-notice element counts as an
   entire block.  I want it to be easy to write at the top level near the
   beginning of the file, without any nested groups. */
legal_notice:
  LEGAL_NOTICE
  { /* Add the current text, temporarily disabling #lineno directives even if
       they are enabled for C code.  The non-reentrancy is dirty but acceptable
       in this code generator, meant to always remain single-threaded. */
    bool old_generate_line = vm->generate_line;
    vm->generate_line = false;
    $$ = jitterc_make_code_struct (vm, JITTERC_TEXT, JITTERC_LINENO);
    JITTERC_APPEND_CODE(vm->legal_notice, & $$);
    vm->generate_line = old_generate_line; }
;

vm_section_contents:
  /* nothing */
| setting vm_section_contents
/*| stack_declaration vm_section_contents*/
;

setting:
  SET identifier string  { jitterc_vm_add_setting (vm, $2, $3);
                           free ($2); }
;

c_section:
  INITIAL_HEADER_C code END /*INITIAL_HEADER_C*/
    { JITTERC_APPEND_CODE(vm->initial_header_c_code, & $2); }
| INITIAL_VM1_C code END /*INITIAL_VM1_C*/
    { JITTERC_APPEND_CODE(vm->initial_vm1_c_code, & $2); }
| INITIAL_VM2_C code END /*INITIAL_VM2_C*/
    { JITTERC_APPEND_CODE(vm->initial_vm2_c_code, & $2); }
| INITIAL_VM_MAIN_C code END /*INITIAL_VM_MAIN_C*/
    { JITTERC_APPEND_CODE(vm->initial_vm_main_c_code, & $2); }
| EARLY_HEADER_C code END /*EARLY_HEADER_C*/
    { JITTERC_APPEND_CODE(vm->early_header_c_code, & $2); }
| LATE_HEADER_C code END /*LATE_HEADER_C*/
    { JITTERC_APPEND_CODE(vm->late_header_c_code, & $2); }
| PRINTER_C code END /*PRINTER_C*/
    { JITTERC_APPEND_CODE(vm->printer_c_code, & $2); }
| REWRITER_C code END /*REWRITER_C*/
    { JITTERC_APPEND_CODE(vm->rewriter_c_code, & $2); }
| EARLY_C code END /*EARLY_C*/
    { JITTERC_APPEND_CODE(vm->early_c_code, & $2); }
| LATE_C code END /*LATE_C*/
    { JITTERC_APPEND_CODE(vm->before_main_c_code, & $2); }
| INITIALIZATION_C code END /*INITIALIZATION_C*/
    { JITTERC_APPEND_CODE(vm->initialization_c_code, & $2); }
| FINALIZATION_C code END /*FINALIZATION_C*/
    { JITTERC_APPEND_CODE(vm->finalization_c_code, & $2); }
| STATE_EARLY_C code END /*STATE_EARLY_C*/
    { JITTERC_APPEND_CODE(vm->state_early_c_code, & $2); }
| STATE_BACKING_STRUCT_C code END /*STATE_BACKING_STRUCT_C*/
    { JITTERC_APPEND_CODE(vm->state_backing_struct_c_code, & $2); }
| STATE_RUNTIME_STRUCT_C code END /*STATE_RUNTIME_STRUCT_C*/
    { JITTERC_APPEND_CODE(vm->state_runtime_struct_c_code, & $2); }
| STATE_INITIALIZATION_C code END /*STATE_INITIALIZATION_C*/
    { JITTERC_APPEND_CODE(vm->state_initialization_c_code, & $2); }
| STATE_RESET_C code END /*STATE_RESET_C*/
    { /* This field is NULL at initialisation. */
      if (vm->state_reset_c_code == NULL)
        vm->state_reset_c_code = jitter_clone_string ("");
      JITTERC_APPEND_CODE(vm->state_reset_c_code, & $2); }
| STATE_FINALIZATION_C code END /*STATE_FINALIZATION_C*/
    { JITTERC_APPEND_CODE(vm->state_finalization_c_code, & $2); }
| INSTRUCTION_BEGINNING_C code END
    { JITTERC_APPEND_CODE(vm->instruction_beginning_c_code, & $2); }
| INSTRUCTION_END_C code END
    { JITTERC_APPEND_CODE(vm->instruction_end_c_code, & $2); }
;

wrapped_functions_section:
  WRAPPED_FUNCTIONS identifiers END /*WRAPPED_FUNCTIONS*/
  { jitterc_clone_list_from (vm->wrapped_functions, $2); } /* FIXME: it would be more consistent to do this by side effects. */
;

wrapped_globals_section:
  WRAPPED_GLOBALS identifiers END /*WRAPPED_GLOBALS*/
  { jitterc_clone_list_from (vm->wrapped_globals, $2); } /* FIXME: it would be more consistent to do this by side effects. */
;

identifiers: /* FIXME: no need for %type here.  I can use side effects like elsewhere.  Or not. */
  /* nothing */           { $$ = gl_list_nx_create_empty (GL_ARRAY_LIST,
                                                          NULL, NULL, NULL,
                                                          true); }
| identifier identifiers  { gl_list_add_last ($2, $1);
                            $$ = $2; }
;

rule_section:
  RULE optional_identifier
  REWRITE rule_instruction_patterns_zero_or_more
  INTO rule_instruction_templates_zero_or_more
  rule_guard
  END
  { struct jitterc_rule *rule
      = jitterc_make_rule ($4,
                           $6,
                           $7,
                           ($2 != NULL
                            ? $2
                            : jitter_clone_string ("unnamed")),
                           JITTERC_LINENO);
    jitterc_add_rule (vm, rule); }
;

optional_identifier:
  /* nothing */  { $$ = NULL; }
| identifier     { $$ = $1; }

rule_guard:
  /* nothing */
  { $$ = jitterc_make_template_expression_boolean (true, JITTERC_LINENO); }
| WHEN rule_expression
  { $$ = $2; }
;

rule_instruction_pattern:
  identifier rule_argument_patterns_zero_or_more
  { $$ = jitterc_make_instruction_pattern ($1, $2, JITTERC_LINENO); }
;

rule_instruction_patterns_zero_or_more:
  /* nothing */
  { $$ = jitterc_make_empty_list (); }
| rule_instruction_patterns_one_or_more
  { $$ = $1; }
;

rule_instruction_patterns_one_or_more:
  rule_instruction_pattern
  { $$ = jitterc_make_empty_list ();
    gl_list_add_last ($$, $1); }
| rule_instruction_pattern SEMICOLON rule_instruction_patterns_one_or_more
  { $$ = $3;
    gl_list_add_first ($$, $1); }
;

rule_argument_pattern:
  bare_argument optional_placeholder
  {
    union jitter_word irrelevant;
    $$ = jitterc_make_argument_pattern ($1.kind,
                                        false,
                                        irrelevant,
                                        $2,
                                        JITTERC_LINENO);
  }
| literal optional_placeholder
  {
    union jitter_word literal = { .fixnum = $1 };
    enum jitterc_instruction_argument_kind literal_kind
      = jitterc_instruction_argument_kind_literal;
    $$ = jitterc_make_argument_pattern (literal_kind,
                                        true,
                                        literal,
                                        $2,
                                        JITTERC_LINENO);
  }
| placeholder
  {
    union jitter_word irrelevant;
    enum jitterc_instruction_argument_kind any_kind
      = jitterc_instruction_argument_kind_unspecified;
    $$ = jitterc_make_argument_pattern (any_kind,
                                        false,
                                        irrelevant,
                                        $1,
                                        JITTERC_LINENO);
  }
;

placeholder:
  RULE_PLACEHOLDER  { /* Strip away the prefix. */
                      $$ = jitter_clone_string (JITTERC_TEXT + 1); }
;

optional_placeholder:
  /* nothing */  { $$ = NULL; }
| placeholder    { $$ = $1; }
;

rule_argument_patterns_zero_or_more:
  /* nothing */
  { $$ = jitterc_make_empty_list (); }
| rule_argument_patterns_one_or_more
  { $$ = $1; }
;

rule_argument_patterns_one_or_more:
  rule_argument_pattern
  { $$ = jitterc_make_empty_list ();
    gl_list_add_last ($$, $1); }
| rule_argument_pattern COMMA rule_argument_patterns_one_or_more
  { $$ = $3;
    gl_list_add_first ($$, $1); }
;

rule_expressions_zero_or_more:
  /* nothing */
  { $$ = jitterc_make_empty_list (); }
| rule_expression
  { $$ = jitterc_make_empty_list ();
    gl_list_add_last ($$, $1); }
| rule_expression COMMA rule_expressions_one_or_more
  { $$ = $3;
    gl_list_add_first ($$, $1); }
;

rule_expressions_one_or_more:
  rule_expression
  { $$ = jitterc_make_empty_list ();
    gl_list_add_last ($$, $1); }
| rule_expression COMMA rule_expressions_one_or_more
  { $$ = $3;
    gl_list_add_first ($$, $1); }
;

rule_expression:
  TRUE_
  { $$ = jitterc_make_template_expression_boolean (true, JITTERC_LINENO); }
| FALSE_
  { $$ = jitterc_make_template_expression_boolean (false, JITTERC_LINENO); }
| literal
  { union jitter_word w = { .fixnum = $1 };
    $$ = jitterc_make_template_expression_fixnum (w, JITTERC_LINENO); }
| placeholder
  { $$ = jitterc_make_template_expression_placeholder ($1, JITTERC_LINENO); }
| OPEN_PAREN rule_expression CLOSE_PAREN
  { $$ = $2; }
| rule_operation
  { $$ = $1; }
;

rule_operation:
  identifier OPEN_PAREN rule_expressions_zero_or_more CLOSE_PAREN
  { $$ = jitterc_make_template_expression_operation ($1, $3, JITTERC_LINENO); }
;

rule_instruction_templates_zero_or_more:
  /* nothing */
  { $$ = jitterc_make_empty_list (); }
| rule_instruction_template
  { $$ = jitterc_make_empty_list ();
    gl_list_add_last ($$, $1); }
  /* FIXME: I would like to remove the SEMICOLON token or make it optional,
     but I have to pay attention to parsing conflicts. */
| rule_instruction_template SEMICOLON rule_instruction_templates_zero_or_more
  { $$ = $3;
    gl_list_add_first ($$, $1); }
;

rule_instruction_template:
  identifier rule_expressions_zero_or_more
  { $$ = jitterc_make_instruction_template ($1, $2, JITTERC_LINENO); }
;

register_class_section:
  REGISTER_CLASS register_or_stack_letter register_class_section_contents END
    { jitterc_vm_register_class_set_letter ($3, $2);
      jitterc_vm_add_register_class (vm, $3); }
;

register_class_section_contents:
  /* nothing */
    { $$ = jitterc_make_register_class (); }
| LONG_NAME string register_class_section_contents
    { jitterc_vm_register_class_set_long_name ($3, $2);
      $$ = $3; }
| C_TYPE string register_class_section_contents
    { jitterc_vm_register_class_set_c_type ($3, $2);
      $$ = $3; }
| C_INITIAL_VALUE string register_class_section_contents
    { jitterc_vm_register_class_set_c_initial_value ($3, $2);
      $$ = $3; }
| FAST_REGISTER_NO literal register_class_section_contents
    { jitterc_vm_register_class_set_fast_register_no ($3, $2);
      $$ = $3; }
| NO_SLOW_REGISTERS register_class_section_contents
    { jitterc_vm_register_class_set_use_slow_registers ($2, 0);
      $$ = $2; }
| SLOW_REGISTERS register_class_section_contents
    { jitterc_vm_register_class_set_use_slow_registers ($2, 1);
      $$ = $2; }
;

stack_section:
  STACK register_or_stack_letter stack_section_contents END
    { jitterc_vm_stack_set_letter ($3, $2);
      jitterc_vm_add_stack (vm, $3); }
;

stack_section_contents:
  /* nothing */
    { $$ = jitterc_vm_make_stack (); }
| LONG_NAME string stack_section_contents
    { jitterc_vm_stack_set_long_name ($3, $2);
      $$ = $3; }
| C_ELEMENT_TYPE string stack_section_contents
    { jitterc_vm_stack_set_c_element_type ($3, $2);
      $$ = $3; }
| ELEMENT_NO literal stack_section_contents
    { jitterc_vm_stack_set_element_no ($3, $2);
      $$ = $3; }
| C_INITIAL_VALUE string stack_section_contents
    { jitterc_vm_stack_set_c_initial_value ($3, $2);
      $$ = $3; }
| NO_GUARD_UNDERFLOW stack_section_contents
    { jitterc_vm_stack_set_guard_underflow ($2, 0);
      $$ = $2; }
| NO_GUARD_OVERFLOW stack_section_contents
    { jitterc_vm_stack_set_guard_overflow ($2, 0);
      $$ = $2; }
| GUARD_UNDERFLOW stack_section_contents
    { jitterc_vm_stack_set_guard_underflow ($2, 1);
      $$ = $2; }
| GUARD_OVERFLOW stack_section_contents
    { jitterc_vm_stack_set_guard_overflow ($2, 1);
      $$ = $2; }
| TOS_OPTIMIZED stack_section_contents
    { jitterc_vm_stack_set_implementation ($2, jitterc_stack_implementation_tos);
      $$ = $2; }
| NON_TOS_OPTIMIZED stack_section_contents
    { jitterc_vm_stack_set_implementation ($2,
                                           jitterc_stack_implementation_no_tos);
      $$ = $2; }
;

register_or_stack_letter:
  REGISTER_OR_STACK_LETTER { $$ = JITTERC_TEXT [0]; }
;

instruction_section:
  INSTRUCTION
  { jitterc_vm_append_instruction (vm, jitterc_make_instruction ()); }
  identifier OPEN_PAREN arguments CLOSE_PAREN properties code END /*INSTRUCTION*/
  { /* Make an instruction, and initialize its fields. */
    struct jitterc_instruction *ins = jitterc_vm_last_instruction (vm);
    ins->name = $3;
    ins->mangled_name = jitterc_mangle (ins->name);
    ins->code = $8.code;
    /* The arguments have already been added one by one by the argument rule. */
    if (ins->hotness == jitterc_hotness_unspecified)
      ins->hotness = jitterc_hotness_hot;
    if (ins->relocatability == jitterc_relocatability_unspecified)
      ins->relocatability = jitterc_relocatability_relocatable;
    if (ins->callerness == jitterc_callerness_unspecified)
      ins->callerness = jitterc_callerness_non_caller;
    if (ins->calleeness == jitterc_calleeness_unspecified)
      ins->calleeness = jitterc_calleeness_non_callee;
    if (ins->returningness == jitterc_returningness_unspecified)
      ins->returningness = jitterc_returningness_non_returning;
    if (ins->has_fast_labels)
      /* An instruction with fast labels can also use non-fast branches, without
         explicitly specifying attributes (Rationale: the branching attribute
         serves to recognise the instruction as potentially defective, and an
         instruction with fast label is potentially defective already). */
      ins->branchingness = jitterc_branchingness_branching;
    if (   ins->has_fast_labels
        && ins->relocatability == jitterc_relocatability_non_relocatable)
      /* FIXME: I might want to allow this. */
      JITTERC_PARSE_ERROR("a non-relocatable instruction has fast labels");
    if (   ins->callerness == jitterc_callerness_caller
        && ins->relocatability == jitterc_relocatability_non_relocatable)
      /* FIXME: I might want to allow this as well. */
      JITTERC_PARSE_ERROR("non-relocatable instructions cannot (currently) be callers");

    /* An instruction which is a caller, callee or returning is also
       automatically branching.  Rationale: the macros needed for branching
       and their replacements are the same also used for procedures. */
    if (   ins->callerness == jitterc_callerness_caller
        || ins->calleeness == jitterc_calleeness_callee
        || ins->returningness == jitterc_returningness_returning)
      {
        if (ins->branchingness == jitterc_branchingness_non_branching)
          JITTERC_PARSE_ERROR ("a non-branching instruction cannot be caller, callee or returning");
        else
          ins->branchingness = jitterc_branchingness_branching;
      }

    /* Branchingness can be affected by callerness, calleeness and
       returningness; only now we can decide on the default branchingness, if
       itsvalue was not explicitly specified. */
    if (ins->branchingness == jitterc_branchingness_unspecified)
      ins->branchingness = jitterc_branchingness_non_branching;
  }
;

arguments:
  /* nothing */
| one_or_more_arguments
;

one_or_more_arguments:
  argument
| argument COMMA one_or_more_arguments
;

argument:
  modes bare_argument
    { struct jitterc_instruction_argument *arg
        = jitterc_make_instruction_argument ();
      arg->mode = $1;
      arg->kind = $2.kind;
      if (arg->kind & jitterc_instruction_argument_kind_register)
        arg->register_class_character = $2.register_class_letter;
      if (arg->kind & jitterc_instruction_argument_kind_literal)
        {
          if (arg->mode & jitterc_instruction_argument_mode_out)
            JITTERC_PARSE_ERROR("a literal cannot be an output");

          /* FIXME: this might need to be generalized or cleaned up
             in the future. */
          arg->literal_type = jitterc_literal_type_fixnum;
        }
      if (arg->kind & jitterc_instruction_argument_kind_label)
        {
          if (arg->mode & jitterc_instruction_argument_mode_out)
            JITTERC_PARSE_ERROR("a label cannot be an output");
        }
      if (arg->kind & jitterc_instruction_argument_kind_fast_label)
        {
          jitterc_vm_last_instruction (vm)->has_fast_labels = true;
          if (arg->mode & jitterc_instruction_argument_mode_out)
            JITTERC_PARSE_ERROR("a fast label cannot be an output");
          if (arg->kind != jitterc_instruction_argument_kind_fast_label)
            JITTERC_PARSE_ERROR("a fast label must be the only kind");
        }
      jitterc_vm_append_argument (vm, arg);
    }
  literals optional_printer_name
    {
      struct jitterc_instruction_argument *arg
        = jitterc_vm_last_argument (vm);
      if (   ! (arg->kind & jitterc_instruction_argument_kind_literal)
             && $4)
        JITTERC_PARSE_ERROR("literals for a non-literal argument");
      if (   ! (arg->kind & jitterc_instruction_argument_kind_literal)
          && ($5 != NULL))
        JITTERC_PARSE_ERROR("a non-literal argument cannot have a printer");
      arg->c_literal_printer_name = $5;
    }
;

optional_printer_name:
  /* nothing */  { $$ = NULL; }
| identifier     { $$ = $1; }
;

modes:
  mode_character modes_rest
    { if ($1 & $2)
        JITTERC_PARSE_ERROR("duplicate mode");
      $$ = $1 | $2; }
;

modes_rest:
  /* nothing */
    { $$ = jitterc_instruction_argument_mode_unspecified; }
  | mode_character modes_rest
    { if ($1 & $2)
        JITTERC_PARSE_ERROR("duplicate mode");
      $$ = $1 | $2; }
;

mode_character:
  IN   { $$ = jitterc_instruction_argument_mode_in; }
| OUT  { $$ = jitterc_instruction_argument_mode_out; }
;

/* FIXME: this special case for REGISTER_OR_STACK_LETTER is ugly, and should be
   simplified or eliminated altogether when I decide on a new syntax for
   kinds. */
bare_argument:
  REGISTER_OR_STACK_LETTER
  {
    enum jitterc_instruction_argument_kind k
      = jitterc_instruction_argument_kind_unspecified;
    char c = JITTERC_TEXT [0];
    switch (c)
      {
      KIND_CASE('n', literal)
      KIND_CASE('l', label)
      KIND_CASE('f', fast_label)
      KIND_CASE_DEFAULT($$, c)
      }
    $$.kind = k;
  }
| BARE_ARGUMENT
  {
    char *text = JITTERC_TEXT;
    enum jitterc_instruction_argument_kind k
      = jitterc_instruction_argument_kind_unspecified;
    int i;
    for (i = 0; text [i] != '\0'; i ++)
      switch (text [i])
        {
        KIND_CASE('n', literal)
        KIND_CASE('l', label)
        KIND_CASE('f', fast_label)
        KIND_CASE_DEFAULT($$, text [i])
        }
    $$.kind = k;
  }
;

properties:
  /* nothing */
| hotness properties
| branchingness properties
| relocatability properties
| callrelatedness properties
;

hotness:
  HOT   { JITTERC_SET_PROPERTY(hotness, hot); }
| COLD  { JITTERC_SET_PROPERTY(hotness, cold); }
;

relocatability:
  RELOCATABLE     { JITTERC_SET_PROPERTY(relocatability, relocatable); }
| NON_RELOCATABLE { JITTERC_SET_PROPERTY(relocatability, non_relocatable); }
;

branchingness:
  NON_BRANCHING { JITTERC_SET_PROPERTY(branchingness, non_branching); }
| BRANCHING     { JITTERC_SET_PROPERTY(branchingness, branching); }
;

callrelatedness:
  CALLER   { JITTERC_SET_PROPERTY(callerness, caller); }
| CALLEE   { JITTERC_SET_PROPERTY(calleeness, callee);
             const struct jitterc_instruction *ins = jitterc_vm_last_instruction (vm);
             if (gl_list_size (ins->arguments) > 0)
               JITTERC_PARSE_ERROR
                  ("a callee instruction cannot have arguments"); }
| RETURNING { JITTERC_SET_PROPERTY(returningness, returning); }
;

/* Sometimes we expect text in a very strict form, so there are special cases in
   the lexicon; but when we want an identifier we indifferently accept either a
   special case, or the general one. */
identifier:
  BARE_ARGUMENT             { $$ = JITTERC_TEXT_COPY; }
| REGISTER_OR_STACK_LETTER  { $$ = JITTERC_TEXT_COPY; }
| IDENTIFIER                { $$ = JITTERC_TEXT_COPY; }
;

string:
  STRING     { /* FIXME: unescape properly. */
               char *text = JITTERC_TEXT;
               text [strlen (text) - 1] = '\0'; text ++;
               $$ = jitter_clone_string (text); }
;

code:
  CODE
  { $$ = jitterc_make_code_struct (vm, JITTERC_TEXT, JITTERC_LINENO); }
;

literal:
  FIXNUM          { /* Since the string has been matched by the scanner
                       the conversion is actually safe in this case. */
                    $$ = jitter_string_to_long_long_unsafe (JITTERC_TEXT); }
| BITSPERWORD     { $$ = JITTER_BITS_PER_WORD; }
| BYTESPERWORD    { $$ = JITTER_BYTES_PER_WORD; }
| LGBYTESPERWORD  { $$ = JITTER_LG_BYTES_PER_WORD; }

literals:
  /* nothing */
           { $$ = false; }
| literal  { struct jitterc_instruction_argument *arg
               = jitterc_vm_last_argument (vm);
             /* FIXME: this will need generalization later on. */
             union jitterc_literal_value literal_value
               = {.fixnum = $1};
             struct jitterc_literal *literal
               = jitterc_make_literal (jitterc_literal_type_fixnum,
                                         literal_value);
             gl_list_add_last (arg->literals, literal); }
  literals { $$ = true; }

%%

void
jitterc_error (YYLTYPE *locp, struct jitterc_vm *vm, yyscan_t jitterc_scanner,
                 char *message)
{
  printf ("%s:%i: %s near \"%s\".\n",
          (vm != NULL) ? vm->source_file_name : "<INPUT>",
          jitterc_get_lineno (jitterc_scanner), message, JITTERC_TEXT);
  exit (EXIT_FAILURE);
}

void
jitterc_scan_error (void *jitterc_scanner)
{
  struct jitterc_vm *vm = NULL; /* A little hack to have vm in scope. */
  JITTERC_PARSE_ERROR("scan error");
}

static struct jitterc_vm *
jitterc_parse_file_star_with_name (FILE *input_file, const char *file_name,
                                   bool generate_line)
{
  yyscan_t scanner;
  jitterc_lex_init (&scanner);
  jitterc_set_in (input_file, scanner);

  struct jitterc_vm *res = jitterc_make_vm ();
  res->source_file_name = jitter_clone_string (file_name);

  /* Set res->generate_line now, before the parsing phase actually starts.  This
     way the code generated at parsing time will be affected. */
  res->generate_line = generate_line;

  /* FIXME: if I ever make parsing errors non-fatal, call jitterc_lex_destroy before
     returning, and finalize the program -- which might be incomplete! */
  if (jitterc_parse (res, scanner))
    jitterc_error (jitterc_get_lloc (scanner), res, scanner, "parse error");
  jitterc_set_in (NULL, scanner);
  jitterc_lex_destroy (scanner);

  /* Now that we have all the unspecialized instructions and all the rules we
     can analyze the VM. */
  jitterc_analyze_vm (res);

  return res;
}

struct jitterc_vm *
jitterc_parse_file_star (FILE *input_file, bool generate_line)
{
  return jitterc_parse_file_star_with_name (input_file, "<stdin>",
                                            generate_line);
}

struct jitterc_vm *
jitterc_parse_file (const char *input_file_name, bool generate_line)
{
  FILE *f;
  if ((f = fopen (input_file_name, "r")) == NULL)
    jitter_fatal ("failed opening file %s", input_file_name);

  /* FIXME: if I ever make parse errors non-fatal, I'll need to close the file
     before returning. */
  struct jitterc_vm *res
    = jitterc_parse_file_star_with_name (f, input_file_name, generate_line);
  fclose (f);
  return res;
}
