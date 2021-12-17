/* Jitter utility: timing function header.

   Copyright (C) 2020 Luca Saiu
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


#ifndef JITTER_TIME_H_
#define JITTER_TIME_H_

#include <jitter/jitter.h>
#include <jitter/jitter-config.h>


/* Introduction.
 * ************************************************************************** */

/* The functionality declared in this header allows the user to measure
   wall-clock time in some system-specific way which is intended to be
   sufficiently precise for benchmarks. */




/* Type definition and data structure.
 * ************************************************************************** */

/* An abstract type holding a pointer to some system-dependent memory structure
   encoding a specific point in time.  Notice that the type being defined is a
   pointer.  No function in this API requires to take *its* address.  */
typedef void *
jitter_point_in_time;

/* Allocate a new point-in-time structure, without setting its content.  Return
   a pointer to the new structure.
   Remark: since the purpose of this functionality is precise time measurement,
   you should notice that allocating a structure as part of the operation being
   timed is inefficient, and may pollute the measurement.  It is recommended to
   allocate point-in-time structures in advance and then simply write into them.
   The same structure can be overwritten and reused over and over again. */
jitter_point_in_time
jitter_point_in_time_make (void)
  __attribute__ ((__returns_nonnull__, __malloc__));

/* Destroy a point-in-time structure allocated by jitter_point_in_time_make. */
void
jitter_point_in_time_destroy (jitter_point_in_time)
  __attribute__ ((__nonnull__ (1)));




/* User functions.
 * ************************************************************************** */

/* Write the present time into the pointed point-in-time-structure, overwriting
   any previous value it contained. */
void
jitter_time_set_now (jitter_point_in_time now)
  __attribute__ ((__nonnull__ (1)));

/* Subtract the time stored in the structure pointed by the second argument
   from the time stored in the structure pointed by the first argument, and
   return the difference in seconds. */
double
jitter_time_subtract (jitter_point_in_time later,
                      jitter_point_in_time earlier)
  __attribute__ ((__nonnull__ (1), __nonnull__ (2), __pure__));

/* Subtract the time stored in the pointed structure from the present time,
   and return the difference in seconds.  This is more convenient than using
   jitter_time_set_now followed by jitter_time_subtract , and might also be
   very slightly more efficient. */
double
jitter_time_subtract_from_now (jitter_point_in_time earlier)
  __attribute__ ((__nonnull__ (1)));


#endif // #ifndef JITTER_TIME_H_
