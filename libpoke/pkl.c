/* pkl.c - Poke compiler.  */

/* Copyright (C) 2019, 2020, 2021, 2022, 2023, 2024, 2025 Jose E.
 * Marchesi */

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

#include <gettext.h>
#define _(str) gettext (str)
#include <stdarg.h>
#include <stdio.h> /* For fopen, etc */
#include <stdlib.h>
#include <string.h>

#include "basename-lgpl.h"

#include "pkt.h"
#include "pk-utils.h"

#include "pkl.h"
#include "pvm-val.h"

#include "pkl-ast.h"
#include "pkl-parser.h"
#include "pkl-pass.h"
#include "pkl-env.h"
#include "pkl-asm.h"

#include "configmake.h"

#define PKL_COMPILING_EXPRESSION 0
#define PKL_COMPILING_PROGRAM    1
#define PKL_COMPILING_STATEMENT  2


/* The `pkl_compiler' struct holds the compiler state.

   LEXICAL_CUCKOLDING_P is 1 if alien tokens are to be recognized.

   ALIEN_TOKEN_FN is the user-provided handler for alien tokens.  This
   field is NULL if the user didn't register a handler.

   ALIEN_DTOKEN_FN is the user-provided handler for delimited alien
   tokens.  This field is NULL if the user didn't register a
   handler.  */

struct pkl_compiler
{
  pkl_env env;  /* Compiler environment.  */
  pvm vm;
  int bootstrapped;
  int compiling;
  int error_on_warning;
  int quiet_p;
  int debug_p;
  char *last_ast_str;
#define PKL_MODULES_STEP 8
  int lexical_cuckolding_p;
  pkl_alien_token_handler_fn alien_token_fn;
  pkl_alien_dtoken_handler_fn alien_dtoken_fn;
};


static int
pkl_load_rt (pkl_compiler compiler, char *poke_rt_pk)
{
  pvm_val exit_exception;

  if (!pkl_execute_file (compiler, poke_rt_pk, &exit_exception)
      || exit_exception != PVM_NULL)
    {
      free (poke_rt_pk);

      pk_term_class ("error");
      pk_puts ("internal error: ");
      pk_term_end_class ("error");
      pk_puts ("compiler failed to bootstrap itself\n");

      pkl_free (compiler);
      return 0;
    }
  free (poke_rt_pk);
  return 1;
}

pkl_compiler
pkl_new (pvm vm, const char *rt_path,
         const char *config_path, uint32_t flags)
{
  pkl_compiler compiler
    = calloc (1, sizeof (struct pkl_compiler));
  pkl_env env;

  if (!compiler)
    goto out_of_memory;

  /* Create the top-level compile-time environment.  This will be used
     for as long as the incremental compiler lives.  */
   env = pkl_env_new ();
   if (!env)
     goto out_of_memory;
   compiler->env = env;

  /* Set the poke virtual machine that the compiler will be generating
     code for.  */
  compiler->vm = vm;

  /* Be verbose by default :) */
  compiler->quiet_p = 0;

  /* No debug by default.  */
  compiler->debug_p = 0;
  compiler->last_ast_str = NULL;

  /* Bootstrap the compiler.  An error bootstraping is an internal
     error and should be reported as such.  */
  {
    char *poke_rt_pk = pk_str_concat (rt_path, "/pkl-rt.pk", NULL);
    if (!poke_rt_pk)
      goto out_of_memory;

    if (!pkl_load_rt (compiler, poke_rt_pk))
      return NULL;

    compiler->bootstrapped = 1;
  }

#define LOAD_STD(PATH,NAME)                                     \
  do                                                            \
    {                                                           \
      pvm_val exit_exception;                                   \
      char *path = pk_str_concat ((PATH), "/" NAME, NULL);      \
      if (!path)                                                \
        goto out_of_memory;                                     \
                                                                \
      if (!pkl_execute_file (compiler, path, &exit_exception)   \
          || exit_exception != PVM_NULL)                        \
        {                                                       \
          free (path);                                          \
          pkl_free (compiler);                                  \
          return NULL;                                          \
        }                                                       \
                                                                \
      free (path);                                              \
    }                                                           \
  while (0)

  /* Load the build configuration of libpoke.  */
  LOAD_STD (config_path, "pkl-config.pk");

  /* Load the standard library.  Note that the standard types may not
     be loaded, depending on how the compiler is configured.  */
  LOAD_STD (rt_path, "std.pk");
  if (!(flags & PKL_F_NOSTDTYPES))
    LOAD_STD (rt_path, "std-types.pk");

  return compiler;

out_of_memory:
  if (compiler)
    pkl_free (compiler);

  pk_term_class ("error");
  pk_puts ("error: ");
  pk_term_end_class ("error");
  pk_puts ("out of memory\n");

  return NULL;
}

void
pkl_free (pkl_compiler compiler)
{
  pkl_env_free (compiler->env);
  free (compiler->last_ast_str);
  free (compiler);
}

extern struct pkl_phase pkl_phase_anal1;
extern struct pkl_phase pkl_phase_anal2;
extern struct pkl_phase pkl_phase_analf;
extern struct pkl_phase pkl_phase_trans1;
extern struct pkl_phase pkl_phase_trans2;
extern struct pkl_phase pkl_phase_trans3;
extern struct pkl_phase pkl_phase_transf;
extern struct pkl_phase pkl_phase_transl;
extern struct pkl_phase pkl_phase_typify1;
extern struct pkl_phase pkl_phase_typify2;
extern struct pkl_phase pkl_phase_promo;
extern struct pkl_phase pkl_phase_fold;
extern struct pkl_phase pkl_phase_gen;

static pvm_program
rest_of_compilation (pkl_compiler compiler,
                     pkl_ast ast,
                     pkl_env env)
{
  pvm_program program;

#define PKL_PHASE(NAME) &pkl_phase_##NAME,
#define PKL_PASS(FLAGS, LEVEL)                                          \
  {                                                                     \
    int flags = (FLAGS);                                                \
    int level = (LEVEL);                                                \
                                                                        \
    struct pkl_phase *phases[] = {
#define PKL_END_PASS                                            \
                                   NULL, };                     \
                                                                \
    if (!pkl_do_pass (compiler, env, ast, phases, flags, level))\
      goto error;                                               \
  }
#include "pkl-passes.def"

  if (compiler->debug_p)
    {
      free (compiler->last_ast_str);
      compiler->last_ast_str = pkl_ast_format (ast->ast);
    }

  program = (pvm_program) ast->payload;
  pkl_ast_free (ast);
  return program;

 error:
  pkl_ast_free (ast);
  return NULL;
}

int
pkl_execute_buffer (pkl_compiler compiler,
                    const char *buffer,
                    const char *source,
                    uint32_t line,
                    uint32_t column,
                    const char **end,
                    pvm_val *exit_exception)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;
  pvm_val val;

  env = pkl_env_dup_toplevel (compiler->env);
  if (!env)
    goto error;
  compiler->compiling = PKL_COMPILING_PROGRAM;

  /* Parse the input routine into an AST.  */
  ret = pkl_parse_buffer (compiler, &env, &ast,
                          PKL_PARSE_PROGRAM, buffer,
                          source, line, column,
                          end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    goto error;

  program = rest_of_compilation (compiler, ast, env);
  if (program == NULL)
    goto error;

  //  pvm_disassemble_program (program);
  pvm_program_make_executable (program);

  /* Execute the program in the poke vm.  Note the return value is
     discarded.  */
  pvm_run (compiler->vm, program, &val, exit_exception);
  pvm_destroy_program (program);

  if (*exit_exception == PVM_NULL)
    {
      pkl_env_free (compiler->env);
      compiler->env = env;
      pkl_env_commit_renames (compiler->env);
    }
  else
    pkl_env_rollback_renames (env);
  return 1;

 error:
  pkl_env_free (env);
  return 0;
}

pvm_program
pkl_compile_statement (pkl_compiler compiler,
                       const char *buffer, const char **end)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;

   env = pkl_env_dup_toplevel (compiler->env);
   if (!env)
     goto error;
   compiler->compiling = PKL_COMPILING_STATEMENT;

   /* Parse the input program into an AST.  */
   ret = pkl_parse_buffer (compiler, &env, &ast,
                           PKL_PARSE_STATEMENT,
                           buffer,
                           NULL /* source */,
                           1 /* line */,
                           1 /* column */,
                           end);
   if (ret == 1)
     /* Parse error.  */
     goto error;
   else if (ret == 2)
     /* Memory exhaustion.  */
     goto error;

   program = rest_of_compilation (compiler, ast, env);
   if (program == NULL)
     goto error;

   pkl_env_free (compiler->env);
   compiler->env = env;
   pkl_env_commit_renames (compiler->env);
   pvm_program_make_executable (program);

   return program;

 error:
   pkl_env_free (env);
   return NULL;
}

int
pkl_execute_statement (pkl_compiler compiler,
                       const char *buffer,
                       const char *source,
                       uint32_t line,
                       uint32_t column,
                       const char **end,
                       pvm_val *val, pvm_val *exit_exception)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;

  env = pkl_env_dup_toplevel (compiler->env);
  if (!env)
    goto error;
  compiler->compiling = PKL_COMPILING_STATEMENT;

  /* Parse the input routine into an AST.  */
  ret = pkl_parse_buffer (compiler, &env, &ast,
                          PKL_PARSE_STATEMENT,
                          buffer,
                          source,
                          line,
                          column,
                          end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    goto error;

  program = rest_of_compilation (compiler, ast, env);
  if (program == NULL)
    goto error;

  pvm_program_make_executable (program);

  /* Execute the routine in the poke vm.  */
  pvm_run (compiler->vm, program, val, exit_exception);
  pvm_destroy_program (program);

  if (*exit_exception == PVM_NULL)
    {
      pkl_env_free (compiler->env);
      compiler->env = env;
      pkl_env_commit_renames (compiler->env);
    }
  else
    pkl_env_rollback_renames (env);
  return 1;

 error:
  pkl_env_free (env);
  return 0;
}

pvm_program
pkl_compile_expression (pkl_compiler compiler,
                        const char *buffer, const char **end)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;

   env = pkl_env_dup_toplevel (compiler->env);
   if (!env)
     goto error;
   compiler->compiling = PKL_COMPILING_EXPRESSION;

   /* Parse the input program into an AST.  */
   ret = pkl_parse_buffer (compiler, &env, &ast,
                           PKL_PARSE_EXPRESSION,
                           buffer,
                           NULL /* source */,
                           1 /* line */,
                           1 /* column */,
                           end);
   if (ret == 1)
     /* Parse error.  */
     goto error;
   else if (ret == 2)
     /* Memory exhaustion.  */
     goto error;

   program = rest_of_compilation (compiler, ast, env);
   if (program == NULL)
     goto error;

   pkl_env_free (compiler->env);
   compiler->env = env;
   pkl_env_commit_renames (compiler->env);
   pvm_program_make_executable (program);

   return program;

 error:
   pkl_env_free (env);
   return NULL;
}

int
pkl_execute_expression (pkl_compiler compiler,
                        const char *buffer,
                        const char *source,
                        uint32_t line,
                        uint32_t column,
                        const char **end,
                        pvm_val *val, pvm_val *exit_exception)
{
  pkl_ast ast = NULL;
  pvm_program program;
  int ret;
  pkl_env env = NULL;

  env = pkl_env_dup_toplevel (compiler->env);
  if (!env)
    goto error;
  compiler->compiling = PKL_COMPILING_EXPRESSION;

  /* Parse the input routine into an AST.  */
  ret = pkl_parse_buffer (compiler, &env, &ast,
                          PKL_PARSE_EXPRESSION,
                          buffer,
                          source,
                          line,
                          column,
                          end);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    goto error;

  program = rest_of_compilation (compiler, ast, env);
  if (program == NULL)
    goto error;

  pvm_program_make_executable (program);

  /* Execute the routine in the poke vm.  */
  pvm_run (compiler->vm, program, val, exit_exception);
  pvm_destroy_program (program);

  if (*exit_exception == PVM_NULL)
    {
      pkl_env_free (compiler->env);
      compiler->env = env;
      pkl_env_commit_renames (compiler->env);
    }
  else
    pkl_env_rollback_renames (env);
  return 1;

 error:
  pkl_env_free (env);
  return 0;
}

int
pkl_execute_file (pkl_compiler compiler, const char *fname,
                  pvm_val *exit_exception)
{
  int ret;
  pkl_ast ast = NULL;
  pvm_program program;
  FILE *fp;
  pkl_env env = NULL;
  pvm_val val;

  compiler->compiling = PKL_COMPILING_PROGRAM;

  fp = fopen (fname, "rb");
  if (!fp)
    {
      perror (fname);
      return 0;
    }

  env = pkl_env_dup_toplevel (compiler->env);
  if (!env)
    goto error;
  ret = pkl_parse_file (compiler, &env,  &ast, fp, fname);
  if (ret == 1)
    /* Parse error.  */
    goto error;
  else if (ret == 2)
    /* Memory exhaustion.  */
    goto error;

  program = rest_of_compilation (compiler, ast, env);
  if (program == NULL)
    goto error;

  pvm_program_make_executable (program);
  fclose (fp);

  /* Execute the program in the poke vm.  */
  pvm_run (compiler->vm, program, &val, exit_exception);
  pvm_destroy_program (program);

  if (*exit_exception == PVM_NULL)
    {
      pkl_env_free (compiler->env);
      compiler->env = env;
      pkl_env_commit_renames (compiler->env);
    }
  else
    pkl_env_rollback_renames (env);
  return 1;

 error:
  fclose (fp);
  pkl_env_free (env);
  return 0;
}

pkl_env
pkl_get_env (pkl_compiler compiler)
{
  return compiler->env;
}

int
pkl_bootstrapped_p (pkl_compiler compiler)
{
  return compiler->bootstrapped;
}

int
pkl_compiling_expression_p (pkl_compiler compiler)
{
  return compiler->compiling == PKL_COMPILING_EXPRESSION;
}

int
pkl_compiling_statement_p (pkl_compiler compiler)
{
  return compiler->compiling == PKL_COMPILING_STATEMENT;
}

int
pkl_error_on_warning (pkl_compiler compiler)
{
  return compiler->error_on_warning;
}

void
pkl_set_error_on_warning (pkl_compiler compiler,
                          int error_on_warning)
{
  compiler->error_on_warning = error_on_warning;
}

int
pkl_quiet_p (pkl_compiler compiler)
{
  return compiler->quiet_p;
}

void
pkl_set_quiet_p (pkl_compiler compiler, int quiet_p)
{
  compiler->quiet_p = quiet_p;
}

int
pkl_debug_p (pkl_compiler compiler)
{
  return compiler->debug_p;
}

void
pkl_set_debug_p (pkl_compiler compiler, int debug_p)
{
  compiler->debug_p = debug_p;
}

char *
pkl_get_last_ast_str (pkl_compiler compiler)
{
  return compiler->last_ast_str;
}

int
pkl_lexical_cuckolding_p (pkl_compiler compiler)
{
  return compiler->lexical_cuckolding_p;
}

void
pkl_set_lexical_cuckolding_p (pkl_compiler compiler,
                              int lexical_cuckolding_p)
{
  compiler->lexical_cuckolding_p = lexical_cuckolding_p;
}

pkl_alien_token_handler_fn
pkl_alien_token_fn (pkl_compiler compiler)
{
  return compiler->alien_token_fn;
}

pkl_alien_dtoken_handler_fn
pkl_alien_dtoken_fn (pkl_compiler compiler)
{
  return compiler->alien_dtoken_fn;
}

void
pkl_set_alien_token_fn (pkl_compiler compiler,
                        pkl_alien_token_handler_fn cb)
{
  compiler->alien_token_fn = cb;
}

void
pkl_set_alien_dtoken_fn (pkl_compiler compiler,
                         pkl_alien_dtoken_handler_fn cb)
{
  compiler->alien_dtoken_fn = cb;
}

pvm_program
pkl_compile_call (pkl_compiler compiler, pvm_val cls, pvm_val *ret,
                  int narg, va_list ap)
{
  pvm_program program;
  pkl_asm pasm;
  pvm_val arg;
  int i;

  pasm = pkl_asm_new (NULL /* ast */, compiler, 1 /* prologue */);

  /* Push the arguments for the function.  */
  for (i = 0; i < narg; ++i)
    {
      arg = va_arg (ap, pvm_val);
      pkl_asm_insn (pasm, PKL_INSN_PUSH, arg);
    }

  /* Call the closure.  */
  pkl_asm_insn (pasm, PKL_INSN_PUSH, cls);
  pkl_asm_insn (pasm, PKL_INSN_CALL);

  program = pkl_asm_finish (pasm, 1 /* epilogue */);
  return program;
}

pvm
pkl_get_vm (pkl_compiler compiler)
{
  return compiler->vm;
}

char *
pkl_resolve_module (pkl_compiler compiler,
                    const char *module,
                    int filename_p)
{
  const char *load_path;
  char *full_filename = NULL;

  if (module == NULL)
    return NULL;

  /* Check if the path is absolute.  */
  if (*module == '/' && pk_file_readable (module) == NULL)
    return strdup (module);

  /* Get the load path from the run-time environment.  */
  {
    pvm_val val;
    int back, over;

    pkl_env compiler_env = pkl_get_env (compiler);
    pvm_env runtime_env = pvm_get_env (pkl_get_vm (compiler));

    if (NULL == pkl_env_lookup (compiler_env,
                                PKL_ENV_NS_MAIN,
                                "load_path",
                                &back, &over))
      return NULL;

    val = pvm_env_lookup (runtime_env, back, over);
    assert (val != PVM_NULL);

    load_path = PVM_VAL_STR (val);
  }

  /* Traverse the directories in the load path and try to load the
     requested module.  */
  {
    const char *ext = filename_p ? "" : ".pk";
    const char *s, *e;

    char *fixed_load_path = pk_str_replace (load_path, "%DATADIR%", PKGDATADIR);

    for (s = fixed_load_path, e = s; *e; s = e + 1)
      {
        /* Ignore empty entries. */
        if ((e = strchrnul (s, ':')) == s)
          continue;

#ifdef _WIN32
        /* Don't use ':' as a path separator if it's between a single
           letter and a '/', since on Windows paths starting like
           this "c:/" are just drive letters of absolute paths. */
        if (e == s + 1
            && ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z'))
            && e[1] == '/')
          e = strchrnul (e + 2, ':');
#endif

        if (asprintf (&full_filename, "%.*s/%s%s", (int)(e - s), s, module,
                      ext)
            == -1)
          break;

        if (pk_file_readable (full_filename) == NULL)
          break;

        free (full_filename);
        full_filename = NULL;
      }

    if (fixed_load_path != load_path)
      free (fixed_load_path);
  }

  return full_filename;
}

int
pkl_load (pkl_compiler compiler, const char *module, pvm_val *exit_exception)
{
  char *module_filename = pkl_resolve_module (compiler,
                                              module,
                                              0 /* filename_p */);

  if (!module_filename)
    return 0;
  return pkl_execute_file (compiler, module_filename, exit_exception);
}

static pkl_ast_node
pvm_type_to_ast_type (pkl_ast ast, pvm_val type)
{
  switch (PVM_VAL_TYP_CODE (type))
    {
    case PVM_TYPE_INTEGRAL:
      {
        size_t size = PVM_VAL_ULONG (PVM_VAL_TYP_I_SIZE (type));
        int signed_p = PVM_VAL_INT (PVM_VAL_TYP_I_SIGNED_P (type));

        return pkl_ast_make_integral_type (ast, size, signed_p);
        break;
      }
    case PVM_TYPE_STRING:
      return pkl_ast_make_string_type (ast);
      break;
    case PVM_TYPE_ARRAY:
      {
        pkl_ast_node elem_type
          =pvm_type_to_ast_type (ast, PVM_VAL_TYP_A_ETYPE (type));
        pkl_ast_node bound = NULL;  /* XXX no bound in pvm types for now.  */

        return pkl_ast_make_array_type (ast, elem_type, bound);
        break;
      }
    case PVM_TYPE_OFFSET:
      {
        pkl_ast_node base_type
          = pvm_type_to_ast_type (ast, PVM_VAL_TYP_O_BASE_TYPE (type));
        pkl_ast_node unit
          = pkl_ast_make_integer (ast, PVM_VAL_ULONG (PVM_VAL_TYP_O_UNIT (type)));
        pkl_ast_node ref_type
          = pvm_type_to_ast_type (ast, PVM_VAL_TYP_O_REF_TYPE (type));

        return pkl_ast_make_offset_type (ast, base_type, unit, ref_type);
        break;
      }
    case PVM_TYPE_VOID:
      return pkl_ast_make_void_type (ast);
      break;
    case PVM_TYPE_STRUCT:
      /* XXX writeme */
      PK_UNREACHABLE ();
      break;
    case PVM_TYPE_CLOSURE:
      /* XXX writeme */
      PK_UNREACHABLE ();
      break;
    default:
      break;
    }

  return NULL;
}

int
pkl_defvar (pkl_compiler compiler,
            const char *varname, pvm_val val)
{
  pkl_ast ast = NULL;
  pkl_ast_node name = NULL;
  pkl_ast_node initial = NULL;
  pkl_ast_node initial_type = NULL;
  pkl_ast_node decl = NULL;

  ast = pkl_ast_init ();
  if (!ast)
    goto error;

  name = pkl_ast_make_identifier (ast, varname);
  if (!name)
    goto error;

  /* The type of the initial.  */
  initial_type = pvm_type_to_ast_type (ast, pvm_typeof (val));
  if (!initial_type)
    goto error;

  /* This ast is not to be compiled.  */
  PKL_AST_TYPE_COMPILED (initial_type) = 1;
  /* Initial is a dummy, with the right type.  */
  initial = pkl_ast_make_integer (ast, 0);
  if (!initial)
    goto error;

  PKL_AST_TYPE (initial) = ASTREF (initial_type);
  decl = pkl_ast_make_decl (ast,
                            PKL_AST_DECL_KIND_VAR,
                            name,
                            initial,
                            "<libpoke>");
  if (!decl)
    goto error;

  if (!pkl_env_register (compiler->env,
                         ast,
                         PKL_ENV_NS_MAIN,
                         varname,
                         decl))
    /* A variable with the given name is already registered.  */
    goto error;

  return 1;

 error:
  pkl_ast_free (ast);
  return 0;
}

int
pkl_tracer_p (pkl_compiler compiler)
{
  pkl_env compiler_env = compiler->env;
  pvm_env runtime_env = pvm_get_env (compiler->vm);
  int back, over;
  pkl_ast_node decl = pkl_env_lookup (compiler_env,
                                      PKL_ENV_NS_MAIN,
                                      "pk_tracer_p",
                                      &back, &over);
  pvm_val val = PVM_NULL;

  if (!compiler->bootstrapped)
    /* For obvious reasons.  ^^ */
    return 0;

  assert (decl != NULL
          && PKL_AST_DECL_KIND (decl) == PKL_AST_DECL_KIND_VAR);
  val = pvm_env_lookup (runtime_env, back, over);
  assert (PVM_IS_INT (val));

  return PVM_VAL_INT (val);
}

pkl_ast_node
pkl_constant_fold (pkl_compiler compiler, pkl_ast ast, pkl_ast_node node)
{
  pkl_ast tmp_ast;
  struct pkl_phase *fold_phases[] = { &pkl_phase_fold, NULL };

  tmp_ast = pkl_ast_init ();
  tmp_ast->ast = ASTREF (node);
  tmp_ast->uid = ast->uid;
  if (!pkl_do_pass (compiler, compiler->env, tmp_ast, fold_phases, 0, 1))
    PK_UNREACHABLE ();

  return tmp_ast->ast;
}
