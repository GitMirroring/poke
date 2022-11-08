/* JitterLisp: printer header.

   Copyright (C) 2017, 2018, 2020 Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with GNU Jitter under the same license.

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


#ifndef JITTERLISP_PRINTER_H_
#define JITTERLISP_PRINTER_H_

#include <stdio.h>

#include "jitterlisp-sexpression.h"
#include <jitter/jitter-print.h>


/* The global print context.
 * ************************************************************************** */

/* This is the print context bound to the terminal or to stdout.  It may support
   styling. */
extern jitter_print_context
jitterlisp_print_context;




/* Low-level print facility: classes.
 * ************************************************************************** */

/* Begin the named class in the given print context.  The implicit class name
   prefix is "jitterlisp_". */
void
jitterlisp_begin_class (jitter_print_context cx, const char *name_suffix);

/* End the last begun class in the given print context. */
void
jitterlisp_end_class (jitter_print_context cx);




/* Print error, warning and logging messages.
 * ************************************************************************** */

/* Print text as an error message, appropriately styled, to the global
   print context. */
void
jitterlisp_log_char_star (const char *message);
void
jitterlisp_print_error_char_star (const char *message);

/* Print a Lisp object as part of an error message, appropriately styled, to the
   global print context. */
void
jitterlisp_log (jitterlisp_object o);
void
jitterlisp_print_error (jitterlisp_object o);




/* Lisp object printer.
 * ************************************************************************** */

/* Print the given Lisp object using the given print context, without
   terminating with '\n'. */
void
jitterlisp_print (jitter_print_context c, jitterlisp_object o)
  __attribute__ ((nonnull (1)));

/* Print the given JitterLisp object into a fresh malloc-allocated
   '\0'-terminated string, and return a pointer to it.  The user is responsible
   for calling free to release memory for the string. */
char *
jitterlisp_print_to_string (jitterlisp_object o)
  __attribute__ ((returns_nonnull));




/* Character names.
 * ************************************************************************** */

/* Characters are either ordinary and simply written with a #\ prefix, like for
   example #\a , or with longer names for other, not so easy to print
   characters such as #\newline .  In the case of longer non-ordinary characters
   we need to keep a mapping between actual characters such as '\n' and their
   name such as "newline".

   These definitions, used in the printer and in the reader as well, have
   nothing to do with char-printing, above. */

/* The name for a non-ordinary character. */
struct jitterlisp_character_name_binding
{
  /* The actual character, for example '\n'. */
  jitter_int character;

  /* The character name without the #\ prefix, for example "newline".  The
     string length is always strictly greater than one. */
  char *name;
};

/* The name bindings for every non-ordinary character, in no particular order.
   It is allowed for one character to have multiple names, but not the
   converse. */
extern const struct jitterlisp_character_name_binding
jitterlisp_non_ordinary_character_name_bindings [];

/* How many non-ordinary characters there are.  This is the size of
   jitterlisp_non_ordinary_character_name_bindings in elements. */
extern const size_t
jitterlisp_non_ordinary_character_name_binding_no;




/* Printer initialisation.
 * ************************************************************************** */

/* Initialise the print subsystem.  This needs to be called after the main
   initialisation function, in order to know whether to enable styling. */
void
jitterlisp_printer_initialize (void);




/* Not for the user: printer finalisation.
 * ************************************************************************** */

/* Finalise the print subsystem.  This is an internal function called by the
   global JitterLisp finalisation function. */
void
jitterlisp_printer_finalize (void);

#endif // #ifndef JITTERLISP_PRINTER_H_
