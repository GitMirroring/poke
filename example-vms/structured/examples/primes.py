## The Structured primes example translated to -*- Python -*-, for benchmarking.

## Copyright (C) 2019 Luca Saiu
## Written by Luca Saiu

## This file is part of the Jitter structured-language example, distributed
## along with GNU Jitter under the same license.

## GNU Jitter is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.

## GNU Jitter is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>.


n = 2
while n < 2000000:
    n_is_prime = True
    candidate_divisor = 2
    while n_is_prime and candidate_divisor * candidate_divisor <= n:
        if n % candidate_divisor == 0:
            n_is_prime = False
        candidate_divisor = candidate_divisor + 1
    if n_is_prime:
        print (n)
    n = n + 1
