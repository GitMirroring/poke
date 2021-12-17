/* JitterLisp: error handling header.

   Copyright (C) 2017, 2018 Luca Saiu
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
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTERLISP_ERROR_H_
#define JITTERLISP_ERROR_H_

#include <setjmp.h>


/* Error handling.
 * ************************************************************************** */

/* This is a very simple exception facility implemented with setjmp and longjmp
   .  It is particularly useful to handle errors graciously, freeing up
   resources in case of errors in interactive use, and letting the user retry.

   This exception subsystem has not been designed to handle recoverable errors
   at the level of VM routines, even if it should be possible to bend it in that
   direction by nested invocations of VM "programs".  Such a solution, while
   probably not the most efficient, appears easy to implement and might serve as
   an example for other applications.

   The exception facility is currently non-reentrant.  Adding a thread-local
   error context would not be difficult. */

/* Try running _jitterlisp_success_statement.  If during the statement execution
   jitterlisp_error, jitterlisp_error_cloned or jitterlisp_reerror is called
   abort the execution and longjmp to _jitterlisp_failure_statement, which is
   not executed if _jitterlisp_success_statement succeeds.  Then go back to
   the statement following the macro call, Unless _jitterlisp_failure_statement
   errors out as well, in which case the execution longjmp's back to the outer
   error handler.
   Internally, we duplicate the call to jitterlisp_error_context_drop in order
   to let the user call jitterlisp_error or jitterlisp_reerror from the failure
   statement, longjmp'ing to the outer context rather than back to the same
   error handler. */
#define JITTERLISP_HANDLE_ERRORS(_jitterlisp_success_statement,       \
                                 _jitterlisp_failure_statement)       \
  do                                                                  \
    {                                                                 \
      struct jitterlisp_error_context *_jitterlisp_new_error_context  \
        = jitterlisp_error_context_push ();                           \
      if (setjmp (_jitterlisp_new_error_context->the_jmp_buf))        \
        {                                                             \
          jitterlisp_error_context_drop ();                           \
          { _jitterlisp_failure_statement; }                          \
        }                                                             \
      else                                                            \
        {                                                             \
          { _jitterlisp_success_statement; }                          \
          jitterlisp_error_context_drop ();                           \
        }                                                             \
    }                                                                 \
  while (false)




/* Error non-local exit.
 * ************************************************************************** */

/* Print the pointed error message if non-NULL, free it if non-NULL and exit the
   current C evaluation context with a longjmp.  The message must be either NULL
   or a malloc-allocated '\0'-terminated string. */
void
jitterlisp_error (char *message_or_NULL)
  __attribute__ ((noreturn));

/* A wrapper around jitterlisp_error, passing a malloc-allocated copy of the
   given message_or_NULL when not NULL.  In this case the caller is responsible
   for releasing memory used for the passed string. */
void
jitterlisp_error_cloned (char *message_or_NULL)
  __attribute__ ((noreturn));

/* Fail again using the previous message_or_NULL.  This is useful to propagate
   failure to an outer context, after the problem was handled in the inner
   context. */
void
jitterlisp_reerror (void)
  __attribute__ ((noreturn));




/* Not for the user: error contexts.
 * ************************************************************************** */

/* The user should manipulate error contexts only using the macros from this
   header.  The data structure fields are subject to change, and even the
   function prototypes in this section only serve to define user macros. */

/* This data structure holds information about the context where to jump back to
   in case of error.  Not for the user. */
struct jitterlisp_error_context
{
  /* The jmp_buf to be used with setjmp and longjmp . */
  jmp_buf the_jmp_buf;
};

/* Return a pointer to the topmost error context, or fail fatally if none
   exists.  Not for the user. */
struct jitterlisp_error_context *
jitterlisp_error_context_top (void)
  __attribute__ ((returns_nonnull));

/* Push an uninitialized error context on the top of the error context stack,
   and return a pointer to the new structure to be initialized.  Not for the
   user. */
struct jitterlisp_error_context *
jitterlisp_error_context_push (void)
  __attribute__ ((returns_nonnull));

/* Drop the topmost error context from the stack.  Fail fatally if there is
   none.  Not for the user. */
void
jitterlisp_error_context_drop (void);




/* Not for the user: error-handling initialization and finalization.
 * ************************************************************************** */

/* These functions are called at initialization/finalization, and the user
   doesn't need to touch them. */

/* Initialize the error-handling subsystem. */
void
jitterlisp_error_initialize (void);

/* Finalize the error-handling subsystem. */
void
jitterlisp_error_finalize (void);

#endif // #ifndef JITTERLISP_ERROR_H_
