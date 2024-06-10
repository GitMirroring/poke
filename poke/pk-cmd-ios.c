/* pk-cmd-ios.c - Commands for operating on IO spaces.  */

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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>
#include <readline.h>
#include "xalloc.h"

#include "poke.h"
#include "pk-cmd.h"
#include "pk-map.h"
#include "pk-utils.h"
#include "pk-ios.h"
#include "pk-table.h"
#if HAVE_HSERVER
#  include "pk-hserver.h"
#endif
#include "pk-repl.h" /* For poke_completion_function */

/* Get a Poke expression and compile it to get an IO space identifier.
   Lexical cuckolding is active to allow referring to IO spaces by
   name with $<...> constructs.  If the expression is invalid or
   doesn't evaluate to a valid IO space id then return NULL.  */

static pk_ios
expr_to_ios (const char *expr)
{
  pk_val val;
  pk_val exit_exception = PK_NULL;
  int ret;

  pk_set_lexical_cuckolding_p (poke_compiler, 1);
  ret = pk_compile_expression (poke_compiler, expr, NULL, &val,
                               &exit_exception);
  pk_set_lexical_cuckolding_p (poke_compiler, 0);

  if (ret != PK_OK || exit_exception != PK_NULL)
    {
      /* The compiler has already printed diagnostics in the
         terminal.  */
      if (exit_exception != PK_NULL)
        poke_handle_exception (exit_exception);
      return NULL;
    }

  /* See that VAL is an IO space, i.e. an int<32>.  */
  if (pk_type_code (pk_typeof (val)) != PK_TYPE_INT
      || pk_int_size (val) != 32)
    return NULL;

  return pk_ios_search_by_id (poke_compiler, pk_int_value (val));
}

static int
pk_cmd_ios (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* ios EXPR */
  pk_ios io;

  assert (argc == 2);

  if (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_NULL)
    /* Do nothing.  */
    return 0;
  else
    {
      assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_STR);

      io = expr_to_ios (PK_CMD_ARG_STR (argv[1]));
      if (io == NULL)
        {
          pk_puts ("error: no such IO space\n");
          return 0;
        }
    }

  pk_ios_set_cur (poke_compiler, io);
  if (poke_interactive_p && !poke_quiet_p)
    pk_printf (_("The current IOS is now `%s'.\n"),
               pk_ios_handler (pk_ios_cur (poke_compiler)));
  return 1;
}

static int
pk_cmd_sub (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  char *handler, *name;
  pk_ios ios;
  uint64_t base, size;

  assert (argc == 5);

  /* Collect and validate arguments.  */

  assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_STR);
  ios = expr_to_ios (PK_CMD_ARG_STR (argv[1]));
  if (ios == NULL)
    return 0;

  assert (PK_CMD_ARG_TYPE (argv[2]) == PK_CMD_ARG_UINT);
  base = PK_CMD_ARG_UINT (argv[2]);

  assert (PK_CMD_ARG_TYPE (argv[3]) == PK_CMD_ARG_UINT);
  size = PK_CMD_ARG_UINT (argv[3]);

  name = (PK_CMD_ARG_TYPE (argv[4]) == PK_CMD_ARG_STR
          ? PK_CMD_ARG_STR (argv[4])
          : NULL);

  /* Build the handler.  */
  if (asprintf (&handler, "sub://%d/0x%" PRIx64 "/0x%" PRIx64 "/%s",
                pk_ios_get_id (ios), base, size, name ? name : "") == -1)
    return 0;

  /* Open the IOS.  */
  if (pk_ios_open (poke_compiler, handler, 0, 1) == PK_IOS_NOID)
    {
      pk_printf (_("Error creating sub IOS %s\n"), handler);
      free (handler);
      return 0;
    }

  free (handler);
  return 1;
}

#define PK_PROC_UFLAGS "mM"
#define PK_PROC_F_MAPS     0x1
#define PK_PROC_F_MAPS_ALL 0x2

static int
pk_cmd_proc (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
#if defined HAVE_PROC

  pk_val pk_cmd_proc = pk_decl_val (poke_compiler, "pk_cmd_proc");
  pk_val retval = PK_NULL;
  pk_val exit_exception = PK_NULL;
  uint64_t pid;

  assert (pk_cmd_proc != PK_NULL);

  /* Get the PID of the process to open.  */
  assert (argc == 2);
  assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_INT);
  pid = PK_CMD_ARG_INT (argv[1]);

  if (pk_call (poke_compiler, pk_cmd_proc, &retval, &exit_exception,
               2,
               pk_make_uint (poke_compiler, pid, 64),
               pk_make_uint (poke_compiler, uflags, 32)) == PK_ERROR
      || exit_exception != PK_NULL)
    PK_UNREACHABLE (); /* This shouldn't happen.  */

  return (pk_int_value (retval) != -1);

#else
  pk_term_class ("error");
  pk_puts (_("error: "));
  pk_term_end_class ("error");
  pk_printf (_("this poke hasn't been built with support for .proc\n"));
  return 0;
#endif /* HAVE_PROC */
}

#define PK_FILE_UFLAGS "c"
#define PK_FILE_F_CREATE 0x1

static int
pk_cmd_file (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* file FILENAME */

  pk_val retval = PK_NULL;
  pk_val exit_exception = PK_NULL;
  pk_val pk_cmd_file = pk_decl_val (poke_compiler, "pk_cmd_file");

  assert (argc == 2);
  assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_STR);

  /* Create a new IO space.  */
  const char *arg_str = PK_CMD_ARG_STR (argv[1]);
  const char *filename = arg_str;
  int create_p = uflags & PK_FILE_F_CREATE;

  if (access (filename, F_OK) == 0
      && create_p)
    /* /c has no effect if the file exists.  */
    create_p = 0;

  if (pk_call (poke_compiler, pk_cmd_file, &retval, &exit_exception,
               2,
               pk_make_string (poke_compiler, filename),
               pk_make_int (poke_compiler, create_p, 32)) == PK_ERROR
      || exit_exception != PK_NULL)
    PK_UNREACHABLE (); /* This shouldn't happen.  */

  if (pk_int_value (retval) != -1)
    {
      pk_open_file_maps (pk_int_value (retval), filename);
      return 1;
    }
  else
    return 0;
}

static int
pk_cmd_close (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* close [#ID]  */

  pk_val pk_cmd_close = pk_decl_val (poke_compiler, "pk_cmd_close");
  pk_val retval = PK_NULL;
  pk_val exit_exception = PK_NULL;
  pk_ios io;
  const char *expr;

  assert (pk_cmd_close != PK_NULL);
  assert (argc == 2);
  assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_STR);
  expr = PK_CMD_ARG_STR (argv[1]);

  if (*expr == '\0')
    io = pk_ios_cur (poke_compiler);
  else
    {
      io = expr_to_ios (expr);
      if (io == NULL)
        {
          pk_printf (_("error: no such IO space\n"));
          return 0;
        }
    }

  if (pk_call (poke_compiler, pk_cmd_close, &retval, &exit_exception,
               1, pk_make_int (poke_compiler, pk_ios_get_id (io), 32)) == PK_ERROR
      || exit_exception != PK_NULL)
    PK_UNREACHABLE (); /* This shouldn't happen.  */

  return 1;
}

static int
pk_cmd_info_ios (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  pk_val pk_info_ios = pk_decl_val (poke_compiler, "pk_info_ios");
  pk_val retval, exit_exception;
  assert (pk_info_ios != PK_NULL);

  assert (argc == 1);

  if (pk_call (poke_compiler, pk_info_ios, &retval, &exit_exception,
               0) == PK_ERROR
      || exit_exception != PK_NULL)
    PK_UNREACHABLE (); /* This shouldn't happen.  */

  return 1;
}

static int
pk_cmd_load_file (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* load FILENAME */

  char *arg;
  char *filename = NULL;
  char *emsg;
  pk_val exit_exception;

  assert (argc == 2);
  arg = PK_CMD_ARG_STR (argv[1]);

  if ((emsg = pk_file_readable (arg)) == NULL)
    filename = arg;
  else if (arg[0] != '/')
    {
      /* Try to open the specified file relative to POKEDATADIR.  */
      if (asprintf (&filename, "%s/%s", poke_datadir, arg) == -1)
        {
          /* filename is undefined now, don't free */
          pk_fatal (_("out of memory"));
          return 0;
        }

      if ((emsg = pk_file_readable (filename)) != NULL)
        goto no_file;
    }
  else
    goto no_file;

  if (pk_compile_file (poke_compiler, filename, &exit_exception)
      != PK_OK)
    /* Note that the compiler emits its own error messages.  */
    goto error;
  if (exit_exception != PK_NULL)
    poke_handle_exception (exit_exception);

  if (filename != arg)
    free (filename);
  return 1;

 no_file:
  pk_puts (emsg);
 error:
  if (filename != arg)
    free (filename);
  return 0;
}

static int
pk_cmd_source_file (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* source FILENAME */

  char *arg;
  char *emsg;

  assert (argc == 2);
  arg = PK_CMD_ARG_STR (argv[1]);

  if ((emsg = pk_file_readable (arg)) != NULL)
    {
      pk_puts (emsg);
      return 0;
    }

  if (!pk_cmd_exec_script (arg))
    return 0;

  return 1;
}

static int
pk_cmd_mem (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* mem NAME */
  /* Create a new memory IO space.  */

  const char *arg_str = PK_CMD_ARG_STR (argv[1]);

  pk_val pk_cmd_mem = pk_decl_val (poke_compiler, "pk_cmd_mem");
  pk_val pk_cmd_mem_unique = pk_decl_val (poke_compiler, "pk_cmd_mem_unique");
  pk_val retval = PK_NULL;
  pk_val exit_exception = PK_NULL;

  assert (pk_cmd_mem != PK_NULL);
  assert (pk_cmd_mem_unique != PK_NULL);
  assert (argc == 2);
  assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_STR);

  if (*arg_str == '\0')
    {
      if (pk_call (poke_compiler, pk_cmd_mem_unique, &retval, &exit_exception,
                   0) == PK_ERROR
          || exit_exception != PK_NULL)
        PK_UNREACHABLE (); /* This shouldn't happen.  */
    }
  else
    {
      if (pk_call (poke_compiler, pk_cmd_mem, &retval, &exit_exception,
                   1, pk_make_string (poke_compiler, arg_str)) == PK_ERROR
          || exit_exception != PK_NULL)
        PK_UNREACHABLE (); /* This shouldn't happen.  */
    }

  return (pk_int_value (retval) != -1);
}

#ifdef HAVE_LIBNBD
static int
pk_cmd_nbd (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* nbd URI */

  assert (argc == 2);
  assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_STR);

  /* Create a new NBD IO space.  */
  const char *arg_str = PK_CMD_ARG_STR (argv[1]);
  char *nbd_name = xstrdup (arg_str);

  if (pk_ios_search (poke_compiler, nbd_name, PK_IOS_SEARCH_F_EXACT) != NULL)
    {
      printf (_("Buffer %s already opened.  Use `.ios IOS' to switch.\n"),
              nbd_name);
      free (nbd_name);
      return 0;
    }

  if (PK_IOS_NOID == pk_ios_open (poke_compiler, nbd_name, 0, 1))
    {
      pk_printf (_("Error creating NBD IOS %s\n"), nbd_name);
      free (nbd_name);
      return 0;
    }

  if (poke_interactive_p && !poke_quiet_p)
    pk_printf (_("The current IOS is now `%s'.\n"),
               pk_ios_handler (pk_ios_cur (poke_compiler)));

  return 1;
}
#endif /* HAVE_LIBNBD */

#ifdef HAVE_MMAP
static int
pk_cmd_mmap (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  /* mmap FILENAME BASE SIZE */
  /* Create a new mmap IO space.  */

  const char *filename;
  uint64_t base, size;

  pk_val pk_cmd_mmap = pk_decl_val (poke_compiler, "pk_cmd_mmap");
  pk_val retval = PK_NULL;
  pk_val exit_exception = PK_NULL;

  assert (pk_cmd_mmap != PK_NULL);
  assert (argc == 4);

  assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_STR);
  filename = PK_CMD_ARG_STR (argv[1]);

  assert (PK_CMD_ARG_TYPE (argv[2]) == PK_CMD_ARG_UINT);
  base = PK_CMD_ARG_UINT (argv[2]);

  assert (PK_CMD_ARG_TYPE (argv[3]) == PK_CMD_ARG_UINT);
  size = PK_CMD_ARG_UINT (argv[3]);

  if (pk_call (poke_compiler, pk_cmd_mmap, &retval, &exit_exception, 3,
               pk_make_string (poke_compiler, filename),
               pk_make_uint (poke_compiler, base, 64),
               pk_make_uint (poke_compiler, size, 64)) == PK_ERROR
      || exit_exception != PK_NULL)
    PK_UNREACHABLE (); /* This shouldn't happen.  */

  return (pk_int_value (retval) != -1);
}
#endif /* HAVE_MMAP */

const struct pk_cmd ios_cmd =
  {"ios", "?s", "", 0, NULL, NULL, pk_cmd_ios, ".ios IOS", poke_completion_function};

const struct pk_cmd file_cmd =
  {"file", "f", PK_FILE_UFLAGS, 0, NULL, NULL, pk_cmd_file, ".file FILE-NAME",
   rl_filename_completion_function};

const struct pk_cmd proc_cmd =
  {"proc", "i", PK_PROC_UFLAGS, 0, NULL, NULL, pk_cmd_proc, ".proc PID", NULL};

const struct pk_cmd sub_cmd =
  {"sub", "s,u,u,?s", "", 0, NULL, NULL, pk_cmd_sub, ".sub IOS, BASE, SIZE, [NAME]",
   poke_completion_function};

const struct pk_cmd mem_cmd =
  {"mem", "s", "", 0, NULL, NULL, pk_cmd_mem, ".mem NAME", NULL};

#ifdef HAVE_LIBNBD
const struct pk_cmd nbd_cmd =
  {"nbd", "s", "", 0, NULL, NULL, pk_cmd_nbd, ".nbd URI", NULL};
#endif

#ifdef HAVE_MMAP
const struct pk_cmd mmap_cmd =
  {"mmap", "s,u,u", "", 0, NULL, NULL, pk_cmd_mmap, ".mmap FILE-NAME, BASE, SIZE",
   rl_filename_completion_function};
#endif

const struct pk_cmd close_cmd =
  {"close", "s", "", PK_CMD_F_REQ_IO, NULL, NULL, pk_cmd_close,
   ".close [IOS]", poke_completion_function};

const struct pk_cmd info_ios_cmd =
  {"ios", "", "", 0, NULL, NULL, pk_cmd_info_ios, ".info ios", NULL};

const struct pk_cmd load_cmd =
  {"load", "f", "", 0, NULL, NULL, pk_cmd_load_file, ".load FILE-NAME",
   rl_filename_completion_function};

const struct pk_cmd source_cmd =
  {"source", "f", "", 0, NULL, NULL, pk_cmd_source_file,
   ".source FILE-NAME", rl_filename_completion_function};
