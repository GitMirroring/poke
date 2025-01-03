/* pk-ios.c - IOS-related functions for poke.  */

/* Copyright (C) 2020, 2021, 2022, 2023, 2024, 2025 Jose E. Marchesi */

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
