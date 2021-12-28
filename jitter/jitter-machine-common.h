/* VM library: declarations for machine-specific functionality.

   Copyright (C) 2017 Luca Saiu
   Updated in 2021 by Luca Saiu
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


#ifndef JITTER_MACHINE_MACHINE_COMMON_H_
#define JITTER_MACHINE_MACHINE_COMMON_H_

#include <stdlib.h>


/* DCache flushing/ICache invalidation.
 * ************************************************************************** */

/* Every machine with assembly support should implement this
   icache-invalidation/dcache-flushing function.  If nothing more than GCC's
   __builtin___clear_cache is needed then the function can do nothing;
   __builtin___clear_cache is always called when flushing. */

/* Invalidate an interval within the instruction cache in some machine-specific
   way.  The generated code starts at the address pointed by from, and is
   byte_no bytes long. */
void
jitter_invalidate_icache (char *from, size_t byte_no)
  __attribute__ ((nonnull (1)));




/* Expressions known at compile time.
 * ************************************************************************** */

/* This functionality relies on GNU builtins, and is not used with dispatches
   meant to be more portable than GCC. */

/* Expand to a true value if the given expression is a constant known at compile
   time whose value is zero; expand to a false value otherwise.  The expression
   is not evaluated in the sense that no side effects are observable, independently
   from the result the macro expands to.

   This macro always expands to a compile-time constant. */
#define JITTER_IS_KNOWN_CONSTANT_ZERO(e)    \
  (__builtin_constant_p (e) && ((jitter_int)(e) == 0))


#endif // #ifndef JITTER_MACHINE_MACHINE_COMMON_H_

