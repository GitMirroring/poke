/* Jitter utility: human prefix header.

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


#ifndef JITTER_HUMAN_PREFIX_H_
#define JITTER_HUMAN_PREFIX_H_

#include <stdbool.h>


/* Introduction.
 * ************************************************************************** */

/* This is a convenience facility for printing floating-point numbers with
   metric (decimal-based) or IEC (binary-based) prefixes, for human-readable
   output. */




/* Function API.
 * ************************************************************************** */

/* Find the best human-readable prefix for the number in.  Write the value of in
   appropriately scaled to the location pointed by out, and write the
   abbreviation for the prefix to the location pointed by scale.  Use metric
   (decimal-based) prefixes if binary is false, and IEC (binary-based) prefixes
   if binary is true. */
void
jitter_human_readable (double *out, const char **scale,
                       double in,
                       bool binary)
  __attribute__ ((nonnull (1), nonnull (2)));




/* Convenience macro API.
 * ************************************************************************** */

/* This is a convenient way of using jitter_human_readable without cluttering
   the code with explicit variable declarations.
   Expand to a sequence of two variable declarations and a statements, without
   any nested blocks.
   In the expansion:
   - an automatic variable is declared, with type double and name
     _jitter_value_name;
   - another automatic variable is declared, named like _jitter_value_name with
     the suffix _prefix -- for example, if _jitter_value_name is given as
     total_size, then the second automatic variable in the expansion is
     named total_size_prefix
   - jitter_human_readable is called, supplying pointers to the two 
     automatic variables, the given value expression, and the the given
     binary expression.  This initializes the two automatic variables.
   The following code in the same C block is supposed to use the two automatic
   variables, typically for printing.
   Example:
     JITTER_HUMAN_READABLE_ (size, blocks * block_size_in_bytes, true);
     printf ("size: %.2f%sB\n", size, size_prefix); */
#define JITTER_HUMAN_READABLE_(_jitter_value_name,              \
                               _jitter_value_exp,               \
                               _jitter_binary)                  \
  double _jitter_value_name;                                    \
  const char *_jitter_value_name ## _prefix;                    \
  jitter_human_readable (& (_jitter_value_name),                \
                            & (_jitter_value_name ## _prefix),  \
                            (_jitter_value_exp),                \
                            (_jitter_binary))


#endif // #ifndef JITTER_HUMAN_PREFIX_H_
