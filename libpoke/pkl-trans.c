/* pkl-trans.c - Transformation phases for the poke compiler.  */

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

#include <gettext.h>
#define _(str) gettext (str)
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <xalloc.h>
#include <stdlib.h>
#include <errno.h>

#include "pk-utils.h"

#include "pkl.h"
#include "pkl-diag.h"
#include "pkl-ast.h"
#include "pkl-pass.h"

/* This file implements several transformation compiler phases which,
   generally speaking, are restartable.  */

/* The trans phases keep a stack of function contexts while they
   operate on the AST.  At any time in the pass, the "current lexical
   function", and some properties relatives to it, are available to
   the trans phase handlers on the top of this stack.

   The following struct implements each entry on the function contexts
   stack.

   NODE is the AST node corresponding to the current lexical function,
   or NULL if not in a function.

   NDROPS is the number of PVM stack values we'd need to drop before
   returning from the current function.

   NPOPES is the number of PVM exception stack handlers that we'd need
   to pope before returning from the current function.

   BACK is the current lexical depth relative to the current
   function.  */

struct pkl_trans_function_ctx
{
  pkl_ast_node node;
  int ndrops;
  int npopes;
  int back;
};

/* The trans phases keep a stack of escapable entities (loops and
   try-until statements) and the corresponding `break' and `continue'
   keywords, while they operate on the AST.

   The following struct implements each entry on the breakable/continuable
   entities stack.

   NODE is the escapable entity (loop or try-until statement).

   NFRAMES is the number of frames in the body of NODE.  */

struct pkl_trans_escapable_ctx
{
  pkl_ast_node node;
  int nframes;
  int npopes;
};

/* The following struct defines the payload of the trans phases.

   ADD_FRAMES is the number of frames to add to lexical addresses.
   This is used in transl.

   FUNCTIONS is a stack of function contexts.

   NEXT_FUNCTION - 1 is the index for the enclosing function in
   FUNCTIONS.  NEXT_FUNCTION is 0 if not in a function.

   ENDIAN is a stack whose top indicates the endianness to be used
   when mapping and writing integral types.

   CUR_ENDIAN is the index to ENDIAN and marks the top of the stack of
   endianness.  Initially PKL_AST_ENDIAN_DFL.

   ESCAPABLES is a stack of escapables.

   NEXT_ESCAPABLE - 1 is the index for the enclosing escapable
   entity.  */

#define PKL_TRANS_MAX_FUNCTION_NEST 32
#define PKL_TRANS_MAX_ENDIAN 25
#define PKL_TRANS_MAX_COMP_STMT_NEST 120

struct pkl_trans_payload
{
  int add_frames;
  struct pkl_trans_function_ctx functions[PKL_TRANS_MAX_FUNCTION_NEST];
  int next_function;
  enum pkl_ast_endian endian[PKL_TRANS_MAX_ENDIAN];
  int cur_endian;
  struct pkl_trans_escapable_ctx escapables[PKL_TRANS_MAX_COMP_STMT_NEST];
  int next_escapable;
  pkl_env env;
};

typedef struct pkl_trans_payload *pkl_trans_payload;

/* Transformation phases initializer and finalizer.  */

static void *
pkl_trans_initialize (pkl_compiler compiler, pkl_env env)
{
  struct pkl_trans_payload *payload = malloc (sizeof (struct pkl_trans_payload));

  if (!payload)
    return NULL;

  memset (payload, 0, sizeof (struct pkl_trans_payload));
  payload->endian[0] = PKL_AST_ENDIAN_DFL;
  payload->env = env;
  return payload;
}

static void
pkl_trans_finalize (void *payload)
{
  free (payload);
}

/* Handling of the stack of endianness.  */

#define PKL_TRANS_PAYLOAD ((pkl_trans_payload) PKL_PASS_PAYLOAD)


#define PKL_TRANS_ENDIAN                          \
  (PKL_TRANS_PAYLOAD->endian[PKL_TRANS_PAYLOAD->cur_endian])

#define PKL_TRANS_PUSH_ENDIAN(ENDIAN)                                   \
  do                                                                    \
    {                                                                   \
      assert (PKL_TRANS_PAYLOAD->cur_endian < PKL_TRANS_MAX_ENDIAN);    \
      PKL_TRANS_PAYLOAD->endian[++PKL_TRANS_PAYLOAD->cur_endian] = (ENDIAN); \
    } while (0)

#define PKL_TRANS_POP_ENDIAN                      \
  do                                              \
    {                                             \
      assert (PKL_TRANS_PAYLOAD->cur_endian > 0); \
      PKL_TRANS_PAYLOAD->cur_endian--;            \
    }                                             \
  while (0)

/* Handling of the stack of function contexts.  */

static int
pkl_trans_in_functions (struct pkl_trans_function_ctx functions[],
                        int next_function,
                        pkl_ast_node function)
{
  int i;
  for (i = 0; i < next_function; i++)
    {
      if (functions[i].node == function)
        return 1;
    }

  return 0;
}

#define PKL_TRANS_FUNCTION_IS_RECURSIVE(function)                 \
  pkl_trans_in_functions (PKL_TRANS_PAYLOAD->functions,           \
                          PKL_TRANS_PAYLOAD->next_function,       \
                          (function))

#define PKL_TRANS_FUNCTION                                              \
  (PKL_TRANS_PAYLOAD->next_function == 0                                \
   ? NULL                                                               \
   : &PKL_TRANS_PAYLOAD->functions[PKL_TRANS_PAYLOAD->next_function - 1])

#define PKL_TRANS_PUSH_FUNCTION(func)                                   \
  do                                                                    \
    {                                                                   \
      assert (PKL_TRANS_PAYLOAD->next_function < PKL_TRANS_MAX_FUNCTION_NEST); \
      PKL_TRANS_PAYLOAD->functions[PKL_TRANS_PAYLOAD->next_function].back = 0; \
      PKL_TRANS_PAYLOAD->functions[PKL_TRANS_PAYLOAD->next_function].ndrops = 0; \
      PKL_TRANS_PAYLOAD->functions[PKL_TRANS_PAYLOAD->next_function].npopes = 0; \
      PKL_TRANS_PAYLOAD->functions[PKL_TRANS_PAYLOAD->next_function++].node \
        = (func);                                                       \
    }                                                                   \
  while (0)

#define PKL_TRANS_POP_FUNCTION                                  \
  do                                                            \
    {                                                           \
      assert (PKL_TRANS_PAYLOAD->next_function > 0);            \
      PKL_TRANS_PAYLOAD->next_function -= 1;                    \
    }                                                           \
  while (0)

/* Handling of the stack of escapable constructions.  */

#define PKL_TRANS_ESCAPABLE                                     \
  (PKL_TRANS_PAYLOAD->next_escapable == 0                       \
   ? NULL                                                       \
   : &PKL_TRANS_PAYLOAD->escapables[PKL_TRANS_PAYLOAD->next_escapable - 1])

#define PKL_TRANS_PUSH_UNESCAPABLE                                      \
  do                                                                    \
    {                                                                   \
      assert (PKL_TRANS_PAYLOAD->next_escapable < PKL_TRANS_MAX_COMP_STMT_NEST); \
      PKL_TRANS_PAYLOAD->escapables[PKL_TRANS_PAYLOAD->next_escapable].nframes \
      = 0;                                                              \
      PKL_TRANS_PAYLOAD->escapables[PKL_TRANS_PAYLOAD->next_escapable].npopes \
      = 0;                                                              \
      PKL_TRANS_PAYLOAD->escapables[PKL_TRANS_PAYLOAD->next_escapable++].node \
      = NULL;                                                           \
    }                                                                   \
  while (0)

#define PKL_TRANS_PUSH_ESCAPABLE(b)                             \
  do                                                            \
    {                                                           \
      assert (PKL_TRANS_PAYLOAD->next_escapable < PKL_TRANS_MAX_COMP_STMT_NEST);\
      PKL_TRANS_PAYLOAD->escapables[PKL_TRANS_PAYLOAD->next_escapable].nframes  \
        = 0;                                                    \
      PKL_TRANS_PAYLOAD->escapables[PKL_TRANS_PAYLOAD->next_escapable].npopes  \
        = 0;                                                    \
      PKL_TRANS_PAYLOAD->escapables[PKL_TRANS_PAYLOAD->next_escapable++].node \
        = (b);                                                  \
    }                                                           \
  while (0)

#define PKL_TRANS_POP_ESCAPABLE                                 \
  do                                                            \
    {                                                           \
      assert (PKL_TRANS_PAYLOAD->next_escapable > 0);           \
      PKL_TRANS_PAYLOAD->next_escapable--;                      \
    }                                                           \
  while (0)

/* The following handler is used in all trans phases and initializes
   the phase payload.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans_pr_program)
{
  PKL_TRANS_PAYLOAD->add_frames = -1;
}
PKL_PHASE_END_HANDLER

/* The following handler is used in all trans phases, and handles
   changing the source file for diagnostics.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans_ps_src)
{
  pkl_ast_node src = PKL_PASS_NODE;
  char *filename = PKL_AST_SRC_FILENAME (src);

  free (PKL_PASS_AST->filename);
  PKL_PASS_AST->filename = filename ? strdup (filename) : NULL;
}
PKL_PHASE_END_HANDLER



/* Compute and set the number of elements in a STRUCT node.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_struct)
{
  pkl_ast_node astruct = PKL_PASS_NODE;
  pkl_ast_node t;
  size_t nelem = 0;

  for (t = PKL_AST_STRUCT_FIELDS (astruct); t; t = PKL_AST_CHAIN (t))
    nelem++;

  PKL_AST_STRUCT_NELEM (astruct) = nelem;
}
PKL_PHASE_END_HANDLER

/* Array types conform an unescapable context.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_type_array)
{
  PKL_TRANS_PUSH_UNESCAPABLE;
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_type_array)
{
  PKL_TRANS_POP_ESCAPABLE;
}
PKL_PHASE_END_HANDLER

/* Struct types start an unescapable context.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_type_struct)
{
  PKL_TRANS_PUSH_UNESCAPABLE;
}
PKL_PHASE_END_HANDLER

/* Compute and set the number of elements, fields and declarations in
   a struct TYPE node.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_type_struct)
{
  pkl_ast_node struct_type = PKL_PASS_NODE;
  pkl_ast_node t;
  size_t nelem = 0, nfield = 0, ncfield = 0, ndecl = 0;

  for (t = PKL_AST_TYPE_S_ELEMS (struct_type); t;
       t = PKL_AST_CHAIN (t))
    {
      nelem++;
      if (PKL_AST_CODE (t) == PKL_AST_STRUCT_TYPE_FIELD)
        {
          if (PKL_AST_STRUCT_TYPE_FIELD_COMPUTED_P (t))
            ncfield++;
          else
            nfield++;
        }
      else
        ndecl++;
    }

  PKL_AST_TYPE_S_NELEM (struct_type) = nelem;
  PKL_AST_TYPE_S_NFIELD (struct_type) = nfield;
  PKL_AST_TYPE_S_NCFIELD (struct_type) = ncfield;
  PKL_AST_TYPE_S_NDECL (struct_type) = ndecl;

  PKL_TRANS_POP_ESCAPABLE;
}
PKL_PHASE_END_HANDLER

/* If the magnitude of an offset is not specified then it defaults to
   1. */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_offset)
{
  pkl_ast_node offset = PKL_PASS_NODE;

  if (PKL_AST_OFFSET_MAGNITUDE (offset) == NULL)
    {
      pkl_ast_node magnitude_type
        = pkl_ast_make_integral_type (PKL_PASS_AST, 32, 1);
      pkl_ast_node magnitude
        = pkl_ast_make_integer (PKL_PASS_AST, 1);

      PKL_AST_TYPE (magnitude) = ASTREF (magnitude_type);
      PKL_AST_OFFSET_MAGNITUDE (offset) = ASTREF (magnitude);
    }
}
PKL_PHASE_END_HANDLER

/* Calculate the number of arguments in funcalls.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_funcall)
{
  pkl_ast_node arg;
  int nargs = 0;

  for (arg = PKL_AST_FUNCALL_ARGS (PKL_PASS_NODE);
       arg;
       arg = PKL_AST_CHAIN (arg))
    nargs++;

  PKL_AST_FUNCALL_NARG (PKL_PASS_NODE) = nargs;
}
PKL_PHASE_END_HANDLER

/* Annotate whether declaration nodes are in the body of a struct
   type, as this implies treating them especially somewhere else.

   In declaration of functions, annotate the name of the declaration
   in the function as the name it was declared with.  This has to be
   done in pre-order because other phases for functions require the
   name.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_decl)
{
  pkl_ast_node decl = PKL_PASS_NODE;

  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_TYPE
      && PKL_AST_TYPE_CODE (PKL_PASS_PARENT) == PKL_TYPE_STRUCT)
    /* Annotate this declaration to be in a struct type.  */
    PKL_AST_DECL_IN_STRUCT_P (decl) = 1;

  if (PKL_AST_DECL_KIND (decl) == PKL_AST_DECL_KIND_FUNC)
    {
      pkl_ast_node name = PKL_AST_DECL_NAME (decl);
      pkl_ast_node function = PKL_AST_DECL_INITIAL (decl);

      PKL_AST_FUNC_NAME (function)
        = xstrdup (PKL_AST_IDENTIFIER_POINTER (name));
    }
}
PKL_PHASE_END_HANDLER

/* Variables that refer to the current function (recursive calls)
   should be marked as such, so `pkl_ast_node_free' knows to not free
   the declaration.  This is to avoid loops in the AST reference
   counting.

   Variables are annotated with the enclosing function, and with their
   lexical nesting level with respect the beginning of the enclosing
   function.

   Variables that refer to parameterless functions are transformed
   into funcalls to these functions, but only if the variables are not
   part of funcall themselves! :)

   Annotate variables that are immediately indexed by a [] operator.
   This is used for certain optimizations in `gen'.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_var)
{
  pkl_ast_node var = PKL_PASS_NODE;
  pkl_ast_node decl = PKL_AST_VAR_DECL (var);

  if (PKL_TRANS_FUNCTION)
    PKL_AST_VAR_FUNCTION (var) = PKL_TRANS_FUNCTION->node;

  if (PKL_AST_DECL_KIND (decl) == PKL_AST_DECL_KIND_FUNC)
    PKL_AST_VAR_IS_RECURSIVE (var)
      = PKL_TRANS_FUNCTION_IS_RECURSIVE (PKL_AST_DECL_INITIAL (decl));

  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) != PKL_AST_FUNCALL)
    {
      pkl_ast_node initial = PKL_AST_DECL_INITIAL (decl);
      pkl_ast_node initial_type = PKL_AST_TYPE (initial);

      if (PKL_AST_TYPE_CODE (initial_type) == PKL_TYPE_FUNCTION
          && !PKL_AST_VAR_IS_PARENTHESIZED (var)
          && (PKL_AST_TYPE_F_NARG (initial_type) == 0
              || pkl_ast_func_all_optargs (initial_type)))
        {
          pkl_ast_node funcall = pkl_ast_make_funcall (PKL_PASS_AST,
                                                       ASTDEREF (var),
                                                       NULL /* args */);

          PKL_AST_LOC (funcall) = PKL_AST_LOC (var);
          PKL_PASS_NODE = ASTREF (funcall);
          PKL_PASS_RESTART = 1;
        }
    }

  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_INDEXER)
    PKL_AST_VAR_IS_INDEXED (var) = 1;
}
PKL_PHASE_END_HANDLER

/* Finish strings, by expanding \-sequences, and emit errors if an
   invalid \-sequence is found.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_string)
{
  pkl_ast_node string = PKL_PASS_NODE;
  char *string_pointer = PKL_AST_STRING_POINTER (string);
  char *new_string_pointer;
  char *p;
  size_t string_length, i;
  bool found_backslash = false;

#define ISODIGIT(c) ((unsigned)(c) - '0' < 8) /* is octal digit */
#define XDIGIT(x) \
  ((unsigned)(x) - '0' < 10 ? (x) - '0' : ((x) | 0x20) - 'a' + 10)

  /* Please keep this code in sync with the string printer in
     pvm-val.c:pvm_print_val.  */

  /* First pass: calculate the size of the resulting string after
     \-expansion, and report errors in the contents of the string.  */
  for (p = string_pointer, string_length = 0; *p != '\0'; ++p)
    {
      string_length++;
      if (*p != '\\')
        continue;

      found_backslash = true;
      ++p;

      if (ISODIGIT (p[0]))
        {
          if (ISODIGIT (p[1]))
            p += ISODIGIT (p[2]) ? 2 : 1;
          continue;
        }

      switch (*p)
        {
        case '\\':
        case 'n':
        case 't':
        case '"':
          break;
        case '\n':
          string_length--;
          break;
        case 'x':
          ++p;
          if (!isxdigit (p[0]))
            {
              PKL_ERROR (PKL_AST_LOC (string),
                         _ ("\\x used with no following hex digits"));
              PKL_PASS_ERROR;
            }
          if (isxdigit (p[1]))
            ++p;
          break;
        default:
          PKL_ERROR (PKL_AST_LOC (string),
                     _ ("invalid \\%c sequence in string"), *p);
          PKL_PASS_ERROR;
        }
    }

  if (!found_backslash)
    goto _exit;

  /* Second pass: compose the new string.  */
  new_string_pointer = xmalloc (string_length + 1);

  for (p = string_pointer, i = 0; *p != '\0'; ++p, ++i)
    {
      if (*p != '\\')
        {
          new_string_pointer[i] = *p;
          continue;
        }
      ++p;
      if (*p == '\n') {
        --i;
        continue;
      }


      /* octal escape sequence */
      if (ISODIGIT (p[0]))
        {
          unsigned int num = p[0] - '0';

          if (ISODIGIT (p[1]))
            {
              ++p;
              num = (num << 3) | (p[0] - '0');
              if (ISODIGIT (p[1]))
                {
                  ++p;
                  num = (num << 3) | (p[0] - '0');
                }
            }
          if (num == '\0')
            {
              PKL_ERROR (PKL_AST_LOC (string),
                         _ ("string literal cannot contain NULL character"));
              PKL_PASS_ERROR;
            }
          else if (num > 255)
            {
              PKL_ERROR (PKL_AST_LOC (string),
                         _ ("octal escape sequence out of range"));
              PKL_PASS_ERROR;
            }
          new_string_pointer[i] = num;
          continue;
        }

      switch (*p)
        {
        case '\\': new_string_pointer[i] = '\\'; break;
        case 'n': new_string_pointer[i] = '\n'; break;
        case 't': new_string_pointer[i] = '\t'; break;
        case '"': new_string_pointer[i] = '"'; break;
        case 'x':
          ++p;
          new_string_pointer[i] = XDIGIT (p[0]);
          if (isxdigit (p[1]))
            {
              new_string_pointer[i] = (XDIGIT (p[0]) << 4) | XDIGIT (p[1]);
              ++p;
            }
          if (new_string_pointer[i] == '\0')
            {
              PKL_ERROR (PKL_AST_LOC (string),
                         _ ("string literal cannot contain NULL character"));
              PKL_PASS_ERROR;
            }
          break;
        default:
          PK_UNREACHABLE ();
        }
    }
  new_string_pointer[i] = '\0';

#undef ISODIGIT
#undef XDIGIT

  free (string_pointer);
  PKL_AST_STRING_POINTER (string) = new_string_pointer;
}
PKL_PHASE_END_HANDLER

/* At this point the second operand of attribute operations is an
   identifier with the name of the attribute (.i.e. "length" for
   foo'length).  Determine the corresponding attribute code, emitting
   an error if the given attribute name is not defined.  Finally, turn
   the binary or ternary expression into an either unary or binary
   expression, depending whether the attribute gets an argument or it
   doesn't.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_op_attr)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node identifier;
  const char *identifier_name;
  enum pkl_ast_attr attr = PKL_AST_ATTR_NONE;

  if (PKL_AST_EXP_ATTR (exp) != PKL_AST_ATTR_NONE)
    PKL_PASS_DONE;

  identifier = PKL_AST_EXP_OPERAND (exp, 1);
  identifier_name = PKL_AST_IDENTIFIER_POINTER (identifier);

  for (attr = 0; pkl_attr_name (attr); ++attr)
    {
      if (STREQ (pkl_attr_name (attr), identifier_name))
        break;
    }

  if (attr == PKL_AST_ATTR_NONE)
    {
      PKL_ERROR (PKL_AST_LOC (identifier),
                 "invalid attribute '%s", identifier_name);
      PKL_PASS_ERROR;
    }

  PKL_AST_EXP_ATTR (exp) = attr;

  /* Turn the expression into an unary or binary expression.  */
  if (PKL_AST_EXP_NUMOPS (exp) == 2)
    {
      PKL_AST_EXP_NUMOPS (exp) = 1;
      pkl_ast_node_free (PKL_AST_EXP_OPERAND (exp, 1));
    }
  else
    {
      PKL_AST_EXP_NUMOPS (exp) = 2;
      pkl_ast_node_free (PKL_AST_EXP_OPERAND (exp, 1));
      PKL_AST_EXP_OPERAND (exp, 1) = PKL_AST_EXP_OPERAND (exp, 2);
    }
}
PKL_PHASE_END_HANDLER

/* Push the function in the stack of function contexts.  Function
   bodies also start an unescapable context.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_func)
{
  PKL_TRANS_PUSH_FUNCTION (PKL_PASS_NODE);
  PKL_TRANS_PUSH_UNESCAPABLE;
}
PKL_PHASE_END_HANDLER

/* Annotate the function's first optional argument as such, and count
   the number of formal arguments the function gets.

   Also, pop the function from the stack of function contexts.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_func)
{
  pkl_ast_node func = PKL_PASS_NODE;
  pkl_ast_node func_args = PKL_AST_FUNC_ARGS (func);
  pkl_ast_node fa;
  int nargs;

  /* Count the number of formal arguments.   */
  for (fa = func_args, nargs = 0; fa; fa = PKL_AST_CHAIN (fa))
    nargs++;
  PKL_AST_FUNC_NARGS (func) = nargs;

  /* Find the first optional formal argument, if any, and set
     first_opt_arg accordingly.  */
  for (fa = func_args; fa; fa = PKL_AST_CHAIN (fa))
    {
      if (PKL_AST_FUNC_ARG_INITIAL (fa))
        {
          PKL_AST_FUNC_FIRST_OPT_ARG (func) = ASTREF (fa);
          break;
        }
    }

  /* Remove this function from stack of functions.  */
  PKL_TRANS_POP_FUNCTION;
  /* Pop the unescapable context.  */
  PKL_TRANS_POP_ESCAPABLE;
}
PKL_PHASE_END_HANDLER

/* Determine the number of formal arguments taken by functions of this
   type and annotate it in the function type.

   Determine the first optional formal argument and annotate it in the
   function type.

   Determine whether the function gets a vararg and annotate it in the
   function type.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_type_function)
{
  pkl_ast_node function_type = PKL_PASS_NODE;
  pkl_ast_node function_type_args = PKL_AST_TYPE_F_ARGS (function_type);

  pkl_ast_node arg;
  size_t nargs = 0;

  /* Count the number of formal arguments taken by functions of this
     type.  */
  for (arg = function_type_args;  arg; arg = PKL_AST_CHAIN (arg))
    nargs++;
  PKL_AST_TYPE_F_NARG (function_type) = nargs;

  /* Find the first optional formal argument, if any, and set
     first_op_arg accordingly.  */
  for (arg = function_type_args; arg; arg = PKL_AST_CHAIN (arg))
    {
      if (PKL_AST_FUNC_TYPE_ARG_OPTIONAL (arg))
        {
          PKL_AST_TYPE_F_FIRST_OPT_ARG (function_type)
            = ASTREF (arg);
          break;
        }
    }

  /* Determine whether the function type gets a vararg.  */
  for (arg = PKL_AST_TYPE_F_ARGS (function_type);
       arg;
       arg = PKL_AST_CHAIN (arg))
    {
      if (PKL_AST_FUNC_TYPE_ARG_VARARG (arg))
        {
          PKL_AST_TYPE_F_VARARG (function_type) = 1;
          break;
        }
    }
}
PKL_PHASE_END_HANDLER

/* Complete trimmers lacking some of their indexes.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_trimmer)
{
  pkl_ast_node trimmer = PKL_PASS_NODE;
  pkl_ast_node entity = PKL_AST_TRIMMER_ENTITY (trimmer);
  pkl_ast_node from = PKL_AST_TRIMMER_FROM (trimmer);
  pkl_ast_node to = PKL_AST_TRIMMER_TO (trimmer);
  pkl_ast_node addend = PKL_AST_TRIMMER_ADDEND (trimmer);

  /* If the FROM index of a trimmer isn't specified, it defaults to
     0UL.  */
  if (!from)
    {
      pkl_ast_node idx_type
        = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);

      from = pkl_ast_make_integer (PKL_PASS_AST, 0);
      PKL_AST_TYPE (from) = ASTREF (idx_type);
      PKL_AST_TRIMMER_FROM (trimmer) = ASTREF (from);
    }

  if (addend)
    {
      /* If an ADDEND is specified, we set `TO' to an expression that
         evaluates to FROM + ADDEND.  */
      pkl_ast_node plus_exp
        = pkl_ast_make_binary_exp (PKL_PASS_AST,
                                   PKL_AST_OP_ADD,
                                   from, addend);

      PKL_AST_TRIMMER_TO (trimmer) = ASTREF (plus_exp);
      PKL_PASS_RESTART = 1;
    }
  else if (!to)
    {
      /* If the TO index of a trimmer isn't specified, it defaults to
         an expression that evaluates to the size of the
         container.  */
      pkl_ast_node length_op = pkl_ast_make_unary_exp (PKL_PASS_AST,
                                                       PKL_AST_OP_ATTR,
                                                       entity);

      PKL_AST_EXP_ATTR (length_op) = PKL_AST_ATTR_LENGTH;
      PKL_AST_TRIMMER_TO (trimmer) = ASTREF (length_op);
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

/* Decode format strings in `format' and `printf'.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_format)
{
  pkl_ast_node format = PKL_PASS_NODE;
  pkl_ast_node args = PKL_AST_FORMAT_ARGS (format);
  pkl_ast_node format_fmt = PKL_AST_FORMAT_FMT (format);
  char *fmt, *p;
  pkl_ast_node t, arg;
  int ntag, nargs = PKL_AST_FORMAT_NARGS (format);
  pkl_ast_node types = NULL, prev_arg = NULL;
  const char *msg = NULL;
  /* XXX this hard limit should go away.  */
#define MAX_CLASS_TAGS 32
  int nclasses = 0;
  char *classes[MAX_CLASS_TAGS];
  int add_new_percent_arg_p = 0;
  int add_new_style_arg_p = 0;
  char *new_style_class = NULL;

  /* Calculate the number of arguments.  */
  for (t = args; t; t = PKL_AST_CHAIN (t))
    nargs++;
  PKL_AST_FORMAT_NARGS (format) = nargs;

  /* If the format string has been already processed, then we are done.  */
  if (PKL_AST_FORMAT_FMT_PROCESSED_P (format))
    PKL_PASS_DONE;

  fmt = PKL_AST_STRING_POINTER (format_fmt);
  p = fmt;

  /* Process the prefix string, if any.  */
  if (*p != '%')
    {
      p = strchrnul (fmt, '%');
      PKL_AST_FORMAT_PREFIX (format) = strndup (fmt, p - fmt);
      if (!PKL_AST_FORMAT_PREFIX (format))
        PKL_ICE (PKL_AST_LOC (format), _("out of memory"));
    }

  /* Process the format string.  */
  prev_arg = NULL;
  for (types = NULL, ntag = 0, arg = args, nclasses = 0;
       *p != '\0';
       prev_arg = arg, arg = PKL_AST_CHAIN (arg))
    {
      pkl_ast_node atype;
      char flag = 0;
      int prefix = -1;

      assert (*p == '%');
      if (ntag >= nargs && p[1] != '%' && p[1] != '>' && p[1] != '<')
        {
          PKL_ERROR (PKL_AST_LOC (format), "not enough format arguments");
          PKL_PASS_ERROR;
        }

      /* Process the optional numerical prefix.  */
      if (p[1] >= '0' && p[1] <= '9')
        {
          prefix = p[1] - '0';
          p++;
        }

      /* Process an optional flag (uppercase letter.)  */
      if (p[1] >= 'A' && p[1] <= 'Z')
        {
          flag = p[1];
          p++;
        }

      /* Make sure this tag supports the given numerical prefix and
         tag.  */
      if ((flag != 0 || prefix != -1)
          && p[1] != 'v')
        {
          if (flag != 0)
            msg = _("invalid flag");
          else
            msg = _("invalid numerical prefix");
          goto invalid_tag;
        }

      /* Now process the rest of the tag.  */
      switch (p[1])
        {
        case '%':
          p += 2;
          add_new_percent_arg_p = 1;
          break;
        case 'v':
          p += 2;
          PKL_AST_FORMAT_ARG_BASE (arg) = 0; /* Arbitrary.  */
          atype = pkl_ast_make_any_type (PKL_PASS_AST);
          PKL_AST_FORMAT_ARG_VALUE_P (arg) = 1;
          PKL_AST_FORMAT_ARG_FORMAT_DEPTH (arg)
            = (prefix == -1 ? 0 : prefix);
          switch (flag)
            {
            case 'T':
              PKL_AST_FORMAT_ARG_FORMAT_MODE (arg)
                = PKL_AST_FORMAT_MODE_TREE;
              break;
            case 'F':
              /* Fallthrough.  */
            case 0:
              PKL_AST_FORMAT_ARG_FORMAT_MODE (arg)
                = PKL_AST_FORMAT_MODE_FLAT;
              break;
            default:
              msg = _("invalid flag");
              goto invalid_tag;
            }
          types = pkl_ast_chainon (types, atype);
          ntag++;
          break;
        case 's':
          p += 2;
          PKL_AST_FORMAT_ARG_BASE (arg) = 10; /* Arbitrary.  */
          atype = pkl_ast_make_string_type (PKL_PASS_AST);
          types = pkl_ast_chainon (types, atype);
          ntag++;
          break;
        case 'c':
          p += 2;
          PKL_AST_FORMAT_ARG_BASE (arg) = 256;  /* Arbitrary */
          atype = pkl_ast_make_integral_type (PKL_PASS_AST, 8, 0);
          types = pkl_ast_chainon (types, atype);
          ntag++;
          break;
        case 'i':
        case 'u':
          {
            unsigned int bits;

            if (p[2] >= '0' && p[2] <= '9')
              {
                int base_idx;

                if (p[3] >= '0' && p[3] <= '9')
                  {
                    bits = (p[2] - '0') * 10 + (p[3] - '0');
                    base_idx = 4;
                  }
                else
                  {
                    bits = p[2] - '0';
                    base_idx = 3;
                  }

                if (bits > 64)
                  {
                    msg = _("base with more than 64 bits");
                    goto invalid_tag;
                  }

                switch (p[base_idx])
                  {
                  case 'b': PKL_AST_FORMAT_ARG_BASE (arg) = 2; break;
                  case 'o': PKL_AST_FORMAT_ARG_BASE (arg) = 8; break;
                  case 'd': PKL_AST_FORMAT_ARG_BASE (arg) = 10; break;
                  case 'x': PKL_AST_FORMAT_ARG_BASE (arg) = 16; break;
                  case 'c':
                    PKL_AST_FORMAT_ARG_BASE (arg) = 256;
                    if (bits != 8)
                      {
                        msg = _("char format only makes sense with 8 bits");
                        goto invalid_tag;
                      }
                    break;
                  default:
                    msg = _("invalid base");
                    goto invalid_tag;
                  }

                if (bits == 0)
                  {
                    msg = _("invalid bit-width");
                    goto invalid_tag;
                  }
                atype = pkl_ast_make_integral_type (PKL_PASS_AST,
                                                    bits, p[1] == 'i');
                types = pkl_ast_chainon (types, atype);

                if (base_idx == 4)
                  p += 5;
                else
                  p += 4;
              }
            else
              {
                if (p[1] == 'u')
                  msg = _("expected decimal digit after %u");
                else
                  msg = _("expected decimal digit after %i");

                goto invalid_tag;
              }
            ntag++;
            break;
          }
        case 'f':
        case 'e':
        case 'g':
          {
            unsigned int bits;

            PKL_AST_FORMAT_ARG_FLOATING_POINT_P (arg) = 1;
            PKL_AST_FORMAT_ARG_FLOATING_POINT_STYLE (arg) = p[1];

            if (p[2] >= '0' && p[2] <= '9')
              {
                int next_idx;

                if (p[3] >= '0' && p[3] <= '9')
                  {
                    bits = (p[2] - '0') * 10 + (p[3] - '0');
                    next_idx = 4;
                  }
                else
                  {
                    bits = p[2] - '0';
                    next_idx = 3;
                  }

                if (bits == 0 || !(/*bits == 16 ||*/ bits == 32 || bits == 64))
                  {
                    msg = _("invalid bit-width for a floating-point number");
                    goto invalid_tag;
                  }
                PKL_AST_FORMAT_ARG_FLOATING_POINT_WIDTH (arg)
                    = bits == 32
                          ? PKL_AST_FORMAT_ARG_FLOATING_POINT_WIDTH_SINGLE
                          : PKL_AST_FORMAT_ARG_FLOATING_POINT_WIDTH_DOUBLE;

                if (p[next_idx] == '.')
                  {
                    int prec_begin_idx;
                    char tmp;
                    long prec;

                    prec_begin_idx = ++next_idx;
                    while (p[next_idx] >= '0' && p[next_idx] <= '9')
                      ++next_idx;
                    if (next_idx == prec_begin_idx)
                      {
                        msg = _("expected a precision number after dot");
                        goto invalid_tag;
                      }
                    tmp = p[next_idx];
                    p[next_idx] = '\0';
                    errno = 0;
                    prec = strtol (&p[prec_begin_idx], NULL, 10);
                    if (errno != 0)
                      {
                        msg = _("invalid precision");
                        goto invalid_tag;
                      }
                    PKL_AST_FORMAT_ARG_FLOATING_POINT_PREC (arg)
                        = (unsigned)prec;
                    p[next_idx] = tmp;
                  }
                else
                  {
                    PKL_AST_FORMAT_ARG_FLOATING_POINT_PREC (arg)
                        = bits == 32 ? 7 : 15;
                  }

                switch (p[next_idx])
                  {
                  case 'b': PKL_AST_FORMAT_ARG_BASE (arg) = 2; break;
                  case 'o': PKL_AST_FORMAT_ARG_BASE (arg) = 8; break;
                  case 'd': PKL_AST_FORMAT_ARG_BASE (arg) = 10; break;
                  case 'x': PKL_AST_FORMAT_ARG_BASE (arg) = 16; break;
                  default:
                    msg = _("invalid base");
                    goto invalid_tag;
                  }

                if (PKL_AST_FORMAT_ARG_BASE (arg) != 10)
                  {
                    msg = _("only base 10 is supported for floating-point numbers");
                    goto invalid_tag;
                  }

                atype = pkl_ast_make_integral_type (PKL_PASS_AST,
                                                    bits, /*signed_p*/ 0);
                types = pkl_ast_chainon (types, atype);

                p += ++next_idx;
              }
            else
              {
                if (p[1] == 'f')
                  msg = _("expected decimal digit after %f");
                else if (p[1] == 'e')
                  msg = _("expected decimal digit after %e");
                else
                  msg = _("expected decimal digit after %g");

                goto invalid_tag;
              }
            ntag++;
            break;
          }
        case '<':
          /* Fallthrough.  */
        case '>':
          {
            int end_sc = 0;
            char *class = xmalloc (strlen (fmt) + 1);
            size_t j;

            end_sc = (p[1] == '>');
            p += 2;

            if (!end_sc)
              {
                /* Empty classes are not allowed.  */
                if (*p == ':')
                  {
                    free (class);
                    msg = _("invalid format specifier");
                    goto invalid_tag;
                  }

                /* Get the name of the styling class.  */
                j = 0;
                while (*p != ':' && *p != '%' && *p != '\0')
                  {
                    class[j++] = *p;
                    p++;
                  }
                class[j] = '\0';

                if (*p != ':')
                  {
                    free (class);
                    msg = _("invalid format specifier");
                    goto invalid_tag;
                  }
                p++; /* Skip the : */

                assert (nclasses < MAX_CLASS_TAGS);
                classes[nclasses++] = class;
              }
            else
              {
                if (nclasses == 0)
                  {
                    free (class);
                    msg = _("unpaired styling class");
                    goto invalid_tag;
                  }
                assert (nclasses > 0);
                class = classes[--nclasses];
              }

            assert (new_style_class == NULL);
            new_style_class = end_sc ? xstrdup (class) : class;
            add_new_style_arg_p = 1;
            break;
          }
        default:
          msg = _("invalid format specifier");
          goto invalid_tag;
        }

      assert (!(add_new_percent_arg_p && add_new_style_arg_p));
      if (add_new_percent_arg_p || add_new_style_arg_p)
        {
          pkl_ast_node new_arg;

          /* Create the new arg and add it to the list of arguments.  */
          new_arg = pkl_ast_make_format_arg (PKL_PASS_AST, NULL);

          if (add_new_percent_arg_p)
            PKL_AST_FORMAT_ARG_SUFFIX (new_arg) = xstrdup ("%");
          else if (add_new_style_arg_p)
            {
              int end_sc = p[-1] == '>';

              assert (new_style_class != NULL);
              if (end_sc)
                PKL_AST_FORMAT_ARG_END_SC (new_arg) = new_style_class;
              else
                PKL_AST_FORMAT_ARG_BEGIN_SC (new_arg) = new_style_class;
              new_style_class = NULL;
            }

          if (arg)
            {
              if (arg == PKL_AST_FORMAT_ARGS (format))
                {
                  /* Prepend.  */
                  PKL_AST_CHAIN (new_arg) = arg;
                  PKL_AST_FORMAT_ARGS (format) = ASTREF (new_arg);
                }
              else
                {
                  /* Add after.  */
                  PKL_AST_CHAIN (new_arg) = PKL_AST_CHAIN (prev_arg);
                  PKL_AST_CHAIN (prev_arg) = ASTREF (new_arg);
                }
            }
          else
            {
              /* Append.  */
              if (!PKL_AST_FORMAT_ARGS (format))
                PKL_AST_FORMAT_ARGS (format) = ASTREF (new_arg);
              else
                PKL_AST_FORMAT_ARGS (format)
                  = pkl_ast_chainon (PKL_AST_FORMAT_ARGS (format), new_arg);
            }

          arg = new_arg;

          /* The type corresponding to new arg is `void'.  */
          atype = pkl_ast_make_void_type (PKL_PASS_AST);
          types = pkl_ast_chainon (types, atype);

          add_new_percent_arg_p = 0;
          add_new_style_arg_p = 0;
        }

      /* Add the optional suffix to the argument.  */
      if (*p != '\0' && *p != '%')
        {
          char *end = strchrnul (p, '%');
          if (PKL_AST_FORMAT_ARG_SUFFIX (arg))
            {
              char* s = NULL;

              if (asprintf(&s, "%s%.*s", PKL_AST_FORMAT_ARG_SUFFIX (arg),
                           (int)(end - p), p) == -1)
                PKL_ICE (PKL_AST_LOC (format), _("out of memory"));
              PKL_AST_FORMAT_ARG_SUFFIX (arg) = s;
            }
          else
            PKL_AST_FORMAT_ARG_SUFFIX (arg) = strndup (p, end - p);
          if (!PKL_AST_FORMAT_ARG_SUFFIX (arg))
            PKL_ICE (PKL_AST_LOC (format), _("out of memory"));
          p = end;
        }
    }

  /* Check that we are not leaving unclosed styling classes.  */
  if (nclasses > 0)
    {
      msg = _("unclosed styling tag");
      goto invalid_tag;
    }

  if (nargs > ntag)
    {
      PKL_ERROR (PKL_AST_LOC (format), "too many format arguments");
      PKL_PASS_ERROR;
    }

  PKL_AST_FORMAT_TYPES (format) = ASTREF (types);

  PKL_AST_FORMAT_FMT_PROCESSED_P (format) = 1;
  PKL_PASS_DONE;

 invalid_tag:
  PKL_ERROR (PKL_AST_LOC (format_fmt),
             "invalid %%- tag in format string: %s", msg);
  PKL_PASS_ERROR;
}
PKL_PHASE_END_HANDLER

/* Compute and set the indexes of all the elements of an ARRAY node
   and set the size of the array consequently.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_array)
{
  pkl_ast_node array = PKL_PASS_NODE;
  pkl_ast_node initializers
    = PKL_AST_ARRAY_INITIALIZERS (array);

  pkl_ast_node tmp;
  size_t index, nelem, ninitializer;

  nelem = 0;
  for (index = 0, tmp = initializers, ninitializer = 0;
       tmp;
       tmp = PKL_AST_CHAIN (tmp), ++ninitializer)
    {
      pkl_ast_node initializer_index_node
        = PKL_AST_ARRAY_INITIALIZER_INDEX (tmp);
      uint64_t initializer_index;
      uint64_t elems_appended, effective_index;

      /* Set the index of the initializer.  */
      if (initializer_index_node == NULL)
        {
          pkl_ast_node initializer_index_type
            = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);

          initializer_index_node
            = pkl_ast_make_integer (PKL_PASS_AST, index);
          PKL_AST_TYPE (initializer_index_node)
            = ASTREF (initializer_index_type);

          PKL_AST_ARRAY_INITIALIZER_INDEX (tmp)
            = ASTREF (initializer_index_node);

          PKL_PASS_RESTART = 1;
          elems_appended = 1;
        }
      else
        {
          if (PKL_AST_CODE (initializer_index_node)
              != PKL_AST_INTEGER)
            {
              PKL_ERROR (PKL_AST_LOC (initializer_index_node),
                         "indexes in array initializers shall be constant");
              PKL_PASS_ERROR;
            }

          initializer_index
            = PKL_AST_INTEGER_VALUE (initializer_index_node);

          if ((int64_t) initializer_index < 0)
            {
              PKL_ERROR (PKL_AST_LOC (initializer_index_node),
                         "array dimensions may not be negative");
              PKL_PASS_ERROR;
            }
          else if ((int64_t) initializer_index < index)
            elems_appended = 0;
          else
            elems_appended = initializer_index - index + 1;
          effective_index = initializer_index;

          PKL_AST_INTEGER_VALUE (initializer_index_node)
            = effective_index;
        }

      index += elems_appended;
      nelem += elems_appended;
    }

  PKL_AST_ARRAY_NELEM (array) = nelem;
  PKL_AST_ARRAY_NINITIALIZER (array) = ninitializer;
}
PKL_PHASE_END_HANDLER

/* Compound statements introduce a lexical level.  Update the current
   function context accordingly.

   If the compound statement is the first operand of an excond
   operator, then it increases the number of exception handlers that
   need to be eventually popped.  Update the current function context
   accordingly */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_comp_stmt)
{
  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_EXP
      && PKL_AST_EXP_CODE (PKL_PASS_PARENT) == PKL_AST_OP_EXCOND)
    {
      if (PKL_TRANS_FUNCTION)
        PKL_TRANS_FUNCTION->npopes++;

      if (PKL_TRANS_ESCAPABLE)
        PKL_TRANS_ESCAPABLE->npopes++;
    }
}
PKL_PHASE_END_HANDLER

/* FOR statements are escapable constructs, push it to current
   escapables stack.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_loop_stmt)
{
  pkl_ast_node stmt = PKL_PASS_NODE;

  PKL_TRANS_PUSH_ESCAPABLE (stmt);
}
PKL_PHASE_END_HANDLER

/* Revert what we did in `pkl_trans1_pr_loop_stmt'.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_loop_stmt)
{
  PKL_TRANS_POP_ESCAPABLE;
}
PKL_PHASE_END_HANDLER

/* Annotate compound statement nodes with the number of variable and
   function declarations occurring in the statement.

   Compound statements introduce a lexical level.  Update the current
   escapable context accordingly.

   If the compound statement is the first operand of an excond
   operator, then it increases the number of exception handlers that
   need to be eventually popped.  Update the current function context
   accordingly */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_comp_stmt)
{
  pkl_ast_node t, comp_stmt = PKL_PASS_NODE;
  int numvars = 0;

  for (t = PKL_AST_COMP_STMT_STMTS (comp_stmt);
       t;
       t = PKL_AST_CHAIN (t))
    {
      if (PKL_AST_CODE (t) == PKL_AST_DECL
          && (PKL_AST_DECL_KIND (t) == PKL_AST_DECL_KIND_VAR
              || PKL_AST_DECL_KIND (t) == PKL_AST_DECL_KIND_FUNC))
        numvars++;
    }

  PKL_AST_COMP_STMT_NUMVARS (comp_stmt) = numvars;

  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_EXP
      && PKL_AST_EXP_CODE (PKL_PASS_PARENT) == PKL_AST_OP_EXCOND)
    {
      if (PKL_TRANS_FUNCTION)
        PKL_TRANS_FUNCTION->npopes--;

      if (PKL_TRANS_ESCAPABLE)
        PKL_TRANS_ESCAPABLE->npopes--;
    }
}
PKL_PHASE_END_HANDLER

/* Push current endianness annotation if necessary.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_struct_type_field)
{
  pkl_ast_node field = PKL_PASS_NODE;
  int endian = PKL_AST_STRUCT_TYPE_FIELD_ENDIAN (field);

  if (endian != PKL_AST_ENDIAN_DFL)
    PKL_TRANS_PUSH_ENDIAN (PKL_AST_STRUCT_TYPE_FIELD_ENDIAN (field));
}
PKL_PHASE_END_HANDLER

/* Annotate struct type fields with the current endianness annotation
   if necessary.  And also pop endianness.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_struct_type_field)
{
  pkl_ast_node field = PKL_PASS_NODE;
  int endian = PKL_AST_STRUCT_TYPE_FIELD_ENDIAN (field);

  if (endian == PKL_AST_ENDIAN_DFL)
    PKL_AST_STRUCT_TYPE_FIELD_ENDIAN (field) = PKL_TRANS_ENDIAN;

  if (endian != PKL_AST_ENDIAN_DFL)
    PKL_TRANS_POP_ENDIAN;
}
PKL_PHASE_END_HANDLER

/* Annotate return statements with their containing functions.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_return_stmt)
{
  pkl_ast_node stmt = PKL_PASS_NODE;

  if (PKL_TRANS_FUNCTION)
    {
      PKL_AST_RETURN_STMT_FUNCTION (stmt)
        = PKL_TRANS_FUNCTION->node; /* Note no ASTREF.  */
      PKL_AST_RETURN_STMT_NPOPES (stmt) = PKL_TRANS_FUNCTION->npopes;
    }

}
PKL_PHASE_END_HANDLER

/* The body of a try statement increases the number of exception
   handlers that need to be eventually popped.  Update the current
   function context accordingly.

   try-until statements are escapable constructs.  Update escapables
   stack accordingly.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_try_stmt_body)
{
  if (PKL_TRANS_FUNCTION)
    PKL_TRANS_FUNCTION->npopes++;

  if (PKL_AST_TRY_STMT_KIND (PKL_PASS_PARENT) == PKL_AST_TRY_STMT_KIND_UNTIL)
    PKL_TRANS_PUSH_ESCAPABLE (PKL_PASS_PARENT);
  else if (PKL_TRANS_ESCAPABLE)
    PKL_TRANS_ESCAPABLE->npopes++;
}
PKL_PHASE_END_HANDLER

/* The body of a try statement increases the number of exception
   handlers that need to be eventually popped.  Update the current
   function context accordingly.

   try-until statements are escapable constructs.  Update escapables
   stack accordingly.  try-catch statements increase the number of
   exception handlers that need to be eventually popped.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_try_stmt_body)
{
  if (PKL_TRANS_FUNCTION)
    PKL_TRANS_FUNCTION->npopes--;

  if (PKL_AST_TRY_STMT_KIND (PKL_PASS_PARENT) == PKL_AST_TRY_STMT_KIND_UNTIL)
    PKL_TRANS_POP_ESCAPABLE;
  else if (PKL_TRANS_ESCAPABLE)
    PKL_TRANS_ESCAPABLE->npopes--;
}
PKL_PHASE_END_HANDLER

/* try-until statements are escapable constructs.  Update escapables
   stack accordingly.  try-catch statements increase the number of
   exception handlers that need to be eventually dropped.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_pr_try_stmt)
{
  if (PKL_AST_TRY_STMT_KIND (PKL_PASS_NODE) == PKL_AST_TRY_STMT_KIND_UNTIL)
    PKL_TRANS_PUSH_ESCAPABLE (PKL_PASS_NODE);
}
PKL_PHASE_END_HANDLER

/* Likewise.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_try_stmt)
{
  if (PKL_AST_TRY_STMT_KIND (PKL_PASS_NODE) == PKL_AST_TRY_STMT_KIND_UNTIL)
    PKL_TRANS_POP_ESCAPABLE;
}
PKL_PHASE_END_HANDLER

/* Annotate break and continue statements with enclosing escapable
   construct, and with the exception handling nesting level with
   respect to the enclosing escapable construct.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_break_continue_stmt)
{
  if (PKL_TRANS_ESCAPABLE)
    {
      PKL_AST_BREAK_CONTINUE_STMT_ENTITY (PKL_PASS_NODE)
        = PKL_TRANS_ESCAPABLE->node;
      PKL_AST_BREAK_CONTINUE_STMT_NPOPES (PKL_PASS_NODE)
        = PKL_TRANS_ESCAPABLE->npopes;
    }
}
PKL_PHASE_END_HANDLER

/* Annotate indexers that are themselves immediately indexed by
   another [] operator.  This is used for certain optimizations in
   `gen'.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_indexer)
{
  pkl_ast_node indexer = PKL_PASS_NODE;

  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) == PKL_AST_INDEXER)
    PKL_AST_INDEXER_IS_INDEXED (indexer) = 1;
}
PKL_PHASE_END_HANDLER

/* Reverse list of outputs and transform them into assignments,
   checking that they are proper l-values.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans1_ps_asm_stmt)
{
  pkl_ast_node asm_stmt = PKL_PASS_NODE;
  pkl_ast_node assignments = NULL, output;

  for (output = PKL_AST_ASM_STMT_OUTPUTS (asm_stmt);
       output;
       output = PKL_AST_CHAIN (output))
    {
      pkl_ast_node ass_stmt;

      if (!pkl_ast_lvalue_p (output))
        {
          PKL_ERROR (PKL_AST_LOC (output),
                     "asm statement output should be a l-value");
          PKL_PASS_ERROR;
        }

      ass_stmt = pkl_ast_make_ass_stmt (PKL_PASS_AST,
                                        output,
                                        NULL /* exp */);
      PKL_AST_LOC (ass_stmt) = PKL_AST_LOC (asm_stmt);
      /* Note the reverse order.  */
      assignments = pkl_ast_chainon (ass_stmt, assignments);
    }

  if (assignments)
    {
      PKL_AST_ASM_STMT_OUTPUTS (asm_stmt) = ASTREF (assignments);
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans1 =
  {
    .initialize = pkl_trans_initialize,
    .finalize = pkl_trans_finalize,

    PKL_PHASE_PS_HANDLER (PKL_AST_SRC, pkl_trans_ps_src),
    PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
    PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT, pkl_trans1_ps_struct),
    PKL_PHASE_PS_HANDLER (PKL_AST_OFFSET, pkl_trans1_ps_offset),
    PKL_PHASE_PS_HANDLER (PKL_AST_FUNCALL, pkl_trans1_ps_funcall),
    PKL_PHASE_PS_HANDLER (PKL_AST_STRING, pkl_trans1_ps_string),
    PKL_PHASE_PS_HANDLER (PKL_AST_VAR, pkl_trans1_ps_var),
    PKL_PHASE_PR_HANDLER (PKL_AST_FUNC, pkl_trans1_pr_func),
    PKL_PHASE_PS_HANDLER (PKL_AST_FUNC, pkl_trans1_ps_func),
    PKL_PHASE_PS_HANDLER (PKL_AST_TRIMMER, pkl_trans1_ps_trimmer),
    PKL_PHASE_PS_HANDLER (PKL_AST_FORMAT, pkl_trans1_ps_format),
    PKL_PHASE_PR_HANDLER (PKL_AST_DECL, pkl_trans1_pr_decl),
    PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY, pkl_trans1_ps_array),
    PKL_PHASE_PR_HANDLER (PKL_AST_COMP_STMT, pkl_trans1_pr_comp_stmt),
    PKL_PHASE_PS_HANDLER (PKL_AST_COMP_STMT, pkl_trans1_ps_comp_stmt),
    PKL_PHASE_PR_HANDLER (PKL_AST_LOOP_STMT, pkl_trans1_pr_loop_stmt),
    PKL_PHASE_PS_HANDLER (PKL_AST_LOOP_STMT, pkl_trans1_ps_loop_stmt),
    PKL_PHASE_PS_HANDLER (PKL_AST_BREAK_CONTINUE_STMT, pkl_trans1_ps_break_continue_stmt),
    PKL_PHASE_PR_HANDLER (PKL_AST_TRY_STMT, pkl_trans1_pr_try_stmt),
    PKL_PHASE_PS_HANDLER (PKL_AST_TRY_STMT, pkl_trans1_ps_try_stmt),
    PKL_PHASE_PR_HANDLER (PKL_AST_TRY_STMT_BODY, pkl_trans1_pr_try_stmt_body),
    PKL_PHASE_PS_HANDLER (PKL_AST_TRY_STMT_BODY, pkl_trans1_ps_try_stmt_body),
    PKL_PHASE_PR_HANDLER (PKL_AST_STRUCT_TYPE_FIELD, pkl_trans1_pr_struct_type_field),
    PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT_TYPE_FIELD, pkl_trans1_ps_struct_type_field),
    PKL_PHASE_PS_HANDLER (PKL_AST_RETURN_STMT, pkl_trans1_ps_return_stmt),
    PKL_PHASE_PS_HANDLER (PKL_AST_INDEXER, pkl_trans1_ps_indexer),
    PKL_PHASE_PS_HANDLER (PKL_AST_ASM_STMT, pkl_trans1_ps_asm_stmt),
    PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_ATTR, pkl_trans1_ps_op_attr),
    PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_trans1_pr_type_array),
    PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_trans1_ps_type_array),
    PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_trans1_pr_type_struct),
    PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_trans1_ps_type_struct),
    PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_FUNCTION, pkl_trans1_ps_type_function),
  };



/* The following handlers annotate expression nodes to reflect whether
   they are literals.  Entities created by the lexer (INTEGER, STRING,
   etc) already have this attribute set if needed. */

/*  Expressions having only literal operands are literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_exp)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  int o, literal_p = 1;

  for (o = 0; o < PKL_AST_EXP_NUMOPS (exp); ++o)
    {
      pkl_ast_node op = PKL_AST_EXP_OPERAND (exp, o);

      literal_p &= PKL_AST_LITERAL_P (op);
      if (!literal_p)
        break;
    }

  /* XXX note that POW is currently not constant-folded.  See comment
     in pkl-fold.c */
  if (PKL_AST_EXP_CODE (exp) == PKL_AST_OP_POW)
    PKL_AST_LITERAL_P (exp) = 0;
  else
    PKL_AST_LITERAL_P (exp) = literal_p;
}
PKL_PHASE_END_HANDLER

/* An offset is a literal if its magnitude is also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_offset)
{
  pkl_ast_node magnitude
    = PKL_AST_OFFSET_MAGNITUDE (PKL_PASS_NODE);

  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (magnitude);
}
PKL_PHASE_END_HANDLER

/* An array is a literal if all its initializers are literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_array)
{
  int literal_p = 1;
  pkl_ast_node t, array = PKL_PASS_NODE;

  for (t = PKL_AST_ARRAY_INITIALIZERS (array); t;
       t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node array_initializer_exp
        = PKL_AST_ARRAY_INITIALIZER_EXP (t);

      literal_p &= PKL_AST_LITERAL_P (array_initializer_exp);
      if (!literal_p)
        break;
    }

  PKL_AST_LITERAL_P (array) = literal_p;
}
PKL_PHASE_END_HANDLER

/* An indexer is a literal if the referred entity element is also a
   literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_indexer)
{
  pkl_ast_node entity = PKL_AST_INDEXER_ENTITY (PKL_PASS_NODE);
  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (entity);
}
PKL_PHASE_END_HANDLER

/* A trim is a literal if the trimmed entity is also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_trimmer)
{
  pkl_ast_node entity = PKL_AST_TRIMMER_ENTITY (PKL_PASS_NODE);
  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (entity);
}
PKL_PHASE_END_HANDLER

/* A struct is a literal if all its element values are literals.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_struct)
{
  pkl_ast_node t;
  int literal_p = 1;

  for (t = PKL_AST_STRUCT_FIELDS (PKL_PASS_NODE); t;
       t = PKL_AST_CHAIN (t))
    {
      pkl_ast_node struct_field_exp = PKL_AST_STRUCT_FIELD_EXP (t);

      literal_p &= PKL_AST_LITERAL_P (struct_field_exp);
      if (!literal_p)
        break;
    }

  PKL_AST_LITERAL_P (PKL_PASS_NODE) = literal_p;
}
PKL_PHASE_END_HANDLER

/* A struct ref is a literal if the value of the referred element is
   also a literal.

   Also, struct references that refer to parameterless methods are
   transformed into funcalls to these methods, but only if the struct
   references are not part of funcall themselves.  This is similar to
   what is done with variable nodes.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_struct_ref)
{
  pkl_ast_node struct_ref = PKL_PASS_NODE;
  pkl_ast_node stct = PKL_AST_STRUCT_REF_STRUCT (struct_ref);

  PKL_AST_LITERAL_P (PKL_PASS_NODE) = PKL_AST_LITERAL_P (stct);

  if (PKL_PASS_PARENT
      && PKL_AST_CODE (PKL_PASS_PARENT) != PKL_AST_FUNCALL)
    {
      pkl_ast_node type = PKL_AST_TYPE (struct_ref);

      if (PKL_AST_TYPE_CODE (type) == PKL_TYPE_FUNCTION
          && !PKL_AST_STRUCT_REF_IS_PARENTHESIZED (struct_ref)
          && (PKL_AST_TYPE_F_NARG (type) == 0
              || pkl_ast_func_all_optargs (type)))
        {
          pkl_ast_node function_rtype = PKL_AST_TYPE_F_RTYPE (type);
          pkl_ast_node funcall = pkl_ast_make_funcall (PKL_PASS_AST,
                                                       ASTDEREF (struct_ref),
                                                       NULL /* args */);

          /* Note that we have to set the type here, because typify1
             is performed before trans2.  */
          PKL_AST_TYPE (funcall) = ASTREF (function_rtype);
          PKL_AST_LOC (funcall) = PKL_AST_LOC (struct_ref);

          PKL_PASS_NODE = ASTREF (funcall);
          PKL_PASS_RESTART = 1;
        }
    }
}
PKL_PHASE_END_HANDLER

/* A cast is considered a literal if the value of the referred element
   is also a literal.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_cast)
{
  PKL_AST_LITERAL_P (PKL_PASS_NODE)
    = PKL_AST_LITERAL_P (PKL_AST_CAST_EXP (PKL_PASS_NODE));
}
PKL_PHASE_END_HANDLER

/* In offset types having another type as their unit, replace it with
   its size in bits.  Emit a diagnostic if the type is not
   complete.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_type_offset)
{
  pkl_ast_node type = PKL_PASS_NODE;
  pkl_ast_node unit_type = PKL_AST_TYPE_O_UNIT (type);
  pkl_ast_node unit;

  if (PKL_AST_CODE (unit_type) != PKL_AST_TYPE)
    /* The unit of this offset is not a type.  Nothing to do.  */
    PKL_PASS_DONE;

  if (PKL_AST_TYPE_COMPLETE (unit_type) != PKL_AST_TYPE_COMPLETE_YES)
    {
      PKL_ERROR (PKL_AST_LOC (type),
                 "offset types only work on complete types");
      PKL_PASS_ERROR;
    }

  /* Calculate the size of the complete type in bytes and put it in
     an integer node.  */
  unit = pkl_ast_sizeof_type (PKL_PASS_AST, unit_type);

  /* Replace the unit type with this expression.  */
  PKL_AST_TYPE_O_UNIT (type) = ASTREF (unit);
  pkl_ast_node_free (unit_type);

  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

/* Add an assignment statement to INCRDECR expressions.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_incrdecr)
{
  pkl_ast_node incrdecr = PKL_PASS_NODE;
  pkl_ast_node incrdecr_stmt = PKL_AST_INCRDECR_ASS_STMT (incrdecr);

  if (!incrdecr_stmt)
    {
      pkl_ast_node incrdecr_exp = PKL_AST_INCRDECR_EXP (incrdecr);
      pkl_ast_node incrdecr_exp_type = PKL_AST_TYPE (incrdecr_exp);
      int incrdecr_sign = PKL_AST_INCRDECR_SIGN (incrdecr);
      pkl_ast_node step, ass_stmt, exp_plus_one;

      int op = (incrdecr_sign == PKL_AST_SIGN_INCR
                ? PKL_AST_OP_ADD : PKL_AST_OP_SUB);

      /* Get the step.  The type of the expression is safe as per
         typify.  */
      step = pkl_ast_type_incr_step (PKL_PASS_AST, incrdecr_exp_type);
      if (!step)
        PKL_ICE (PKL_AST_NOLOC,
                 "pkl_ast_type_incr_step failed in pkl_trans2_ps_incrdecr");

      /* Build a statement EXP = EXP +/- STEP  */
      exp_plus_one = pkl_ast_make_binary_exp (PKL_PASS_AST, op,
                                              incrdecr_exp, step);
      PKL_AST_TYPE (exp_plus_one) = ASTREF (incrdecr_exp_type);
      ass_stmt = pkl_ast_make_ass_stmt (PKL_PASS_AST,
                                        incrdecr_exp, exp_plus_one);
      PKL_AST_LOC (ass_stmt) = PKL_AST_LOC (incrdecr);

      PKL_AST_INCRDECR_ASS_STMT (incrdecr) = ASTREF (ass_stmt);
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

/* Calculate the size of struct type fields that are complete.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_struct_type_field)
{
  pkl_ast_node field = PKL_PASS_NODE;
  pkl_ast_node field_type = PKL_AST_STRUCT_TYPE_FIELD_TYPE (field);

  if (pkl_ast_type_is_complete (field_type) == PKL_AST_TYPE_COMPLETE_YES)
    {
      PKL_AST_STRUCT_TYPE_FIELD_SIZE (field)
        = ASTREF (pkl_ast_sizeof_type (PKL_PASS_AST, field_type));
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

/* Compute the attributes of values with complete types.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_op_attr)
{
  pkl_ast_node exp = PKL_PASS_NODE;
  pkl_ast_node exp_type = PKL_AST_TYPE (exp);
  pkl_ast_node operand = PKL_AST_EXP_OPERAND (exp, 0);
  pkl_ast_node operand_type = PKL_AST_TYPE (operand);

  switch (PKL_AST_EXP_ATTR (exp))
    {
    case PKL_AST_ATTR_LENGTH:
      {
        size_t length = 0;
        pkl_ast_node len;

        if (PKL_AST_LITERAL_P (operand))
          switch (PKL_AST_CODE (operand))
            {
            case PKL_AST_STRING:
              length = PKL_AST_STRING_LENGTH (operand);
              break;
            case PKL_AST_ARRAY:
              length = PKL_AST_ARRAY_NELEM (operand);
              break;
            default:
              PKL_PASS_DONE;
            }
        else if (PKL_AST_TYPE_CODE (operand_type) == PKL_TYPE_STRUCT
                 && (PKL_AST_TYPE_COMPLETE (operand_type)
                     == PKL_AST_TYPE_COMPLETE_YES))
          length = PKL_AST_TYPE_S_NELEM (operand_type);
        else
          PKL_PASS_DONE;

        /* The type of 'length is uint<64>  */

        len = pkl_ast_make_integer (PKL_PASS_AST, length);
        PKL_AST_TYPE (len) = ASTREF (exp_type);
        PKL_PASS_NODE = ASTREF (len);
        pkl_ast_node_free (exp);
        PKL_PASS_RESTART = 1;
        PKL_PASS_DONE;
      }
    case PKL_AST_ATTR_SIZE:
      {
        pkl_ast_node size, off;

        if (PKL_AST_LITERAL_P (operand)
            && PKL_AST_CODE (operand) == PKL_AST_STRING)
          {
            size = pkl_ast_make_integer (PKL_PASS_AST,
                                         (PKL_AST_STRING_LENGTH (operand) + 1)
                                           * 8);
            PKL_AST_TYPE (size) = PKL_AST_TYPE_O_BASE_TYPE (exp_type);
          }
        else if (PKL_AST_TYPE_COMPLETE (operand_type)
                   == PKL_AST_TYPE_COMPLETE_YES)
          size = pkl_ast_sizeof_type (PKL_PASS_AST, operand_type);
        else
          PKL_PASS_DONE;

        off = pkl_ast_make_offset (PKL_PASS_AST, size,
                                   PKL_AST_TYPE_O_UNIT (exp_type));
        PKL_AST_TYPE (off) = ASTREF (exp_type);
        PKL_PASS_NODE = ASTREF (off);
        pkl_ast_node_free (exp);
        PKL_PASS_RESTART = 1;
        PKL_PASS_DONE;
      }
    default:
      break;
    }
}
PKL_PHASE_END_HANDLER

/* Transform assignment statements to l-value bconc operators into an
   equivalent sequence of assignments.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans2_ps_ass_stmt)
{
  pkl_ast_node lvalue = PKL_AST_ASS_STMT_LVALUE (PKL_PASS_NODE);

  if (PKL_AST_CODE (lvalue) == PKL_AST_EXP
      && PKL_AST_EXP_CODE (lvalue) == PKL_AST_OP_BCONC)
    {
      pkl_ast_node repl = pkl_ast_handle_bconc_ass_stmt (PKL_PASS_AST,
                                                         PKL_PASS_NODE);
      pkl_ast_node_free (PKL_PASS_NODE);
      PKL_PASS_NODE = ASTREF (repl);
      PKL_PASS_RESTART = 1;
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans2 =
  {
    .initialize = pkl_trans_initialize,
    .finalize = pkl_trans_finalize,

    PKL_PHASE_PS_HANDLER (PKL_AST_SRC, pkl_trans_ps_src),
    PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
    PKL_PHASE_PS_HANDLER (PKL_AST_EXP, pkl_trans2_ps_exp),
    PKL_PHASE_PS_HANDLER (PKL_AST_OFFSET, pkl_trans2_ps_offset),
    PKL_PHASE_PS_HANDLER (PKL_AST_ARRAY, pkl_trans2_ps_array),
    PKL_PHASE_PS_HANDLER (PKL_AST_INDEXER, pkl_trans2_ps_indexer),
    PKL_PHASE_PS_HANDLER (PKL_AST_TRIMMER, pkl_trans2_ps_trimmer),
    PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT, pkl_trans2_ps_struct),
    PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT_REF, pkl_trans2_ps_struct_ref),
    PKL_PHASE_PS_HANDLER (PKL_AST_CAST, pkl_trans2_ps_cast),
    PKL_PHASE_PS_HANDLER (PKL_AST_INCRDECR, pkl_trans2_ps_incrdecr),
    PKL_PHASE_PS_HANDLER (PKL_AST_ASS_STMT, pkl_trans2_ps_ass_stmt),
    PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_OFFSET, pkl_trans2_ps_type_offset),
    PKL_PHASE_PS_HANDLER (PKL_AST_STRUCT_TYPE_FIELD, pkl_trans2_ps_struct_type_field),
    PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_ATTR, pkl_trans2_ps_op_attr),
  };



/* SIZEOF nodes whose operand is a complete type should be replaced
   with an offset.  */

PKL_PHASE_BEGIN_HANDLER (pkl_trans3_ps_op_sizeof)
{
  pkl_ast_node node = PKL_PASS_NODE;
  pkl_ast_node op = PKL_AST_EXP_OPERAND (node, 0);
  pkl_ast_node offset, offset_type, unit, unit_type;

  if (PKL_AST_TYPE_COMPLETE (op)
      != PKL_AST_TYPE_COMPLETE_YES)
    {
      PKL_ERROR (PKL_AST_LOC (op),
                 "invalid operand to sizeof");
      PKL_PASS_ERROR;
    }

  {
    /* Calculate the size of the complete type in bytes and put it in
       an integer node.  */
    pkl_ast_node magnitude
      = pkl_ast_sizeof_type (PKL_PASS_AST, op);

    /* Build an offset with that magnitude, and unit bits.  */
    unit_type = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);

    unit = pkl_ast_make_integer (PKL_PASS_AST, PKL_AST_OFFSET_UNIT_BITS);
    PKL_AST_TYPE (unit) = ASTREF (unit_type);

    offset = pkl_ast_make_offset (PKL_PASS_AST, magnitude, unit);

    offset_type = pkl_ast_make_offset_type (PKL_PASS_AST,
                                            PKL_AST_TYPE (magnitude),
                                            unit, NULL /* ref_type */);
    PKL_AST_TYPE (offset) = ASTREF (offset_type);
  }

  pkl_ast_node_free (PKL_PASS_NODE);
  PKL_PASS_NODE = ASTREF (offset);
  PKL_PASS_RESTART = 1;
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_trans3 =
  {
    .initialize = pkl_trans_initialize,
    .finalize = pkl_trans_finalize,

    PKL_PHASE_PS_HANDLER (PKL_AST_SRC, pkl_trans_ps_src),
    PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
    PKL_PHASE_PS_OP_HANDLER (PKL_AST_OP_SIZEOF, pkl_trans3_ps_op_sizeof),
  };



/* Mark compound statements that do not contain any declaration as
   "frameless".  */

PKL_PHASE_BEGIN_HANDLER (pkl_transf_ps_comp_stmt)
{
  pkl_ast_node comp_stmt = PKL_PASS_NODE;
  pkl_ast_node stmt = NULL;

  PKL_AST_COMP_STMT_FRAMELESS_P (comp_stmt) = 1;
  for (stmt = PKL_AST_COMP_STMT_STMTS (comp_stmt);
       stmt;
       stmt = PKL_AST_CHAIN (stmt))
    {
      if (PKL_AST_CODE (stmt) == PKL_AST_DECL
          && (PKL_AST_DECL_KIND (stmt) == PKL_AST_DECL_KIND_VAR
              || PKL_AST_DECL_KIND (stmt) == PKL_AST_DECL_KIND_FUNC))
        {
          PKL_AST_COMP_STMT_FRAMELESS_P (comp_stmt) = 0;
          break;
        }
    }
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_transf =
  {
    .initialize = pkl_trans_initialize,
    .finalize = pkl_trans_finalize,

    PKL_PHASE_PS_HANDLER (PKL_AST_SRC, pkl_trans_ps_src),
    PKL_PHASE_PS_HANDLER (PKL_AST_COMP_STMT, pkl_transf_ps_comp_stmt),
  };

#define PKL_TRANS_ENV (PKL_TRANS_PAYLOAD->env)

/* FOR statements introduce a lexical level if they use an iterator or
   have a head of declarations.  Update the current function context
   accordingly.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_loop_stmt)
{
  pkl_ast_node stmt = PKL_PASS_NODE;
  pkl_ast_node iterator = PKL_AST_LOOP_STMT_ITERATOR (stmt);
  pkl_ast_node head = PKL_AST_LOOP_STMT_HEAD (stmt);
  pkl_ast_node condition = PKL_AST_LOOP_STMT_CONDITION (stmt);
  pkl_ast_node tail = PKL_AST_LOOP_STMT_TAIL (stmt);
  pkl_ast_node body = PKL_AST_LOOP_STMT_BODY (stmt);
  pkl_ast_node t = NULL;

  /* while (CONDITION) BODY
     for (HEAD; CONDITION; TAIL) BODY
     for (ITERATOR) BODY  */

  PKL_TRANS_PUSH_ESCAPABLE (stmt);

  if (iterator)
    {
      pkl_ast_node container = PKL_AST_LOOP_STMT_ITERATOR_CONTAINER (iterator);
      pkl_ast_node decl = PKL_AST_LOOP_STMT_ITERATOR_DECL (iterator);

      PKL_PASS_SUBPASS (container);
      PKL_TRANS_ENV = pkl_env_push_frame (PKL_TRANS_ENV);
      PKL_PASS_SUBPASS (decl);

      if (PKL_TRANS_FUNCTION)
        {
          PKL_TRANS_FUNCTION->back++;
          PKL_TRANS_FUNCTION->ndrops += 3;
        }
    }

  if (head)
    {
      PKL_TRANS_ENV = pkl_env_push_frame (PKL_TRANS_ENV);
      if (PKL_TRANS_FUNCTION)
        PKL_TRANS_FUNCTION->back++;
      for (t = head; t; t = PKL_AST_CHAIN (t))
        PKL_PASS_SUBPASS (t);
    }

  if (condition)
    for (t = condition; t; t = PKL_AST_CHAIN (t))
      PKL_PASS_SUBPASS (t);
  if (tail)
    for (t = tail; t; t = PKL_AST_CHAIN (t))
      PKL_PASS_SUBPASS (t);

  PKL_PASS_SUBPASS (body);

  if (PKL_TRANS_FUNCTION && iterator)
    PKL_TRANS_FUNCTION->ndrops -= 3;

  if (iterator || head)
    {
      PKL_TRANS_ENV = pkl_env_pop_frame (PKL_TRANS_ENV);
      if (PKL_TRANS_FUNCTION)
        PKL_TRANS_FUNCTION->back--;
    }

  PKL_TRANS_POP_ESCAPABLE;
  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/* Compound statements introduce a lexical level, but only if they
   have one or more declarations inside.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_comp_stmt)
{
  pkl_ast_node comp_stmt = PKL_PASS_NODE;

  if (!PKL_AST_COMP_STMT_FRAMELESS_P (comp_stmt))
    {
      PKL_TRANS_ENV = pkl_env_push_frame (PKL_TRANS_ENV);
      if (PKL_TRANS_FUNCTION)
        PKL_TRANS_FUNCTION->back++;
      if (PKL_TRANS_ESCAPABLE)
        PKL_TRANS_ESCAPABLE->nframes++;
    }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_comp_stmt)
{
  pkl_ast_node comp_stmt = PKL_PASS_NODE;

  if (!PKL_AST_COMP_STMT_FRAMELESS_P (comp_stmt))
    {
      PKL_TRANS_ENV = pkl_env_pop_frame (PKL_TRANS_ENV);
      if (PKL_TRANS_FUNCTION)
        PKL_TRANS_FUNCTION->back--;
      if (PKL_TRANS_ESCAPABLE)
        PKL_TRANS_ESCAPABLE->nframes--;
    }
}
PKL_PHASE_END_HANDLER

/* TRY-CATCH statement whose `catch' part gets an argument introduce a
   new lexical level right before evaluating the format argument (the
   exception passed to the handler).  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_try_stmt)
{
  pkl_ast_node try_stmt = PKL_PASS_NODE;
  pkl_ast_node handler = PKL_AST_TRY_STMT_HANDLER (try_stmt);
  pkl_ast_node body = PKL_AST_TRY_STMT_BODY (try_stmt);
  pkl_ast_node arg = PKL_AST_TRY_STMT_ARG (try_stmt);
  pkl_ast_node exp = PKL_AST_TRY_STMT_EXP (try_stmt);
  int kind = PKL_AST_TRY_STMT_KIND (try_stmt);

  if (kind == PKL_AST_TRY_STMT_KIND_UNTIL)
    PKL_TRANS_PUSH_ESCAPABLE (try_stmt);
  PKL_PASS_SUBPASS (body);
  if (kind == PKL_AST_TRY_STMT_KIND_UNTIL)
    PKL_TRANS_POP_ESCAPABLE;

  if (exp)
    PKL_PASS_SUBPASS (exp);
  if (arg)
    {
      PKL_TRANS_ENV = pkl_env_push_frame (PKL_TRANS_ENV);
      if (PKL_TRANS_ESCAPABLE)
        PKL_TRANS_ESCAPABLE->nframes++;
      PKL_PASS_SUBPASS (arg);
    }
  if (handler)
    PKL_PASS_SUBPASS (handler);
  if (arg)
    {
      PKL_TRANS_ENV = pkl_env_pop_frame (PKL_TRANS_ENV);
      if (PKL_TRANS_ESCAPABLE)
        PKL_TRANS_ESCAPABLE->nframes--;
    }

  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/* Function definitions introduce a lexical level for the funtion's
   arguments.

   Note that we have to use a breaking handler because the return type
   may have expressions in it (like an array bounder) and these shall
   be computed out of the new lexical environment.

   In methods there is an implicit argument SELF that has lexical
   address (0,0).  This handler registers it.

   The handler also maintains a stack of functions.  */

/* XXX do not introduce lexical level if the function has no
   arguments!.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_func)
{
  pkl_ast_node func = PKL_PASS_NODE;
  pkl_ast_node ret_type = PKL_AST_FUNC_RET_TYPE (func);
  pkl_ast_node args = PKL_AST_FUNC_ARGS (func);
  pkl_ast_node body = PKL_AST_FUNC_BODY (func);
  pkl_ast_node arg;

  PKL_TRANS_PUSH_FUNCTION (PKL_PASS_NODE);
  PKL_TRANS_PUSH_UNESCAPABLE;

  if (ret_type && !PKL_AST_TYPE_NAME (ret_type))
    PKL_PASS_SUBPASS (ret_type);

  PKL_TRANS_ENV = pkl_env_push_frame (PKL_TRANS_ENV);

  if (PKL_AST_FUNC_METHOD_P (func))
    {
      /* Register an argument SELF for the method's initial implicit
         argument.  */
      pkl_ast_node self = pkl_ast_make_identifier (PKL_PASS_AST, "SELF");

      if (!pkl_env_register (PKL_TRANS_ENV,
                             PKL_PASS_AST,
                             PKL_ENV_NS_MAIN,
                             PKL_AST_IDENTIFIER_POINTER (self),
                             pkl_ast_make_decl (PKL_PASS_AST,
                                                PKL_AST_DECL_KIND_VAR,
                                                self,
                                                pkl_ast_make_integer (PKL_PASS_AST, 0),
                                                NULL /* source */)))
        {
          PKL_ICE (PKL_AST_LOC (func),
                   "transl: could not register entry for SELF");
          PKL_PASS_ERROR;
        }
    }

  for (arg = args; arg; arg = PKL_AST_CHAIN (arg))
    PKL_PASS_SUBPASS (arg);
  PKL_PASS_SUBPASS (body);
  PKL_TRANS_ENV = pkl_env_pop_frame (PKL_TRANS_ENV);

  PKL_TRANS_POP_FUNCTION;
  PKL_TRANS_POP_ESCAPABLE;
  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/* Function formal arguments shall be registered to the lexical
   enviroment.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_func_arg)
{
  pkl_ast_node arg = PKL_PASS_NODE;
  pkl_ast_node arg_identifier = PKL_AST_FUNC_ARG_IDENTIFIER (arg);
  pkl_ast_node dummy = pkl_ast_make_integer (PKL_PASS_AST, 0);
  PKL_AST_TYPE (dummy) = ASTREF (PKL_AST_FUNC_ARG_TYPE (arg));
  pkl_ast_node arg_decl = NULL;

  arg_decl = pkl_ast_make_decl (PKL_PASS_AST,
                                PKL_AST_DECL_KIND_VAR,
                                arg_identifier,
                                dummy,
                                NULL /* source */);
  PKL_AST_LOC (arg_decl) = PKL_AST_LOC (arg);

  if (!pkl_env_register (PKL_TRANS_ENV,
                         PKL_PASS_AST,
                         PKL_ENV_NS_MAIN,
                         PKL_AST_IDENTIFIER_POINTER (arg_identifier),
                         arg_decl))
    {
      PKL_ICE (PKL_AST_LOC (arg_identifier),
               "transl: duplicated argument name `%s' in function declaration",
               PKL_AST_IDENTIFIER_POINTER (arg_identifier));
      PKL_PASS_ERROR;
    }
}
PKL_PHASE_END_HANDLER

/* Do not traverse type names: the lexical environment of the aliased
   type has been already constructed from the pertinent
   declaration.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_type_alias)
{
  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/* Array types conform an unescapable context.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_type_array)
{
  PKL_TRANS_PUSH_UNESCAPABLE;
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_type_array)
{
  PKL_TRANS_POP_ESCAPABLE;
}
PKL_PHASE_END_HANDLER

/* Struct type specifiers introduce a lexical level.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_type_struct)
{
  PKL_TRANS_ENV = pkl_env_push_frame (PKL_TRANS_ENV);
  if (PKL_TRANS_FUNCTION)
    PKL_TRANS_FUNCTION->back++;
  PKL_TRANS_PUSH_UNESCAPABLE;

  /* Register dummies for the locals used in
     pkl-gen.pks:struct_mapper, excluding OFFSET.  */
  {
    int i;
    for (i = 0; i < 5; ++i)
      {
        char *name;
        pkl_ast_node id;
        pkl_ast_node decl;

        asprintf (&name, "@*UNUSABLE_OFF_%d*@", i);
        id = pkl_ast_make_identifier (PKL_PASS_AST, name);
        decl = pkl_ast_make_decl (PKL_PASS_AST,
                                  PKL_AST_DECL_KIND_VAR,
                                  id, NULL /* initial */,
                                  NULL /* source */);

        if (!pkl_env_register (PKL_TRANS_ENV, PKL_PASS_AST, PKL_ENV_NS_MAIN, name, decl))
          {
            PKL_ICE (PKL_AST_LOC (PKL_PASS_NODE),
                     "transl: could not register dummy in pkl_transl_pr_type_struct");
            PKL_PASS_ERROR;
          }
      }
  }

  /* Now register OFFSET with a type of offset<uint<64>,1> */
  {
    pkl_ast_node decl, type;
    pkl_ast_node offset_identifier
      = pkl_ast_make_identifier (PKL_PASS_AST, "OFFSET");
    pkl_ast_node offset_magnitude
      = pkl_ast_make_integer (PKL_PASS_AST, 0);
    pkl_ast_node offset_unit
      = pkl_ast_make_integer (PKL_PASS_AST, 1);
    pkl_ast_node offset;

    type = pkl_ast_make_integral_type (PKL_PASS_AST, 64, 0);
    PKL_AST_TYPE (offset_magnitude) = ASTREF (type);
    PKL_AST_TYPE (offset_unit) = ASTREF (type);

    offset = pkl_ast_make_offset (PKL_PASS_AST,
                                  offset_magnitude,
                                  offset_unit);
    type = pkl_ast_make_offset_type (PKL_PASS_AST,
                                     type,
                                     offset_unit,
                                     NULL /* ref_type */);
    PKL_AST_TYPE (offset) = ASTREF (type);

    decl = pkl_ast_make_decl (PKL_PASS_AST,
                              PKL_AST_DECL_KIND_VAR,
                              offset_identifier,
                              offset,
                              NULL /* source */);

    if (!pkl_env_register (PKL_TRANS_ENV,
                           PKL_PASS_AST,
                           PKL_ENV_NS_MAIN,
                           PKL_AST_IDENTIFIER_POINTER (offset_identifier),
                           decl))
      {
        PKL_ICE (PKL_AST_LOC (PKL_PASS_NODE),
                 "transl: error registerting OFFSET in pkl_transl_pr_type_struct");
        PKL_PASS_ERROR;
      }
  }
}
PKL_PHASE_END_HANDLER

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_type_struct)
{
  PKL_TRANS_ENV = pkl_env_pop_frame (PKL_TRANS_ENV);
  if (PKL_TRANS_FUNCTION)
    PKL_TRANS_FUNCTION->back--;
  PKL_TRANS_POP_ESCAPABLE;
}
PKL_PHASE_END_HANDLER


/* This handler determines and sets the lexical address (back, over)
   of the variable reference according to the compile-time
   environment.

   It also determines the lexical depth of the variable within its
   containing function or method, if any.

   Finally, it also implements some checks that use that information,
   that perhaps would be best implemented in a separated "anall"
   pass.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_var)
{
  pkl_ast_node var = PKL_PASS_NODE;
  pkl_ast_node var_name = PKL_AST_VAR_NAME (var);

  if (PKL_AST_VAR_BACK (var) != -1 && PKL_AST_VAR_OVER (var) != -1)
    PKL_PASS_DONE;

  if (!pkl_env_lookup (PKL_TRANS_ENV, PKL_ENV_NS_MAIN,
                       PKL_AST_IDENTIFIER_POINTER (var_name),
                       &PKL_AST_VAR_BACK (var),
                       &PKL_AST_VAR_OVER (var)))
    {
      PKL_ERROR (PKL_AST_LOC (var), "undefined variable '%s'",
                 PKL_AST_IDENTIFIER_POINTER (var_name));
      PKL_PASS_ERROR;
    }

  /* Set the lexical depth of the variable within its containing
     function.  */
  if (PKL_TRANS_FUNCTION)
    PKL_AST_VAR_FUNCTION_BACK (var) = PKL_TRANS_FUNCTION->back;

  /* A method can only refer to struct fields and methods defined in
     the same struct.  */
  {
    pkl_ast_node var = PKL_PASS_NODE;
    pkl_ast_node var_decl = PKL_AST_VAR_DECL (var);
    pkl_ast_node var_function = PKL_AST_VAR_FUNCTION (var);

    const int in_method_p
      = var_function && PKL_AST_FUNC_METHOD_P (var_function);
    const int var_is_method_p
      = (PKL_AST_DECL_KIND (var_decl) == PKL_AST_DECL_KIND_FUNC
         && PKL_AST_FUNC_METHOD_P (PKL_AST_DECL_INITIAL (var_decl)));
    const int var_is_field_p
      = PKL_AST_DECL_STRUCT_FIELD_P (var_decl);

    if (in_method_p && (var_is_field_p || var_is_method_p))
      {
        const char *what
          = var_is_method_p ? "method" : "field";

        int back = PKL_AST_VAR_BACK (var);
        int function_back = PKL_AST_VAR_FUNCTION_BACK (var);

        if (back != (function_back + 1))
          {
            pkl_ast_node var_name = PKL_AST_VAR_NAME (var);

            PKL_ERROR (PKL_AST_LOC (var),
                       "referred %s `%s', not in this struct back=%d function_back=%d",
                       what, PKL_AST_IDENTIFIER_POINTER (var_name),
                       back, function_back);
            PKL_PASS_ERROR;
          }
      }
  }
}
PKL_PHASE_END_HANDLER

/* Annotate return statements with the number of drops and popes they
   have to perform according to the current lexical environment.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_return_stmt)
{
  pkl_ast_node stmt = PKL_PASS_NODE;

  if (!PKL_TRANS_FUNCTION)
    {
      PKL_ICE (PKL_AST_LOC (PKL_PASS_NODE),
               "transl: RETURN is not inside a function");
      PKL_PASS_ERROR;
    }

  PKL_AST_RETURN_STMT_NDROPS (stmt) = PKL_TRANS_FUNCTION->ndrops;
}
PKL_PHASE_END_HANDLER

/* Declarations shall be made available to the lexical environment.
   If the declaration is a `defun', then the identifier shall be
   available for the function body to allow for recursive calls.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_decl)
{
  /* The top-level environment is to be preserved.  */
  if (pkl_env_toplevel_p (PKL_TRANS_ENV))
    PKL_PASS_DONE;
  else
    {
      pkl_ast_node decl = PKL_PASS_NODE;
      pkl_ast_node decl_name = PKL_AST_DECL_NAME (decl);
      pkl_ast_node decl_initial = PKL_AST_DECL_INITIAL (decl);
      int decl_kind = PKL_AST_DECL_KIND (decl);

      if (decl_kind != PKL_AST_DECL_KIND_FUNC)
        PKL_PASS_SUBPASS (decl_initial);

      if (!pkl_env_register (PKL_TRANS_ENV,
                             PKL_PASS_AST,
                             PKL_ENV_NS_MAIN,
                             PKL_AST_IDENTIFIER_POINTER (decl_name),
                             decl))
        {
          PKL_ICE (PKL_AST_LOC (decl),
                   "transl: entity `%s' is already in the lexical environment",
                   PKL_AST_IDENTIFIER_POINTER (decl_name));
          PKL_PASS_ERROR;
        }

      if (decl_kind == PKL_AST_DECL_KIND_FUNC)
        PKL_PASS_SUBPASS (decl_initial);
    }

  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

/* Annotate break and continue statements with their lexical nesting
   level with respect to the enclosing escapable construct.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_ps_break_continue_stmt)
{
  if (PKL_TRANS_ESCAPABLE)
    {
      PKL_AST_BREAK_CONTINUE_STMT_NFRAMES (PKL_PASS_NODE)
        = PKL_TRANS_ESCAPABLE->nframes;
    }
}
PKL_PHASE_END_HANDLER

/* Struct type fields shall register the field name in the lexical
   environment.  */

PKL_PHASE_BEGIN_HANDLER (pkl_transl_pr_struct_type_field)
{
  pkl_ast_node field = PKL_PASS_NODE;
  pkl_ast_node name = PKL_AST_STRUCT_TYPE_FIELD_NAME (field);
  pkl_ast_node type = PKL_AST_STRUCT_TYPE_FIELD_TYPE (field);
  pkl_ast_node size = PKL_AST_STRUCT_TYPE_FIELD_SIZE (field);
  pkl_ast_node initializer = PKL_AST_STRUCT_TYPE_FIELD_INITIALIZER (field);
  pkl_ast_node optcond_pre = PKL_AST_STRUCT_TYPE_FIELD_OPTCOND_PRE (field);
  pkl_ast_node constraint = PKL_AST_STRUCT_TYPE_FIELD_CONSTRAINT (field);
  pkl_ast_node optcond_post = PKL_AST_STRUCT_TYPE_FIELD_OPTCOND_POST (field);
  pkl_ast_node label = PKL_AST_STRUCT_TYPE_FIELD_LABEL (field);

  if (name)
    PKL_PASS_SUBPASS (name);
  if (!PKL_AST_TYPE_NAME (type))
    PKL_PASS_SUBPASS (type);
  if (size)
    PKL_PASS_SUBPASS (size);
  if (initializer)
    PKL_PASS_SUBPASS (initializer);
  if (optcond_pre)
    PKL_PASS_SUBPASS (optcond_pre);

  /* Register the field, but only if it is not a computed field.  */
  if (!PKL_AST_STRUCT_TYPE_FIELD_COMPUTED_P (field))
  {
    pkl_ast_node dummy, decl;
    pkl_ast_node identifier
      = (name != NULL ? name : pkl_ast_make_identifier (PKL_PASS_AST, ""));

    dummy = pkl_ast_make_integer (PKL_PASS_AST, 0);
    PKL_AST_TYPE (dummy) = ASTREF (type);
    decl = pkl_ast_make_decl (PKL_PASS_AST,
                              PKL_AST_DECL_KIND_VAR,
                              identifier, dummy,
                              NULL /* source */);
    PKL_AST_DECL_STRUCT_FIELD_P (decl) = 1;
    PKL_AST_LOC (decl) = PKL_AST_LOC (field);

    if (!pkl_env_register (PKL_TRANS_ENV,
                           PKL_PASS_AST,
                           PKL_ENV_NS_MAIN,
                           PKL_AST_IDENTIFIER_POINTER (identifier),
                           decl))
      {
        PKL_ICE (PKL_AST_LOC (field),
                 "transl: duplicated struct element '%s'",
                 PKL_AST_IDENTIFIER_POINTER (name));
        PKL_PASS_ERROR;
      }
  }

  if (constraint)
    PKL_PASS_SUBPASS (constraint);
  if (optcond_post)
    PKL_PASS_SUBPASS (optcond_post);
  if (label)
    PKL_PASS_SUBPASS (label);
  PKL_PASS_BREAK;
}
PKL_PHASE_END_HANDLER

struct pkl_phase pkl_phase_transl =
  {
    .initialize = pkl_trans_initialize,
    .finalize = pkl_trans_finalize,

    PKL_PHASE_PR_HANDLER (PKL_AST_PROGRAM, pkl_trans_pr_program),
    PKL_PHASE_PS_HANDLER (PKL_AST_VAR, pkl_transl_ps_var),
    PKL_PHASE_PR_HANDLER (PKL_AST_COMP_STMT, pkl_transl_pr_comp_stmt),
    PKL_PHASE_PS_HANDLER (PKL_AST_COMP_STMT, pkl_transl_ps_comp_stmt),
    PKL_PHASE_PR_HANDLER (PKL_AST_TRY_STMT, pkl_transl_pr_try_stmt),
    PKL_PHASE_PR_HANDLER (PKL_AST_LOOP_STMT, pkl_transl_pr_loop_stmt),
    PKL_PHASE_PR_HANDLER (PKL_AST_FUNC, pkl_transl_pr_func),
    PKL_PHASE_PS_HANDLER (PKL_AST_FUNC_ARG, pkl_transl_ps_func_arg),
    PKL_PHASE_PS_HANDLER (PKL_AST_RETURN_STMT, pkl_transl_ps_return_stmt),
    PKL_PHASE_PR_HANDLER (PKL_AST_DECL, pkl_transl_pr_decl),
    PKL_PHASE_PR_HANDLER (PKL_AST_STRUCT_TYPE_FIELD, pkl_transl_pr_struct_type_field),
    PKL_PHASE_PS_HANDLER (PKL_AST_BREAK_CONTINUE_STMT, pkl_transl_ps_break_continue_stmt),
    PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_ALIAS, pkl_transl_pr_type_alias),
    PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_transl_pr_type_array),
    PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_ARRAY, pkl_transl_ps_type_array),
    PKL_PHASE_PR_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_transl_pr_type_struct),
    PKL_PHASE_PS_TYPE_HANDLER (PKL_TYPE_STRUCT, pkl_transl_ps_type_struct),
  };
