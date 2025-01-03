/* pk-cmd-compiler.c - Poke compiler related dot-commands.  */

/* Copyright (C) 2023, 2024, 2025 Jose E. Marchesi */

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

#include "poke.h"
#include "pk-cmd.h"
#include "pk-utils.h"
#include "pk-repl.h" /* For poke_completion_function */

static int
pk_cmd_compiler_ast (int argc, struct pk_cmd_arg argv[], uint64_t uflags)
{
  const char *expr;

  assert (argc == 2);
  assert (PK_CMD_ARG_TYPE (argv[1]) == PK_CMD_ARG_STR);

  expr = PK_CMD_ARG_STR (argv[1]);

  /* Compile the given string as a statement in debug mode.  Then
     fetch the AST textual representation from the incremental
     compiler and print it.   */

  {
    pk_val exit_exception;

    pk_set_debug_p (poke_compiler, 1);
    /* Note that the compiler will print its own error messages in
       case the following call fails.  */
    if (pk_compile_expression (poke_compiler, expr,
                               NULL, NULL, &exit_exception) == PK_OK
        && exit_exception == PK_NULL)
      {
        const char *ast_str = pk_get_debug_ast (poke_compiler);

        assert (ast_str != NULL);
        pk_printf ("%s", ast_str);
      }
    pk_set_debug_p (poke_compiler, 0);
  }

  return 1;
}

struct pk_trie *compiler_trie;

const struct pk_cmd compiler_ast_cmd =
  {"ast", "s", "", 0, NULL, NULL, pk_cmd_compiler_ast,
   ".compiler ast EXPR", poke_completion_function};

extern struct pk_cmd null_cmd; /* pk-cmd.c  */

const struct pk_cmd *compiler_cmds[] =
  {
    &compiler_ast_cmd,
    &null_cmd
  };

static char *
compiler_completion_function (const char *x, int state)
{
  return pk_cmd_completion_function (compiler_cmds, x, state);
}

const struct pk_cmd compiler_cmd =
  {"compiler", "", "", 0, compiler_cmds, &compiler_trie, NULL, ".compiler (ast)",
   compiler_completion_function};
