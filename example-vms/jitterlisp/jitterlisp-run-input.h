/* JitterLisp: running from files, C strings, REPL: header.

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


#ifndef JITTERLISP_RUN_INPUT_H_
#define JITTERLISP_RUN_INPUT_H_

/* JitterLisp does rely on Gnulib for portability. */
#include <config.h>

#include <stdio.h>

#include "jitterlisp.h"




/* Run s-expressions parsed from a C string.
 * ************************************************************************** */

/* Parse s-expressions from the pointed string and run each of them.  Print the
   last result unless it's #<nothing> . */
void
jitterlisp_run_from_string (const char *string)
  __attribute__ ((nonnull (1)));



/* Run the Lisp library.
 * ************************************************************************** */

/* Run the Lisp library which is held in a constant C string directly within the
   executable.  Don't ever print the result, which in this case is guaranteed to
   be uninteresting. */
void
jitterlisp_run_library (void);




/* Run s-expressions parsed from input streams.
 * ************************************************************************** */

/* Parse s-expressions from the pointed input stream and run each of them.  The
   steram must be already open at entry, and is not closed at exit. */
void
jitterlisp_run_from_stream (FILE *input)
  __attribute__ ((nonnull (1)));




/* Run s-expressions parsed from input files.
 * ************************************************************************** */

/* Parse s-expressions from the named file (opened for input, used and then
 closed by this function) and run each of them.  If the pointed string is "-"
 then read from the standard input instead of opening a file stream. */
void
jitterlisp_run_from_named_file (const char *path_name)
  __attribute__ ((nonnull (1)));

/* Parse s-expressions from the every input file specified in the command
   line, in the order.  This uses jitterlisp_run_from_named_file on the
   file names from input_file_path_names in jitterlisp_settings . */
void
jitterlisp_run_from_input_files (void);




/* Run s-expressions interactively from a REPL.
 * ************************************************************************** */

/* Print a welcome banner also including the GPL legal notes.  This should be
   printed on interactive runs. */
void
jitterlisp_interactive_banner (void);

/* Run the JitterLisp interactive REPL until the reader finds #<eof>.  Treat
   errors (at either read or eval time) internally, without ever propagating
   them out: the program shouldn't abort just because of user mistakes in
   interactive use. */
void
jitterlisp_repl (void);

#endif // #ifndef JITTERLISP_RUN_INPUT_H_
