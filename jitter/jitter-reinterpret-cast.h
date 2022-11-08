/* Jitter: reinterpret-cast type conversion.

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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#ifndef JITTER_REINTERPRET_CAST_H_
#define JITTER_REINTERPRET_CAST_H_

#include <jitter/jitter-config.h> /* For feature macros. */


/* Introduction.
 * ************************************************************************** */

/* When working with tagged data in efficient implementation of
   dynamically-typed systems it is useful to convert between floating-point and
   integer data, altering the object type but *not* its bit pattern.  This kind
   of conversion in C++ is called reinterpret_cast.

   C lacks reinterpret_cast as a syntactic feature, but it can be emulated.
   This header provides one such emulation.

   For the use case which is relevant in Jitter a reinterpret-cast only makes
   sense between types having exactly the same size.  This holds in particular
   for jitter_float and jitter_int, and for jitter_float and jitter_uint .  */



/* Feature test.
 * ************************************************************************** */

/* This implementation is efficient and does not rely on function calls.  It
   requires some relatively advanced but supposedly portable features; no
   required feature is more recent than C99.
   Since I have never seen this trick used elsewhere I am carefully checking
   for the presence of each required feature, in order to prevent portability
   problem to lesser-known compilers. */

/* Fail in a clean way if the compiler does not support all the language
   features we need. */
#if ! defined (JITTER_HAVE_NON_CONSTANT_AGGREGATE_INITIALIZERS) \
    || ! defined (JITTER_HAVE_COMPOUND_LITERALS) \
    || ! defined (JITTER_HAVE_COMPOUND_LITERALS)
# warning "The C compiler features needed for reinterpret-cast conversion are:"
# warning "* non-constant aggregate initialisers;"
# warning "* designated initialisers;"
# warning "* compound literals."
# warning ""
# warning "This compiler does not support all the necessary features."
# warning "Please report this as a Jitter bug, specifying what compiler you are"
# warning "using.  Please attach a copy of your file jitter/jitter-config.h ."
#endif




/* Reinterpret-cast conversion: implementation.
 * ************************************************************************** */

/* Given a destination type, a source type and a source expression expand to
   an expression evaluating to the source expression, reinterpret-cast to the
   destination type.
   This assumes, without checking, that the two types have the same size.  This
   macro is for internal use; see the following definition. */
#define _JITTER_REINTERPRET_CAST_EXPRESSION(to_type, from_type,                \
                                            from_expression)                   \
  ((/* Use a designated initialiser to build a union with two fields, one per  \
       type, initialised with the "from" expression... */                      \
    (union                                                                     \
     {                                                                         \
       from_type _jitter_from;                                                 \
       to_type _jitter_to;                                                     \
     }) {._jitter_from = (from_expression)})                                   \
     /* ...and extract the other field from the same union.  That will be the  \
        result. */                                                             \
    ._jitter_to)

/* Exactly like _JITTER_REINTERPRET_CAST_EXPRESSION, but also perform a
   compile-time check on the two type sizes if possible; fail at run time in
   case of different sizes. */
#if defined (JITTER_HAVE_GNU_C_STATEMENT_EXPRESSIONS)
  /* This trick relies on GNU C's statement expressions.  Unfortunately I can
     find no way of failing at compile time rather than at run time, even if the
     if condition is a constant expression. */
# define JITTER_REINTERPRET_CAST(to_type, from_type, from_expression)          \
    ({ /* The condition is a constant expression, which will always disappear  \
          at compile time. */                                                  \
       if (sizeof (from_type) != sizeof (to_type))                             \
         printf ("cannot reinterpret-cast from from %iB %s to %iB %s",         \
                 (int) sizeof (from_type), # from_type,                        \
                 (int) sizeof (to_type), # to_type);                           \
       /* Actually do the work. */                                             \
       _JITTER_REINTERPRET_CAST_EXPRESSION(to_type, from_type,                 \
                                           (from_expression)); })
#else
  /* Without statement expressions we cannot check for type compatibility.
     Correct uses of the macro will still work. */
# define JITTER_REINTERPRET_CAST(to_type, from_type, from_expression)  \
    _JITTER_REINTERPRET_CAST_EXPRESSION(to_type, from_type,            \
                                        (from_expression)); })
#endif

#endif // #ifndef JITTER_REINTERPRET_CAST_H_
