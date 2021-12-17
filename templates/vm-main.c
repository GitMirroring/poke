/* VM default frontend for vmprefix VM.

   Copyright (C) 2016, 2017, 2018, 2019, 2020, 2021 Luca Saiu
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


/* Generated file warning.
 * ************************************************************************** */

/* Unless this file is named exactly "vm-main.c" , without any prefix, you are
   looking at a machine-generated derived file.  The original source is the
   vm-main.c template from Jitter. */




/* Include headers.
 * ************************************************************************** */

#ifdef HAVE_CONFIG_H
/* Use Gnulib, if available; on GNU everything should work even without it.

   FIXME: I might want to find a solution for other systems, but they are not
   prioritary. */
# include <config.h>
#endif // #ifdef HAVE_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <argp.h>

#include <jitter/jitter.h>

/* Include optional headers. */
#ifdef JITTER_HAVE_SETRLIMIT
# include <sys/resource.h> /* For getrlimit and setrlimit . */
#endif // #ifdef JITTER_HAVE_SETRLIMIT

#include <jitter/jitter-parse-int.h>
#include <jitter/jitter-fatal.h>

#include "vmprefix-vm.h"




/* Command line handling.
 * ************************************************************************** */

/* The way to print defects. */
enum vmprefix_print_defects_how
  {
    vmprefix_print_defects_how_summary,
    vmprefix_print_defects_how_list,
    vmprefix_print_defects_how_replacements,
    vmprefix_print_defects_how_no
  };

/* All the information encoded by the user in the command line. */
struct vmprefix_main_command_line
{
  bool debug;
  enum vmprefix_print_defects_how print_defects;
  bool profile_specialized;
  bool profile_unspecialized;
  bool progress_on_stderr;
  bool print_locations, print_routine, disassemble_routine, run_routine;
  bool slow_literals_only, slow_registers_only;
  bool optimization_rewriting;
  char *input_file;
  char *objdump_name;
  char *objdump_options; /* Use default options when NULL. */
  bool objdump_name_overridden;

#ifdef JITTER_HAVE_ALARM
  /* The wall-clock run time limit in seconds, or 0 if there is no limit. */
  unsigned int wall_clock_run_time_limit;
#endif // #ifdef JITTER_HAVE_ALARM

#ifdef JITTER_HAVE_SETRLIMIT
  /* The CPU time limit in seconds, or RLIM_INFINITY if there is no limit. */
  rlim_t cpu_time_limit;
#endif // #ifdef JITTER_HAVE_SETRLIMIT
};

/* Numeric identifiers for --no-* , or more in general "default" options having
   no short equivalent and specifying the default behavior; these are
   particularly convenient for interactive use where a complex command line is
   modified frequently.  Each case must have a value which is not an ASCII
   character. */
enum vmprefix_vm_negative_option
  {
    vmprefix_vm_negative_option_no_cross_disassemble = -1,
    vmprefix_vm_negative_option_no_disassemble = -2,
    vmprefix_vm_negative_option_no_debug = -3,
    vmprefix_vm_negative_option_no_dry_run = -4,
    vmprefix_vm_negative_option_no_print_locations = -5,
    vmprefix_vm_negative_option_no_print_routine = -6,
    vmprefix_vm_negative_option_no_print_defects = -7,
    vmprefix_vm_negative_option_no_profile_specialized = -8,
    vmprefix_vm_negative_option_no_profile_unspecialized = -9,
    vmprefix_vm_negative_option_no_progress_on_stderr = -10,
    vmprefix_vm_negative_option_no_slow_literals_only = -11,
    vmprefix_vm_negative_option_no_slow_registers_only = -12,
    vmprefix_vm_negative_option_no_slow_only = -13,
    vmprefix_vm_negative_option_optimization_rewriting = -14
  };

/* Numeric keys for options having only a long format.  These must not conflict
   with any value in enum vmprefix_vm_negative_option . */
enum vmprefix_vm_long_only_option
  {
    vmprefix_vm_long_only_option_print_locations = -109,
    vmprefix_vm_long_only_option_print_defects = -110,
    vmprefix_vm_long_only_option_profile_specialized = -111,
    vmprefix_vm_long_only_option_profile_unspecialized = -112,
    vmprefix_vm_long_only_option_dump_jitter_version = -113,
    vmprefix_vm_long_only_option_slow_only = -114
  };

/* Update our option state with the information from a single command-line
   option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct vmprefix_main_command_line *cl = state->input;
  switch (key)
    {
      /* FIXME: within the case I could use state->arg_num to get the current
         non-option argument index, if needed. */
    case ARGP_KEY_INIT:
      /* Set reasonable default values. */
      cl->debug = false;
      cl->progress_on_stderr = false;
      cl->print_locations = false;
      cl->print_defects = vmprefix_print_defects_how_no;
      cl->profile_specialized = false;
      cl->profile_unspecialized = false;
      cl->print_routine = false;
      cl->disassemble_routine = false;
      cl->run_routine = true;
      cl->slow_literals_only = false;
      cl->slow_registers_only = false;
      cl->optimization_rewriting = true;
      cl->input_file = NULL;
      cl->objdump_name = JITTER_OBJDUMP;
      cl->objdump_name_overridden = false;
      cl->objdump_options = NULL;
#ifdef JITTER_HAVE_ALARM
      cl->wall_clock_run_time_limit = 0;
#endif // #ifdef JITTER_HAVE_ALARM
#ifdef JITTER_HAVE_SETRLIMIT
      cl->cpu_time_limit = RLIM_INFINITY;
#endif // #ifdef JITTER_HAVE_SETRLIMIT
      break;
    case 't':
      {
        jitter_long_long limit;
        if (jitter_string_to_long_long_inconvenient (arg, & limit) != 0)
          argp_error (state, "--time-limit: invalid integer %s", arg);
#ifdef JITTER_HAVE_ALARM
        if (limit < 0)
          argp_error (state, "--time-limit: negative time %s", arg);
        else if (limit > UINT_MAX)
          argp_error (state, "--time-limit: integer %s out of range", arg);
        else if (limit > 0)
          cl->wall_clock_run_time_limit = limit;
#else
        fprintf (stderr, "warning: alarm is disabled\n");
#endif // #ifdef JITTER_HAVE_ALARM
        }
      break;
    case 'c':
      {
        jitter_long_long limit;
        if (jitter_string_to_long_long_inconvenient (arg, & limit) != 0)
          argp_error (state, "--cpu-time-limit: invalid integer %s", arg);
#ifdef JITTER_HAVE_SETRLIMIT
        if (limit < 0)
          argp_error (state, "--cpu-time-limit: negative time %" JITTER_PRIill,
                      limit);
        else if (limit > 0)
          cl->cpu_time_limit = limit;
#else
        fprintf (stderr, "warning: setrlimit is disabled\n");
#endif // #ifdef JITTER_HAVE_SETRLIMIT
        }
      break;
    case 'd':
      cl->debug = true;
      break;
    case vmprefix_vm_long_only_option_dump_jitter_version:
      printf ("%s\n", JITTER_PACKAGE_VERSION);
      exit (EXIT_SUCCESS);
    case vmprefix_vm_negative_option_no_debug:
      cl->debug = false;
      break;
    case 'e':
      cl->progress_on_stderr = true;
      break;
    case vmprefix_vm_negative_option_no_progress_on_stderr:
      cl->progress_on_stderr = false;
      break;
    case vmprefix_vm_long_only_option_print_locations:
      cl->print_locations = true;
      break;
    case vmprefix_vm_long_only_option_print_defects:
      if      (! strcmp (arg, "summary"))
        cl->print_defects = vmprefix_print_defects_how_summary;
      else if (! strcmp (arg, "list"))
        cl->print_defects = vmprefix_print_defects_how_list;
      else if (! strcmp (arg, "replacements"))
        cl->print_defects = vmprefix_print_defects_how_replacements;
      else if (! strcmp (arg, "no"))
        cl->print_defects = vmprefix_print_defects_how_no;
      else
        argp_error (state, "invalid --print-defects argument \"%s\": "
                    "not one of \"summary\", \"list\", "
                    "\"replacements\", \"no\".", arg);
      break;
    case vmprefix_vm_long_only_option_profile_specialized:
      cl->profile_specialized = true;
      break;
    case vmprefix_vm_long_only_option_profile_unspecialized:
      cl->profile_unspecialized = true;
      break;
    case 'p':
      cl->print_routine = true;
      break;
    case vmprefix_vm_negative_option_no_print_locations:
      cl->print_locations = false;
      break;
    case vmprefix_vm_negative_option_no_print_routine:
      cl->print_routine = false;
      break;
    case vmprefix_vm_negative_option_no_print_defects:
      cl->print_defects = vmprefix_print_defects_how_no;
      break;
    case vmprefix_vm_negative_option_no_profile_specialized:
      cl->profile_specialized = false;
      break;
    case vmprefix_vm_negative_option_no_profile_unspecialized:
      cl->profile_unspecialized = false;
      break;
    case 'b':
      cl->objdump_name = arg;
      cl->objdump_name_overridden = true;
      break;
    case 'B':
      cl->objdump_options = arg;
      break;
    case 'D':
      cl->disassemble_routine = true;
      if (! cl->objdump_name_overridden)
        cl->objdump_name = (arg != NULL) ? arg : JITTER_OBJDUMP;
      break;
    case vmprefix_vm_negative_option_no_disassemble:
    case vmprefix_vm_negative_option_no_cross_disassemble:
      cl->disassemble_routine = false;
      break;
    case 'C':
      cl->disassemble_routine = true;
#if defined(JITTER_CROSS_COMPILING) && defined (JITTER_CROSS_OBJDUMP)
      if (! cl->objdump_name_overridden)
        cl->objdump_name = JITTER_CROSS_OBJDUMP;
#else
      if (! cl->objdump_name_overridden)
        cl->objdump_name = JITTER_OBJDUMP;
#endif // #if defined(JITTER_CROSS_COMPILING) && defined (JITTER_CROSS_OBJDUMP)
      break;
    case 'n':
      cl->run_routine = false;
      break;
    case vmprefix_vm_negative_option_no_dry_run:
      cl->run_routine = true;
      break;
    case 'L':
      cl->slow_literals_only = true;
      break;
    case 'R':
      cl->slow_registers_only = true;
      break;
    case vmprefix_vm_long_only_option_slow_only:
      cl->slow_literals_only = true;
      cl->slow_registers_only = true;
      break;
    case vmprefix_vm_negative_option_no_slow_literals_only:
      cl->slow_literals_only = false;
      break;
    case vmprefix_vm_negative_option_no_slow_registers_only:
      cl->slow_registers_only = false;
      break;
    case vmprefix_vm_negative_option_no_slow_only:
      cl->slow_literals_only = false;
      cl->slow_registers_only = false;
      break;
    case 'r':
      cl->optimization_rewriting = false; /* The default is true. */
      break;
    case vmprefix_vm_negative_option_optimization_rewriting:
      cl->optimization_rewriting = true;
      break;

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
      break;

    /* case ARGP_KEY_FINI: /\* If this case is omitted, the default is sensible. *\/ */
    /*   break; */

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

/* Command-line option specification. */
static struct argp_option vmprefix_main_option_specification[] =
  {/* Most frequently used options. */
   {NULL, '\0', NULL, OPTION_DOC, "Frequently used options:", 10},
   {"print-routine", 'p', NULL, 0,
    "Print back the parsed routine"},
   {"dry-run", 'n', NULL, 0,
    "Do not actually run the routine"},
   {"no-run", '\0', NULL, OPTION_ALIAS },
   /* Most frequently used negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 11},
   {"no-dry-run", vmprefix_vm_negative_option_no_dry_run, NULL, 0,
    "Actually run the parsed routine (default)"},
   {"no-print-routine", vmprefix_vm_negative_option_no_print_routine, NULL, 0,
    "Don't print back the parsed routine (default)"},

   /* Disassembly options. */
   {NULL, '\0', NULL, OPTION_DOC, "Disassembly options:", 20},
   {"objdump", 'b', "OBJDUMP", 0,
    "If disassemblying use the given objdump program, overriding the default"},
   {"objdump-options", 'B', "OPTS", 0,
    "If disassemblying use the given objdump options, overriding the default"
    " (" JITTER_OBJDUMP_OPTIONS
    ").  Options for raw dumping and for setting the correct endianness are "
    " provided in any case, but user options take precedence"},
   {"disassemble", 'D', "OBJDUMP", OPTION_ARG_OPTIONAL,
    "Disassemble native " JITTER_HOST_TRIPLET " code using the given objdump"
    " program (default: " JITTER_OBJDUMP
    ") with, unless overridden, the default options"},
   {"cross-disassemble", 'C', NULL, 0,
#if defined(JITTER_CROSS_COMPILING) && defined (JITTER_CROSS_OBJDUMP)
    "Cross-disassemble " JITTER_HOST_TRIPLET " code (presumably through"
    " an emulator running on " JITTER_BUILD_TRIPLET
    " , on which this program was compiled) using "
    JITTER_CROSS_OBJDUMP " by default"
#else
    "Disassemble native " JITTER_HOST_TRIPLET " code, using " JITTER_OBJDUMP
    " by default with, unless overridden, the default options"
    " (this option exists for compatibility with cross-compiled builds)"
#endif // #if defined(JITTER_CROSS_COMPILING) && defined (JITTER_CROSS_OBJDUMP)
   },
   {"print-locations", vmprefix_vm_long_only_option_print_locations, NULL, 0,
    "Print data-location information, mapping VM structures to hardware "
    "resources"},
   /* Disassembly negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 21},
   {"no-disassemble", vmprefix_vm_negative_option_no_disassemble, NULL, 0,
    "Don't disassemble the parsed routine (default)"},
   {"no-cross-disassemble", vmprefix_vm_negative_option_no_cross_disassemble,
    NULL, 0,
    "Don't cross-disassemble the parsed routine (default)"},
   {"no-print-locations", vmprefix_vm_negative_option_no_print_locations, NULL, 0,
    "Don't print data location information (default)"},

   /* Debugging, testing and benchmarking options. */
   {NULL, '\0', NULL, OPTION_DOC,
    "Debugging, testing and benchmarking options:", 30},
   {"progress-on-stderr", 'e', NULL, 0,
    "Show progress information on stderr instead of stdout"},
   {"debug", 'd', NULL, 0, "Enable debugging" },
   {"print-defects", vmprefix_vm_long_only_option_print_defects, "HOW", 0,
    "Print information about defective instructions.  HOW can be "
    "\"summary\" (only show the number of defect and replacement counts), "
    "\"list\" (show which specialised instructions are defective), "
    "\"replacements\" (show which specialised instructions replaces which), "
    "\"no\" (print nothing).  "
    "Default: no"},
   {"profile-specialized", vmprefix_vm_long_only_option_profile_specialized,
    NULL, 0,
    "Print VM specialised instruction profiling information, if configured in"},
   {"profile-unspecialized", vmprefix_vm_long_only_option_profile_unspecialized,
    NULL, 0,
    "Print VM unspecialised  instruction profiling information, if configured "
    "in"},
   {"slow-literals-only", 'L', NULL, 0,
    "Use slow literals even where fast literals would be available"
    " (this is mostly useful to measure the speedup introduced by fast"
    " literals, or possibly to benchmark a worst-case scenario)"},
   {"slow-registers-only", 'R', NULL, 0,
    "Use slow registers even when fast registers would be available"
    " (this is mostly useful to measure the speedup introduced by fast"
    " registers, or to benchmark a worst-case scenario)"},
   {"slow-only", vmprefix_vm_long_only_option_slow_only, NULL, 0,
    "Equivalent to passing both --slow-literals-only and"
    " --slow-registers-only"},
   {"dump-jitter-version", vmprefix_vm_long_only_option_dump_jitter_version,
    NULL, 0,
    "Print the Jitter version only, without any surrounding text; this "
    "is convenient for scripts" },
   {"dump-version", '\0', NULL, OPTION_ALIAS },
   {"no-optimization-rewriting", 'r', NULL, 0,
    "Disable optimization rewriting (this is mostly useful for debugging "
    "rewrite rules and for measuring the speedup they introduce)" },
   /* Debugging, testing and benchmarking negative options. */
   {NULL, '\0', NULL, OPTION_DOC, "", 31},
   {"no-progress-on-stderr", vmprefix_vm_negative_option_no_progress_on_stderr,
    NULL, 0, "Show progress information on stdout (default)"},
   {"no-debug", vmprefix_vm_negative_option_no_debug,
    NULL, 0, "Disable debugging (default)"},
   {"no-print-defects", vmprefix_vm_negative_option_no_print_defects, NULL, 0,
    "Don't print defect and replacement information, as with "
    "--print-defects=no (default)"},
   {"no-profile-specialized", vmprefix_vm_negative_option_no_profile_specialized,
    NULL, 0, "Disable specialized instruction profiling (default)"},
   {"no-profile-unspecialized", vmprefix_vm_negative_option_no_profile_unspecialized,
    NULL, 0, "Disable unspecialized instruction profiling (default)"},
   {"no-slow-literals-only", vmprefix_vm_negative_option_no_slow_literals_only,
    NULL, 0, "Use fast literals when possible (default)"},
   {"no-slow-registers-only", vmprefix_vm_negative_option_no_slow_registers_only,
    NULL, 0, "Use fast registers when possible (default)"},
   {"no-slow-only", vmprefix_vm_negative_option_no_slow_only, NULL, 0,
    "Equivalent to passing both --no-slow-literals-only and"
    " --no-slow-registers-only (default)"},
   {"optimization-rewriting", vmprefix_vm_negative_option_optimization_rewriting,
    NULL, 0, "Enable optimization rewriting (default)"},

   /* Test suite options. */
   {NULL, '\0', NULL, OPTION_DOC, "Options mostly useful for test suites:", 40},
   {"time-limit", 't', "S", 0,
    "Fail if wall-clock run time exceeds S seconds (0 for no limit)"
#ifndef JITTER_HOST_OS_IS_GNU
    "; this uses the alarm function, which may interfere with sleep on"
    " non-GNU systems where sleep is implemented with SIGALRM"
#endif // #ifndef JITTER_HOST_OS_IS_GNU
#ifndef JITTER_HAVE_ALARM
    "  (Ignored on this configuration.)"
#endif // #ifndef JITTER_HAVE_SETRLIMIT
   },
   {"cpu-time-limit", 'c', "S", 0,
    "Fail if CPU time exceeds S seconds (0 for no limit)"
#ifndef JITTER_HAVE_SETRLIMIT
    "  (Ignored on this configuration.)"
#endif // #ifndef JITTER_HAVE_SETRLIMIT
   },

   /* Common GNU-style options. */
   {NULL, '\0', NULL, OPTION_DOC, "Common GNU-style options:", -1},
   /* These are automatically generated. */

   /* Option terminator. */
   { 0 }};

/* Customised text text to print on --help and --version . */
static void
the_argp_program_version_hook (FILE * restrict stream, struct argp_state *s)
{
  const struct jitter_vm_configuration *c = vmprefix_vm_configuration;

  const char *instrumentation
    = jitter_vm_instrumentation_to_string (c->instrumentation);
  fprintf (stream,
           "%s VM driver, %s%s%s dispatch "
           "(" JITTER_PACKAGE_NAME ") " JITTER_PACKAGE_VERSION "\n",
           c->lower_case_prefix,
           instrumentation,
           (strlen (instrumentation) > 0 ? ", " : ""),
           c->dispatch_human_readable);
  fprintf
     (stream,
      "Copyright (C) 2021 Luca Saiu.\n"
      JITTER_PACKAGE_NAME " comes with ABSOLUTELY NO WARRANTY.\n"
      "You may redistribute copies of GNU Jitter under the terms of the GNU\n"
      "General Public License, version 3 or any later version published\n"
      "by the Free Software Foundation.  For more information see the\n"
      "file named COPYING.\n"
      "\n"
      "Written by Luca Saiu <http://ageinghacker.net> (GNU Jitter, its\n"
      "runtime, this driver program).\n");
}
void (*argp_program_version_hook) (FILE * restrict stream, struct argp_state *s)
  = the_argp_program_version_hook;
const char *argp_program_bug_address = JITTER_PACKAGE_BUGREPORT;

/* The parser main data structure. */
static struct argp argp =
  {
    vmprefix_main_option_specification,
    parse_opt,
    "FILE.vm",
    "Run a routine encoded as a text file on the " VMPREFIX_VM_NAME
    " VM, using " JITTER_DISPATCH_NAME_STRING " dispatch."
  };




/* Main function.
 * ************************************************************************** */

/* Print a heading.  This is useful for defects. */
static void
vmprefix_print_heading (jitter_print_context ctx, const char *heading)
{
  jitter_print_begin_class (ctx, "vmprefix-comment");
  if (vmprefix_vm->defect_no == 0)
    jitter_print_char_star (ctx, "No defects.");
  else
    {
      jitter_print_char_star (ctx, heading);
      jitter_print_char_star (ctx, ":");
    }
  jitter_print_end_class (ctx);
  jitter_print_char_star (ctx, "\n");
}

int
main (int argc, char **argv)
{
  /* Parse our arguments; every option seen by 'parse_opt' will
     be reflected in 'arguments'. */
  struct vmprefix_main_command_line cl;
  argp_parse (&argp, argc, argv,
              0,//ARGP_IN_ORDER,
              0, &cl);

  /* Initialise the GNU Libtextstyle wrapper, if used. */
#ifdef JITTER_WITH_LIBTEXTSTYLE
  jitter_print_libtextstyle_initialize ();

  /* FIXME: this should be less crude, but is enough for checking that the
     libtextstyle wrapper works. */
  char *style_file_name = "vmprefix-style.css";
  styled_ostream_t ostream
    = styled_ostream_create (STDOUT_FILENO, "(stdout)", TTYCTL_AUTO,
                             style_file_name);
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE

  /* Make a print context, using the Libtextstyle wrapper if possible. */
  jitter_print_context ctx
#ifdef JITTER_WITH_LIBTEXTSTYLE
    = jitter_print_context_make_libtextstyle (ostream);
#else
    = jitter_print_context_make_file_star (stdout);
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE

  FILE *progress;
  if (cl.progress_on_stderr)
    progress = stderr;
  else
    progress = stdout;

#ifdef JITTER_HAVE_ALARM
  /* Set a limit to the wall-clock run time, if so requested on the command
     line. */
  if (cl.wall_clock_run_time_limit != 0)
    alarm (cl.wall_clock_run_time_limit);
#endif // #ifdef JITTER_HAVE_ALARM

#ifdef JITTER_HAVE_SETRLIMIT
  /* Set a limit to the CPU time, if so requested on the command line. */
  if (cl.cpu_time_limit != RLIM_INFINITY)
    {
      if (cl.debug)
        fprintf (progress, "Setting resource limits...\n");
      struct rlimit limit;
      bool getrlimit_failed = false;
      if (getrlimit (RLIMIT_CPU, & limit) != 0)
        {
          fprintf (stderr, "warning: getrlimit failed\n");
          getrlimit_failed = true;
        }
      limit.rlim_cur = cl.cpu_time_limit;
      if (! getrlimit_failed)
        if (setrlimit (RLIMIT_CPU, & limit) != 0)
          jitter_fatal ("setrlimit failed");
    }
#endif // #ifdef JITTER_HAVE_SETRLIMIT

  if (cl.debug)
    fprintf (progress, "Initializing...\n");
  vmprefix_initialize ();

  /* Make an empty VM routine, and set options as requested by the user. */
  struct vmprefix_mutable_routine *r = vmprefix_make_mutable_routine ();
  if (cl.debug)
    fprintf (progress,
             "Options:\n"
             "* slow literals only: %s\n"
             "* slow registers only: %s\n"
             "* optimization rewriting: %s\n",
             cl.slow_literals_only ? "yes" : "no",
             cl.slow_registers_only ? "yes" : "no",
             cl.optimization_rewriting ? "yes" : "no");
  vmprefix_set_mutable_routine_option_slow_literals_only
     (r, cl.slow_literals_only);
  vmprefix_set_mutable_routine_option_slow_registers_only
     (r, cl.slow_registers_only);
  vmprefix_set_mutable_routine_option_optimization_rewriting
     (r, cl.optimization_rewriting);

  /* Print the VM configuration if in debugging mode. */
  if (cl.debug)
    vmprefix_print_vm_configuration (progress, vmprefix_vm_configuration);

  if (cl.debug)
    fprintf (progress, "Parsing...\n");
  if (! strcmp (cl.input_file, "-"))
    vmprefix_parse_mutable_routine_from_file_star (stdin, r);
  else
    vmprefix_parse_mutable_routine_from_file (cl.input_file, r);
  if (cl.debug)
    fprintf (progress, "The requried slow register number is %li per class.\n",
             (long) r->slow_register_per_class_no);

  /* Make an executable jittery routine. */
  if (cl.debug)
    fprintf (progress, "Making executable...\n");
  struct vmprefix_executable_routine *er
    = vmprefix_make_executable_routine (r);

  /* Print defect information. */
  switch (cl.print_defects)
    {
    case vmprefix_print_defects_how_summary:
      /* Do not print a heading line: this report is meant to be compact. */
      vmprefix_defect_print_summary (ctx);
      break;
    case vmprefix_print_defects_how_list:
      vmprefix_print_heading (ctx, "Defects");
      vmprefix_defect_print (ctx, 0);
      break;
    case vmprefix_print_defects_how_replacements:
      vmprefix_print_heading (ctx, "Defect replacements");
      vmprefix_defect_print_replacement_table (ctx, 0);
      break;
    case vmprefix_print_defects_how_no:
      /* Print nothing. */
      break;
    default:
      jitter_fatal ("invalid value for cl.print_defects: this should never "
                    "happen");
    }

  if (cl.print_routine)
    {
      if (cl.debug)
        fprintf (progress, "Printing back the routine...\n");
      vmprefix_mutable_routine_print (ctx, r);
    }

  if (cl.print_locations)
    {
      if (cl.debug)
        fprintf (progress, "Printing data location information...\n");
      vmprefix_dump_data_locations (ctx);
    }

  if (cl.disassemble_routine)
    {
      if (cl.debug)
        fprintf (progress, "Disassembling...\n");
      vmprefix_executable_routine_disassemble (ctx, er, true, cl.objdump_name,
                                               cl.objdump_options);
    }

  /* If we dumped data locations or printed back or disassembled the routine,
     this run is not performance critical and we afford a couple more syscalls.
     Flush the output buffer, so that the routine is visible before running, and
     possibly crashing. */
  if (cl.print_locations || cl.print_routine || cl.disassemble_routine)
    {
      jitter_print_flush (ctx);
      fflush (stdout);
      fflush (stderr);
    }

  if (cl.run_routine)
    {
      if (cl.debug)
        fprintf (progress, "Initializing VM state...\n");
      struct vmprefix_state s;
      vmprefix_state_initialize (& s);

      if (cl.debug)
        fprintf (progress, "Interpreting...\n");
      vmprefix_execute_executable_routine (er, & s);

      if (cl.profile_specialized)
        {
          if (cl.debug)
            fprintf (progress, "Printing specialised profile...\n");
          struct vmprefix_profile_runtime *pr
            = vmprefix_state_profile_runtime (& s);
          vmprefix_profile_runtime_print_specialized (ctx, pr);
        }

      if (cl.profile_unspecialized)
        {
          if (cl.debug)
            fprintf (progress, "Printing unspecialised profile...\n");
          struct vmprefix_profile_runtime *pr
            = vmprefix_state_profile_runtime (& s);
          vmprefix_profile_runtime_print_unspecialized (ctx, pr);
        }

      if (cl.debug)
        fprintf (progress, "Finalizing VM state...\n");
      vmprefix_state_finalize (& s);
    }

  if (cl.debug)
    fprintf (progress, "Destroying the routine data structure...\n");
  /* Destroy the Jittery routine in both its versions, executable and
     non-executable. */
  vmprefix_destroy_executable_routine (er);
  vmprefix_destroy_mutable_routine (r);

  if (cl.debug)
    fprintf (progress, "Finalizing...\n");
  vmprefix_finalize ();

  /* Destroy the print context. */
  jitter_print_context_destroy (ctx);

  /* End the ostream and finalise the GNU Libtextstyle wrapper, if used. */
#ifdef JITTER_WITH_LIBTEXTSTYLE
  styled_ostream_free (ostream);
  jitter_print_libtextstyle_finalize ();
#endif // #ifdef JITTER_WITH_LIBTEXTSTYLE

  if (cl.debug)
    fprintf (progress, "Still alive at exit.\n");
  return 0;
}
