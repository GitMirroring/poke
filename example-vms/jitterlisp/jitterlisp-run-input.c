/* JitterLisp: running from files, C strings, REPL.

   Copyright (C) 2017, 2018, 2019, 2020 Luca Saiu
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


/* Include the Gnulib header. */
#include <config.h>

#include "jitterlisp-run-input.h"

#include <stdio.h>
#include <string.h>

#include "jitterlisp.h"

#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-string.h> // jitter_clone_string: probably to remove
#include <jitter/jitter-time.h>




/* Run multiple s-expressions from a reader state.
 * ************************************************************************** */

/* Keep reading and evaluating every s-expression from the given reader state
   until the reader returns #<eof>, then destroy the reader state.  In case of
   errors free the resources correctly and propagate; otherwise return the
   result of the last form. */
static jitterlisp_object
jitterlisp_run_and_destroy_reader_state (struct jitterlisp_reader_state *rstate)
{
  /* The initialization of res is necessary for the case where the input
     contains no forms and therefore jitterlisp_eval_globally is never
     called. */
  jitterlisp_object res = JITTERLISP_NOTHING;
  bool success = true;
  JITTERLISP_HANDLE_ERRORS(
    {
      /* Read and execute until #<eof> or until the first error. */
      jitterlisp_object form;
      while (! JITTERLISP_IS_EOF (form = jitterlisp_read (rstate)))
        res = jitterlisp_eval_globally (form);
    },
    {
      /* If we arrived here there was an error. */
      success = false;
    });

  /* Free the resources in either case, success or error. */
  jitterlisp_destroy_reader_state (rstate);

  /* If we failed propagate the error outside, otherwise return the result of
     the last form. */
  if (! success)
    jitterlisp_reerror ();
  else
    return res;
}




/* Run s-expressions parsed from a C string.
 * ************************************************************************** */

/* A function factoring jitterlisp_run_from_string and jitterlisp_run_library ,
   with an additional boolean argument making the execution non-verbose as
   appropriate for the library. */
static void
jitterlisp_run_from_string_internal (const char *string, bool library_verbosity)
{
  if (! library_verbosity && jitterlisp_settings.verbose)
    {
      jitterlisp_log_char_star ("Running from string \"");
      jitterlisp_log_char_star (string);
      jitterlisp_log_char_star ("\"...\n");
    }
  struct jitterlisp_reader_state *rstate
    = jitterlisp_make_string_reader_state (string);
  jitterlisp_object result
    = jitterlisp_run_and_destroy_reader_state (rstate);
  if (! library_verbosity && (jitterlisp_settings.print_nothing_results
                              || ! JITTERLISP_IS_NOTHING(result)))
    {
      jitterlisp_log (result);
      jitterlisp_log_char_star ("\n");
    }
  if (! library_verbosity && jitterlisp_settings.verbose)
    {
      jitterlisp_log_char_star ("Done running from string \"");
      jitterlisp_log_char_star (string);
      jitterlisp_log_char_star ("\"...\n");
    }
}

void
jitterlisp_run_from_string (const char *string)
{
  jitterlisp_run_from_string_internal (string, false);
}




/* Run the Lisp library.
 * ************************************************************************** */

void
jitterlisp_run_library (void)
{
  if (jitterlisp_settings.verbose)
    jitterlisp_log_char_star ("Running the library...\n");
  jitterlisp_run_from_string_internal (jitterlisp_library_string, true);
  if (jitterlisp_settings.verbose)
    jitterlisp_log_char_star ("Done running the library.\n");
}




/* Run s-expressions parsed from input streams.
 * ************************************************************************** */

void
jitterlisp_run_from_stream (FILE *input)
{
  struct jitterlisp_reader_state *rstate
    = jitterlisp_make_stream_reader_state (input);
  jitterlisp_run_and_destroy_reader_state (rstate);
}




/* Run s-expressions parsed from input files.
 * ************************************************************************** */

void
jitterlisp_run_from_named_file (const char *path_name)
{
  if (jitterlisp_settings.verbose)
    printf ("Running from file \"%s\"...\n", path_name);

  /* Open an input stream. */
  FILE *in;
  if (! strcmp (path_name, "-"))
    in = stdin;
  else
    in = fopen (path_name, "r");
  if (in == NULL)
    jitterlisp_error_cloned ("could not open input file");

  /* Read forms from the input stream and run each of them. */
  jitterlisp_run_from_stream (in);

  /* Close the input stream. */
  fclose (in);
  if (jitterlisp_settings.verbose)
    printf ("...Done running from file \"%s\".\n", path_name);
}

void
jitterlisp_run_from_input_files (void)
{
  size_t input_file_path_name_no
    = (jitterlisp_settings.input_file_path_names.used_size
       / sizeof (char *));
  const char **input_file_path_names =
    jitter_dynamic_buffer_to_pointer
       (& jitterlisp_settings.input_file_path_names);
  int i;
  for (i = 0; i < input_file_path_name_no; i ++)
    jitterlisp_run_from_named_file (input_file_path_names [i]);
}




/* Run s-expressions interactively from a REPL.
 * ************************************************************************** */

void
jitterlisp_repl (void)
{
  if (jitterlisp_settings.verbose)
    jitterlisp_log_char_star ("Running the REPL...\n");

  /* Make a readline reader state.  It will be used to read every form
     from the REPL, even in case of parse errors. */
  struct jitterlisp_reader_state *rstate
    = jitterlisp_make_readline_reader_state ("jitterlisp> ");

  /* Loop: read a form, exit if it's #<eof>, otherwise evaluate it.  In case of
     any error, at either read or evaluation time, just continue with the next
     iteration. */
  jitter_point_in_time time_before = jitter_point_in_time_make ();
  do
    {
      JITTERLISP_HANDLE_ERRORS(
        {
          /* Flush the output before printing the prompt, if any.  This will
             make printed characters always visible, even if a newline has not
             been printed yet. */
          jitter_print_flush (jitterlisp_print_context);

          jitterlisp_object form = jitterlisp_read (rstate);
          if (JITTERLISP_IS_EOF (form))
            goto out;
          jitter_time_set_now (time_before);
          jitterlisp_object result = jitterlisp_eval_globally (form);
          double elapsed_time = jitter_time_subtract_from_now (time_before);
          char message_buffer [100];
          /* Print the elapsed time according to the settings. */
          switch (jitterlisp_settings.time)
            {
            case jitterlisp_time_yes:
              sprintf (message_buffer, "[Evaluated in %.3fs]\n", elapsed_time);
              jitterlisp_log_char_star (message_buffer);
              break;
            case jitterlisp_time_verbose:
              jitterlisp_begin_class (jitterlisp_print_context, "log");
              jitter_print_char_star (jitterlisp_print_context, "[");
              jitterlisp_print (jitterlisp_print_context, form);
              sprintf (message_buffer, " evaluated in %.3fs]\n", elapsed_time);
              jitter_print_char_star (jitterlisp_print_context, message_buffer);
              jitterlisp_end_class (jitterlisp_print_context);
              break;
            case jitterlisp_time_no:
              /* Do nothing. */
              break;
            default:
              jitter_fatal ("invalid jitterlisp_settings.time");
            }
          /* Print the result, unless it's #<nothing> and printing #<nothing>
             results is disabled. */
          if (jitterlisp_settings.print_nothing_results
              || ! JITTERLISP_IS_NOTHING (result))
            {
              jitterlisp_print (jitterlisp_print_context, result);
              jitter_print_char (jitterlisp_print_context, '\n');
            }
        },
        {
          /* Do nothing on error. */
          jitterlisp_print_error_char_star
             ("User error in the REPL.  Continuing.\n");
        });
    }
  while (true);

  /* We got out of the loop.  Only now we can destroy the reader. */
 out:
  jitter_point_in_time_destroy (time_before);
  jitterlisp_destroy_reader_state (rstate);

  if (jitterlisp_settings.verbose)
    jitterlisp_log_char_star ("...Done running the REPL.\n");

  /* Print a goodbye message.  It is acceptable to do it unconditionally, since
     this is interactive use. */
  //jitterlisp_log_char_star ("Goodbye.\n");
  jitterlisp_begin_class (jitterlisp_print_context, "banner");
  jitter_print_char_star (jitterlisp_print_context, "Goodbye.\n");
  jitterlisp_end_class (jitterlisp_print_context);
}
