/* Jitter: header supplying functions and macros which some systems lack.
   Copyright (C) 2017, 2019, 2020 Luca Saiu
   Updated in 2021 and 2022 by Luca Saiu
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


#ifndef JITTER_MISSING_H_
#define JITTER_MISSING_H_

#include <jitter/jitter-config.h>
#include <stdio.h>
#include <stddef.h> /* for offsetof, in case it is supported. */


/* Struct offset computation.
 * ************************************************************************** */

/* If offsetof is missign I can emulate with a macro which should be portable
   in practice even if not according to the letter of the Standard. */
#if ! defined (JITTER_HAVE_OFFSETOF)
# define offsetof(type_name, field_name)             \
    ((size_t)                                        \
     ((char *) & (((type_name *) NULL)->field_name)  \
      - (char *) NULL))
#endif // ! defined (JITTER_HAVE_OFFSETOF)




/* Alignment qualifiers.
 * ************************************************************************** */

/* If alignas is missing I can emulate it with a GNU C attribute.  In practice
   alignas is only used (in jitter-heap.h and friends) with GCC, so the issue is
   just supporting old versions. */
#if ! defined (JITTER_HAVE_ALIGNAS)
# define alignas(thing) __attribute__((aligned (thing)))
# define alignof __alignof__
#endif // ! defined (JITTER_HAVE_ALIGNAS)




/* GNU C attributes.
 * ************************************************************************** */

/* C compilers not supporting the GNU exensions will not recognize attributes.
   A simple fix is redefining the attribute keyword as a macro. */
#if ! defined (JITTER_HAVE_GNU_C_ATTRIBUTE)
# define attribute(ignored_attributes)      /* Nothing. */
# define __attribute__(ignored_attributes)  /* Nothing. */
#endif /* ! defined (JITTER_HAVE_GNU_C_ATTRIBUTE) */

/* After the previous definition, non-GNU C compilers will not have any problem,
   since every attribute use will be macroexpanded away; therefore it would be
   useless, in the following, to conditionalize over the attribute syntax
   availability.  What remains to be solved is older GNU C compilers not knowing
   about more recently introduced attributes. */
#if ! defined (JITTER_HAVE_ATTRIBUTE_NO_REORDER)
# define no_reorder      /* Nothing. */
# define __no_reorder__  /* Nothing. */
  /* On configurations where no_reorder is not defined, which will be the case
     for very old GCCs, configure tries to add -fno-toplevel-reorder to CFLAGS .
     That is a more brutal, global way of enforcing ordering. */
#endif /* #if ! defined (JITTER_HAVE_ATTRIBUTE_NO_REORDER) */
#if ! defined (JITTER_HAVE_ATTRIBUTE_RETURNS_NONNULL)                   \
    && /* Redefining these two macros does not play well with Gnulib's  \
          workaround definitions for Clang. */                          \
       ! defined (JITTER_HAVE_CLANG)
# define returns_nonnull      /* Nothing. */
# define __returns_nonnull__  /* Nothing. */
#endif /* #if ! defined (JITTER_HAVE_ATTRIBUTE_RETURNS_NONNULL) */




/* GCC builtins.
 * ************************************************************************** */

/* The logic relying on __builtin_constant_p is probably all conditionalised on
   GCC, but it is also easy to simulate it where missing with a dummy
   compatibility macro which treats every expression as non-constant. */
#if ! defined (JITTER_HAVE_GCC_BUILTIN_CONSTANT_P)
# define __builtin_constant_p(expression) \
    0
#endif /* # if ! defined (JITTER_HAVE_GCC_BUILTIN_CONSTANT_P) */

/* We can work with any compiler missing __builtin_expect by defining it as a
   macro. */
#if ! defined (JITTER_HAVE_GCC_BUILTIN_EXPECT)
# define __builtin_expect(expression, expected_value) \
    (expression)
#endif /* # if ! defined (JITTER_HAVE_GCC_BUILTIN_EXPECT) */

/* Non-GCC compilers or very old GCCs miss the __bultin_unreachable builtin. */
#if ! defined (JITTER_HAVE_GCC_BUILTIN_UNREACHABLE)
# define __builtin_unreachable()                          \
    /* It is acceptable to compile this into nothing. */  \
    do { /* Nothing*/ } while (false)
#endif /* # if ! defined (JITTER_HAVE_GCC_BUILTIN_UNREACHABLE) */




/* I/O functions.
 * ************************************************************************** */

/* The functions declared here are no-ops if we can live with them doing
   nothing; otherwise they fail fatally when called.  Each function
   implementation in jitter-missing.c is disabled by a CPP conditional if
   the system is found to have a real implementation at configure time. */

/* Do nothing. */
void
flockfile (FILE *f)
  __attribute__ ((nonnull (1)));

/* Do nothing. */
void
funlockfile (FILE *f)
  __attribute__ ((nonnull (1)));

#endif // #ifndef JITTER_MISSING_H_
