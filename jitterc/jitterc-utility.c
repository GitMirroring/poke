/* Jitter: utility functions for jitterc.

   Copyright (C) 2017 Luca Saiu
   Updated in 2019 by Luca Saiu
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


/* Include the Gnulib header. */
#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <xalloc.h>
#include <gl_xlist.h>
#include <gl_array_list.h>

#include <jitter/jitter-fatal.h>
#include "jitterc-utility.h"


/* Gnulib list utility functions.
 * ************************************************************************** */

gl_list_t
jitterc_make_empty_list (void)
{
  return gl_list_nx_create_empty (GL_ARRAY_LIST, NULL, NULL, NULL, true);
}

gl_list_t
jitterc_clone_list (const gl_list_t list)
{
  gl_list_t res
    = gl_list_nx_create_empty (GL_ARRAY_LIST, NULL, NULL, NULL, true);
  size_t size = gl_list_size (list);
  int i;
  for (i = 0; i < size; i ++)
    gl_list_add_last (res, gl_list_get_at (list, i));
  return res;
}

void
jitterc_clone_list_from (gl_list_t to, const gl_list_t from)
{
  size_t size = gl_list_size (from);
  int i;
  for (i = 0; i < size; i ++)
    gl_list_add_last (to, gl_list_get_at (from, i));
}

void
jitterc_empty_list (gl_list_t list)
{
  size_t size = gl_list_size (list);
  int i;
  for (i = size - 1; i >= 0; i --)
    gl_list_remove_at (list, i);
}




/* Unique-string-list utility functions, based on Gnulib lists.
 * ************************************************************************** */

bool
jitterc_list_has_string (gl_list_t list, const char *a_string)
{
  /* Scan the list, and return as soon as we find the first matching element. */
  size_t length = gl_list_size (list);
  int i;
  for (i = 0; i < length; i ++)
    if (! strcmp ((char *) gl_list_get_at (list, i), a_string))
      return true;

  /* We didn't find a matching element. */
  return false;
}

void
jitterc_list_add_string_unique (gl_list_t list, const char *a_string)
{
  if (! jitterc_list_has_string (list, a_string))
    gl_list_add_last (list, a_string);
}
