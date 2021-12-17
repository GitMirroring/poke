/* Jitter: driver.

   Copyright (C) 2017 Luca Saiu
   Updated in 2019 and 2021 by Luca Saiu
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


/* Include the Gnulib header. */
#include <config.h>

#include <argp.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <gl_xlist.h>

#include <jitter/jitter-cpp.h>
#include <jitter/jitter-parse-int.h>

#include "jitterc-vm.h"
#include "jitterc-generate.h"
#include "jitterc-utility.h"
#include "jitterc-parser.h"


/* The directory where flags are.  Of course this is only useful if the
   package is installed, but this default can be changed with the command-line
   option --flag-directory .
   The macro definition is convenient as it is, without surrounding parens,
   since it can be directly concatenated as a string literal. */
#define JITTERC_DEFAULT_FLAG_DIRECTORY  \
  JITTER_STRINGIFY(JITTER_FLAGDIR)

/* The directory where templates are.  Of course this is only useful if the
   package is installed, but this default can be changed with the command-line
   option --template-directory .
   The macro definition is convenient as it is, without surrounding parens,
   since it can be directly concatenated as a string literal. */
#define JITTERC_DEFAULT_TEMPLATE_DIRECTORY  \
  JITTER_STRINGIFY(JITTER_TEMPLATEDIR)

/* All the information encoded by the user in the command line. */
struct jitterc_command_line
{
  bool verbose, count_specialized_instructions_and_exit;
  bool generate_line;
  bool generate_frontend;
  char *input_file;
  char *flag_directory;
  char *template_directory;
  char *output_directory;
  int max_fast_register_no_per_class; /* -1 means unlimited. */
  int max_nonresidual_literal_no;  /* -1 means unlimited. */
};

/* Numeric identifiers for --no-* , or more in general "default" options having
   no short equivalent and specifying the default behavior; these are
   particularly convenient for interactive use where a complex command line is
   modified frequently.  Each case must have a value which is not an ASCII
   character. */
enum jitterc_negative_option
  {
    jitterc_negative_option_no_count_specialized_instructions = -1,
    jitterc_negative_option_no_frontend = -2,
    jitterc_negative_option_no_verbose = -3,
    jitterc_negative_option_generate_line = -4
  };

/* Numeric keys for options having only a long format.  These must not conflict
   with any value in enum jitterc_negative_option . */
enum jitterc_long_only_option
  {
    jitterc_long_only_option_dump_version = -6
  };

/* Command-line option specification. */
static struct argp_option jitterc_option_specification[] =
  {/* Mandatory options. */
   {NULL, '\0', NULL, OPTION_DOC, "Mandatory options:", 10},
   {"output", 'o', "PATH", 0,
    "Directory to write to"},
   {"output-directory", '\0', NULL, OPTION_ALIAS },

   /* Frequently used options. */
   {NULL, '\0', NULL, OPTION_DOC, "Frequently used options:", 20},
   {"max-fast-register-no", 'r', "N", 0,
    "Fast registers number limit per class -- any register past the N-th"
    " will not be fast, in any class (default: no limit)"},
   {"max-nonresidual-no",'n', "N", 0,
    "Maximum nonresidual number per meta-instruction, or -1 for unlimited"},
   {"frontend", 'f', NULL, 0,
    "Generate a vm-main.c file as well, containing a main function for the "
    "default frontend (default: no)"},
   {"main", '\0', NULL, OPTION_ALIAS },
   /* Frequently used negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 21},
   {"no-frontend", jitterc_negative_option_no_frontend, NULL, 0,
    "Don't generate vm-main.c (default)"},
   {"no-main", '\0', NULL, OPTION_ALIAS,
    "Don't generate vm-main.c (default)"},

   /* Tuning options. */
   {NULL, '\0', NULL, OPTION_DOC, "Tuning options:", 30},
   {"count-specialized-instructions", 'c', NULL, 0,
    "Display the number of specialized instruction and exit without "
    "generating C files"},
   {"count", '\0', NULL, OPTION_ALIAS},
   /* Tuning negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 31},
   {"no-count-specialized-instructions",
    jitterc_negative_option_no_count_specialized_instructions,
    NULL, 0, "Generate files instead of counting and exiting (default)"},
   {"no-count", '\0', NULL, OPTION_ALIAS},

   /* Debugging options. */
   {NULL, '\0', NULL, OPTION_DOC, "Debugging options:", 40},
   {"verbose", 'v', NULL, 0,
    "Show progress information at run time" },
   {"no-line", 'l', NULL, 0,
    "Don't generate #line directives referring the .jitter file.  This is "
    "useful for locating errors in the VM specification such as fogotten "
    "braces, and for debugging Jitter itself"},
   /* Debugging negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 41},
   {"no-verbose", jitterc_negative_option_no_verbose, NULL, 0,
    "Don't show progress information (default)"},
   {"line", jitterc_negative_option_generate_line, NULL, 0,
    "Generate #line directives (default)"},

   /* Debugging/scripting options. */
   {NULL, '\0', NULL, OPTION_DOC, "Debugging and scripting options:", 50},
   {"dump-version", jitterc_long_only_option_dump_version, NULL, 0,
    "Print the Jitter version only, without any surrounding text; this "
    "is convenient for scripts" },

   /* Rarely used options. */
   {NULL, '\0', NULL, OPTION_DOC,
    "Specialized, rarely used options (for an non-installed jitter):", 60},
   {"template-directory", 'T', "PATH", 0,
    "Directory holding template files (default "
    JITTERC_DEFAULT_TEMPLATE_DIRECTORY
    " ).  Useful when Jitter is not installed" },
   {"flag-directory", 'F', "PATH", 0,
    "Directory holding architecture-specific flag files (default "
    JITTERC_DEFAULT_FLAG_DIRECTORY
    " ).  Useful when Jitter is not installed" },

   /* Common GNU-style options. */
   {NULL, '\0', NULL, OPTION_DOC, "Common GNU-style options:", -1},
   /* These are automatically generated. */

   /* Option terminator. */
   { 0 }};

/* Short help text to print on --help and --version .  Defaults are
   supplied by the library if these are not defined; I guess they
   use weak symbols on reasonable platforms. */
const char *argp_program_version
  = "jitter (" PACKAGE_NAME ") " PACKAGE_VERSION "\n"
    "Copyright (C) 2021 Luca Saiu.\n"
    "GNU Jitter comes with ABSOLUTELY NO WARRANTY.\n"
    "You may redistribute copies of GNU Jitter under the terms of the GNU\n"
    "General Public License, version 3 or any later version published\n"
    "by the Free Software Foundation.  For more information see the\n"
    "file named COPYING.\n"
    "\n"
    "Written by Luca Saiu <http://ageinghacker.net>.";
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Forward-declaration.  I like having argp defined here, before parse_opt which
   is quite long. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state);

/* The parser main data structure. */
static struct argp argp =
  { jitterc_option_specification,
    parse_opt,
    "-o OUTPUTDIRECTORY FILE.jitter",
    "Generate virtual machine C code." };

/* Update our option state with the information from a single command-line
   option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct jitterc_command_line *cl = state->input;
  switch (key)
    {
      /* FIXME: within the case I could use state->arg_num to get the current
         non-option argument index, if needed. */
    case ARGP_KEY_INIT:
      /* Set sensible default values. */
      cl->verbose = false;
      cl->count_specialized_instructions_and_exit = false;
      cl->generate_frontend = false;
      cl->generate_line = true;
      cl->input_file = NULL;
      cl->flag_directory = JITTERC_DEFAULT_FLAG_DIRECTORY;
      cl->template_directory = JITTERC_DEFAULT_TEMPLATE_DIRECTORY;
      cl->output_directory = NULL;
      cl->max_fast_register_no_per_class = -1;
      cl->max_nonresidual_literal_no = -1;
      break;
    case 'v':
      cl->verbose = true;
      break;
    case jitterc_negative_option_no_verbose:
      cl->verbose = false;
      break;
    case 'c':
      cl->count_specialized_instructions_and_exit = true;
      break;
    case jitterc_long_only_option_dump_version:
      printf ("%s\n", JITTER_PACKAGE_VERSION);
      exit (EXIT_SUCCESS);
    case jitterc_negative_option_no_count_specialized_instructions:
      cl->count_specialized_instructions_and_exit = false;
      break;
    case 'f':
      cl->generate_frontend = true;
      break;
    case 'l': /* The default is true. */
      cl->generate_line = false;
      break;
    case jitterc_negative_option_generate_line:
      cl->generate_line = true;
      break;
    case jitterc_negative_option_no_frontend:
      cl->generate_frontend = false;
      break;
    case 'F':
      cl->flag_directory = arg;
      break;
    case 'T':
      cl->template_directory = arg;
      break;
    case 'o':
      cl->output_directory = arg;
      break;
    case 'r':
      /* By convention a value of -1 means no limit, but that is the default: if
         the user changes it then the new value must set an actual limit. */
      cl->max_fast_register_no_per_class = jitter_string_to_natural (arg);
      if (cl->max_fast_register_no_per_class < 0)
        argp_error (state, "the fast register number limit must be a natural.");
      break;
    case 'n':
      {
        jitter_long_long conversion;
        if (jitter_string_to_long_long_inconvenient (arg, &conversion) != 0)
          argp_error (state,
                      "the maximum nonresidual number must be an integer.");
        if (conversion < 0 && conversion != -1)
          argp_error
             (state,
              "the maximum nonresidual number must be a natural or -1 .");
        cl->max_nonresidual_literal_no = conversion;
        break;
      }
    case ARGP_KEY_ARG:
      cl->input_file = arg;
      break;
    /* case ARGP_KEY_NO_ARGS: /\* If this case is omitted, the default is sensible. *\/ */
    /*   argp_error (state, "you have to supply the input file\n"); */
    /*   break; */

    /* case ARGP_KEY_SUCCESS: /\* If this case is omitted, the default is sensible. *\/ */
    /*   break; */

    case ARGP_KEY_END:
      if (state->arg_num != 1)
        argp_error (state, "you gave %i input files instead of one.",
                    (int)state->arg_num);
      if (cl->output_directory == NULL)
        argp_error (state, "you have to supply the output directory.");
      break;

    /* case ARGP_KEY_FINI: /\* If this case is omitted, the default is sensible. *\/ */
    /*   break; */

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  /* Parse our arguments; cl will contain the information provided in the
     command line. */
  struct jitterc_command_line cl;
  argp_parse (&argp, argc, argv,
              0,//ARGP_IN_ORDER,
              0, &cl);

  if (cl.verbose)
    printf ("input_file = %s\n"
            "output_directory = %s\n"
            "generate_frontend = %s\n"
            "verbose = %s\n"
            "generate_line = %s\n"
            "max_fast_register_no_per_class = %i\n"
            "max_nonresidual_literal_no = %i\n"
            "template directory = %s\n",
            cl.input_file,
            cl.output_directory,
            cl.generate_frontend ? "yes" : "no",
            cl.verbose ? "yes" : "no",
            cl.generate_line ? "yes" : "no",
            (int) cl.max_fast_register_no_per_class,
            (int) cl.max_nonresidual_literal_no,
            cl.template_directory);

  struct jitterc_vm *vm;
  if (! strcmp (cl.input_file, "-"))
    vm = jitterc_parse_file_star (stdin, cl.generate_line);
  else
    vm = jitterc_parse_file (cl.input_file, cl.generate_line);

  jitterc_specialize (vm,
                      cl.max_fast_register_no_per_class,
                      cl.max_nonresidual_literal_no);

  if (cl.count_specialized_instructions_and_exit)
    {
      printf ("%li\n", (long) gl_list_size (vm->specialized_instructions));
      exit (EXIT_SUCCESS);
    }

  if (cl.verbose)
    {
      //jitterc_print_specialized_instruction_forest  (vm->specialized_instruction_forest);
      printf ("Specialized instructions are %li.\n",
              (long) gl_list_size (vm->specialized_instructions));
      printf ("The maximal residual arity is %li.\n",
              (long) vm->max_residual_arity);

      printf ("Wrapped globals:\n  ");
      int i;
      size_t wrapped_global_no = gl_list_size (vm->wrapped_globals);
      for (i = 0; i < wrapped_global_no; i ++)
        {
          printf ("%s", (char*) gl_list_get_at (vm->wrapped_globals, i));
          if (i != wrapped_global_no - 1)
            printf (", ");
        }
      printf ("\n");

      printf ("Wrapped functions:\n  ");
      size_t wrapped_function_no = gl_list_size (vm->wrapped_functions);
      for (i = 0; i < wrapped_function_no; i ++)
        {
          printf ("%s", (char*) gl_list_get_at (vm->wrapped_functions, i));
          if (i != wrapped_function_no - 1)
            printf (", ");
        }
      printf ("\n");
    }

  jitterc_generate (vm, cl.generate_frontend,
                    cl.template_directory, cl.output_directory);

  if (cl.verbose)
    printf ("Still alive at the end.\n");
  exit (EXIT_SUCCESS);
}
