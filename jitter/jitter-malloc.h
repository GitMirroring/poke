/* Jitter: safe malloc wrappers.

   Copyright (C) 2017, 2020, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of Jitter.

   Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTER_MALLOC_H_
#define JITTER_MALLOC_H_

#include <stdlib.h>




/* Safe malloc wrappers, not using Gnulib for minimality.
 * ************************************************************************** */

/* Allocate char_no chars with malloc and return its result, as long as it is
   non-NULL (or the requested size is zero); fail fatally if allocation
   fails.
   This is a trivial wrapper around malloc which fails fatally on error, instead
   of returning a result to check.  */
void *
jitter_xmalloc (size_t char_no)
  __attribute__ ((__malloc__));

/* Allocate char_no chars with realloc in place of the pointed buffer and return
   realloc's result, as long as it is non-NULL (or the new requested size is
   zero); fail fatally on reallocation failure.
   This is a trivial wrapper around realloc which fails fatally on error,
   instead of returning a result to check.  */
void *
jitter_xrealloc (void *previous, size_t char_no)
  __attribute__ ((__warn_unused_result__));


#endif // #ifndef JITTER_MALLOC_H_
