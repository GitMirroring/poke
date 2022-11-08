/* Jitter utility: timing functions.

   Copyright (C) 2020 Luca Saiu
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


#include <jitter/jitter-time.h>

#include <time.h>
#include <sys/time.h>

#include <jitter/jitter.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-config.h>




/* Portability layer.
 * ************************************************************************** */

/* Any reasonable C library should implement struct timeval and the related C
   API.  However some sysems will implement some more precise alternative as
   well.  This funcionality is a thin layer intended to abstract over system
   differences, and use the more precise facility where available. */

/* Define a few simple macros allowing to abstract over the different APIs,
   which luckily are very similar. */
#if defined (JITTER_HAVE_CLOCK_GETTIME)
  /* This configuration has struct timespec , which allows, at least as far as
     the API goes, nanosecond resolution.  GNU/Linux appears to actually
     implement nanosecond resolution, if clock_getres is to be trusted:
       struct timespec foo;
       int q = clock_getres (CLOCK_REALTIME, & foo);
       printf (">>> %i: %20.10f\n", q, JITTER_TIME_TO_DOUBLE (& foo));
     This prints 
       >>> 0:         0.0000000010
     on my system. */
# define JITTER_TIME_STRUCT  \
    struct timespec
# define JITTER_TIME_FUNCTION(_jitter_pointer_arg)                  \
    (clock_gettime (CLOCK_REALTIME,                                 \
                    (JITTER_TIME_STRUCT *) (_jitter_pointer_arg)))
# define JITTER_TIME_TO_DOUBLE(_jitter_pointer_arg)  \
    ((_jitter_pointer_arg)->tv_nsec * 1e-9           \
     + (_jitter_pointer_arg)->tv_sec)
#else
  /* Fall back on struct timeval , which is only precise to the millisecond. */
# define JITTER_TIME_STRUCT  \
    struct timeval
# define JITTER_TIME_FUNCTION(_jitter_pointer_arg)                \
    (gettimeofday ((JITTER_TIME_STRUCT *) (_jitter_pointer_arg),  \
                   NULL))
# define JITTER_TIME_TO_DOUBLE(_jitter_pointer_arg)  \
    ((_jitter_pointer_arg)->tv_usec * 1e-6           \
     + (_jitter_pointer_arg)->tv_sec)
#endif




/* Implementation.
 * ************************************************************************** */

jitter_point_in_time
jitter_point_in_time_make (void)
{
  /* No initialisation is needed or performed. */
  return jitter_xmalloc (sizeof (JITTER_TIME_STRUCT));
}

void
jitter_point_in_time_destroy (jitter_point_in_time p)
{
  free (p);
}

void
jitter_time_set_now (jitter_point_in_time now)
{
  JITTER_TIME_FUNCTION (now);
}

double
jitter_time_subtract (jitter_point_in_time later_abstract,
                      jitter_point_in_time earlier_abstract)
{
  JITTER_TIME_STRUCT *later = later_abstract;
  JITTER_TIME_STRUCT *earlier = earlier_abstract;
  return JITTER_TIME_TO_DOUBLE (later) - JITTER_TIME_TO_DOUBLE (earlier);
}

double
jitter_time_subtract_from_now (jitter_point_in_time earlier)
{
  /* Measure the time now, writing into an automatic variable. */
  JITTER_TIME_STRUCT now;
  jitter_time_set_now (& now);

  /* I am keeping the result in a variable in the hope of preventing sibling
     call optimization from rewriting the code in a way that passes a pointer to
     an automatic variable in a dead frame. */
  double result = jitter_time_subtract (& now, earlier);
  return result;
}
