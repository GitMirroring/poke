/* Jittery structured language example: Flex scanner.

   Copyright (C) 2016, 2017 Luca Saiu
   Updated in 2019 and 2020 by Luca Saiu
   Updated in 2021 by Luca Saiu
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


/* Preliminary C code.  This is not included in the generated header.
 * ************************************************************************** */

%{
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "structured-syntax.h"
#include "structured-parser.h"


/* Re-declare two automatically-defined flex functions with
   __attribute__((unused)), to prevent an annoying GCC warning. */
static int input  (yyscan_t yyscanner)
  __attribute__ ((unused));
static void yyunput (int c, char * yy_bp , yyscan_t yyscanner)
  __attribute__ ((unused));

/* Provide aliases for a few identifiers not renamed by %option prefix. */
#define YYSTYPE STRUCTURED_STYPE
#define YYLTYPE STRUCTURED_LTYPE
%}


/* Flex options.
 * ************************************************************************** */

%option bison-bridge bison-locations nodefault noyywrap prefix="structured_"
%option reentrant yylineno


/* Fundamental regular expression definitions.
 * ************************************************************************** */

/* BINARY_NATURAL       0b[0-1]+
OCTAL_NATURAL        0(o?)[0-7]+ */
DECIMAL_NATURAL      (0|([1-9][0-9]*))
/* HEXADECIMAL_NATURAL  0x([0-9]|[a-f]|[A-F])+ */

/* BINARY_INTEGER       [-+]?{BINARY_NATURAL}
OCTAL_INTEGER        [-+]?{OCTAL_NATURAL} */
DECIMAL_INTEGER      [-+]?{DECIMAL_NATURAL}
/* HEXADECIMAL_INTEGER  [-+]?{HEXADECIMAL_NATURAL} */

IDENTIFIER           [_a-zA-Z][-+._~@/\\a-zA-Z0-9]*

WHITESPACE           [\ \t\n\r\f]+
NEWLINE              [\n\r\f]
COMMENT              "//".*{NEWLINE}
/* STRING             \"([^"]|(\\\"))*\" */


/* End of the definition section.
 * ************************************************************************** */

%%

 /* Rules section (comments in this section cannot start at column 0).
 * ************************************************************************** */

({WHITESPACE}|{COMMENT})+ { /* Do nothing. */ }
"skip"                    { return SKIP; }
":="                      { return SET_TO; }
"print"                   { return PRINT; }
"input"                   { return INPUT; }
";"                       { return SEMICOLON; }
","                       { return COMMA; }
"begin"                   { return BEGIN_; }
"end"                     { return END; }
"if"                      { return IF; }
"then"                    { return THEN; }
"else"                    { return ELSE; }
"elif"                    { return ELIF; }
"while"                   { return WHILE; }
"do"                      { return DO; }
"repeat"                  { return REPEAT; }
"until"                   { return UNTIL; }
"return"                  { return RETURN; }
"("                       { return OPEN_PAREN; }
")"                       { return CLOSE_PAREN; }
"undefined"               { return UNDEFINED; }
 /* {BINARY_INTEGER}          { return BINARY_LITERAL; }
{OCTAL_INTEGER}           { return OCTAL_LITERAL; } */
{DECIMAL_INTEGER}         { return DECIMAL_LITERAL; }
"true"                    { return TRUE; }
"false"                   { return FALSE; }
 /* {HEXADECIMAL_INTEGER}     { return HEXADECIMAL_LITERAL; } */
"+"                       { return PLUS; }
"-"                       { return MINUS; }
"*"                       { return TIMES; }
"/"                       { return DIVIDED; }
"mod"                     { return REMAINDER; }
"="                       { return EQUAL; }
"<>"                      { return DIFFERENT; }
"<"                       { return LESS; }
"<="                      { return LESS_OR_EQUAL; }
">"                       { return GREATER; }
">="                      { return GREATER_OR_EQUAL; }
"and"                     { return LOGICAL_AND; }
"or"                      { return LOGICAL_OR; }
"not"                     { return LOGICAL_NOT; }
"var"                     { return VAR; }
"procedure"               { return PROCEDURE; }
{IDENTIFIER}              { return VARIABLE; }
.                         { structured_scan_error (yyscanner); }

%%
