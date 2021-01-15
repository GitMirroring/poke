/* Jitter: Flex scanner.

   Copyright (C) 2016, 2017, 2018, 2020 Luca Saiu
   Updated in 2019 and 2021 by Luca Saiu
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


/* Preliminary C code.  This is not included in the generated header.
 * ************************************************************************** */

%{
/* Include the Gnulib header. */
#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "jitterc-vm.h"
#include "jitterc-parser.h"


/* Re-declare two automatically-defined flex functions with
   __attribute__((unused)), to prevent an annoying GCC warning. */
static int input  (yyscan_t yyscanner)
  __attribute__ ((unused));
static void yyunput (int c, char * yy_bp , yyscan_t yyscanner)
  __attribute__ ((unused));

/* Given some text matching the {CODE} rule, return it stripped
   of the initial "code", the trailing "end" and the optional
   comments on both lines..
   This destructively alters the memory pointed by text, and
   returns a pointer within it. */
char*
jitterc_strip_code (char *text)
{
  int i;
  /* Find the beginning of the last line, searching right-to-left.  There is
     always a separate newline character at the very end, preceded by a line
     containing optional whitespace, then "end", then optional whitespace and an
     optional comment. */
  for (i = strlen (text) - 1 - 1 /* Skip the one newline at the very end. */;
       text [i] != '\n';
       i --)
    /* Do nothing. */;

  /* Within the last line find the first 'e' character; that is where the "end"
     begins and therefore the actual code ends.  Replace the 'e' character with
     a string terminator. */
  for (; text [i] != 'e'; i ++)
    /* Do nothing. */;
  text [i] = '\0';

  /* Ignore characters from the beginning up to the first newline character.
     What comes before is "code" plus optional whitespace and an optional
     comment, in Jitter syntax, which would be invalid in C. */
  for (i = 0; text [i] != '\n'; i ++)
    /* Do nothing. */;

  /* Now i contains the offset of the first newline character from the
     beginning.  We want to skip as many characters. */
  return text + i;
}

/* Provide aliases for a few identifiers not renamed by %option prefix. */
#define YYSTYPE JITTERC_STYPE
#define YYLTYPE JITTERC_LTYPE

%}


/* Flex options.
 * ************************************************************************** */

%option bison-bridge bison-locations nodefault noyywrap prefix="jitterc_"
%option reentrant yylineno


/* Fundamental regular expression definitions.
 * ************************************************************************** */

/* FIXME: factor the number parsing and constant expression functionality from
   program.[ly] within this frontend, as far as possible. */
UNSIGNEDFIXNUM   0|([1-9][0-9]*)
FIXNUM           [-+]?{UNSIGNEDFIXNUM}
IDENTIFIER       [_a-zA-Z][-+._~@/\\a-zA-Z0-9]*

REGISTER_OR_STACK_LETTER             [a-z]
UPPER_CASE_REGISTER_OR_STACK_LETTER  [A-Z]
BARE_ARGUMENT                        ([nlf]|{UPPER_CASE_REGISTER_OR_STACK_LETTER})+

REGISTER         "%r"{UNSIGNEDFIXNUM}
WHITESPACE       [\ \t\n\r\f]+
NEWLINE          [\n\r\f]
COMMENT          "#".*{NEWLINE}
STRING           \"([^"]|(\\\"))*\"


/* Single-line regular expression definitions.
 * ************************************************************************** */

/* Flex definitions with names starting in "LINE" match only within a single
   line, never recognizing '\n', '\r' or '\f'. */

LINECOMMENT      "#".*
LINEWHITESPACE    [\ \t]+
LINENOTENDORWHITESPACE    [^e\ \t\n\r\f]|(e{LINENOTNDORWHITESPACE})
LINENOTNDORWHITESPACE     [^n\ \t\n\r\f]|(n{LINENOTDORWHITESPACE})
LINENOTDORWHITESPACE      [^d\ \t\n\r\f]
LINENOTCOMMENT             [^\n\r\f#]+


/* Code block definition.
 * ************************************************************************** */

/* The C code in code..end blocks is delimited according to a a very rigid
   whitespace-sensitive format, in order to let the user include her C code
   without being bothered by Jitter syntax, and to make the Emacs major mode
   simpler.  On the other hand this recognizer in Flex is not simple at all
   despite the language being regular, and deserves a few comments.

   The rest of Jitter's syntax, notably the use of "end" when not closing code
   blocks, is much more lax. */

/* A code block is made by the opeining line, zero or more content lines
   holding the actual C code, and one closing line. */
CODEBLOCK       {CODEOPENLINE}{CODECONTENTLINE}*{CODECLOSELINE}

/* The opening line starts with the "code" keyword, optionally preceded and followed
   by whitespace and a comment, and ends with a mandatory newline. */
CODEOPENLINE    "code"{LINEWHITESPACE}?{LINECOMMENT}?{NEWLINE}

/* The closing line has a syntax similar to the opening line.  It starts with the
   "end" keyword, optionally preceded and followed by whitespace and optionally
   followed by a comment, and ends with a mandatory newline. */
CODECLOSELINE   {LINEWHITESPACE}?"end"{LINEWHITESPACE}?{LINECOMMENT}?{NEWLINE}

/* Each content line has optional whitespace, the actual line content, and a
   mandatory newline.  Notice that this does not force the Jitter file to end
   in a newline, since code a block cannot be the last token according to the
   Bison syntax. */
CODECONTENTLINE {LINEWHITESPACE}?{CODECONTENTLINEMIDDLE}{NEWLINE}

/* The actual line content has one of three possible formats.  It can be:
   - empty ;
   - nonempty text *not* matching "end" but allowed to match a Jitter comment,
     followed by zero or more non-newline characters, which are allowed to
     match "end";
   - "end" followed by zero or more non-newline whitespace character, one
     non-newline non-whitespace non-comment-opening character, then any number
     of non-newline characters. */
CODECONTENTLINEMIDDLE {CODECONTENTLINEMIDDLE1}|{CODECONTENTLINEMIDDLE2}|{CODECONTENTLINEMIDDLE3}
CODECONTENTLINEMIDDLE1 ""
CODECONTENTLINEMIDDLE2 {LINENOTENDORWHITESPACE}[^\n\r\f]*
CODECONTENTLINEMIDDLE3 end{LINEWHITESPACE}?[^\ \t\n\r\f#][^\n\r\f]*


/* End of the definition section.
 * ************************************************************************** */

%%

 /* Rules section (comments in this section cannot start at column 0).
 * ************************************************************************** */

{CODEBLOCK}               { yytext = jitterc_strip_code (yytext);
                            return CODE; }
({WHITESPACE}|{COMMENT})+ { /* Do nothing. */ }
"end"                     { return END; }
"vm"                      { return VM; }
"set"                     { return SET; }
"stack"                   { return STACK; }

  /*"letter"                  { return LETTER; }*/
"c-type"                  { return C_TYPE; }
"c-element-type"          { return C_ELEMENT_TYPE; }
"c-initial-value"         { return C_INITIAL_VALUE; }

"register-class"          { return REGISTER_CLASS; }
"fast-register-no"        { return FAST_REGISTER_NO; }
"no-slow-registers"       { return NO_SLOW_REGISTERS; }
"slow-registers"          { return SLOW_REGISTERS; }

"long-name"               { return LONG_NAME; }
"element-no"              { return ELEMENT_NO; }
"non-tos-optimized"       { return NON_TOS_OPTIMIZED; }
"tos-optimized"           { return TOS_OPTIMIZED; }
"no-guard-underflow"      { return NO_GUARD_UNDERFLOW; }
"no-guard-overflow"       { return NO_GUARD_OVERFLOW; }
"guard-underflow"         { return GUARD_UNDERFLOW; }
"guard-overflow"          { return GUARD_OVERFLOW; }

"initial-header-c"        { return INITIAL_HEADER_C; }
"initial-vm1-c"           { return INITIAL_VM1_C; }
"initial-vm2-c"           { return INITIAL_VM2_C; }
"initial-vm-main-c"       { return INITIAL_VM_MAIN_C; }
"initial-vmmain-c"        { /* An alias. */ return INITIAL_VM_MAIN_C; }
"early-header-c"          { return EARLY_HEADER_C; }
"late-header-c"           { return LATE_HEADER_C; }
"printer-c"               { return PRINTER_C; }
"rewriter-c"              { return REWRITER_C; }
"early-c"                 { return EARLY_C; }
"late-c"                  { return LATE_C; }
"initialization-c"        { return INITIALIZATION_C; }
"finalization-c"          { return FINALIZATION_C; }
"state-early-c"           { return STATE_EARLY_C; }
"state-struct-backing-c"  { return STATE_BACKING_STRUCT_C; }
"state-struct-runtime-c"  { return STATE_RUNTIME_STRUCT_C; }
"state-initialization-c"  { return STATE_INITIALIZATION_C; }
"state-reset-c"           { return STATE_RESET_C; }
"state-finalization-c"    { return STATE_FINALIZATION_C; }
"instruction-beginning-c" { return INSTRUCTION_BEGINNING_C; }
"instruction-end-c"       { return INSTRUCTION_END_C; }

"wrapped-functions"       { return WRAPPED_FUNCTIONS; }
"wrapped-globals"         { return WRAPPED_GLOBALS; }
"instruction"             { return INSTRUCTION; }
"("                       { return OPEN_PAREN; }
")"                       { return CLOSE_PAREN; }
"?"                       { return IN; }
"!"                       { return OUT; }
","                       { return COMMA; }
";"                       { return SEMICOLON; }
{STRING}                  { return STRING; }
{FIXNUM}                  { return FIXNUM; }
"BITSPERWORD"             { return BITSPERWORD; }
"BYTESPERWORD"            { return BYTESPERWORD; }
"LGBYTESPERWORD"          { return LGBYTESPERWORD; }
"hot"                     { return HOT; }
"cold"                    { return COLD; }
"caller"                  { return CALLER; }
"callee"                  { return CALLEE; }
"returning"               { return RETURNING; }
"relocatable"             { return RELOCATABLE; }
"non-relocatable"         { return NON_RELOCATABLE; }
"rule"                    { return RULE; }
"when"                    { return WHEN; }
"$"{IDENTIFIER}           { return RULE_PLACEHOLDER; }
"true"                    { return TRUE_; }
"false"                   { return FALSE_; }
"rewrite"                 { return REWRITE; }
"into"                    { return INTO; }

{REGISTER_OR_STACK_LETTER} { /* When expecting an identifier the parser accepts
                                a REGISTER_OR_STACK_LETTER as well. */
                             return REGISTER_OR_STACK_LETTER; }
{BARE_ARGUMENT}           { /* When expecting an identifier the parser accepts
                               a BARE_ARGUMENT as well. */
                            return BARE_ARGUMENT; }
{IDENTIFIER}              { return IDENTIFIER; }
.                         { jitterc_scan_error (yyscanner); }

%%
