/* Jitter: utility header for jitterc.

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


#ifndef JITTERC_UTILITY_H_
#define JITTERC_UTILITY_H_

/* Include the Gnulib header. */
#include <config.h>
#include <gl_list.h>


/* Gnulib list utility functions.
 * ************************************************************************** */

/* Return a fresh empty (array-) list. */
gl_list_t
jitterc_make_empty_list (void)
  __attribute__ ((returns_nonnull));

/* Return a freshly-allocated copy of the given (array-) list.  The copy
   contains a fresh spine and a copy of the elements, but such elements are
   pointers; therefore the copy shares data with the original. */
gl_list_t
jitterc_clone_list (const gl_list_t list)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Copy every element from the from into into the to list, at the end, without
   altering the original.  The to list contains a copy of the elements but such
   elements are pointers; therefore the copy shares data with the original. */
void
jitterc_clone_list_from (gl_list_t to, const gl_list_t from)
  __attribute__ ((nonnull (1, 2)));

/* Remove every element from the given list.  The elements are actually
   pointers, and the pointed objects are not freed. */
void
jitterc_empty_list (gl_list_t list)
  __attribute__ ((nonnull (1)));




/* Unique-string-list utility functions, based on Gnulib lists.
 * ************************************************************************** */

/* Return true iff the given list of strings contains at least one element
   structurally equal to the pointed string.  Linear worst-case complexity. */
bool
jitterc_list_has_string (gl_list_t list, const char *string)
  __attribute__ ((pure, nonnull (1, 2)));

/* Add the pointed string to the given unsorted list of strings if not already
   present.  The string is not cloned so the list, if modified, shares structure
   with the argument.  Linear worst-case complexity. */
void
jitterc_list_add_string_unique (gl_list_t list, const char *string)
  __attribute__ ((nonnull (1, 2)));

#endif // #ifndef JITTERC_UTILITY_H_
