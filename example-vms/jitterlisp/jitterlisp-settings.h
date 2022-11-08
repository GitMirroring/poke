/* JitterLisp: global settings: header.

   Copyright (C) 2017, 2018, 2019 Luca Saiu
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


#ifndef JITTERLISP_SETTINGS_H_
#define JITTERLISP_SETTINGS_H_

#include <stdbool.h>

#include <jitter/jitter-dynamic-buffer.h>


/* JitterLisp global settings.
 * ************************************************************************** */

/* The type associated to the repl setting field.  After command line parsing
   is over this can be used as a boolean. */
enum jitterlisp_run_repl
  {
    /* Do not run the REPL. */
    jitterlisp_run_repl_no = 0,

    /* Run the REPL. */
    jitterlisp_run_repl_yes = 1,

    /* Run the REPL iff there are no files specified on the command line.
       The setting value reverts to one of the other two values after
       command-line parsing ends. */
    jitterlisp_run_repl_default = 2,
  };

/* The way to time interactive commands in the REPL. */
enum jitterlisp_time
  {
    /* Do not show command evaluation time. */
    jitterlisp_time_no = 0,

    /* Show command evaluation time. */
    jitterlisp_time_yes = 1,

    /* Show command evaluation time, also repeating the command being evaluated
       in the output.  This is convenient when the user executes multiple
       commands in the same line. */
    jitterlisp_time_verbose = 2
  };

/* Global settings for JitterLisp. */
struct jitterlisp_settings
{
  /* Non-false iff the output needs to be verbose. */
  bool verbose;

  /* Non-false iff littering output needs to be verbose.  Ignored when not
     littering. */
  bool verbose_litter;

  /* Non-false iff we have to load the Lisp library. */
  bool library;

  /* If false don't print #<nothing> when it's the result of an evaluation to be
     printed.  No setting suppresses explicit printing of values, for example by
     the display procedure, or the printing of #<nothing> as a component of a
     larger structure. */
  bool print_nothing_results;

  /* If true print uninterned symbols in a compact notation using an index
     rather than an address.  This may be less precise, but is convenient
     for reading machine-rewritten ASTs. */
  bool print_compact_uninterned_symbols;

  /* If true destroy non-executable routines for compiled closures.  This makes
     disassembling less friendly and VM-disassembling essentially useless, but
     saves memory. */
  bool free_routines;

  /* If true use the cross-disassembler (as found by Jitter's configure script)
     rather than the native disassembler.  This is useful for developing using
     emulators. */
  bool cross_disassembler;

  /* If true then enable optimization rewriting. */
  bool optimization_rewriting;

  /* Non-false iff output s-expressions need to be colorized with terminal
     escape sequences. */
  bool colorize;

  /* The way interactive REPL commands should be timed. */
  enum jitterlisp_time time;

  /* Some s-expressions provided from the command line to evaluate, or NULL. */
  char *sexps_string;

  /* A dynamic buffer with char * elements, each containing a filename.
     Possibly empty. */
  struct jitter_dynamic_buffer input_file_path_names;

  /* Provide an interactive REPL. */
  enum jitterlisp_run_repl repl;
};

/* The one global variable holding JitterLisp settings.  This is initialized
   with reasonable values from jitterlisp_settings_set_default , then filled
   from the argp parser called in main and never changed after that point. */
extern struct jitterlisp_settings
jitterlisp_settings;

/* Fill jitterlisp_settings with default values. */
void
jitterlisp_settings_set_default (void);




/* Not fo the user: finalization.
 * ************************************************************************** */

/* The functionality here is called by JitterLisp's global finalization. */

/* Finalize the settings data structure, freeing resources. */
void
jitterlisp_settings_finalize (void);

#endif // #ifndef JITTERLISP_SETTINGS_H_
