/* Jitter: safe malloc wrappers.

   Copyright (C) 2017, 2020 Luca Saiu
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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#include "jitter-malloc.h"
#include "jitter-fatal.h"

#include <stdio.h>
#include <stdlib.h>


/* Safe malloc wrappers not using Gnulib, for minimality.
 * ************************************************************************** */

void *
jitter_xmalloc (size_t size_in_chars)
{
  /* Special case: return NULL if the requested size is zero.  The check for
     NULL below would fail without this extra check. */
  if (size_in_chars == 0)
    return NULL;

  /* If we arrived here then size_in_chars is not zero. */

  void *res = malloc (size_in_chars);
  if (res == NULL)
    jitter_fatal ("could not allocate %lu bytes\n",
                  (unsigned long) size_in_chars);

  return res;
}

void *
jitter_xrealloc (void *previous, size_t size_in_chars)
{
  /* See the comment in jitter_xmalloc above.  Again I have to also
     support the case where size_in_chars is zero. */
  if (size_in_chars == 0)
    {
      free (previous);
      return NULL;
    }

  /* If we arrived here then size_in_chars is not zero. */

  void *res = realloc (previous, size_in_chars);
  if (res == NULL)
    jitter_fatal ("could not reallocate %lu bytes\n",
                  (unsigned long) size_in_chars);

  return res;
}
