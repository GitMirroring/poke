/* Jitter: test-case output normaliser.

   Copyright (C) 2017, 2020 Luca Saiu
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


/* Rationale.
 * ************************************************************************** */

/* The problem is filtering a test case output into a one-line normalized form,
   easy to compare for equality with a hand-written expected output, in Jitter's
   test suite.

   After becoming very annoyed by the lack of portability of sed scipts and tr
   command lines I decided to write this stupid program myself.  I have been
   surprised twice in a few days by the counter-intuitive behavior of two
   different, and quite widely used, sed implementations; with tr I have not had
   negative experiences yet, but the Autoconf manual warnings have alerted me.

   Just relying on GNU semantics is unfortunately not feasible here if I want
   portability.  My strive for portability has already improved the quality of
   the code, and within reasonable limits is a good thing. */




/* Specification.
 * ************************************************************************** */

/* This program performs the following transformation:

   - '\r' characters are completely ignored in the input, and never produced in
     the output: an input consisting of only '\r' characters is considered
     empty;
   - empty input becomes empty output;
   - in non-empty input every '\n' character except one at the very end of
     the input (possibly followed by '\r' characters only) becomes a '/'
     character;
   - the last character in the input, if '\n', remains '\n' in the output;
   - if the last character in the input is not '\n' then an '\n' is appended to
   the output.

   The input cannot contain '\0' or '/' characters.  (Rationale: '/' characters
   could be supported as ordinary characters, but I expect they will almost
   always be used by mistake: the fact that they become undistinghishable from
   '\n' in most circumstances makes them unsuitable to used in test cases).

   This syntax makes it very convenient to express the expected output, even
   consisting of mulitple lines, within one line, as encoded by this program.
   Each test case output will be filtered through this program and compared.

   Examples:
     ""             -->  ""
     "\r"           -->  ""
     "\n"           -->  "\n"
     "\n\n"         -->  "/\n"
     "a"            -->  "a\n"
     "a\nb"         -->  "a/b\n"
     "a\nb\n"       -->  "a/b\n"
     "a\nb\nc"      -->  "a/b/c\n"
     "a\nb\nc\n"    -->  "a/b/c\n"
     "a\nb\nc\n\n"  -->  "a/b/c/\n"  */




/* Implementation.
 * ************************************************************************** */

/* Include the Gnulib header. */
#include <config.h>

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>

/* The data expressed in the command line. */
struct jitterc_command_line
{
  /* Nothing useful here.  Adding one field just to avoid an empty struct for
     portability. */
  int useless;
};

/* Command-line option specification. */
static struct argp_option jitterc_option_specification[] =
  {
   /* Common GNU-style options. */
   {NULL, '\0', NULL, OPTION_DOC, "Common GNU-style options:", -1},

   /* Option terminator. */
   { 0 }
  };

/* Short help text to print on --help and --version .  Defaults are
   supplied by the library if these are not defined; I guess they
   use weak symbols on reasonable platforms. */
const char *argp_program_version
  = "filter-test-case-output (" PACKAGE_NAME " " PACKAGE_VERSION ")\n"
    "Copyright (C) 2020 Luca Saiu.\n"
    "Jitter comes with ABSOLUTELY NO WARRANTY.\n"
    "You may redistribute copies of Jitter under the terms of the GNU General Public\n"
    "License, version 3 or any later version published by the Free Software Foundation.\n"
    "For more information see the file named COPYING in the source distribution.\n\n"
    "Written by Luca Saiu <http://ageinghacker.net>.";
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

/* Forward declaration. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state);

/* The parser main data structure. */
static struct argp argp =
  { jitterc_option_specification,
    parse_opt,
    "",
    "Filter stdin, supposed to be a test case output, into one line" };

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  /* struct jitterc_command_line *cl = state->input; */
  switch (key)
    {
    case ARGP_KEY_ARG:
      argp_error (state, "this program must have no non-option arguments");
      break;

    /*
    case ARGP_KEY_END:
      if (state->arg_num != 0)
        argp_error (state, "you gave %i input files instead of zero.",
                    (int)state->arg_num);
      break;
    */

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static void
work (void)
{
  int c;
  char previous = '\0';

  /* First phase: read one character; until there are no more characters, print
     the previous one, transformed as appropriate for one input chraracter which
     is not the last one, and remember what we have just read. */
  while ((c = getchar ()) != EOF)
    {
      if (previous != '\0')
        putchar (previous);
      
      switch (c)
        {
        case '\r':
          /* Ignore '\r' characters, so that I can reliably compare the output
             with an expected string stored in a  text file in Unix-format. */
          continue;

        case '\n':
          previous = '/';
          break;

        default:
          previous = c;
        }
    }

  /* We have arrived at the end of the output.  Complete the output if needed. */
  switch (previous)
    {
    case '\0':
      break;
    case '/':
      putchar ('\n');
      break;
    default:
      putchar (previous);
      putchar ('\n');
    }
}

int
main (int argc, char **argv)
{
  /* Parse our arguments; cl will contain the information provided in the
     command line, if I decide to ever put anything there. */
  struct jitterc_command_line cl __attribute__ ((unused));
  argp_parse (&argp, argc, argv,
              0,//ARGP_IN_ORDER,
              0, &cl);

  /* Do the actual useful work. */
  work ();
  
  exit (EXIT_SUCCESS);
}
