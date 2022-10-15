/* VM-independent routine code frontend: flex scanner.

   Copyright (C) 2016, 2017 Luca Saiu
   Updated in 2019 and 2020 by Luca Saiu
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


%{
  #include <jitter/jitter.h>
  #include <jitter/jitter-routine-parser.h>

  /* Re-declare two automatically-defined flex functions with
     __attribute__((unused)), to prevent an annoying GCC warning. */
  static int input  (yyscan_t yyscanner)
    __attribute__ ((unused));
  static void yyunput (int c, char * yy_bp , yyscan_t yyscanner)
    __attribute__ ((unused));

/* Provide aliases for a few identifiers not renamed by %option prefix. */
#define YYSTYPE JITTER_STYPE
#define YYLTYPE JITTER_LTYPE
%}
%option bison-bridge bison-locations nodefault noyywrap prefix="jitter_"
%option reentrant yylineno

BINARY_NATURAL       0b[0-1]+
OCTAL_NATURAL        0(o?)[0-7]+
DECIMAL_NATURAL      (0|([1-9][0-9]*))
HEXADECIMAL_NATURAL  0x([0-9]|[a-f]|[A-F])+

BINARY_INTEGER       [-+]?{BINARY_NATURAL}
OCTAL_INTEGER        [-+]?{OCTAL_NATURAL}
DECIMAL_INTEGER      [-+]?{DECIMAL_NATURAL}
HEXADECIMAL_INTEGER  [-+]?{HEXADECIMAL_NATURAL}

IDENTIFIER       [_a-zA-Z][-+._~@/\\a-zA-Z0-9]*
LABEL_PREFIX     "$"
REGISTER_LETTER  [a-z]
REGISTER         "%"{REGISTER_LETTER}{DECIMAL_NATURAL}

WHITESPACE       [\ \t\n\r]+
COMMENT          "#".*(\n|\r)

%%

({WHITESPACE}|{COMMENT})+ { /* Do nothing. */ }
{BINARY_INTEGER}          { return SIGNED_BINARY_LITERAL; }
{OCTAL_INTEGER}           { return SIGNED_OCTAL_LITERAL; }
{DECIMAL_INTEGER}         { return SIGNED_DECIMAL_LITERAL; }
{HEXADECIMAL_INTEGER}     { return SIGNED_HEXADECIMAL_LITERAL; }
{BINARY_INTEGER}[uU]      { return UNSIGNED_BINARY_LITERAL; }
{OCTAL_INTEGER}[uU]       { return UNSIGNED_OCTAL_LITERAL; }
{DECIMAL_INTEGER}[uU]     { return UNSIGNED_DECIMAL_LITERAL; }
{HEXADECIMAL_INTEGER}[uU] { return UNSIGNED_HEXADECIMAL_LITERAL; }
{REGISTER}                { return REGISTER; }
"+"                       { return PLUS; }
"-"                       { return MINUS; }
"*"                       { return TIMES; }
"/"                       { return DIV; }
"%"                       { return MOD; }
"+"[uU]                   { return UNSIGNED_PLUS; }
"-"[uU]                   { return UNSIGNED_MINUS; }
"*"[uU]                   { return UNSIGNED_TIMES; }
"/"[uU]                   { return UNSIGNED_DIV; }
"%"[uU]                   { return UNSIGNED_MOD; }
"("                       { return OPEN_PARENS; }
")"                       { return CLOSE_PARENS; }
"BYTESPERWORD"            { return BYTESPERWORD; }
"LGBYTESPERWORD"          { return LGBYTESPERWORD; }
"BITSPERWORD"             { return BITSPERWORD; }
{IDENTIFIER}              { return INSTRUCTION_NAME; }
{LABEL_PREFIX}{IDENTIFIER}    { return LABEL_LITERAL; }
{LABEL_PREFIX}{IDENTIFIER}":" { return LABEL; }
,                         { return COMMA; }
.                         { return INVALID_TOKEN; }

%%
