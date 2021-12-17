/* Jitter utility: convenient integer parsing.

   Copyright (C) 2017 Luca Saiu
   Updated in 2020 by Luca Saiu
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


#include <stdlib.h>
#include <assert.h>

#include <jitter/jitter.h>
#include <jitter/jitter-fatal.h>
#include "jitter-parse-int.h"


int
jitter_string_to_long_long_inconvenient_radix (const char *s,
                                               jitter_long_long *result_pointer,
                                               unsigned radix)
{
  assert (s != NULL);
  assert (result_pointer != NULL);
  char *end;
  jitter_long_long res = jitter_strtoll (s, & end, 10);
  if (s [0] == '\0' || end [0] != '\0')
    return -1;

  * result_pointer = res;
  return 0;
}

int
jitter_string_to_long_long_inconvenient (const char *s,
                                         jitter_long_long *result_pointer)
{
  return jitter_string_to_long_long_inconvenient_radix (s, result_pointer, 10);
}

jitter_long_long
jitter_string_to_long_long_unsafe_radix (const char *s, unsigned radix)
{
  jitter_long_long res;
  int conversion_result
    = jitter_string_to_long_long_inconvenient_radix (s, &res, radix);
  if (conversion_result != 0)
    jitter_fatal ("could not convert to jitter_long_long with radix %u "
                  " (this should never happen)",
                  radix);
  return res;
}

jitter_long_long
jitter_string_to_long_long_unsafe (const char *s)
{
  return jitter_string_to_long_long_unsafe_radix (s, 10);
}

jitter_long_long
jitter_string_to_natural_radix (const char *s, unsigned radix)
{
  jitter_long_long res;
  int conversion_result
    = jitter_string_to_long_long_inconvenient_radix (s, &res, radix);
  if (conversion_result != 0 || res < 0)
    return -1;

  return res;
}

jitter_long_long
jitter_string_to_natural (const char *s)
{
  return jitter_string_to_natural_radix (s, 10);
}
