/* Fibonacci example in -*- C -*-, to be compared against JitterLisp.

   Copyright (C) 2021 Luca Saiu
   Updated in 2022 by Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with GNU Jitter under the same license.

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


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* The Fibonacci sequence, recursive, exponential-time.
 * ********************************************************************* */

long
fibo (long n)
{
  if (n < 2)
    return n;
  else
    return fibo (n - 2) + fibo (n - 1);
}




/* Main.
 * ********************************************************************* */

static void
error (char *program_name)
{
  printf ("Synopsis: %s NUMBER\n", program_name);
  exit (-1);
}

int
main (int argc, char **argv)
{
  char *rest;
  errno = 0;
  if (argc != 2)
    error (argv [0]);

  long n = strtol (argv [1], & rest, 10);
  if (* rest != '\0' || errno != 0)
    error (argv [0]);

  printf ("%li\n", fibo (n));
  return 0;
}
