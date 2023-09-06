/* JitterLisp: reader header.

   Copyright (C) 2017, 2018 Luca Saiu
   Updated in 2023 by Luca Saiu
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


#ifndef JITTERLISP_READER_H_
#define JITTERLISP_READER_H_

#include <jitter/jitter.h>

#include <stdio.h>

#include "jitterlisp-sexpression.h"




/* Char-reading definitions.
 * ************************************************************************** */

/* This API makes it easy to use the same functions for reading from a stream,
   from a string in memory or from some other source defined by the user. */

/* This type is purely conventional: the actual char-reader state type depends
   on the char-reader function, which receives a pointer to a char-reader
   state which it will cast to the appropriate pointer type. */
typedef void * jitterlisp_char_reader_state;

/* A char-reading function, at every call, returns either a valid char from the
   input or EOF.  The exact nature of the state depends on the reader function,
   but the function will update the pointed state after reading each
   character. */
typedef int (*jitterlisp_char_reader_function)
   (jitterlisp_char_reader_state *char_reader_state_pointer);

/* A function freeing up resources for the pointed char-reader state. */
typedef void (*jitterlisp_char_reader_finalizer)
   (jitterlisp_char_reader_state *char_reader_state_pointer);




/* Predefined char-readers.
 * ************************************************************************** */

/* A char-reader function reading from a '\0'-terminated string.  The argument,
   declared as jitterlisp_char_reader_state * for compatibility with
   jitterlisp_char_reader_function , is actually of type const char ** .  The
   function increments the pointed const char * pointer after reading each
   character, unless it has reached the terminator already.
   The user can destroy the pointed string after reading is over; in that case,
   however, it's her responsibility to keep a pointer to the original beginning
   of the string: at every character read the pointer-to-pointer-to-char which
   is passed as the char-reader state will be dereferenced, and the pointed
   pointer-to-char advanced. */
int
jitterlisp_string_char_reader_function
   (jitterlisp_char_reader_state *file_star_star)
  __attribute__ ((nonnull (1)));

/* A char-reader function reading from a FILE * input stream.  The argument,
   declared as jitterlisp_char_reader_state * for compatibility with
   jitterlisp_char_reader_function , is actually of type FILE ** (two levels of
   pointers), and the stream must be open for reading at at the correct
   position.  After reading is over the caller may close the stream. */
int
jitterlisp_stream_char_reader_function
   (jitterlisp_char_reader_state *file_star_star)
  __attribute__ ((nonnull (1)));




/* Reader state.
 * ************************************************************************** */

/* In order to read s-expressions from a stream, a string or something else
   the user needs to make a "reader state" structure, containing some data
   to be kept between one s-expression and the next. */

/* The reader state is an abstract type, whose definition is subject to change.
   The user should only use the functions below to make and destroy instances of
   this struct. */
struct jitterlisp_reader_state;

/* A function called after each successful parsing of a top-level s-expression
   on a pointer to the char-reader state, a pointer to the reader state, and the
   s-expression which was just parsed.
   Rationale: this exists so that I'm able to use readline from a Lisp
   function reading *one* s-expression, but possibly spanning multiple lines,
   inside an ordinary REPL.  The idea is stopping after the inner reader reads
   *one* s-expression, checking that there are no trailing tokens. */
typedef void (*jitterlisp_post_parsing_hook)
   (jitterlisp_char_reader_state *crspp,
    struct jitterlisp_reader_state *rsp,
    jitterlisp_object o);

/* Return a pointer to a new reader state made from the given char-reader
   function, the given char-reader state, the given char-reader finalizer
   and the given post-parsing hool; the finalizer and hook are allowed to be
   NULL, in which case no action is performed.

   Notice that the char-reader state is not pointed, differently from the
   argument in jitterlisp_char_reader_function : the char-reader state here is
   *copied*, and then a pointer to it is passed to
   jitterlisp_char_reader_function .

   This function needs to read the first character of the input, and therefore
   is potentially blocking. */
// FIXME: make it non-blocking.
struct jitterlisp_reader_state*
jitterlisp_make_reader_state
   (jitterlisp_char_reader_function char_reader,
    jitterlisp_char_reader_state char_reader_state,
    jitterlisp_char_reader_finalizer char_reader_finalizer,
    jitterlisp_post_parsing_hook post_parsing_hook)
  __attribute__ ((nonnull (1, 2), returns_nonnull));

/* Destroy the given reader state, freeing up its resources.  Calling this does
   not explicitly close files or streams, however. */
void
jitterlisp_destroy_reader_state (struct jitterlisp_reader_state *rs)
  __attribute__ ((nonnull (1)));




/* Reader state convenience functions.
 * ************************************************************************** */

/* The functions below, all simple wrappers around jitterlisp_make_reader_state
   , have the advantage of hiding char readers from the user.  The user is still
   responsible for releasing any external resource for the input such as file
   descriptors, streams or string memory. */

/* Return a pointer to a fresh reader state reading from the pointed stream,
   which must be open for reading.
   This function needs to read the first character (not token) of the input,
   and therefore is potentially blocking. */
// FIXME: make it non-blocking.
struct jitterlisp_reader_state*
jitterlisp_make_stream_reader_state (FILE *input)
  __attribute__ ((nonnull (1), returns_nonnull));

/* Return a pointer to a fresh reader state reading from the pointed
   '\0'-terminated string.
   This function needs to read the first character (not token) of the input, and
   therefore is potentially blocking. */
// FIXME: make it non-blocking.
struct jitterlisp_reader_state*
jitterlisp_make_string_reader_state (const char *string)
  __attribute__ ((nonnull (1), returns_nonnull));

/* Return a fresh reader state reading s-expression from readline calls,
   automatically performed as needed with the given prompt.  The prompt
   is copied  internally and the user is allowed to release or ovewrite
   its memory immediately.  The s-expressions being read are allowed to
   span multiple lines.
   This functions is blocking for the same reasons as the ones above. */
// FIXME: make it non-blocking.
struct jitterlisp_reader_state*
jitterlisp_make_readline_reader_state (const char *prompt)
  __attribute__ ((nonnull (1), returns_nonnull));

/* Like jitterlisp_make_readline_reader_state but read only one s-expression,
   still allowed to span multiple lines.  Fail with a parse error if there is
   any trailing garbage, including other complete s-expressions, after the
   one s-expression we are supposed to read.
   This functions is blocking for the same reasons as the ones above.
   Rationale: see the comment before jitterlisp_post_parsing_hook . */
// FIXME: make it non-blocking.
struct jitterlisp_reader_state*
jitterlisp_make_readline_one_reader_state (const char *prompt)
  __attribute__ ((nonnull (1), returns_nonnull));




/* S-expression reader.
 * ************************************************************************** */

/* Return the next s-expression read from the given reader state, or #<eof> if
   there is nothing more to read. */
jitterlisp_object
jitterlisp_read (struct jitterlisp_reader_state *rs)
  __attribute__ ((nonnull (1)));




/* S-expression readline convenience reader.
 * ************************************************************************** */

/* Read and return exactly one s-expression (allowed to span multiple lines)
   using readline.  This is a convenience wrapper over the functions above,
   hiding reader states from the user. */
jitterlisp_object
jitterlisp_read_readline_one (const char *prompt)
  __attribute__ ((nonnull (1)));


#endif // #ifndef JITTERLISP_READER_H_
