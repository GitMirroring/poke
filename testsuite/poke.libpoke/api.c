/* api.c -- Unit tests for the libpoke public API */

/* Copyright (C) 2020, 2021, 2022, 2023 The poke authors */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include "read-file.h"
#include "libpoke.h"

/* DejaGnu should not use gnulib's vsnprintf replacement here.  */
#undef vsnprintf
#include <dejagnu.h>

#include "term-if.h"

#define STREQ(a, b) (strcmp ((a), (b)) == 0)

#define T(name, cond)                                                         \
  do                                                                          \
    {                                                                         \
      if (cond)                                                               \
        pass (name);                                                          \
      else                                                                    \
        fail (name);                                                          \
    }                                                                         \
  while (0)

static pk_compiler
test_pk_compiler_new (void)
{
  pk_compiler pkc;
  struct pk_term_if tif;

  pkc = pk_compiler_new (NULL);
  T ("pk_compiler_new_1", pkc == NULL);

#define TT(n)                                                                 \
  do                                                                          \
    {                                                                         \
      pkc = pk_compiler_new (&tif);                                           \
      T ("pk_compiler_new_" #n, pkc == NULL);                                 \
    }                                                                         \
  while (0)

  memset (&tif, 0, sizeof (struct pk_term_if));
  TT (2);

  tif.flush_fn = poke_term_if.flush_fn;
  TT (3);

  tif.puts_fn = poke_term_if.puts_fn;
  TT (5);

  tif.printf_fn = poke_term_if.printf_fn;
  TT (6);

  tif.indent_fn = poke_term_if.indent_fn;
  TT (7);

  tif.class_fn = poke_term_if.class_fn;
  TT (8);

  tif.end_class_fn = poke_term_if.end_class_fn;
  TT (9);

  tif.hyperlink_fn = poke_term_if.hyperlink_fn;
  TT (10);

  tif.end_hyperlink_fn = poke_term_if.end_hyperlink_fn;
  TT (11);

  tif.get_color_fn = poke_term_if.get_color_fn;
  TT (12);

  tif.get_bgcolor_fn = poke_term_if.get_bgcolor_fn;
  TT (13);

  tif.set_color_fn = poke_term_if.set_color_fn;
  TT (14);

  tif.set_bgcolor_fn = poke_term_if.set_bgcolor_fn;
  pkc = pk_compiler_new (&tif);
  T ("pk_compiler_new_15", pkc != NULL);

#undef TT

  return pkc;
}

static void
test_pk_compiler_free (pk_compiler pkc)
{
  pk_compiler_free (NULL);
  pk_compiler_free (pkc);
}

static void
test_pk_keyword_p (pk_compiler pkc)
{
  T ("pk_keyword_p_1", pk_keyword_p (pkc, "if"));
  T ("pk_keyword_p_2", !pk_keyword_p (pkc, "foo"));
}

static void
test_pk_load (pk_compiler pkc)
{
  /* An invalid value for pk_val, just to make sure pk_load is
     modifying it.  */
  pk_val exception = 0;

  T ("pk_load_1", pk_load (pkc, "std", &exception) == PK_OK);
  T ("pk_load_1 exception", exception == PK_NULL);

  exception = 0;  /* Again resetting to an invalid value.  */

  T ("pk_load_2", pk_load (pkc, "a-module_which-does_not-exist",
                           &exception) == PK_ERROR);
  /* Still has the invalid value.  */
  T ("pk_load_1 exception", exception == 0);
}

static void
test_pk_ios (pk_compiler pkc)
{
#define N 4
  int ios_id[N];
  pk_ios ios[N];
  uint64_t flags = 0;

  ios_id[0] = pk_ios_open (pkc, "*foo*", flags, /*set_cur_p*/ 1);
  T ("pk_ios_open_1", ios_id[0] == 0);
  ios[0] = pk_ios_cur (pkc);
  T ("pk_ios_cur_1", ios[0] != NULL);

  ios_id[1] = pk_ios_open (pkc, "*foobar*", flags, /*set_cur_p*/ 0);
  T ("pk_ios_open_2", ios_id[1] == 1);
  T ("pk_ios_cur_2", ios[0] == pk_ios_cur (pkc));

  ios_id[2] = pk_ios_open (pkc, "*funfoo*", flags, /*set_cur_p*/ 0);
  T ("pk_ios_open_3", ios_id[2] == 2);
  T ("pk_ios_cur_3", ios[0] == pk_ios_cur (pkc));

  ios_id[3] = pk_ios_open (pkc, "*baz*", flags, /*set_cur_p*/ 0);
  T ("pk_ios_open_4", ios_id[3] == 3);
  T ("pk_ios_cur_4", ios[0] == pk_ios_cur (pkc));

  ios[1] = pk_ios_search_by_id (pkc, ios_id[1]);
  ios[2] = pk_ios_search_by_id (pkc, ios_id[2]);
  ios[3] = pk_ios_search_by_id (pkc, ios_id[3]);
  T ("pk_ios_search_by_id_1", ios[1] != NULL);
  T ("pk_ios_search_by_id_2", ios[2] != NULL);
  T ("pk_ios_search_by_id_3", ios[3] != NULL);

  T ("pk_ios_get_id_1", pk_ios_get_id (ios[0]) == ios_id[0]);
  T ("pk_ios_get_id_2", pk_ios_get_id (ios[1]) == ios_id[1]);
  T ("pk_ios_get_id_3", pk_ios_get_id (ios[2]) == ios_id[2]);
  T ("pk_ios_get_id_4", pk_ios_get_id (ios[3]) == ios_id[3]);

  T ("pk_ios_handler_1", pk_ios_handler (ios[0]) != NULL);
  T ("pk_ios_handler_2", pk_ios_handler (ios[1]) != NULL);
  T ("pk_ios_handler_3", pk_ios_handler (ios[2]) != NULL);
  T ("pk_ios_handler_4", pk_ios_handler (ios[3]) != NULL);

  T ("pk_ios_handler_5", STREQ (pk_ios_handler (ios[0]), "*foo*"));
  T ("pk_ios_handler_6", STREQ (pk_ios_handler (ios[1]), "*foobar*"));
  T ("pk_ios_handler_7", STREQ (pk_ios_handler (ios[2]), "*funfoo*"));
  T ("pk_ios_handler_8", STREQ (pk_ios_handler (ios[3]), "*baz*"));

  T ("pk_ios_search_1",
     pk_ios_search (pkc, "/some/non-existent/thing", PK_IOS_SEARCH_F_PARTIAL)
         == NULL);

  T ("pk_ios_search_2",
     pk_ios_search (pkc, "baz", PK_IOS_SEARCH_F_PARTIAL) == ios[3]);

  T ("pk_ios_search_3",
     pk_ios_search (pkc, "baz*", PK_IOS_SEARCH_F_PARTIAL) == ios[3]);

  T ("pk_ios_search_4",
     pk_ios_search (pkc, "*baz", PK_IOS_SEARCH_F_PARTIAL) == ios[3]);

  T ("pk_ios_search_5",
     pk_ios_search (pkc, "*baz*", PK_IOS_SEARCH_F_PARTIAL) == ios[3]);

  T ("pk_ios_search_6",
     pk_ios_search (pkc, "az", PK_IOS_SEARCH_F_PARTIAL) == ios[3]);

  T ("pk_ios_search_7",
     pk_ios_search (pkc, "z", PK_IOS_SEARCH_F_PARTIAL) == ios[3]);

  T ("pk_ios_search_8",
     pk_ios_search (pkc, "bar*", PK_IOS_SEARCH_F_PARTIAL) == ios[1]);

  T ("pk_ios_search_9",
     pk_ios_search (pkc, "bar", PK_IOS_SEARCH_F_PARTIAL) == ios[1]);

  T ("pk_ios_search_10",
     pk_ios_search (pkc, "foo", PK_IOS_SEARCH_F_PARTIAL) == NULL);

  T ("pk_ios_search_11",
     pk_ios_search (pkc, "ba", PK_IOS_SEARCH_F_PARTIAL) == NULL);

  T ("pk_ios_search_12",
     pk_ios_search (pkc, "", PK_IOS_SEARCH_F_EXACT) == NULL);

  T ("pk_ios_search_13",
     pk_ios_search (pkc, "", PK_IOS_SEARCH_F_PARTIAL) == NULL);

  for (int i = N; i != 0; --i)
    pk_ios_close (pkc, ios[i - 1]);

#undef N
}

int
main ()
{
  pk_compiler pkc;

  pkc = test_pk_compiler_new ();

  test_pk_keyword_p (pkc);
  test_pk_load (pkc);
  test_pk_ios (pkc);
  test_pk_compiler_free (pkc);

  return 0;
}
