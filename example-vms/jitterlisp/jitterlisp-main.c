/* JitterLisp: driver.

   Copyright (C) 2017, 2018, 2019, 2021 Luca Saiu
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


/* Include the Gnulib header. */
#include <config.h>

#include <argp.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <jitter/jitter.h>
#include <jitter/jitter-cpp.h>
#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>

#include "jitterlisp.h"


/* Command line handling using argp.
 * ************************************************************************** */

/* An enumerate to represent options with no short version.  These
   are options negating some other option: any negative option serves
   to reset a setting to its default value. */
enum jitterlisp_negative_option
  {
    jitterlisp_negative_option_no_verbose = -50,
    jitterlisp_negative_option_verbose_litter,
    jitterlisp_negative_option_library,
    jitterlisp_negative_option_no_compact_uninterned,
    jitterlisp_negative_option_omit_nothing,
    jitterlisp_negative_option_repl,
    jitterlisp_negative_option_colorize,
    jitterlisp_negative_option_no_cross_disassembler,
    jitterlisp_negative_option_no_free_routines,
    jitterlisp_negative_option_no_time,
    jitterlisp_negative_option_optimization_rewriting
  };

/* Numeric keys for options having only a long format.  These must not conflict
   with any value in enum jitterlisp_negative_option . */
enum jitterlisp_long_only_option
  {
    jitterlisp_long_only_option_no_colorize = -100,
    jitterlisp_long_only_option_no_verbose_litter,
    jitterlisp_long_only_option_no_library,
    jitterlisp_long_only_option_no_omit_nothing,
    jitterlisp_long_only_option_no_repl,
    jitterlisp_long_only_option_dump_version,
    jitterlisp_long_only_option_cross_disassembler,
    jitterlisp_long_only_option_free_routines,
    jitterlisp_long_only_option_time,
    jitterlisp_long_only_option_no_optimization_rewriting
  };

/* Command-line option specification. */
static struct argp_option jitterlisp_option_specification[] =
  {/* Read-Eval-Print Loop options. */
   {NULL, '\0', NULL, OPTION_DOC, "Read-Eval-Print Loop options:", 10},
   {"no-repl", jitterlisp_long_only_option_no_repl, NULL, 0,
    "Run non-interactively, without a REPL" },
   {"batch", 'q', NULL, OPTION_ALIAS },
   /* Read-Eval-Print Loop negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 11},
   {"repl", jitterlisp_negative_option_repl, NULL, 0,
    "Run interactively, with a REPL (default unless files are given on the "
    "command line)"},
   {"no-batch", '\0', NULL, OPTION_ALIAS },

   /* Command-line form evaluation. */
   {NULL, '\0', NULL, OPTION_DOC, "Command-line form evaluation:", 20},
   {"eval", 'e', "SEXPRS", 0,
    "Evaluate the given Lisp forms after running the files (if any) "
    "and before running the REPL (if enabled)" },

   /* Interaction options. */
   {NULL, '\0', NULL, OPTION_DOC, "Interaction options:", 30},
   {"no-omit-nothing", jitterlisp_long_only_option_no_omit_nothing, NULL, 0,
    "Show interactive evaluation results even when they are #<nothing>" },
   {"no-colorize", jitterlisp_long_only_option_no_colorize, NULL, 0,
    "Do not colorise s-expressions with terminal escape sequences"
#ifndef JITTER_WITH_LIBTEXTSTYLE
    " (no effect in this configuration)"
#endif // #ifndef JITTER_WITH_LIBTEXTSTYLE
   },
   {"compact-uninterned", 'c', NULL,
    0, "Print uninterned symbols in compact notation" },
   {"cross-disassembler", jitterlisp_long_only_option_cross_disassembler, NULL,
    0, "Use the cross-disassembler instead of the native disassembler" },
   {"free-routines", jitterlisp_long_only_option_free_routines, NULL,
    0, "Destroy non-executable routines for compiled closures; this"
    "makes debugging less friendly, but saves memory"},
   {"no-verbose-litter", jitterlisp_long_only_option_no_verbose_litter, NULL, 0,
    "Don't show littering information at run time"
#ifndef JITTERLISP_LITTER
    " (no effect with this executable)"
#endif // #ifndef JITTERLISP_LITTER
   },
   {"time", jitterlisp_long_only_option_time, "TIME", OPTION_ARG_OPTIONAL,
    "Unless TIME is \"no\" time interactive commands in the REPL, showing "
    "elapsed time.  TIME can be \"no\", \"verbose\" (also repeat the command "
    "being timed in the output) or \"yes\".  Not specifying TIME is equivalent "
    "to \"yes\".  Not giving the option is equivalent to \"no\"."},
   /* Interaction negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 31},
   {"omit-nothing", jitterlisp_negative_option_omit_nothing, NULL, 0,
    "Omit #<nothing> interactive evaluation results (default)"},
   {"colorize", jitterlisp_negative_option_colorize, NULL, 0,
    "Colorise s-expressions (default)"
#ifndef JITTER_WITH_LIBTEXTSTYLE
    " (no effect in this configuration)"
#endif // #ifndef JITTER_WITH_LIBTEXTSTYLE
    },
   {"no-compact-uninterned", jitterlisp_negative_option_no_compact_uninterned,
    NULL, 0, "Don't print uninterned symbols in compact notation (default)" },
   {"no-cross-disassembler", jitterlisp_negative_option_no_cross_disassembler,
    NULL, 0, "Use the native disassembler instead of the cross-disassembler "
    "(default)" },
   {"no-free-routines", jitterlisp_negative_option_no_free_routines, NULL,
    0, "Keep non-executable routines for compiled closures (default)"},
   {"verbose-litter", jitterlisp_negative_option_verbose_litter, NULL, 0,
    "Show littering information at run time (default"
#ifndef JITTERLISP_LITTER
    ", no effect with this exectuable"
#endif // #ifndef JITTERLISP_LITTER
    ")"},
   {"no-time", jitterlisp_negative_option_no_time, NULL, 0,
    "Don't time interactive commands (default); equivalent to --time=no"},

   /* Debugging options. */
   {NULL, '\0', NULL, OPTION_DOC, "Debugging options:", 40},
   {"verbose", 'v', NULL, 0,
    "Show progress information at run time" },
   {"no-library", jitterlisp_long_only_option_no_library, NULL, 0,
    "Don't load the Lisp library" },
   {"no-optimization-rewriting",
    jitterlisp_long_only_option_no_optimization_rewriting, NULL, 0,
    "Disable optimization rewriting (this is mostly useful for debugging "
    "rewrite rules and for measuring the speedup they introduce)" },
   /* Debugging negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 41},
   {"no-verbose", jitterlisp_negative_option_no_verbose, NULL, 0,
    "Don't show progress information (default)"},
   {"library", jitterlisp_negative_option_library, NULL, 0,
    "Load the Lisp library (default)" },
   {"optimization-rewriting",
    jitterlisp_negative_option_optimization_rewriting, NULL, 0,
    "Enable optimization rewriting (default)" },

   /* Scripting options. */
   {NULL, '\0', NULL, OPTION_DOC, "Scripting options:", 50},
   {"dump-version", jitterlisp_long_only_option_dump_version, NULL, 0,
    "Print the JitterLisp version only, without any surrounding text, and "
    "exit with success; this is convenient for scripts" },

   /* Common GNU-style options. */
   {NULL, '\0', NULL, OPTION_DOC, "Common GNU-style options:", -1},
   /* These are automatically generated. */

   /* Option terminator. */
   { 0 }};

/* Customised text text to print on --help and --version . */
static void
the_argp_program_version_hook (FILE * restrict stream, struct argp_state *s)
{
  const struct jitter_vm_configuration *c = jitterlispvm_vm_configuration;

  const char *instrumentation
    = jitter_vm_instrumentation_to_string (c->instrumentation);
  fprintf (stream,
           "JitterLisp with %s%s%s dispatch "
           "(" JITTER_PACKAGE_NAME ") " JITTER_PACKAGE_VERSION "\n",
           instrumentation,
           (strlen (instrumentation) > 0 ? ", " : ""),
           c->dispatch_human_readable);
  fprintf
     (stream,
      "Copyright (C) 2021 Luca Saiu.\n"
      "JitterLisp comes with ABSOLUTELY NO WARRANTY.\n"
      "You may redistribute copies of JitterLisp under the terms of the\n"
      "GNU General Public License, version 3 or any later version\n"
      "published by the Free Software Foundation.  For more information\n"
      "see the file named COPYING.\n"
      "\n"
      "Written by Luca Saiu <http://ageinghacker.net>.\n");
}
void (*argp_program_version_hook) (FILE * restrict stream, struct argp_state *s)
  = the_argp_program_version_hook;
const char *argp_program_bug_address = "<" JITTER_PACKAGE_BUGREPORT ">";

/* Forward-declaration.  I like having argp defined here, before parse_opt which
   is quite long. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state);

/* The parser main data structure. */
static struct argp argp =
  {
    jitterlisp_option_specification,
    parse_opt,
    "[FILE.lisp|-]...",
    "Run a JitterLisp program and/or a JitterLisp interactive REPL."
    "\v"
    JITTER_PACKAGE_NAME " home page: <" JITTER_PACKAGE_URL ">.\n"
    "General help using GNU software: <https://www.gnu.org/gethelp>."
  };

/* Update our option state with the information from a single command-line
   option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* Because of how parse_opt is called sp points to jitterlisp_settings . */
  struct jitterlisp_settings *sp = state->input;
  switch (key)
    {
    /* Read-Eval-Print Loop options. */
    case jitterlisp_long_only_option_no_repl:
    case 'q':
      sp->repl = jitterlisp_run_repl_no;
      break;

    /* Read-Eval-Print Loop negative options. */
    case jitterlisp_negative_option_repl:
      sp->repl = jitterlisp_run_repl_yes;
      break;

    /* Command-line s-expression evaluation. */
    case 'e':
      sp->sexps_string = arg;
      break;

    /* Interaction options. */
    case jitterlisp_long_only_option_no_omit_nothing:
      sp->print_nothing_results = true;
      break;
    case jitterlisp_long_only_option_no_colorize:
      sp->colorize = false;
      break;
    case 'c':
      sp->print_compact_uninterned_symbols = true;
      break;
    case jitterlisp_long_only_option_cross_disassembler:
      sp->cross_disassembler = true;
      break;
    case jitterlisp_long_only_option_free_routines:
      sp->free_routines = true;
      break;
    case jitterlisp_long_only_option_no_verbose_litter:
      sp->verbose_litter = false;
      break;
    case jitterlisp_long_only_option_time:
      if (arg == NULL || ! strcmp (arg, "yes"))
        sp->time = jitterlisp_time_yes;
      else if (! strcmp (arg, "no"))
        sp->time = jitterlisp_time_no;
      else if (! strcmp (arg, "verbose"))
        sp->time = jitterlisp_time_verbose;
      else
        argp_error (state, "the --time option argument was \"%s\" intead of "
                    "\"yes\", \"no\" or \"verbose\"", arg);
      break;

    /* Interaction negative options. */
    case jitterlisp_negative_option_omit_nothing:
      sp->print_nothing_results = false;
      break;
    case jitterlisp_negative_option_colorize:
      sp->colorize = true;
      break;
    case jitterlisp_negative_option_no_compact_uninterned:
      sp->print_compact_uninterned_symbols = false;
      break;
    case jitterlisp_negative_option_no_cross_disassembler:
      sp->cross_disassembler = false;
      break;
    case jitterlisp_negative_option_no_free_routines:
      sp->free_routines = false;
      break;
    case jitterlisp_negative_option_verbose_litter:
      sp->verbose_litter = true;
      break;
    case jitterlisp_negative_option_no_time:
      sp->time = jitterlisp_time_no;
      break;

    /* Debugging options. */
    case 'v':
      sp->verbose = true;
      break;
    case jitterlisp_long_only_option_no_library:
      sp->library = false;
      break;
    case jitterlisp_long_only_option_no_optimization_rewriting:
      sp->optimization_rewriting = false;
      break;

    /* Debugging negative options. */
    case jitterlisp_negative_option_no_verbose:
      sp->verbose = false;
      break;
    case jitterlisp_negative_option_library:
      sp->library = true;
      break;
    case jitterlisp_negative_option_optimization_rewriting:
      sp->optimization_rewriting = true;
      break;

    /* Scripting options. */
    case jitterlisp_long_only_option_dump_version:
      printf ("%s\n", JITTER_PACKAGE_VERSION);
      exit (EXIT_SUCCESS);

    /* Non-option arguments. */
    case ARGP_KEY_ARG:
      jitter_dynamic_buffer_push (& sp->input_file_path_names, & arg,
                                  sizeof (char *));
      break;

    /* Handle anything else. */
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}




/* Main function.
 * ************************************************************************** */

int
main (int argc, char **argv)
{
  /* Initialize JitterLisp.  This among the rest initializes the settings data
     structure with default values, so it must be called before argp_parse ,
     which may change those.  This does *not* initialise the print subsystem;
     we need to parse the options before doing that. */
  jitterlisp_initialize ();

  /* Parse our arguments; jitterlisp_settings will contain the information
     provided in the command line.  We also define the "settings pointer"
     sp as a pointer to it, for convenience. */
  argp_parse (& argp, argc, argv, 0, 0, & jitterlisp_settings);

  /* Now we know whether styling is enabled, and we can initialise the
     print subsystem as well. */
  jitterlisp_printer_initialize ();
  
  /* In case no REPL option was given decide whether to run it: yes iff no
     file was given. */
  struct jitterlisp_settings * const sp = & jitterlisp_settings;
  if (sp->repl == jitterlisp_run_repl_default)
    sp->repl = (sp->input_file_path_names.used_size != 0
                ? jitterlisp_run_repl_no
                : jitterlisp_run_repl_yes);
  /* From now on sp->repl can be used as a boolean. */

  /* If running interactively print the banner, as per the GPL. */
  if (sp->repl)
    jitterlisp_interactive_banner ();

  /* Run input files and s-expressions from the command-line, halting at the
     first error; still free the resources before exiting, even in case of
     error.
     Rationale: this level of attention to memory leaks would not be justified
     here at the top level where failure is effectively fatal and the process is
     about to terminate anyway, automaically freeing resources; however this is
     a good place to test the non-local error handling mechanism, and an example
     for users. */
  int return_code = EXIT_SUCCESS;
  JITTERLISP_HANDLE_ERRORS(
    {
      /* Run the library, unless disabled. */
      if (sp->library)
        jitterlisp_run_library ();

      /* Run the input files. */
      jitterlisp_run_from_input_files ();

      /* Evaluate s-expressions from the command line, if any. */
      if (sp->sexps_string != NULL)
        jitterlisp_run_from_string (sp->sexps_string);
    },
    {
      /* On failure: */
      printf ("Failed when evaluating input files or command-line "
              "s-expressions.\n");
      return_code = EXIT_FAILURE;
    });

  /* Run the REPL if enabled, and only if we didn't fail before.  Failure works
     differently in the REPL: every single command can fail, but such failures
     are handled internally and don't propagate: we don't want to kill the
     process every time the user makes a mistake in interactive use. */
  if (sp->repl && return_code != EXIT_FAILURE)
    jitterlisp_repl ();

  /* Finalize JitterLisp, freeing up resources.  This of course wouldn't be
     needed right before exiting, but is convenient when checking for memory
     leaks with Valgrind which this way won't show false positives. */
  jitterlisp_finalize ();

  /* Return success or failure, as we decided before.  Yes, we go to the trouble
     of freeing resources even on fatal errors: see the comment above. */
  return return_code;
}
