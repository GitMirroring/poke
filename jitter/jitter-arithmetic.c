/* Jitter: general-purpose integer arithmetic functions.

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
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */

#include <jitter/jitter.h>
#include <jitter/jitter-arithmetic.h>
#include <jitter/jitter-fatal.h>




/* Number of digits of an integer.
 * ************************************************************************** */

/* A positive integer n can be represented in exactly
      floor (log_{base} (n)) + 1
   digits. */

/* Return the floor of the logarithm of the given number in the given base.
   Rationale: I do not want to require libm just because of two functions that
   I can replace here very easily. */
static int
jitter_floor_log (jitter_uint base, jitter_uint n)
{
  /* Compute the power of the base needed to reach (or pass) n. */
  jitter_uint res = 0;
  jitter_uint guess = 1;
  while (guess < n)
    {
      guess *= base;
      res ++;
    }

  /* If we passed n then the logarithm of n is not natural, and res is now
     holding its ceiling -- but we want its floor. */
  if (guess > n)
    res --;
  return res;
}

int
jitter_digit_no_unsigned (jitter_uint number, unsigned radix)
{
  /* Every other user function eventually calls this function; this is therefore
     an appropriate place to put the one sanity check. */
  if (radix < 2)
    jitter_fatal ("jitter_digit_no_unsigned: radix less than 2");

  if (number == 0)
    return 1;
  else
    return jitter_floor_log (radix, number) + 1;
}

int
jitter_digit_no (jitter_int number, unsigned radix)
{
  if (number < 0)
    return 1 + jitter_digit_no_unsigned (- (jitter_uint) number, radix);
  else
    return jitter_digit_no_unsigned (number, radix);
}

int
jitter_digit_no_radix_10 (jitter_int number)
{
  return jitter_digit_no (number, 10);
}

int
jitter_digit_no_unsigned_radix_10 (jitter_uint number)
{
  return jitter_digit_no_unsigned (number, 10);
}
