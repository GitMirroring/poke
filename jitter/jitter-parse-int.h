/* Jitter utility: convenient integer parsing header.

   Copyright (C) 2017 Luca Saiu
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


#ifndef JITTER_PARSE_INT_H_
#define JITTER_PARSE_INT_H_

#include <jitter/jitter.h>

/* Convert the base-10 integer fully encoded in the given string to a long long,
   and on success store it in the address pointed by result.  A string not
   encoding a base-10 integer or with trailing garbage after the digit is
   considered incorrect.  Return 0 on success, or a nonzero value if the string
   is incorrect. */
int
jitter_string_to_long_long_inconvenient (const char *s,
                                         jitter_long_long *result)
  __attribute__ ((nonnull (1, 2)));

/* Like jitter_string_to_long_long_inconvenient , but using the given radix
   instead of 10.  No radix prefix is accepted. */
int
jitter_string_to_long_long_inconvenient_radix (const char *s,
                                               jitter_long_long *result,
                                               unsigned radix)
  __attribute__ ((nonnull (1, 2)));

/* Return the base-10 natural fully encoded in the given string, or a negative
   number if the string does not encode a natural or has trailing garbage after
   the digits. */
jitter_long_long
jitter_string_to_natural_radix (const char *s, unsigned radix)
  __attribute__ ((nonnull (1)));

/* Like jitter_string_to_natural , but using the given radix instead of 10.  No
   radix prefix is accepted. */
jitter_long_long
jitter_string_to_natural (const char *s)
  __attribute__ ((nonnull (1)));

/* Return the base-10 integer fully encoded in the given string, which is
   assumed to contain an optional sign followed by digits (either just zero or
   starting with a nonzero digit) and not to have trailing garbage after the
   digits.  No error-checking is performed, but this is still suitable to be
   called after a scanner has already recognized the pattern. */
jitter_long_long
jitter_string_to_long_long_unsafe (const char *s)
  __attribute__ ((nonnull (1)));

/* Like jitter_string_to_long_long_unsafe , but using the given radix instead of
   10.  No radix prefix is accepted. */
jitter_long_long
jitter_string_to_long_long_unsafe_radix (const char *s, unsigned radix)
  __attribute__ ((nonnull (1)));

#endif // #ifndef JITTER_PARSE_INT_H_
