
/* pokefmt.l - Template system to embed Poke code in files.  */

/* Copyright (C) 2023 Mohammad-Reza Nabipoor.  */

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

/* clang-format off */

%option noyywrap 8bit warn nodefault noinput nounput reentrant
%option outfile="pokefmt.c"
%option header-file="pokefmt.h"
%option extra-type="struct source_range *"

%top{
#include <config.h>
}

%x POKE_STMT POKE_EXPR

%{
#define POKE_STMT_PARTIAL (POKE_EXPR + 1)
#define POKE_EXPR_PARTIAL (POKE_EXPR + 2)
%}

%{
/* line[1], col[1] pair represents the current location.
   line[0], col[0] pair represents the beginning of current/previous
   Poke code section.

   The range is a half-open interval [(l0,c0), (l1,c1)).  */
struct source_range
{
  int line[2];
  int col[2];
};
%}

%%

"\\%{"  {
          yyextra->col[1] += 3;
          ECHO;
        }
"\\%("  {
          yyextra->col[1] += 3;
          ECHO;
        }

"%{"    {
          yyextra->line[0] = yyextra->line[1];
          yyextra->col[0] = yyextra->col[1];
          yyextra->col[1] += 2;
          BEGIN (POKE_STMT);
        }
"%("    {
          yyextra->line[0] = yyextra->line[1];
          yyextra->col[0] = yyextra->col[1];
          yyextra->col[1] += 2;
          BEGIN (POKE_EXPR);
        }

<POKE_STMT>{

[^}\n]*       |
"}"+[^}%\n]*  {
                yyextra->col[1] += yyleng;
                return POKE_STMT_PARTIAL;
              }
\n+           {
                yyextra->line[1] += yyleng;
                yyextra->col[1] = 1;
                return POKE_STMT_PARTIAL;
              }
"}"+"%"       {
                yyextra->col[1] += yyleng;
                BEGIN (INITIAL);
                return POKE_STMT;
              }

}

<POKE_EXPR>{

[^)\n]*       |
")"+[^)%\n]*  {
                yyextra->col[1] += yyleng;
                return POKE_EXPR_PARTIAL;
              }
\n+           {
                yyextra->line[1] += yyleng;
                yyextra->col[1] = 1;
                return POKE_EXPR_PARTIAL;
              }
")"+"%"       {
                yyextra->col[1] += yyleng;
                BEGIN (INITIAL);
                return POKE_EXPR;
              }

}

.   {
      ++yyextra->col[1];
      ECHO;
    }
\n  {
      ++yyextra->line[1];
      yyextra->col[1] = 1;
      ECHO;
    }

%%

// clang-format on

#include <assert.h>
#include <err.h>
#include <string.h>

#include "configmake.h"
#include "libpoke.h"
#include "pk-utils.h"

struct poke
{
  pk_compiler compiler;
  FILE *output;
  char *filename;
};

void poke_init (struct poke *, const char *poke_src_file, FILE *output);
void poke_free (struct poke *);

static void poke_stmt (struct poke *, const char *poke_src,
                       const struct source_range *);
static void poke_expr (struct poke *, const char *poke_src,
                       const struct source_range *);

int
main (int argc, char *argv[])
{
  struct poke poke;
  yyscan_t scanner;
  int section_type;
  struct source_range range = {
    .line = { 1, 1 },
    .col = { 1, 1 },
  };
  char *poke_src;
  size_t poke_src_len;
  size_t poke_src_cap;

  poke_src_len = 0;
  poke_src_cap = 1024;
  poke_src = malloc (poke_src_cap);
  if (poke_src == NULL)
    err (1, "malloc() failed");

#define SRC_APPEND(src, srclen)                                               \
  do                                                                          \
    {                                                                         \
      size_t _srclen = (srclen);                                              \
      size_t _rem = poke_src_cap - poke_src_len;                              \
      if (_rem < _srclen + 1)                                                 \
        {                                                                     \
          poke_src_cap += _srclen + 1024;                                     \
          poke_src = realloc (poke_src, poke_src_cap);                        \
          if (poke_src == NULL)                                               \
            err (1, "realloc() failed");                                      \
        }                                                                     \
      memcpy (poke_src + poke_src_len, (src), _srclen);                       \
      poke_src_len += _srclen;                                                \
      poke_src[poke_src_len] = '\0';                                          \
    }                                                                         \
  while (0)

  poke_init (&poke, /*FIXME*/ NULL, stdout);

  if (yylex_init_extra (&range, &scanner) != 0)
    err (1, "yylex_init_extra() failed");

  while ((section_type = yylex (scanner)))
    {
      char *chunk;
      int chunk_len;

      switch (section_type)
        {
        case POKE_STMT:
        case POKE_EXPR:
        case POKE_STMT_PARTIAL:
        case POKE_EXPR_PARTIAL:
          chunk = yyget_text (scanner);
          chunk_len = yyget_leng (scanner);
          break;
        default:
          PK_UNREACHABLE ();
        }

      switch (section_type)
        {
        case POKE_STMT:
        case POKE_EXPR:
          chunk_len -= 2; /* Skip "}%" or ")%".  */
        }
      SRC_APPEND (chunk, chunk_len);

      switch (section_type)
        {
        case POKE_STMT:
          poke_stmt (&poke, poke_src, &range);
          poke_src_len = 0;
          break;
        case POKE_EXPR:
          SRC_APPEND (";", 1);
          poke_expr (&poke, poke_src, &range);
          poke_src_len = 0;
          break;
        }
    }

  free (poke_src);
  yylex_destroy (scanner);
  poke_free (&poke);
  return 0;
}

//--- poke

// terminal IO functions
static void
tif_flush (void)
{
  fflush (stdout);
}
static void
tif_puts (const char *s)
{
  printf ("%s", s);
}
static void
tif_printf (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
}
static void
tif_indent (unsigned int level, unsigned int step)
{
  putchar ('\n');
  for (unsigned int i = 0; i < step * level; ++i)
    putchar (' ');
}
static void
tif_class (const char *name)
{
  (void)name;
}
static int
tif_class_end (const char *name)
{
  (void)name;
  return 1;
}
static void
tif_hlink (const char *name, const char *id)
{
  (void)name;
  (void)id;
}
static int
tif_hlink_end (void)
{
  return 1;
}
static struct pk_color
tif_color (void)
{
  static struct pk_color c = {
    .red = 0,
    .green = 0,
    .blue = 0,
  };
  return c;
}
static struct pk_color
tif_bgcolor (void)
{
  static struct pk_color c = {
    .red = 255,
    .green = 255,
    .blue = 255,
  };
  return c;
}
static void
tif_color_set (struct pk_color c)
{
  (void)c;
}
static void
tif_bgcolor_set (struct pk_color c)
{
  (void)c;
}

void
poke_init (struct poke *pk, const char *poke_src_file, FILE *output)
{
  static struct pk_term_if tif = {
    .flush_fn = tif_flush,
    .puts_fn = tif_puts,
    .printf_fn = tif_printf,
    .indent_fn = tif_indent,
    .class_fn = tif_class,
    .end_class_fn = tif_class_end,
    .hyperlink_fn = tif_hlink,
    .end_hyperlink_fn = tif_hlink_end,
    .get_color_fn = tif_color,
    .get_bgcolor_fn = tif_bgcolor,
    .set_color_fn = tif_color_set,
    .set_bgcolor_fn = tif_bgcolor_set,
  };
  int ret;
  pk_val pexc;

  pk->filename = strdup ("<stdin>");
  pk->output = output;
  pk->compiler = pk_compiler_new (&tif);
  if (pk->compiler == NULL)
    errx (1, "pk_compiler_new() failed");

  /* Add load paths to the incremental compiler.  */
  {
    pk_val load_path = pk_decl_val (pk->compiler, "load_path");
    const char *poke_datadir = getenv ("POKEDATADIR");
    const char *poke_picklesdir = getenv ("POKEPICKLESDIR");
    const char *poke_load_path = getenv ("POKE_LOAD_PATH");
    char *pokefmt_appdir = getenv ("POKEFMTAPPDIR");
    char *user_load_path = NULL;
    char *new_path;

    if (poke_datadir == NULL)
      poke_datadir = PKGDATADIR;
    if (pokefmt_appdir == NULL)
      pokefmt_appdir = "%DATADIR%/pokefmt";
    if (poke_picklesdir == NULL)
      poke_picklesdir = "%DATADIR%/pickles";
    if (poke_load_path)
      {
        user_load_path = pk_str_concat (poke_load_path, ":", NULL);
        if (user_load_path == NULL)
          err (1, "pk_str_concat() failed");
      }

    new_path = pk_str_concat (user_load_path ? user_load_path : "",
                              pk_string_str (load_path), ":", pokefmt_appdir,
                              ":", poke_picklesdir, NULL);
    if (new_path == NULL)
      err (1, "pk_str_concat() failed");
    pk_decl_set_val (pk->compiler, "load_path", pk_make_string (new_path));

    free (new_path);
    free (user_load_path);
  }

  if (pk_load (pk->compiler, "pokefmt") != PK_OK)
    errx (1, "pk_load() failed for pokefmt.pk");

  if (poke_src_file)
    {
      ret = pk_compile_file (pk->compiler, poke_src_file, &pexc);
      if (ret != PK_OK)
        errx (1, "pk_compile_file() failed for user-provided file (%s)",
              poke_src_file);
      else if (pexc != PK_NULL)
        /* FIXME improve the error message.  */
        errx (1, "unhandled exception while running user-provided file (%s)",
              poke_src_file);
    }

  if (pk_decl_val (pk->compiler, "pokefmt_expr_printer") == PK_NULL)
    errx (1, "pokefmt_expr_printer Poke function is not available");
}

void
poke_free (struct poke *pk)
{
  free (pk->filename);
  pk_compiler_free (pk->compiler);
}

static void
poke_stmt (struct poke *poke, const char *poke_src,
           const struct source_range *r)
{
  int res;
  pk_val exc;

  assert (poke_src != NULL);

  res = pk_compile_buffer_with_loc (
      poke->compiler, poke_src, poke->filename, r->line[0],
      r->col[0] + /* Skip "%{" or "%(". */ 2, NULL, &exc);
  if (res != PK_OK)
    errx (
        1,
        "pk_compile_buffer() failed for Poke code in range %d,%d-%d,%d: '%s'",
        r->line[0], r->col[0], r->line[1], r->col[1], poke_src);
  else if (exc != PK_NULL)
    /* TODO improve the error message.  */
    errx (1,
          "unhandled exception happend during execution of Poke code in range"
          " %d,%d-%d,%d: '%s'",
          r->line[0], r->col[0], r->line[1], r->col[1], poke_src);
}

static void
poke_expr (struct poke *poke, const char *poke_src,
           const struct source_range *r)
{
  int res;
  pk_val val, exc, printer, printer_ret, printer_exc;

  assert (poke_src != NULL);

  res = pk_compile_statement_with_loc (
      poke->compiler, poke_src, poke->filename, r->line[0],
      r->col[0] + /* Skip "%{" or "%(". */ 2, NULL, &val, &exc);
  if (res != PK_OK)
    /* TODO add location info.  */
    errx (1,
          "pk_compile_statement() failed for Poke expression in range "
          "%d,%d-%d,%d: '%s'",
          r->line[0], r->col[0], r->line[1], r->col[1], poke_src);
  else if (exc != PK_NULL)
    /* TODO improve the error message.  */
    errx (1,
          "unhandled exception happend during execution of Poke code in range"
          " %d,%d-%d,%d: '%s'",
          r->line[0], r->col[0], r->line[1], r->col[1], poke_src);
  else if (val == PK_NULL)
    errx (1,
          "expected a Poke expression, got a Poke statement in Poke code in "
          "range %d,%d-%d,%d: '%s'",
          r->line[0], r->col[0], r->line[1], r->col[1], poke_src);

  printer = pk_decl_val (poke->compiler, "pokefmt_expr_printer");
  res = pk_call (poke->compiler, printer, &printer_ret, &printer_exc,
                 /*narg*/ 1, val);
  if (res != PK_OK || printer_exc != PK_NULL)
    errx (1, "pk_call() failed for pokefmt_expr_printer during printing "
         "the result of Poke expression in range %d,%d-%d,%d: '%s'",
         r->line[0], r->col[0], r->line[1], r->col[1], poke_src);
}
