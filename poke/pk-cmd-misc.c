/* pk-cmd-misc.c - Miscellaneous dot-commands.  */

/* Copyright (C) 2019, 2020, 2021, 2022, 2023, 2024 Jose E. Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h> /* For system.  */
#include "xalloc.h"

#include "findprog.h"
#include "readline.h"

#include "poke.h"
#include "pk-cmd.h"
#include "pk-utils.h"

static int
pk_cmd_exit (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* exit CODE */

  int code;
  assert (argc == 2);
  if (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_NULL)
    code = 0;
  else
    code = (int) PK_CMD_ARG_INT (argv[1]);

  poke_exit_p = 1;
  poke_exit_code = code;
  return 1;
}

static int
pk_cmd_version (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* version */
  pk_print_version (1 /* hand_p */);
  return 1;
}

/* Call the info command for the poke documentation, using
   the requested node.  */
static int
pk_cmd_doc (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  int ret = 1;

  /* This command is inherently interactive.  So if we're not
     supposed to be in interactive mode, then do nothing.  */
  if (poke_interactive_p)
  {
    char *cmd = NULL;

    /* Unless the doc viewer is set to `less', try first to use
       `info'.  */

    if (STRNEQ (pk_var_string ("pk_doc_viewer"), "less"))
      {
        const char info_prog_name[] = "info";
        const char *ip = find_in_path (info_prog_name);
        if (STRNEQ (ip, info_prog_name))
          {
            int size = 0;
            int bytes = 64;
            do
              {
                size = bytes + 1;
                cmd = xrealloc (cmd, size);
                bytes = snprintf (cmd, size, "info -f \"%s/poke.info\"",
                                  poke_infodir);
              }
            while (bytes >= size);

            if (argv[1].type == PK_CMD_ARG_STR)
              {
                const char *node = argv[1].val.str;
                cmd = xrealloc (cmd, bytes + 7 + strlen (node));
                strcat (cmd, " -n \"");
                strcat (cmd, node);
                strcat (cmd, "\"");
              }
          }
      }

    /* Fallback to `less' if no `info' was found.  */
    if (cmd == NULL)
      {
        const char info_prog_name[] = "less";
        const char *ip = find_in_path (info_prog_name);
        if (STREQ (ip, info_prog_name))
          {
            pk_term_class ("error");
            pk_puts ("error: ");
            pk_term_end_class ("error");
            pk_puts ("a suitable documentation viewer is not installed.\n");
            return 0;
          }

        asprintf (&cmd, "less -p '%s' %s/poke.text",
                  argv[1].val.str, poke_docdir);
        assert (cmd != NULL);
      }

    /* Open the documentation at the requested page.  */
    ret = (0 == system (cmd));
    free (cmd);
  }

  return ret;
}

static int
pk_cmd_jmd (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  pk_val pk_cmd_jmd = pk_decl_val (poke_compiler, "pk_cmd_jmd");
  pk_val exit_exception = PK_NULL;
  pk_val retval = PK_NULL;

  assert (argc == 1);
  assert (pk_cmd_jmd != PK_NULL);

  if (pk_call (poke_compiler, pk_cmd_jmd, &retval, &exit_exception,
               0) == PK_ERROR
      || exit_exception != PK_NULL)
    PK_UNREACHABLE ();

  return 1;
}

/* A completer to provide the node names of the info
   documentation.  */
char *
doc_completion_function (const char *x, int state)
{
  static char **nodelist = NULL;
  if (nodelist == NULL)
    {
      int n_nodes = 0;
      char nlfile[256];
      snprintf (nlfile, 256, "%s/nodelist", poke_docdir);
      FILE *fp = fopen (nlfile, "r");
      if (fp == NULL)
        return NULL;
      char *lineptr = NULL;
      size_t size = 0;
      while (!feof (fp))
        {
          int x = getline (&lineptr, &size, fp);
          if (x != -1)
            {
              nodelist = xrealloc (nodelist, ++n_nodes * sizeof (*nodelist));
              lineptr [strlen (lineptr) - 1] = '\0';
              nodelist[n_nodes - 1] = xstrdup (lineptr);
            }
        }
      fclose (fp);
      free (lineptr);
      nodelist = xrealloc (nodelist, ++n_nodes * sizeof (*nodelist));
      nodelist[n_nodes - 1] = NULL;
    }

  static int idx = 0;
  if (state == 0)
    idx = 0;
  else
    ++idx;

  int len = strlen (x);
  while (1)
    {
      const char *name = nodelist[idx];
      if (name == NULL)
        break;

      if (strncmp (name, x, len) == 0)
        return xstrdup (name);

      idx++;
    }

  return NULL;
}


const struct pk_cmd quit_cmd =
  {"quit", "?i", "", 0, NULL, NULL, pk_cmd_exit, ".quit [CODE]", NULL};

const struct pk_cmd exit_cmd =
  {"exit", "?i", "", 0, NULL, NULL, pk_cmd_exit, ".exit [CODE]", NULL};

const struct pk_cmd version_cmd =
  {"version", "", "", 0, NULL, NULL, pk_cmd_version, ".version", NULL};

const struct pk_cmd jmd_cmd =
  {"jmd", "", "", 0, NULL, NULL, pk_cmd_jmd, ".jmd", NULL};

const struct pk_cmd doc_cmd =
  {"doc", "?s", "", 0, NULL, NULL, pk_cmd_doc, ".doc [section]", doc_completion_function};
