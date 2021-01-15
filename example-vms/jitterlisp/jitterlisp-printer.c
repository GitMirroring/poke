/* JitterLisp: printer.

   Copyright (C) 2017, 2018, 2019, 2020 Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with Jitter under the same license.

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


/* Include the Gnulib header. */
#include <config.h>

#include "jitterlisp-printer.h"

#include <stdio.h>
#include <unistd.h>

#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-print.h>

#include "jitterlisp-settings.h"
#include "jitterlisp-sexpression.h"
#include "jitterlisp-ast.h"




/* Sharing-recognition hashing.
 * ************************************************************************** */

/* This hash table data structure serves to avoid infinite loops when printing
   circular structures, and to show which substructures are shared (currently
   without showing *what* is shared, but this can be improved).
   Since this data structure and the printing process in general doesn't use
   Lisp heap allocation in this case it's safe to simply hash on tagged
   objects, even if they are pointers, even with a moving GC -- which is not
   there yet but should come at some point. */

/* Initialize a sharing table, already allocated by the user.  This is
   intended to be used on a pointer to an automatic variable. */
static void
jitterlisp_sharing_table_initialize (struct jitter_hash_table *t)
{
  jitter_hash_initialize (t);
}

/* Finalize a sharing table, already allocated by the user.  This is
   intended to be used on a pointer to an automatic variable. */
static void
jitterlisp_sharing_table_finalize (struct jitter_hash_table *t)
{
  jitter_word_hash_finalize (t, jitter_do_nothing_on_word);
}

/* Return non-false iff the given object is already in the sharing table. */
static bool
jitterlisp_sharing_table_has (struct jitter_hash_table *t,
                              jitterlisp_object o)
{
  return jitter_word_hash_table_has (t, o);
}

/* Add the given object to the sharing table, if the object can potentially
   be circular.  Do nothing otherwise.  This doesn't check whether the object is
   already in the table: it would be a useless source of inefficiency.  */
static void
jitterlisp_sharing_table_add (struct jitter_hash_table *t,
                              jitterlisp_object o)
{
  /* A Lisp object whose printed representation cannot contain other Lisp
     object should not be kept in the table; do nothing in that case. */
  if (! JITTERLISP_IS_RECURSIVE(o))
    return;

  /* Add an entry to the table.  The value is not used. */
  union jitter_word useless = { .fixnum = 0 };
  jitter_word_hash_table_add (t, o, useless);
}




/* Character names.
 * ************************************************************************** */

const struct jitterlisp_character_name_binding
jitterlisp_non_ordinary_character_name_bindings []
  =
    {
      { '\0', "nul" },
      { ' ',  "space" },
      { '\n', "newline" },
      { '\n', "linefeed" },
      { '\r', "cr" },
      { '\r', "return" },
      { '\f', "page" }
    };

const size_t
jitterlisp_non_ordinary_character_name_binding_no
  = (sizeof (jitterlisp_non_ordinary_character_name_bindings)
     / sizeof (const struct jitterlisp_character_name_binding));




/* Char-printing utility.
 * ************************************************************************** */

/* Use the given char-printer to emit a printed representation of the given
   character, be it ordinary or non-ordinary. */
static void
jitterlisp_print_character_name (jitter_print_context cx, jitter_int c)
{
  /* Print the #\ prefix, which is the same for ordinary and non-ordinary
     characters. */
  jitter_print_char_star (cx, "#\\");

  /* Look for the first name binding for c as a non-ordinary character.  If one
     exists, print it and return. */
  int i;
  for (i = 0; i < jitterlisp_non_ordinary_character_name_binding_no; i ++)
    if (jitterlisp_non_ordinary_character_name_bindings [i].character == c)
      {
        char *name = jitterlisp_non_ordinary_character_name_bindings [i].name;
        jitter_print_char_star (cx, name);
        return;
      }

  /* Since we haven't found a binding c must be an ordinary character.  Print it
     as it is. */
  jitter_print_char (cx, c);
}

/* Begin the named class in the given print context, unless colorising has
   been disabled. */
void
jitterlisp_begin_class (jitter_print_context cx, const char *name_suffix)
{
  char buffer [1000];
  sprintf (buffer, "jitterlisp-%s", name_suffix);
  jitter_print_begin_class (cx, buffer);
}

/* End the last begun class in the given print context, unless colorising has
   been disabled. */
void
jitterlisp_end_class (jitter_print_context cx)
{
  jitter_print_end_class (cx);
}




/* S-expression printer.
 * ************************************************************************** */

/* Forward declaration.  Print the given object in the given print context using
   the pointed sharing table. */
static void
jitterlisp_print_recursive (jitter_print_context cx,
                            struct jitter_hash_table *st,
                            jitterlisp_object o);


/* Print o as the cdr of a cons, with the car already printed and the
   surrounding parentheses printed by the caller, using the given
   char-printer. */
static void
jitterlisp_print_cdr (jitter_print_context cx,
                      struct jitter_hash_table *st, jitterlisp_object o)
{
  /* Show sharing. */
  if (jitterlisp_sharing_table_has (st, o))
    {
      jitterlisp_begin_class (cx, "cons");
      jitter_print_char_star (cx, " . ");
      jitterlisp_end_class (cx);
      jitterlisp_begin_class (cx, "circular");
      jitter_print_char_star (cx, "...");
      jitterlisp_end_class (cx);
      return;
    }
  jitterlisp_sharing_table_add (st, o);

  if (JITTERLISP_IS_EMPTY_LIST(o))
    {
      /* There is nothing to print, not even a space: the caller has already
         written the open parens and will append the matching closed parens
         right after this function returns. */
    }
  else if (JITTERLISP_IS_CONS(o))
    {
      /* So, o is another cons: print o's car as the next list element, but
         first separate it from the previous element, which must exist if we got
         here, with a space. */
      jitterlisp_begin_class (cx, "cons");
      jitter_print_char (cx, ' ');
      jitterlisp_end_class (cx);
      struct jitterlisp_cons * const c = JITTERLISP_CONS_DECODE(o);
      jitterlisp_print_recursive (cx, st, c->car);

      /* We're still within a list or improper/dotted list and so we'll keep
         using cdr notation for o's cdr, without adding more parens.  If o's cdr
         is still a cons then the recursive call will prepend a space to the
         elements. */
      jitterlisp_print_cdr (cx, st, c->cdr);
    }
  else
    {
      /* The innermost cdr of the spine is not (): this is an improper/dotted
         list. */
      jitterlisp_begin_class (cx, "cons");
      jitter_print_char_star (cx, " . ");
      jitterlisp_end_class (cx);
      jitterlisp_print_recursive (cx, st, o);
    }
}

/* Print the pointed s-expressions in order starting from the given initial
   pointer and going on for element_no elements.  Use a single space as a
   separator before each element, including the first. */
static void
jitterlisp_print_subs (jitter_print_context cx,
                       struct jitter_hash_table *st,
                       jitterlisp_object *elements, size_t element_no)
{
  int i;
  for (i = 0; i < element_no; i ++)
    {
      jitterlisp_begin_class (cx, "ast");
      jitter_print_char (cx, ' ');
      jitterlisp_end_class (cx);
      jitterlisp_print_recursive (cx, st, elements [i]);
    }
}

/* Print the pointed AST to the given context using the pointed share table. */
static void
jitterlisp_print_ast (jitter_print_context cx,
                      struct jitter_hash_table *st, struct jitterlisp_ast *ast)
{
  /* There's no need to check for sharing here: this function is only called
     by jitterlisp_print_recursive which has already done it on the same
     argument, and the AST subs are printed thru jitterlisp_print_recursive . */

  jitterlisp_begin_class (cx, "ast");
  jitter_print_char_star (cx, "[");
  switch (ast->case_)
    {
    case jitterlisp_ast_case_literal:
      jitter_print_char_star (cx, "literal");
      break;
    case jitterlisp_ast_case_variable:
      jitter_print_char_star (cx, "variable");
      break;
    case jitterlisp_ast_case_define:
      jitter_print_char_star (cx, "define");
      break;
    case jitterlisp_ast_case_if:
      jitter_print_char_star (cx, "if");
      break;
    case jitterlisp_ast_case_setb:
      jitter_print_char_star (cx, "set!");
      break;
    case jitterlisp_ast_case_while:
      jitter_print_char_star (cx, "while");
      break;
    case jitterlisp_ast_case_primitive:
      jitter_print_char_star (cx, "primitive "); /* Space.  See below. */
      break;
    case jitterlisp_ast_case_call:
      jitter_print_char_star (cx, "call");
      break;
    case jitterlisp_ast_case_lambda:
      jitter_print_char_star (cx, "lambda");
      break;
    case jitterlisp_ast_case_let:
      jitter_print_char_star (cx, "let");
      break;
    case jitterlisp_ast_case_sequence:
      jitter_print_char_star (cx, "sequence");
      break;
    default:
      jitter_print_char_star (cx, "invalid]");
      jitterlisp_end_class (cx);
      return;
    }
  jitterlisp_end_class (cx);
  /* I can have a special case for primitives: instead of printing the entire
     primitive object, which is very verbose, just print the primitive name when
     occurring within a primitive AST.  There is no ambiguity, and the notation
     gets much leaner: */
  if (ast->case_ == jitterlisp_ast_case_primitive)
    {
      struct jitterlisp_primitive * const primitive
        = JITTERLISP_PRIMITIVE_DECODE(ast->subs [0]);
      jitterlisp_begin_class (cx, "primitive");
      jitter_print_char_star (cx, primitive->name);
      jitterlisp_end_class (cx);
      jitterlisp_print_subs (cx, st, ast->subs + 1, ast->sub_no - 1);
    }
  else
    /* Default non-primitive case: print every sub using its own decoration. */
    jitterlisp_print_subs (cx, st, ast->subs, ast->sub_no);
  jitterlisp_begin_class (cx, "ast");
  jitter_print_char_star (cx, "]");
  jitterlisp_end_class (cx);
}

static void
jitterlisp_print_recursive (jitter_print_context cx,
                            struct jitter_hash_table *st, jitterlisp_object o)
{
  /* Before printing anything, check whether we have printed this object
     already.  If so print a sharing indicator and just return; otherwise add
     the object to the table for the next time (as long as it's a potential
     source of sharing) and go on.

     A special case: ignore AST sharing when printing.  Sharing sub-ASTs,
     particularly literals and variables, is harmless, and ASTs must not be
     circular anyway: if they were, we'd in trouble for reasons much worse than
     printing.  Shared literal *values* inside ASTs are treated as always.  We
     still keep track of ASTs in the table, since the information may be needed
     for an output notation when we print shared structures in an explicit way
     in the future. */
  if (JITTERLISP_IS_AST(o))
    {
      if (! jitterlisp_sharing_table_has (st, o))
        jitterlisp_sharing_table_add (st, o);
    }
  else if (jitterlisp_sharing_table_has (st, o))
    {
      jitterlisp_begin_class (cx, "circular");
      jitter_print_char_star (cx, "...");
      jitterlisp_end_class (cx);
      return;
    }
  else
    jitterlisp_sharing_table_add (st, o);

  /* Print the object according to its type. */
  if (JITTERLISP_IS_FIXNUM(o))
    {
      jitter_int decoded = JITTERLISP_FIXNUM_DECODE(o);
      jitterlisp_begin_class (cx, "fixnum");
      jitter_print_long_long (cx, 10, decoded);
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_UNIQUE(o))
    {
      jitter_uint index = JITTERLISP_UNIQUE_DECODE(o);
      if (index < JITTERLISP_UNIQUE_OBJECT_NO)
        {
          jitterlisp_begin_class (cx, "unique");
          jitter_print_char_star (cx,
                                  jitterlisp_unique_object_names [index]);
        }
      else
        {
          jitterlisp_begin_class (cx, "invalid");
          jitter_print_char_star (cx, "#<invalid-unique-object:");
          jitter_print_long_long (cx, 10, index);
          jitter_print_char (cx, '>');
        }
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_CHARACTER(o))
    {
      jitterlisp_begin_class (cx, "character");
      jitter_int c = JITTERLISP_CHARACTER_DECODE(o);
      jitterlisp_print_character_name (cx, c);
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_SYMBOL(o))
    {
      struct jitterlisp_symbol *s = JITTERLISP_SYMBOL_DECODE(o);
      if (s->name_or_NULL != NULL)
        {
          /* Print an interned symbol. */
          jitterlisp_begin_class (cx, "interned-symbol");
          jitter_print_char_star (cx, s->name_or_NULL);
        }
      else if (jitterlisp_settings.print_compact_uninterned_symbols)
        {
          /* Print an uninterned symbol in compact notation. */
          jitterlisp_begin_class (cx, "uninterned-symbol");
          jitter_print_char_star (cx, "#<u");
          jitter_print_long_long (cx, 10, (jitter_long_long) s->index);
          jitter_print_char_star (cx, ">");
        }
      else
        {
          /* Print an uninterned symbol in the default notation. */
          jitterlisp_begin_class (cx, "uninterned-symbol");
          jitter_print_char_star (cx, "#<uninterned:");
          jitter_print_char_star (cx, "0x");
          jitter_print_long_long (cx, 16, (jitter_uint) s);
          jitter_print_char_star (cx, ">");
        }
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_COMPILED_CLOSURE(o))
    {
      struct jitterlisp_closure *c = JITTERLISP_CLOSURE_DECODE(o);
      struct jitterlisp_compiled_closure *cc = & c->compiled;
      jitterlisp_begin_class (cx, "closure");
      jitter_print_char_star (cx, "#<compiled-closure ");
      jitter_print_long_long (cx, 10, c->in_arity);
      jitter_print_char_star (cx, "-ary");
      jitter_print_char_star (cx, " nonlocals ");
      jitterlisp_print_recursive (cx, st, cc->nonlocals);
      jitter_print_char_star (cx, ">");
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_INTERPRETED_CLOSURE(o))
    {
      struct jitterlisp_interpreted_closure * const ic
        = & JITTERLISP_CLOSURE_DECODE(o)->interpreted;
      jitterlisp_begin_class (cx, "closure");
      jitter_print_char_star (cx, "#<interpreted-closure ");
      jitterlisp_print_recursive (cx, st, ic->environment);
      jitter_print_char (cx, ' ');
      jitterlisp_print_recursive (cx, st, ic->formals);
      jitter_print_char (cx, ' ');
      jitterlisp_print_recursive (cx, st, ic->body);
      jitter_print_char (cx, '>');
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_NON_PRIMITIVE_MACRO(o))
    {
      struct jitterlisp_interpreted_closure * const closure
        = JITTERLISP_NON_PRIMITIVE_MACRO_DECODE(o);
      jitterlisp_begin_class (cx, "non-primitive-macro");
      jitter_print_char_star (cx, "#<macro ");
      jitterlisp_print_recursive (cx, st, closure->environment);
      jitter_print_char (cx, ' ');
      jitterlisp_print_recursive (cx, st, closure->formals);
      jitter_print_char (cx, ' ');
      jitterlisp_print_recursive (cx, st, closure->body);
      jitter_print_char_star (cx, ">");
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_PRIMITIVE(o))
    {
      struct jitterlisp_primitive * const primitive
        = JITTERLISP_PRIMITIVE_DECODE(o);
      jitterlisp_begin_class (cx, "primitive");
      jitter_print_char_star (cx, "#<primitive ");
      jitter_print_char_star (cx, primitive->name);
      jitter_print_char_star (cx, " ");
      jitter_print_long_long (cx, 10, primitive->in_arity);
      jitter_print_char_star (cx, "-ary>");
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_PRIMITIVE_MACRO(o))
    {
      struct jitterlisp_primitive * const primitive
        = JITTERLISP_PRIMITIVE_MACRO_DECODE(o);
      jitterlisp_begin_class (cx, "primitive_macro");
      jitter_print_char_star (cx, "#<primitive macro ");
      jitter_print_char_star (cx, primitive->name);
      jitter_print_char_star (cx, ">");
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_BOX(o))
    {
      jitterlisp_begin_class (cx, "box");
      jitter_print_char_star (cx, "#<box ");
      jitterlisp_print_recursive (cx, st, JITTERLISP_EXP_B_A_GET(o));
      jitter_print_char (cx, '>');
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_CONS(o))
    {
      struct jitterlisp_cons * const c = JITTERLISP_CONS_DECODE(o);
      jitterlisp_object car = c->car;
      jitterlisp_object cdr = c->cdr;
      jitterlisp_begin_class (cx, "cons");
      jitter_print_char (cx, '(');
      jitterlisp_print_recursive (cx, st, car);
      jitterlisp_print_cdr (cx, st, cdr);
      jitter_print_char (cx, ')');
      jitterlisp_end_class (cx);
    }
  else if (JITTERLISP_IS_AST(o))
    {
      struct jitterlisp_ast * const ast = JITTERLISP_AST_DECODE(o);
      jitterlisp_print_ast (cx, st, ast);
    }
  else if (JITTERLISP_IS_VECTOR(o))
    {
      const struct jitterlisp_vector * const v = JITTERLISP_VECTOR_DECODE(o);
      jitterlisp_begin_class (cx, "vector");
      jitter_print_char_star (cx, "#(");
      int i;
      int element_no = JITTERLISP_FIXNUM_DECODE(v->element_no);
      for (i = 0; i < element_no; i ++)
        {
          jitterlisp_print_recursive (cx, st, v->elements [i]);
          if (i < (element_no - 1))
            jitter_print_char (cx, ' ');
        }
      jitter_print_char (cx, ')');
      jitterlisp_end_class (cx);
    }
  else
    {
      jitterlisp_begin_class (cx, "invalid");
      jitter_print_char_star (cx, "#<invalid-or-unknown>");
      jitterlisp_end_class (cx);
    }
}




/* Lisp object printer: user functions.
 * ************************************************************************** */

void
jitterlisp_print (jitter_print_context cx, jitterlisp_object o)
{
  /* Make a sharing table. */
  struct jitter_hash_table st;
  jitterlisp_sharing_table_initialize (& st);

  /* Print the object using the table. */
  jitterlisp_print_recursive (cx, & st, o);

  /* We're done with the sharing table. */
  jitterlisp_sharing_table_finalize (& st);
}

char *
jitterlisp_print_to_string (jitterlisp_object o)
{
  /* Make a temporary memory print context. */
  jitter_print_context cx = jitter_print_context_make_memory ();

  /* Print to it. */
  jitterlisp_print (cx, o);

  /* Return a copy of the printed content into in already malloc-allocated
     buffer.  Destroy the context, which does not destroy the copy. */
  char *res = jitter_print_context_get_memory (cx, NULL);
  jitter_print_context_destroy (cx);
  return res;
}




/* Print error, warning and logging messages.
 * ************************************************************************** */

static void
jitterlisp_print_char_star_internal (char *class_suffix, const char *message)
{
  jitterlisp_begin_class (jitterlisp_print_context, class_suffix);
  jitter_print_char_star (jitterlisp_print_context, message);
  jitterlisp_end_class (jitterlisp_print_context);
  jitter_print_flush (jitterlisp_print_context);

}
static void
jitterlisp_print_internal (char *class_suffix, jitterlisp_object o)
{
  jitterlisp_begin_class (jitterlisp_print_context, class_suffix);
  jitterlisp_print (jitterlisp_print_context, o);
  jitterlisp_end_class (jitterlisp_print_context);
  jitter_print_flush (jitterlisp_print_context);
}

void
jitterlisp_print_error_char_star (const char *message)
{
  jitterlisp_print_char_star_internal ("error", message);
}
void
jitterlisp_print_error (jitterlisp_object o)
{
  jitterlisp_print_internal ("error", o);
}
void
jitterlisp_log_char_star (const char *message)
{
  jitterlisp_print_char_star_internal ("log", message);
}
void
jitterlisp_log (jitterlisp_object o)
{
  jitterlisp_print_internal ("log", o);
}






/* Initialisation and finalisation.
 * ************************************************************************** */

/* The global print context */
jitter_print_context
jitterlisp_print_context = NULL /* For defensiveness's sake. */;

void
jitterlisp_printer_initialize (void)
{
  /* Initialise the GNU Libtextstyle wrapper, if used. */
#ifdef JITTER_WITH_LIBTEXTSTYLE
  jitter_print_libtextstyle_initialize ();
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE

  /* If GNU Libtextstyle is used and colorisation is enabled, initialise the
     print context using a Libtextstyle ostream... */
#ifdef JITTER_WITH_LIBTEXTSTYLE
  if (jitterlisp_settings.colorize)
    {
      char *style_file_name = "jitterlisp-style.css"; // FIXME: this is barbaric.
      styled_ostream_t ostream
        = styled_ostream_create (STDOUT_FILENO, "(stdout)", TTYCTL_AUTO,
                                 style_file_name);
      jitterlisp_print_context
        = jitter_print_context_make_libtextstyle (ostream);
    }
  else
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE
    {
      /* ...Otherwise use a non-styling context. */
      jitterlisp_print_context
        = jitter_print_context_make_file_star (stdout);
    }
}

void
jitterlisp_printer_finalize (void)
{
  jitter_print_context_destroy (jitterlisp_print_context);
  jitterlisp_print_context = NULL /* For defensiveness's sake. */;

#ifdef JITTER_WITH_LIBTEXTSTYLE
  jitter_print_libtextstyle_finalize ();
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE
}
