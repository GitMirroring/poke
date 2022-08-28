/* Jitter: float reinterpret-cast conversion test.

   Copyright (C) 2022 Luca Saiu
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

#include <jitter/jitter.h>
#include <jitter/jitter-fatal.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <math.h>

/* It is interesting to look at what code the C compiler generates for these
   functions. */
jitter_uint
float_to_uint (jitter_float x)
{
  return JITTER_REINTERPRET_CAST_FLOAT_TO_UINT (x);
}
jitter_int
float_to_int (jitter_float x)
{
  return JITTER_REINTERPRET_CAST_FLOAT_TO_INT (x);
}
jitter_float
uint_to_float (jitter_uint x)
{
  return JITTER_REINTERPRET_CAST_UINT_TO_FLOAT (x);
}
jitter_float
int_to_float (jitter_int x)
{
  return JITTER_REINTERPRET_CAST_INT_TO_FLOAT (x);
}

void
test (const char *name, double x)
{
  jitter_float xf = x;
  jitter_uint  xu = float_to_uint (xf);
  jitter_int   xi = float_to_int (xf);
  jitter_float xuf = uint_to_float (xu);
  jitter_float xif = int_to_float (xi);

  printf ("%20s: %f\n", name, xf);
  printf ("%20s  %f reinterpreted as " JITTER_UINT_FORMAT "\n", "", xf, xu);
  printf ("%20s  %f reinterpreted as " JITTER_INT_FORMAT "\n", "", xf, xi);
  printf ("%20s  ...back to %f\n", "", xuf);
  printf ("%20s  ...back to %f\n", "", xif);
  printf ("\n");

  if (xf != xuf)
    jitter_fatal ("%s: invalid conversion between jitter_float and jitter_uint",
                  name);
  if (xf != xif)
    jitter_fatal ("%s: invalid conversion between jitter_float and jitter_int",
                  name);
}

int
main (void)
{
  test ("zero", 0);
  test ("one", 1);
  test ("one and a half", 1.5);
  test ("minus one and a half", -1.5);
  test ("seven", 7);
  test ("minus seven", -7);
  test ("pi", M_PI);
  test ("e", M_E);
  test ("biggish number", 1234567890);
  test ("big fractional part", 0.1234567890);

  printf ("All good.\n");
  return EXIT_SUCCESS;
}
