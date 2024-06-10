/* pk-ios.c - IOS-related functions for poke.  */

/* Copyright (C) 2020, 2021, 2022, 2023, 2024 Jose E. Marchesi */

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
#include <regex.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include "xalloc.h"

#include "poke.h"
#include "pk-ios.h"
#include "pk-utils.h"
#include "pk-map.h"

void
pk_open_file_maps (int ios_id, const char *filename)
{
  if (pk_var_int ("pk_auto_map_p"))
  {
    int i;
    pk_val auto_map;
    pk_val nelem;

    auto_map = pk_decl_val (poke_compiler, "auto_map");
    if (auto_map == PK_NULL)
      pk_fatal ("auto_map is PK_NULL");

    nelem = pk_array_nelem (auto_map);
    for (i = 0; i < pk_uint_value (nelem); ++i)
      {
        pk_val auto_map_entry;
        pk_val regex, mapname;
        regex_t regexp;
        regmatch_t matches;

        auto_map_entry = pk_array_elem_value (auto_map, i);
        if (pk_val_kind (auto_map_entry) != PK_VAL_ARRAY
            || pk_uint_value (pk_array_nelem (auto_map_entry)) != 2)
          pk_fatal ("invalid entry in auto_val");

        regex = pk_array_elem_value (auto_map_entry, 0);
        if (pk_val_kind (regex) != PK_VAL_STRING)
          pk_fatal ("regexp should be a string in an auto_val entry");

        mapname = pk_array_elem_value (auto_map_entry, 1);
        if (pk_val_kind (mapname) != PK_VAL_STRING)
          pk_fatal ("mapname should be a string in an auto_val entry");

        if (regcomp (&regexp, pk_string_str (regex),
                     REG_EXTENDED | REG_NOSUB) != 0)
          {
            pk_term_class ("error");
            pk_puts ("error: ");
            pk_term_end_class ("error");

            pk_printf ("invalid regexp `%s' in auto_map.  Skipping entry.\n",
                       pk_string_str (regex));
          }
        else
          {
            if (regexec (&regexp, filename, 1, &matches, 0) == 0)
              {
                /* Load the map.  */

                const char *map_handler
                  = pk_map_resolve_map (pk_string_str (mapname),
                                        0 /* handler_p */);

                if (!map_handler)
                  {
                    pk_term_class ("error");
                    pk_puts ("warning: ");
                    pk_term_end_class ("error");

                    pk_printf ("auto-map: unknown map `%s'",
                               pk_string_str (mapname));
                    regfree (&regexp);
                    break;
                  }

                if (!pk_map_load_file (ios_id, map_handler, NULL))
                  {
                    pk_term_class ("error");
                    pk_puts ("error: ");
                    pk_term_end_class ("error");

                    pk_printf ("auto-map: loading `%s'\n",
                               pk_string_str (mapname));
                    regfree (&regexp);
                    break;
                  }

                if (poke_interactive_p && !poke_quiet_p
                    && ! pk_var_int ("pk_prompt_maps_p"))
                  pk_printf ("auto-map: map `%s' loaded\n",
                             pk_string_str (mapname));
              }

            regfree (&regexp);
          }
      }
  }

  return;
}

static struct pk_alien_token alien_token;

static struct pk_alien_token *
pk_ios_alien_token_handler (char delimiter,
                            const char *id, char **errmsg)
{
  pk_ios ios;
  char *handler;
  size_t id_len = strlen (id);

  assert (delimiter == '<');

  if (id_len < 3)
    goto error;

  /* Extract the IOS handler from the <...> string.  */
  handler = xmalloc (id_len - 2 + 1);
  memcpy (handler, id + 1, id_len - 2);
  handler[id_len - 2] = '\0';

  ios = pk_ios_search (poke_compiler, handler, PK_IOS_SEARCH_F_PARTIAL);
  if (ios)
    {
      /* The IO space alien token resolves to an int<32>.  */
      alien_token.kind = PK_ALIEN_TOKEN_INTEGER;
      alien_token.value.integer.width = 32;
      alien_token.value.integer.signed_p = 1;
      alien_token.value.integer.magnitude = pk_ios_get_id (ios);

      return &alien_token;
    }

error:
  *errmsg = xstrdup ("invalid IO space");
  return NULL;
}

void
pk_ios_init ()
{
  /* Install the handler for delimited alien tokens that recognizes IO
     spaced by handler.  */
  pk_set_alien_dtoken_fn (poke_compiler, pk_ios_alien_token_handler);
}

void
pk_ios_shutdown ()
{
  /* Nothing to do there for now.  */
}
