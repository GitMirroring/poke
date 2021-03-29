/* Jittery structured language example: main.

   Copyright (C) 2017, 2019, 2020, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of the Jitter structured-language example, distributed
   along with Jitter under the same license.

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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <jitter/jitter-fatal.h>

#include "structuredvm-vm.h"

#include "structured-syntax.h"
#include "structured-parser.h"
#include "structured-code-generator.h"
#include "structured-code-generator-stack.h"
#include "structured-code-generator-register.h"


/* Why this source file parses argc and argv directly.
 * ************************************************************************** */

/* This example does not use argp, differently from the rest of Jitter, in order
   to keep portability without requiring Gnulib.  The intent here is to simplify
   the example build system for users looking at how to adopt Jitter for their
   own code, even at a small cost in complexity within this source file.

   The command-line interface of the structued program is simple, but civilized
   enough to respect the GNU conventions.

   This example uses routines through the unified API. */




/* Global variables.
 * ************************************************************************** */

/* The program name as it was invoked from the shell, or in other words a copy
   of the pointer in argv [0] , globally visible. */
static char *structured_program_name;

/* See the comment about help_section_indentation in jitter-config.in . */
#define STRUCTURED_HELP_SECTION_INDENTATION  ""




/* Utility functions for the command line.
 * ************************************************************************** */

/* Print a fatal error message and exit with failure, in response to an
   incorrect command line.  The other_information string is printed right
   after the error message, with no preceeding space.  It is a crude but
   convenient way of providing an "argument" to error messages. */
static void
structured_usage (char *error_message, char *other_information)
{
  fprintf (stderr, "%s: %s%s.\n", structured_program_name,
           error_message, other_information);
  fprintf (stderr, "Try '%s --help' for more information.\n",
           structured_program_name);

  exit (EXIT_FAILURE);
}

/* Print a section heading in --help , with the given heading title. */
static void
structured_help_section (const char *title)
{
  printf ("\n" STRUCTURED_HELP_SECTION_INDENTATION "%s:\n", title);
}

/* Print command-line interface help and exit with success. */
static void
structured_help (void)
{
  const struct jitter_vm_configuration *c = structuredvm_vm_configuration;
  enum jitter_vm_instrumentation instrumentation = c->instrumentation;

  printf ("Usage: %s [OPTION...] FILE.structured\n", structured_program_name);
  printf ("   or: %s [OPTION...] -\n", structured_program_name);
  printf ("Run a structured-language program on a Jittery VM, using the\n");
  printf ("%s dispatch %s.\n", c->dispatch_human_readable,
          jitter_vm_instrumentation_to_string (instrumentation));

  structured_help_section ("Debugging options");
  printf ("      --disassemble                print hardware machine instructions\n");
  printf ("      --cross-disassemble          use the cross-disassembler rather than\n");
  printf ("                                   the native disassembler; also enable\n");
  printf ("                                   disassembly as per --disassemble\n");
  printf ("      --print-defects              print information about defective\n");
  printf ("                                   specialized instructions and replacements\n");
  printf ("      --profile-specialized        print profiling information for\n");
  printf ("                                   specialized instructions %s\n",
          (instrumentation != jitter_vm_instrumentation_none
           ? ""
           : " if available (disabled: recompile with -DPROFILE_COUNT=1 or -DPROFILE_SAMPLE=1)"));
  printf ("      --profile-unspecialized      like --profile-specialized, for\n");
  printf ("                                   unspecialized instructions\n");
  printf ("      --print-locations            print the mapping between VM structures\n");
  printf ("                                   and hardware structures, to help humans\n");
  printf ("                                   read the disassembly\n");
  printf ("      --dry-run                    do not actually run the program\n");
  printf ("      --print-routine, --print     print VM instructions\n");
  printf ("      --no-dry-run                 run the program (default)\n");
  printf ("      --no-print-defects           do not print information about defective\n");
  printf ("                                   specialized instructions and replacements\n");
  printf ("                                   (default)\n");
  printf ("      --no-profile-specialized     omit profiling information for specialized\n");
  printf ("                                   instructions (default)\n");
  printf ("      --no-profile-unspecialized   omit profiling information for unspecialized\n");
  printf ("                                   instructions (default)\n");
  printf ("      --no-print-locations         do not print locations (default)\n");
  printf ("      --no-print-routine,\n");
  printf ("      --no-print                   do not print VM instructions (default)\n");

  structured_help_section ("Benchmarking options");
  printf ("      --slow-literals-only         disable fast literals\n");
  printf ("      --slow-registers-only        disable fast registers\n");
  printf ("      --slow-only                  disable both fast literals and fast\n");
  printf ("                                   registers, like with --slow-literals-only\n");
  printf ("                                   and --slow-registers-only\n");
  printf ("      --no-optimization-rewriting  disable optimization rewriting\n");
  printf ("      --no-slow-literals-only      enable fast literals (default)\n");
  printf ("      --no-slow-registers-only     enable fast registers (default)\n");
  printf ("      --no-slow-only               enable both fast literals and fast\n");
  printf ("                                   registers, like with --no-slow-literals-only\n");
  printf ("                                   and --no-slow-registers-only (default)\n");
  printf ("      --optimization-rewriting     enable optimization rewriting (default)\n");

  structured_help_section ("Code generation options");
  printf ("      --stack                      generate stack-based instructions\n");
  printf ("      --register                   generate register-based instructions (default)\n");

  structured_help_section ("Common GNU-style options");
  printf ("      --help                       give this help list and exit\n");
  printf ("      --version                    print program version and exit\n");

  printf ("\n");
  printf ("An \"--\" argument terminates option processing.\n");

  printf ("\n");
  printf ("Report bugs to " JITTER_PACKAGE_BUGREPORT ".\n");

  exit (EXIT_SUCCESS);
}

/* Print version information and exit with success. */
static void
structured_version (void)
{
  const struct jitter_vm_configuration *c = structuredvm_vm_configuration;
  const char *instrumentation
    = jitter_vm_instrumentation_to_string (c->instrumentation);

  printf ("structured, %s%s%s dispatch ("
          JITTER_PACKAGE_NAME ") " JITTER_PACKAGE_VERSION "\n",
          instrumentation,
          (strlen (instrumentation) > 0 ? ", " : ""),
          c->dispatch_human_readable);
  printf ("Copyright (C) 2021 Luca Saiu.\n"
          "Jitter comes with ABSOLUTELY NO WARRANTY.\n"
          "You may redistribute copies of Jitter under the terms of the GNU\n"
          "General Public License, version 3 or any later version published\n"
          "by the Free Software Foundation.  For more information see the\n"
          "file named COPYING.\n"
          "\n"
          "Written by Luca Saiu  <http://ageinghacker.net>.\n");

  exit (EXIT_SUCCESS);
}




/* Command-line handling.
 * ************************************************************************** */

/* A specifier for the code generator which is being used. */
enum structured_code_generator
  {
    /* Generate stack-based instructions */
    structured_code_generator_stack,

    /* Generate register-based instructions */
    structured_code_generator_register
  };

/* The state encoded in a user command line. */
struct structured_command_line
{
  /* True iff we should print back the VM routine. */
  bool print;

  /* True iff we should use the cross-disassembler rather than the native
     disassembler for the VM routine.  If false, use the native
     disassembler.  */
  bool cross_disassemble;

  /* True iff we should disassemble the VM routine. */
  bool disassemble;

  /* True iff we should print information about defective specialised
     instructions and their replacements. */
  bool print_defects;

  /* True iff we should print profiling information, respectively for
     specialised and unspecialised instructions. */
  bool profile_specialized;
  bool profile_unspecialized;

  /* True iff we should print data locations. */
  bool print_locations;

  /* True iff we should not actually run the VM routine. */
  bool dry_run;

  /* True iff we should disable fast literals, for benchmarking a worst-case
     scenario or for comparing with some other implementation. */
  bool slow_literals_only;

  /* Like slow_literals_only, but for fast registers. */
  bool slow_registers_only;

  /* True iff we should enable optimization rewriting. */
  bool optimization_rewriting;

  /* Which code generator is being used. */
  enum structured_code_generator code_generator;

  /* Pathname of the program source to be loaded. */
  char *program_path;
};

/* Inizialize the command-line state to sensible defaults; make the program path
   intentionally invalid to catch errors. */
static void
structured_initialize_command_line (struct structured_command_line *cl)
{
  cl->print = false;
  cl->cross_disassemble = false;
  cl->disassemble = false;
  cl->print_defects = false;
  cl->profile_specialized = false;
  cl->profile_unspecialized = false;
  cl->print_locations = false;
  cl->dry_run = false;
  cl->optimization_rewriting = true;
  cl->slow_literals_only = false;
  cl->slow_registers_only = false;
  cl->code_generator = structured_code_generator_register;
  cl->program_path = NULL;
}

/* Set the program name in the pointed command line structure to the given
   value, or fail fatally if the name was already set. */
static void
structured_set_command_line_program (struct structured_command_line *cl,
                                     char *arg)
{
  if (cl->program_path != NULL)
    structured_usage ("more than one program given; the second is ", arg);
  cl->program_path = arg;
}

/* Fill the pointed command-line data structure with information from the
   actual command line. */
static void
structured_parse_command_line (struct structured_command_line *cl,
                               int argc, char **argv)
{
  structured_program_name = argv [0];
  structured_initialize_command_line (cl);

  int i;
  bool handle_options = true;
  for (i = 1; i < argc; i ++)
    {
      /* Get the current argument, be it option or non-option. */
      char *arg = argv [i];

      /* If we are still handling options but the current argument is "--" then
         stop doing that, and don't handle the current argument any further. */
      if (handle_options && ! strcmp (arg, "--"))
        {
          handle_options = false;
          continue;
        }

      /* Handle arg, as an option or a pathname. */
      if      (handle_options && ! strcmp (arg, "--help"))
        structured_help ();
      else if (handle_options && ! strcmp (arg, "--version"))
        structured_version ();
      else if (handle_options && ! strcmp (arg, "--disassemble"))
        cl->disassemble = true;
      else if (handle_options && ! strcmp (arg, "--cross-disassemble"))
        {
          cl->cross_disassemble = true;
          cl->disassemble = true;
        }
      else if (handle_options && ! strcmp (arg, "--print-defects"))
        cl->print_defects = true;
      else if (handle_options && ! strcmp (arg, "--profile-specialized"))
        cl->profile_specialized = true;
      else if (handle_options && ! strcmp (arg, "--profile-unspecialized"))
        cl->profile_unspecialized = true;
      else if (handle_options && ! strcmp (arg, "--no-print-defects"))
        cl->print_defects = false;
      else if (handle_options && ! strcmp (arg, "--no-profile-unspecialized"))
        cl->profile_unspecialized = false;
      else if (handle_options && ! strcmp (arg, "--no-profile-specialized"))
        cl->profile_specialized = false;
      else if (handle_options && ! strcmp (arg, "--print-locations"))
        cl->print_locations = true;
      else if (handle_options && ! strcmp (arg, "--no-print-locations"))
        cl->print_locations = false;
      else if (handle_options && ! strcmp (arg, "--slow-literals-only"))
        cl->slow_literals_only = true;
      else if (handle_options && ! strcmp (arg, "--no-slow-literals-only"))
        cl->slow_literals_only = false;
      else if (handle_options && ! strcmp (arg, "--slow-registers-only"))
        cl->slow_registers_only = true;
      else if (handle_options && ! strcmp (arg, "--no-slow-registers-only"))
        cl->slow_registers_only = false;
      else if (handle_options && ! strcmp (arg, "--slow-only"))
        {
          cl->slow_literals_only = true;
          cl->slow_registers_only = true;
        }
      else if (handle_options && ! strcmp (arg, "--no-slow-only"))
        {
          cl->slow_literals_only = false;
          cl->slow_registers_only = false;
        }
      else if (handle_options && ! strcmp (arg, "--optimization-rewriting"))
        cl->optimization_rewriting = true;
      else if (handle_options && ! strcmp (arg, "--no-optimization-rewriting"))
        cl->optimization_rewriting = false;
      else if (handle_options && ! strcmp (arg, "--stack"))
        cl->code_generator = structured_code_generator_stack;
      else if (handle_options && ! strcmp (arg, "--register"))
        cl->code_generator = structured_code_generator_register;
      else if (handle_options && (! strcmp (arg, "--print")
                                  || ! strcmp (arg, "--print-routine")))
        cl->print = true;
      else if (handle_options && (! strcmp (arg, "--no-print")
                                  || ! strcmp (arg, "--no-print-routine")))
        cl->print = false;
      else if (handle_options && ! strcmp (arg, "--dry-run"))
        cl->dry_run = true;
      else if (handle_options && ! strcmp (arg, "--no-dry-run"))
        cl->dry_run = false;
      else if (handle_options && strlen (arg) > 1 && arg [0] == '-')
        structured_usage ("unrecognized option ", arg);
      else if (handle_options && strlen (arg) > 1 && arg [0] != '-')
        structured_set_command_line_program (cl, arg);
      else
        structured_set_command_line_program (cl, arg);
    }

  /* Still not having a program name at the end is an error. */
  if (cl->program_path == NULL)
    structured_usage ("program name missing", "");
}




/* Execute what the command line says.
 * ************************************************************************** */

/* Do what the pointed command line data structure says. */
static void
structured_work (struct structured_command_line *cl)
{
  /* Parse a structured-language program into an AST. */
  struct structured_program *p;
  if (! strcmp (cl->program_path, "-"))
    p = structured_parse_file_star (stdin);
  else
    p = structured_parse_file (cl->program_path);

  /* Initialize the structured-VM subsystem. */
  structuredvm_initialize ();

  /* Initialise the GNU Libtextstyle wrapper, if used. */
#ifdef JITTER_WITH_LIBTEXTSTYLE
  jitter_print_libtextstyle_initialize ();

  /* FIXME: this should be less crude, but is enough for checking that the
     libtextstyle wrapper works. */
  char *style_file_name = "structured-style.css";
  styled_ostream_t ostream
    = styled_ostream_create (STDOUT_FILENO, "(stdout)", TTYCTL_AUTO,
                             style_file_name);
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE

  /* Make a print context. */
  jitter_print_context ctx
#ifdef JITTER_WITH_LIBTEXTSTYLE
    = jitter_print_context_make_libtextstyle (ostream);
#else
    = jitter_print_context_make_file_star (stdout);
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE

  /* Make an empty Jittery routine and set options for it as needed. */
  structuredvm_routine vmr
    = structuredvm_make_routine ();
  structuredvm_set_routine_option_slow_literals_only
     (vmr, cl->slow_literals_only);
  structuredvm_set_routine_option_slow_registers_only
     (vmr, cl->slow_registers_only);
  structuredvm_set_routine_option_optimization_rewriting
     (vmr, cl->optimization_rewriting);

  /* Translate the AST program into a Jittery routine. */
  switch (cl->code_generator)
    {
    case structured_code_generator_stack:
      structured_translate_program_stack (vmr, p);
      break;
    case structured_code_generator_register:
      structured_translate_program_register (vmr, p);
      break;
    default:
      jitter_fatal ("unknwon code generator (bug): %i", (int) cl->code_generator);
    }

  /* Here, if I were not using the unified API, I would need to make an
     executable Jittery routine from a mutable Jittery routine by calling
     structuredvm_make_executable_routine.  However the unified API makes this
     automatic. */

  /* Print defect information if the user asked to */
  if (cl->print_defects)
    structuredvm_defect_print_summary (ctx);

  /* Print and/or disassemble the routine as requested. */
  if (cl->print)
    structuredvm_routine_print (ctx, vmr);
  if (cl->cross_disassemble)
    cl->disassemble = true;
  if (cl->print_locations)
    structuredvm_dump_data_locations (ctx);
  if (cl->disassemble)
    structuredvm_routine_disassemble (ctx,
                                      vmr, true,
                                      (cl->cross_disassemble
                                       ? JITTER_CROSS_OBJDUMP
                                       : JITTER_OBJDUMP),
                                      NULL);

  /* Run the Jittery routine in a temporary state, unless this is a dry run. */
  if (! cl->dry_run)
    {
      struct structuredvm_state s;
      structuredvm_state_initialize (& s);
      structuredvm_execute_routine (vmr, & s);
      if (cl->profile_specialized)
        {
          struct structuredvm_profile_runtime *p
            = structuredvm_state_profile_runtime (& s);
          structuredvm_profile_runtime_print_specialized (ctx, p);
        }
      if (cl->profile_unspecialized)
        {
          struct structuredvm_profile_runtime *p
            = structuredvm_state_profile_runtime (& s);
          structuredvm_profile_runtime_print_unspecialized (ctx, p);
        }
      structuredvm_state_finalize (& s);

      /* Destroy the Jittery routine.  Since here the reference count is exactly
         one by construction structuredvm_destroy_routine would work just as
         well, but in more complex cases where routines are shared by objects
         destroyed at multiple times the user will want to unpin, like in this
         example. */
      structuredvm_unpin_routine (vmr);
    }

  /* Destroy the print context. */
  jitter_print_context_destroy (ctx);

  /* End the ostream and finalise the GNU Libtextstyle wrapper, if used. */
#ifdef JITTER_WITH_LIBTEXTSTYLE
  styled_ostream_free (ostream);
  jitter_print_libtextstyle_finalize ();
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE

  /* Finalize the structured-VM subsystem. */
  structuredvm_finalize ();
}




/* Main function.
 * ************************************************************************** */

int
main (int argc, char **argv)
{
  /* Parse the command-line arguments, including options. */
  struct structured_command_line cl;
  structured_parse_command_line (& cl, argc, argv);

  /* Do what was requested on the command line. */
  structured_work (& cl);

  /* Exit with success, if we're still alive. */
  return EXIT_SUCCESS;
}
