/* Jitter: custom contextual printing: GNU libtextstyle wrapper.

   Copyright (C) 2020 Luca Saiu
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


/* Include Jitter headers, particularly the print context facility.  Before
   including jitter.h I need to define JITTER_INTERNAL , so as to avoid the
   sanity checks which would make compilation fail in case a dispatch is not
   specified as a CPP macro.  Nothing depends on dispatches here. */
#define JITTER_INTERNAL
#include <jitter/jitter.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-print.h>




/* Fail if we are compiling this on a configuration where we should not.
 * ************************************************************************** */

/* This should not even be compiled if Jitter has not been configured with
   support for GNU Libtextstyle. */
#if ! defined (JITTER_HAVE_LIBTEXTSTYLE)
# error "jitter/jitter-print-libtextstyle.c should not be compiled if Jitter"
# error "was not configured with support for GNU libtextstyle.  This C file"
# error "is part (in fact the entirety) of the sources for the wrapper"
# error "library libjitter-libtextstyle.la which Jitter's Makefile.am should"
# error "have disabled in your configuration.  Have you found a build system"
# error "bug?"
#endif // #if ! defined (JITTER_HAVE_LIBTEXTSTYLE)




/* Include the headers required by GNU Libtextstyle. */
#include <string.h>  /* For strcmp . */
#include <textstyle.h>


/* Context kind definition: libtextstlye context kind.
 * ************************************************************************** */

/* This context kind uses an styled_ostream_t object, which is already a
   poitner,  as its data.  There is no need to allocate or destroy data from
   this wrapper. */

/* Print the given single char using the ostream as data. */
static int
jitter_print_context_libtextstyle_print_chars (jitter_print_context_data data,
                                               const char *p, size_t char_no)
{
  /* Write and assume success, since ostream_write_mem does not notify of
     errors through its result. */
  ostream_write_mem ((styled_ostream_t) data, p, char_no);
  return 0;
}

/* Flush the ostream data. */
static int
jitter_print_context_libtextstyle_flush (jitter_print_context_data data)
{
  /* Since ostream_flush returns void, assume success. */
  ostream_flush ((styled_ostream_t) data, FLUSH_ALL);
  return 0;
}

/* Return non-false iff the given decoration name and type are recognised by
   Libtextstyle.  This is useful for ignoring unknown decorations or apparently
   known decorations with an unexpected value type. */
static bool /*jitter_bool*/
jitter_print_context_libtextstyle_valid_decoration
   (const jitter_print_decoration_name name,
    enum jitter_print_decoration_type type)
{
  return ((type == jitter_print_decoration_type_string)
          && (! strcmp (name, JITTER_PRINT_DECORATION_NAME_CLASS)
              || ! strcmp (name, JITTER_PRINT_DECORATION_NAME_HYPERLINK)));
}

/* Begin using the given decoration. */
static int
jitter_print_context_libtextstyle_begin_decoration
   (jitter_print_context_data data,
    const jitter_print_decoration_name name,
    enum jitter_print_decoration_type type,
    const union jitter_print_decoration_value *value)
{
  /* Do nothing if the decoration is invalid. */
  if (! jitter_print_context_libtextstyle_valid_decoration (name, type))
    return 0;

  /* If we are here we can assume that the decoration type is string,
     and that the name is either JITTER_PRINT_DECORATION_NAME_CLASS or
     JITTER_PRINT_DECORATION_NAME_HYPERLINK. */
  if (! strcmp (name, JITTER_PRINT_DECORATION_NAME_CLASS))
    styled_ostream_begin_use_class ((styled_ostream_t) data, value->string);
  else // (! strcmp (name, JITTER_PRINT_DECORATION_NAME_HYPERLINK))
    styled_ostream_set_hyperlink ((styled_ostream_t) data, value->string, NULL);

  /* Assume success, like libtextstyle does. */
  return 0;
}

/* Stop using the given decoration. */
static int
jitter_print_context_libtextstyle_end_decoration
   (jitter_print_context_data data,
    const jitter_print_decoration_name name,
    enum jitter_print_decoration_type type,
    const union jitter_print_decoration_value *value)
{
  /* Do nothing if the decoration is invalid. */
  if (! jitter_print_context_libtextstyle_valid_decoration (name, type))
    return 0;

  /* If we are here we can assume that the decoration type is string,
     and that the name is either JITTER_PRINT_DECORATION_NAME_CLASS or
     JITTER_PRINT_DECORATION_NAME_HYPERLINK. */
  if (! strcmp (name, JITTER_PRINT_DECORATION_NAME_CLASS))
    styled_ostream_end_use_class ((styled_ostream_t) data, value->string);
  else // (! strcmp (name, JITTER_PRINT_DECORATION_NAME_HYPERLINK))
    styled_ostream_set_hyperlink ((styled_ostream_t) data, NULL, NULL);

  /* Assume success, like libtextstyle does. */
  return 0;
}




/* Globals, initialisation and finalisation.
 * ************************************************************************** */

static jitter_print_context_kind
jitter_print_context_kind_libtextstyle
  = NULL /* Out of defensiveness. */;

static bool /*jitter_bool*/
jitter_print_libtextstyle_initialized = false;

void
jitter_print_libtextstyle_initialize (void)
{
  if (jitter_print_libtextstyle_initialized)
    jitter_fatal ("jitter_print_libtextstyle_initialize: initialised twice");

  /* Make a context kind struct full of NULL function pointers. */
  jitter_print_context_kind_libtextstyle
    = jitter_print_context_kind_make_trivial ();

  /* Set its fields that should be non-NULL. */
  jitter_print_context_kind_libtextstyle->print_chars
    = jitter_print_context_libtextstyle_print_chars;
  jitter_print_context_kind_libtextstyle->flush
    = jitter_print_context_libtextstyle_flush;
  jitter_print_context_kind_libtextstyle->begin_decoration
    = jitter_print_context_libtextstyle_begin_decoration;
  jitter_print_context_kind_libtextstyle->end_decoration
    = jitter_print_context_libtextstyle_end_decoration;

  jitter_print_libtextstyle_initialized = true;
}

void
jitter_print_libtextstyle_finalize (void)
{
  if (! jitter_print_libtextstyle_initialized)
    jitter_fatal ("jitter_print_libtextstyle_finalize: not initialised");

  jitter_print_context_kind_destroy (jitter_print_context_kind_libtextstyle);
  jitter_print_context_kind_libtextstyle = NULL;  /* Out of defensiveness. */

  jitter_print_libtextstyle_initialized = false;
}




/* Making a Libtextstyle context.
 * ************************************************************************** */

jitter_print_context
jitter_print_context_make_libtextstyle (styled_ostream_t ostream)
{
  return jitter_print_context_make (jitter_print_context_kind_libtextstyle,
                                    ostream);
}
