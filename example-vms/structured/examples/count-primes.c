/* The Structured count-primes example translated into -*- C -*-, for
   benchmarking.

   Copyright (C) 2019, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of the Jitter structured-language example, distributed
   along with GNU Jitter under the same license.

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
#include <stdbool.h>

int main (void)
{
  long n = 2, prime_no = 0;
  while (n < 2000000)
    {
      bool n_is_prime = true;
      long candidate_divisor = 2;
      while (n_is_prime && candidate_divisor * candidate_divisor <= n)
        {
          if (n % candidate_divisor == 0)
            n_is_prime = false;
          candidate_divisor ++;
        }
      if (n_is_prime)
        prime_no ++;
      n ++;
    }
  printf ("%li\n", prime_no);
  return 0;
}
