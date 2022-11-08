/* JitterLisp: error handling.

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


#include "jitterlisp-error.h"
#include "jitterlisp-printer.h"

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-string.h>


/* Global data.
 * ************************************************************************** */

/* A stack of struct jitterlisp_error_context objects.  The top element is the
   innermost pushed context where to return. */
static struct jitter_dynamic_buffer
jitterlisp_error_context_stack;

/* The last failure data which was sent; this is either NULL or a
   malloc-allocated '\0'-terminated string.
   Rationale: this is useful to keep as a global in order to propagate failures
   from inner to outer contexts. */
static char *
jitterlisp_last_message_or_NULL = NULL;

/* Free jitterlisp_last_message_or_NULL and set it to NULL, if not NULL
   already. */
static void
jitterlisp_reset_last_message_or_NULL (void)
{
  if (jitterlisp_last_message_or_NULL == NULL)
    return;

  free (jitterlisp_last_message_or_NULL);
  jitterlisp_last_message_or_NULL = NULL;
}

/* Change the value of jitterlisp_last_message_or_NULL to the given
   malloc-allocated string or NULL, freeing the previous value if needed. */
static void
jitterlisp_set_last_message_or_NULL (char *message_or_NULL)
{
  /* Do nothing when we are replacing the last message with itself (in the
     identity-by-equality sense). */
  if (message_or_NULL == jitterlisp_last_message_or_NULL)
    return;

  jitterlisp_reset_last_message_or_NULL ();
  jitterlisp_last_message_or_NULL = message_or_NULL;
}




/* Error contexts.
 * ************************************************************************** */

/* Return non-false iff there exists at least one error context in the current
   stack. */
static bool
jitterlisp_error_context_stack_empty (void)
{
  return jitterlisp_error_context_stack.used_size == 0;
}

struct jitterlisp_error_context *
jitterlisp_error_context_push (void)
{
  return jitter_dynamic_buffer_reserve (& jitterlisp_error_context_stack,
                                        sizeof (struct jitterlisp_error_context));
}

void
jitterlisp_error_context_drop (void)
{
  if (jitterlisp_error_context_stack_empty ())
    jitter_fatal ("jitterlisp_error_context_drop: there is no context");

  jitter_dynamic_buffer_pop (& jitterlisp_error_context_stack,
                             sizeof (struct jitterlisp_error_context));
}

struct jitterlisp_error_context *
jitterlisp_error_context_top (void)
{
  if (jitterlisp_error_context_stack_empty ())
    jitter_fatal ("jitterlisp_error_context_top: there is no context");

  char *first_unused_char
    = jitter_dynamic_buffer_first_unused_char (& jitterlisp_error_context_stack);
  return ((struct jitterlisp_error_context *)
          (first_unused_char - sizeof (struct jitterlisp_error_context)));
}




/* Not for the user: error-handling initialization and finalization.
 * ************************************************************************** */

void
jitterlisp_error_initialize (void)
{
  if (jitterlisp_last_message_or_NULL != NULL)
    jitter_fatal ("last_message_or_NULL not NULL at initialization: "
                  "not finalized properly?");
  jitter_dynamic_buffer_initialize (& jitterlisp_error_context_stack);
}

void
jitterlisp_error_finalize (void)
{
  jitterlisp_reset_last_message_or_NULL ();
  jitter_dynamic_buffer_finalize (& jitterlisp_error_context_stack);
}




/* Error non-local exit.
 * ************************************************************************** */

void
jitterlisp_error (char *message_or_NULL)
{
  jitterlisp_set_last_message_or_NULL (message_or_NULL);

  // FIXME: move the print somewhere else.
  if (message_or_NULL != NULL)
    {
      jitterlisp_print_error_char_star ("ERRORING OUT: ");
      jitterlisp_print_error_char_star (message_or_NULL);
      jitterlisp_print_error_char_star ("\n");
    }

  if (! jitterlisp_error_context_stack_empty ())
    {
      struct jitterlisp_error_context *jitterlisp_top_error_context
        = jitterlisp_error_context_top ();
      longjmp (jitterlisp_top_error_context->the_jmp_buf, 1);
    }
  else
    jitter_fatal ("erroring out with empty error context stack");
}

void
jitterlisp_reerror (void)
{
  jitterlisp_error (jitterlisp_last_message_or_NULL);
}




/* String-cloning wrapper error wrapper.
 * ************************************************************************** */

void
jitterlisp_error_cloned (char *message_or_NULL)
{
  jitterlisp_error (jitter_clone_string (message_or_NULL));
}
