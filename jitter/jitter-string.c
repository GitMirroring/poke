/* Jitter: string utility functions: implementation.

   Copyright (C) 2017, 2021 Luca Saiu
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


#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "jitter-fatal.h"
#include "jitter-malloc.h"
#include "jitter-string.h"

char *
jitter_clone_string (const char *s)
{
  assert (s != NULL);
  size_t size = strlen (s);
  char * res = jitter_xmalloc (size + 1);
  strcpy (res, s);
  return res;
}

char *
jitter_escape_string (const char *s)
{
  assert (s != NULL);
  size_t s_length = strlen (s);
  /* No escape in C for one character (we are not generating
     octal/hexadecimal/Unicode escapes) is longer than two characters, so the
     growth ratio is bounded by two. */
  size_t initial_allocated_size
    = s_length * /* growth ratio */ 2 + /* '\0' */ 1;
  char *res = jitter_xmalloc (initial_allocated_size);
  size_t res_length = 0;

#define JITTER_ADD_BACKSLASH_THEN(character)  \
  do                                          \
    {                                         \
      res [res_length ++] = '\\';             \
      res [res_length ++] = (character);      \
    }                                         \
  while (false)

  /* Copy every chracters from s to res, keeping count of the new size of res
     and inserting escapes. */
  int i;
  char c;
  for (i = 0; i < s_length; i ++)
    switch (c = s [i])
      {
      case '\\':
      case '\'':
      case '\"': JITTER_ADD_BACKSLASH_THEN (c); break;

      case '\n': JITTER_ADD_BACKSLASH_THEN ('n'); break;
      case '\r': JITTER_ADD_BACKSLASH_THEN ('r'); break;
      case '\t': JITTER_ADD_BACKSLASH_THEN ('r'); break;
      case '\f': JITTER_ADD_BACKSLASH_THEN ('f'); break;

      case '\a': jitter_fatal ("alert characters should not be used here");
      case '\b': jitter_fatal ("backspace characters should not be used here");

      default:   res [res_length ++] = c;
      }
  res [res_length ++] = '\0';

  /* Trim the result. */
  return jitter_xrealloc (res, res_length + 1);
}
