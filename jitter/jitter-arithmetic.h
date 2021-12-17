/* Jitter: general-purpose integer arithmetic macro header.

   Copyright (C) 2019, 2020, 2021 Luca Saiu
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


#ifndef JITTER_ARITHMETIC_H_
#define JITTER_ARITHMETIC_H_

#include <stdint.h>

#include <jitter/jitter.h>
#include <jitter/jitter-bitwise.h>


/* This header assumes two's complement arithmetic.
 * ************************************************************************** */

/* This all assumes two's complement arithmetic, in the meaning of the values
   which are computed and in many cases also for the purpose of the computation
   itself. */




/* Constants related to the representation of integers.
 * ************************************************************************** */

/* Expand to an expression whose result is an uint64_t bitmask, holding a single
   1 bit at the position of the sign bit and 0 elsewhere, assuming a two's
   complement representation in the given number of bits.  If the expression
   given for the number of bit is a constant expression then the expansion is a
   constant expression as well.  The number of bits may be any natural number up
   to 64 included. */
#define JITTER_SIGN_BIT_FOR_BITS(bit_no)  \
  ((uint64_t) 1 << ((bit_no) - 1))

/* Expand to a constant expression, like the expansion of
   JITTER_SIGN_BIT_FOR_BITS , for the number of bits in a Jitter word. */
#define JITTER_SIGN_BIT                            \
  JITTER_SIGN_BIT_FOR_BITS (JITTER_BITS_PER_WORD)




/* Integer representation limits.
 * ************************************************************************** */

/* Expand to a constant int64_t expression whose value is the most negative
   number representable with the given number of bits in two's complement.  The
   given number of bits must not be greater than 64. */
#define JITTER_MOST_NEGATIVE_SIGNED_IN_BITS(bit_no)  \
  ((int64_t) - (((uint64_t) 1) << ((bit_no) - 1)))

/* Expand to a constant int64_t expression whose value is the most positive
   number representable with the given number of bits in two's complement.  The
   given number of bits must not be greater than 64. */
#define JITTER_MOST_POSITIVE_SIGNED_IN_BITS(bit_no)     \
  ((int64_t) ((((uint64_t) 1) << ((bit_no) - 1)) - 1))

/* Expand to a constant uint64_t expression whose value is the largest number
   representable, unsigned, with the given number of bits.  The given number of
   bits must not be greater than 64. */
#define JITTER_MAXIMUM_UNSIGNED_IN_BITS(bit_no)  \
  ((((uint64_t) 1) << (bit_no)) - 1)

/* The values of JITTER_MOST_NEGATIVE_SIGNED_IN_BITS,
   JITTER_MOST_POSITIVE_SIGNED_IN_BITS and JITTER_MAXIMUM_UNSIGNED_IN_BITS for
   word-sized types. */
#define JITTER_MOST_NEGATIVE_SIGNED                             \
  ((jitter_int)                                                 \
   JITTER_MOST_NEGATIVE_SIGNED_IN_BITS (JITTER_BITS_PER_WORD))
#define JITTER_MOST_POSTITIVE_SIGNED                             \
  ((jitter_int)                                                  \
   JITTER_MOST_POSTITIVE_SIGNED_IN_BITS (JITTER_BITS_PER_WORD))
#define JITTER_MAXIMUM_UNSIGNED                             \
  ((jitter_uint)                                            \
   JITTER_MAXIMUM_UNSIGNED_IN_BITS (JITTER_BITS_PER_WORD))

/* Expand to an expression evaluating to a Boolean.  The result will be
   non-false iff the given argument, taken as a word-sized unsigned integer,
   fits within the specified number of bits.  For example 0, 4, 45 or 255 fit in
   8 bits, but 256 does not.  The number of bits does not need to be a power of
   two and can even be greater than 63 -- in which case the result will evaluate
   to non-false, since any given number will be exactly representable in 64 or
   more bits.
   The expansion will be a constant expression if the arguments are constant
   expressions; however the arguments may be evaluated multiple times. */
#define JITTER_FITS_IN_BITS_ZERO_EXTENDED(word, bit_no)  \
  (((bit_no) >= 64)                                      \
   || (((uint64_t) (word))                               \
       <= JITTER_MAXIMUM_UNSIGNED_IN_BITS (bit_no)))

/* Like JITTER_FITS_IN_BITS_ZERO_EXTENDED, but use sign extension rather than
   zero extension. */
#define JITTER_FITS_IN_BITS_SIGN_EXTENDED(word, bit_no)        \
  (((bit_no) >= 64)                                            \
   || ((((int64_t) (uint64_t) (word))                          \
        >= JITTER_MOST_NEGATIVE_SIGNED_IN_BITS (bit_no))       \
       && (((int64_t) (uint64_t) (word))                       \
           <= JITTER_MOST_POSITIVE_SIGNED_IN_BITS (bit_no))))




/* Integer division rounding up.
 * ************************************************************************** */

/* Given two expressions of some unsigned integer type expand to an expression
   of the same type evaluating to the ceiling of the quotient of the arguments.
   This may evaluate the arguments multiple times, but expands to a constant
   expression if the arguments are constant expressions. */
#define JITTER_QUOTIENT_CEILING(ua, ub)                                  \
  /* Bruno Haible suggested the alternative (ua + ub - 1) / ub which is  \
     useful in some contexts, but unfortunately can overflow. */         \
  ((ua) != 0                                                             \
   ? (((ua) - 1) / (ub) + 1)                                             \
   : 0)




/* Integer overflow checking.
 * ************************************************************************** */

/* This functionality provides ways of checking for overflow on signed integers
   before actually attempting an operation which would result in undefined
   behavior in C.

   At this time only signed integer operations are supported. */

/* Given a C integer type in its unsigned and signed variants, expressions a and
   b evaluating to integers of the given signed type and a number of significant
   bits (which a and b must fit in), expand to a Boolean expression, non-false
   iff the sum of a and b would overflow the same given number of bits.
   Arguments may be evaluated multiple times; bit_no should be a constant
   expression for an efficient expansion.
   The implementation will use GCC's overflow-checking primitives if available
   and if supported in the given number of bits. */
#ifdef JITTER_HAVE_GCC_OVERFLOW_CHECKING
# define JITTER_WOULD_PLUS_OVERFLOW(__jitter_unsigned_type,                \
                                    __jitter_signed_type,                  \
                                    a, b, bit_no)                          \
  (((bit_no) == 64)                                                        \
   ? JITTER_WOULD_PLUS_OVERFLOW_GCC (int64_t, (a), (b))                    \
   : (((bit_no) == 32)                                                     \
      ? JITTER_WOULD_PLUS_OVERFLOW_GCC (int32_t, (a), (b))                 \
      : (((bit_no) == 16)                                                  \
         ? JITTER_WOULD_PLUS_OVERFLOW_GCC (int16_t, (a), (b))              \
         : (((bit_no) == 8)                                                \
            ? JITTER_WOULD_PLUS_OVERFLOW_GCC (int8_t, (a), (b))            \
            : JITTER_WOULD_PLUS_OVERFLOW_NON_GCC (__jitter_unsigned_type,  \
                                                  __jitter_signed_type,    \
                                                  (a), (b), (bit_no))))))
#else // The GCC builtin is not available in this configuration.
# define JITTER_WOULD_PLUS_OVERFLOW(__jitter_unsigned_type,      \
                                    __jitter_signed_type,        \
                                    a, b, bit_no)                \
    JITTER_WOULD_PLUS_OVERFLOW_NON_GCC (__jitter_unsigned_type,  \
                                        __jitter_signed_type,    \
                                        (a), (b), (bit_no))
#endif // #ifdef JITTER_HAVE_GCC_OVERFLOW_CHECKING

/* A halper macro in the implementation of JITTER_WOULD_PLUS_OVERFLOW for the
   generic non-builtin case.  This does not evaluate to a Boolean, but to a
   signed word: the sum operation overflows iff the result of the expansion is
   negative.
   This is intended for use in a conditional fast branch checking the sign. */
#define JITTER_WOULD_PLUS_OVERFLOW_SIGNED_WORD(__jitter_unsigned_type,  \
                                               __jitter_signed_type,    \
                                               a, b, bit_no)            \
  /* The formula, like others below in this file, is a generalization   \
     to an arbitrary number of bits of an idea in Hacker's Delight,     \
     §2. */                                                             \
  ((__jitter_signed_type)                                               \
   (~ ((__jitter_unsigned_type) (a)                                     \
       ^ (__jitter_unsigned_type) (b))                                  \
    & (((__jitter_unsigned_type) (a))                                   \
       ^ (((__jitter_unsigned_type) (a)                                 \
           + (__jitter_unsigned_type) (b))))))

/* The generic non-builtin implementation of JITTER_WOULD_PLUS_OVERFLOW, with
   the same API.  This is not intended for the user to call directly. */
#define JITTER_WOULD_PLUS_OVERFLOW_NON_GCC(__jitter_unsigned_type,   \
                                           __jitter_signed_type,     \
                                           a, b, bit_no)             \
  (JITTER_WOULD_PLUS_OVERFLOW_SIGNED_WORD (__jitter_unsigned_type,   \
                                           __jitter_signed_type,     \
                                           (a), (b), (bit_no))       \
   & JITTER_SIGN_BIT_FOR_BITS(bit_no))

/* The GCC implementation of JITTER_WOULD_PLUS_OVERFLOW.  This is not intended
   for the user to call directly, and in fact is only correct when the given
   integer type matches the result width.  Again, not for the user. */
#define JITTER_WOULD_PLUS_OVERFLOW_GCC(__jitter_signed_type, a, b)  \
  (__builtin_add_overflow_p ((__jitter_signed_type) (a),            \
                             (__jitter_signed_type) (b),            \
                             (__jitter_signed_type) -1))

/* Like JITTER_WOULD_PLUS_OVERFLOW, but checking overflow for subtraction; the
   sign-bit idea is the same. */
#ifdef JITTER_HAVE_GCC_OVERFLOW_CHECKING
# define JITTER_WOULD_MINUS_OVERFLOW(__jitter_unsigned_type,                \
                                     __jitter_signed_type,                  \
                                     a, b, bit_no)                          \
  (((bit_no) == 64)                                                         \
   ? JITTER_WOULD_MINUS_OVERFLOW_GCC (int64_t, (a), (b))                    \
   : (((bit_no) == 32)                                                      \
      ? JITTER_WOULD_MINUS_OVERFLOW_GCC (int32_t, (a), (b))                 \
      : (((bit_no) == 16)                                                   \
         ? JITTER_WOULD_MINUS_OVERFLOW_GCC (int16_t, (a), (b))              \
         : (((bit_no) == 8)                                                 \
            ? JITTER_WOULD_MINUS_OVERFLOW_GCC (int8_t, (a), (b))            \
            : JITTER_WOULD_MINUS_OVERFLOW_NON_GCC (__jitter_unsigned_type,  \
                                                   __jitter_signed_type,    \
                                                   (a), (b), (bit_no))))))
#else // No GCC builtin available.
# define JITTER_WOULD_MINUS_OVERFLOW(__jitter_unsigned_type,      \
                                     __jitter_signed_type,        \
                                     a, b, bit_no)                \
    JITTER_WOULD_MINUS_OVERFLOW_NON_GCC (__jitter_unsigned_type,  \
                                         __jitter_signed_type,    \
                                         (a), (b), (bit_no))
#endif // #ifdef JITTER_HAVE_GCC_OVERFLOW_CHECKING
#define JITTER_WOULD_MINUS_OVERFLOW_SIGNED_WORD(__jitter_unsigned_type,  \
                                                __jitter_signed_type,    \
                                                a, b, bit_no)            \
  /* This formula comes from Hacker's Delight §2, again. */              \
  ((__jitter_signed_type)                                                \
   ((((__jitter_unsigned_type) (a) - (__jitter_unsigned_type) (b))       \
     ^ (__jitter_unsigned_type) (a))                                     \
    & (((__jitter_unsigned_type) (a) - (__jitter_unsigned_type) (b))     \
       ^ ~ (__jitter_unsigned_type) (b))))
#define JITTER_WOULD_MINUS_OVERFLOW_NON_GCC(__jitter_unsigned_type,   \
                                            __jitter_signed_type,     \
                                            a, b, bit_no)             \
  (JITTER_WOULD_MINUS_OVERFLOW_SIGNED_WORD (__jitter_unsigned_type,   \
                                            __jitter_signed_type,     \
                                            (a), (b), (bit_no))       \
   & JITTER_SIGN_BIT_FOR_BITS(bit_no))
#define JITTER_WOULD_MINUS_OVERFLOW_GCC(__jitter_signed_type, a, b)  \
  (__builtin_sub_overflow_p ((__jitter_signed_type) (a),             \
                             (__jitter_signed_type) (b),             \
                             (__jitter_signed_type) -1))

/* Like JITTER_WOULD_PLUS_OVERFLOW and JITTER_WOULD_MINUS_OVERFLOW, but checking
   overflow for multiplication; unfortunately the fallback non-builtin case for
   multiplication is less pretty and the idea of the sign bit does not apply.
   Apart from this the logic is the same as for sum and subtraction. */
#ifdef JITTER_HAVE_GCC_OVERFLOW_CHECKING
# define JITTER_WOULD_TIMES_OVERFLOW(__jitter_unsigned_type,                \
                                     __jitter_signed_type,                  \
                                     a, b, bit_no)                          \
  (((bit_no) == 64)                                                         \
   ? JITTER_WOULD_TIMES_OVERFLOW_GCC (int64_t, (a), (b))                    \
   : (((bit_no) == 32)                                                      \
      ? JITTER_WOULD_TIMES_OVERFLOW_GCC (int32_t, (a), (b))                 \
      : (((bit_no) == 16)                                                   \
         ? JITTER_WOULD_TIMES_OVERFLOW_GCC (int16_t, (a), (b))              \
         : (((bit_no) == 8)                                                 \
            ? JITTER_WOULD_TIMES_OVERFLOW_GCC (int8_t, (a), (b))            \
            : JITTER_WOULD_TIMES_OVERFLOW_NON_GCC (__jitter_unsigned_type,  \
                                                   __jitter_signed_type,    \
                                                   (a), (b), (bit_no))))))
#else // No GCC builtin available.
# define JITTER_WOULD_TIMES_OVERFLOW(__jitter_unsigned_type,     \
                                     __jitter_signed_type,       \
                                     a, b, bit_no)               \
    JITTER_WOULD_TIMES_OVERFLOW_NON_GCC (__jitter_unsigned_type,  \
                                         __jitter_signed_type,    \
                                         (a), (b), (bit_no))
#endif // #ifdef JITTER_HAVE_GCC_OVERFLOW_CHECKING
#define JITTER_WOULD_TIMES_OVERFLOW_NON_GCC(__jitter_unsigned_type,   \
                                            __jitter_signed_type,     \
                                            a, b, bit_no)             \
  /* This formula comes from Hacker's Delight §2, once more.  Notice  \
     that, differently from the cases for sum and subtraction, this   \
     works by case analysis and the result is a Boolean.  This will   \
     certainly be inefficient. */                                     \
  (((a) > 0)                                                          \
   ? (((b) > 0)                                                       \
      ? ((a) > ((__jitter_signed_type)                                \
                JITTER_MOST_POSITIVE_SIGNED_IN_BITS (bit_no)          \
                / (b)))                                               \
      : ((b) < ((__jitter_signed_type)                                \
                JITTER_MOST_NEGATIVE_SIGNED_IN_BITS (bit_no)          \
                / (a))))                                              \
   : (((b) > 0)                                                       \
      ? ((a) < ((__jitter_signed_type)                                \
                JITTER_MOST_NEGATIVE_SIGNED_IN_BITS (bit_no)          \
                / (b)))                                               \
      : ((a) != 0                                                     \
         && (b) < ((__jitter_signed_type)                             \
                   JITTER_MOST_POSITIVE_SIGNED_IN_BITS (bit_no)       \
                   / (a)))))
#define JITTER_WOULD_TIMES_OVERFLOW_GCC(__jitter_signed_type, a, b)  \
  (__builtin_mul_overflow_p ((__jitter_signed_type) (a),             \
                             (__jitter_signed_type) (b),             \
                             (__jitter_signed_type) -1))

/* Like the previous operations, checking overflow for division.  GCC has no
   builtin for this case. */
#define JITTER_WOULD_DIVIDED_OVERFLOW(__jitter_unsigned_type,  \
                                      __jitter_signed_type,    \
                                      a, b, bit_no)            \
  /* The cast on the result seems gratuitous, but the size of  \
     the result (not really the sign) may be important as      \
     this macro may occur in inline asm operands. */           \
  ((__jitter_unsigned_type)                                    \
   ((b) == 0                                                   \
    || ((__jitter_signed_type) (a)                             \
        == ((__jitter_signed_type)                             \
            JITTER_MOST_NEGATIVE_SIGNED_IN_BITS (bit_no))      \
        && (__jitter_signed_type) (b) == -1)))

/* Like the previous operations, checking overflow for remainder. */
#define JITTER_WOULD_REMAINDER_OVERFLOW(__jitter_unsigned_type,  \
                                        __jitter_signed_type,    \
                                        a, b, bit_no)            \
  ((b) == 0)

/* The non-overflow remainder condition.  See the "negative condition" comments
   in generate-fast-branches.in.m4sh for the rationale. */
#define JITTER_WOULD_REMAINDER_NOT_OVERFLOW(__jitter_unsigned_type,  \
                                            __jitter_signed_type,    \
                                            a, b, bit_no)            \
  (b)




/* Integer overflow checking and constant expressions.
 * ************************************************************************** */

/* The code for checking overflow and for overflow-checking operations often
   benefits from optimizations, consisting in not generating conditionals in
   inline asm when the condition of overflow or non-overflow is known at
   compile time.  The logic for this optimization is in
      jitter/jitter-fast-branch-machine-generated.h
   , which is machine-generated by the script obtained from preprocessing
      scripts/generate-fast-branches.in.m4sh
   .

   The GCC builtin __builtin_constant_p fails to return true in several common
   cases which could be resolved at compile time, particularly with arguments
   including GCC's own overflow-checking builtins.  Instead of just using
   __builtin_constant_p on a Boolean expression involving overflow-checking
   builtins to check if a C condition can be resolved at compile time we
   can use the macros here, which are designed to evaluate to constants more
   easily.

   The expressions given as arguments must have no side effects.  The expansion
   (of non-side-effecting arguments) will always be a constant expression, even
   if the arguments are non-constant expressions.

   This will not of course catch every possible case where the property is
   known, as that would not even be computable, but should be both a correct
   conservative approximation, and good enough to optimize away many redundant
   overflow checks.  I have never seen a case where GCC is not able to generate
   straight-line code after one of these macros determined that the condition
   was constant.  Still, even if that happened, it would be just a matter of
   suboptimal performance, and not of correctness.

   The macros in this section are only used for advanced dispatches, which rely
   on GCC anyway.  No other compiler is supported. */

/* The following helper macros are all used internally in the rest of this
   section.
   Given a non-side-effecting expression, constant or not, expand to a constant
   expression evaluating to a Boolean telling, respectively, whether the
   expression is known to evaluate to the given value or known not to, whether
   an expression is known to evaluate to the same result as another, known to
   evaluate to a different result, known to evaluate to a positive or to a
   non-negative result. */
#define JITTER_KNOWN_TO_BE(expression, value)  \
  (__builtin_constant_p (expression)           \
   && (expression) == (value))
#define JITTER_KNOWN_NOT_TO_BE(expression, value)  \
  (__builtin_constant_p (expression)               \
   && (expression) != (value))
#define JITTER_KNOWN_TO_BE_EQUAL(expression_a, expression_b)  \
  (__builtin_constant_p ((expression_a) == (expression_b))    \
   && (expression_a) == (expression_b))
#define JITTER_KNOWN_TO_BE_DIFFERENT(expression_a, expression_b)  \
  (__builtin_constant_p ((expression_a) != (expression_b))        \
   && (expression_a) != (expression_b))
#define JITTER_KNOWN_TO_BE_POSITIVE(expression_a)  \
  (__builtin_constant_p ((expression_a) >= 0)      \
   && (expression_a) > 0)
#define JITTER_KNOWN_TO_BE_NONNEGATIVE(expression_a)  \
  (__builtin_constant_p ((expression_a) >= 0)         \
   && (expression_a) >= 0)

/* Given two non-side-effecting expressions, constant or not, evaluating to
   jitter_int, expand to a Boolean constant expression evaluating to true iff
   the overflow status of a plus operation between the results of the two
   expression is known at compile time.  In case the result is true the
   operations is either known to definietely overflow, or to definitely not
   overflow; which case it is can be checked from the other macros above.

   Implementation note: the expansion, even if complex, consists in a
   disjunction of predicates, the simplest of which are near the beginning. */
#define JITTER_PLUS_OVERFLOWS_KNOWN_CONSTANT_GCC(a, b)                   \
  ((/* Both operands are known constants: the operation might            \
       overflow, and if so we know it at compile time. */                \
    __builtin_constant_p (a)                                             \
    && __builtin_constant_p (b))                                         \
   || (/* For every b: 0 + b does not overflow. */                       \
       JITTER_KNOWN_TO_BE ((a), 0))                                      \
   || (/* For every a: a + 0 does not overflow. */                       \
       JITTER_KNOWN_TO_BE ((b), 0))                                      \
   || (/* We may know whether a + b overflows, from the macro here. */   \
       __builtin_constant_p                                              \
          (JITTER_WOULD_PLUS_OVERFLOW_NON_GCC (jitter_uint,              \
                                               jitter_int,               \
                                               (a),                      \
                                               (b),                      \
                                               JITTER_BITS_PER_WORD))))

/* Like JITTER_PLUS_OVERFLOWS_KNOWN_CONSTANT_GCC, for the minus operation. */
#define JITTER_MINUS_OVERFLOWS_KNOWN_CONSTANT_GCC(a, b)                  \
  ((/* Both operands are known constants: the operation might            \
       overflow, and if so we know it at compile time. */                \
    __builtin_constant_p (a)                                             \
    && __builtin_constant_p (b))                                         \
   || (/* For every a: a - 0 = a does not overflow.  Notice that         \
          we cannot say the same for the left operand, since             \
          in two's complement negatives have a wider range. */           \
       JITTER_KNOWN_TO_BE ((b), 0))                                      \
   || (/* For every a: a - a = 0 does not overflow. */                   \
       JITTER_KNOWN_TO_BE_EQUAL ((a), (b)))                              \
   || (/* We may know whether a - b overflows, from the macro here. */   \
       __builtin_constant_p                                              \
          (JITTER_WOULD_MINUS_OVERFLOW_NON_GCC (jitter_uint,             \
                                                jitter_int,              \
                                                (a),                     \
                                                (b),                     \
                                                JITTER_BITS_PER_WORD))))

/* Like JITTER_PLUS_OVERFLOWS_KNOWN_CONSTANT_GCC, for the times operation. */
#define JITTER_TIMES_OVERFLOWS_KNOWN_CONSTANT_GCC(a, b)                  \
  ((/* Both operands are known constants: the operation might            \
       overflow, and if so we know it at compile time. */                \
    __builtin_constant_p (a)                                             \
    && __builtin_constant_p (b))                                         \
   || (/* For every b: 0 * b = 0 does not overflow. */                   \
       JITTER_KNOWN_TO_BE ((a), 0))                                      \
   || (/* For every a: a * 0 = 0 does not overflow. */                   \
       JITTER_KNOWN_TO_BE ((b), 0))                                      \
   || (/* For every b: 1 * b = b does not overflow. */                   \
       JITTER_KNOWN_TO_BE ((a), 1))                                      \
   || (/* For every a: 1 * a = a does not overflow. */                   \
       JITTER_KNOWN_TO_BE ((b), 1))                                      \
   || (/* For every b: -1 * b does not overflow, as long                 \
          as b is non-negaive. */                                        \
       JITTER_KNOWN_TO_BE ((a), -1)                                      \
       && JITTER_KNOWN_TO_BE_NONNEGATIVE (b))                            \
   || (/* For every a: a * -1 does not overflow, as long                 \
          as a is non-negative. */                                       \
       JITTER_KNOWN_TO_BE ((b), -1)                                      \
       && JITTER_KNOWN_TO_BE_NONNEGATIVE (a))                            \
   || (/* We may know whether a * b overflows, from the macro here. */   \
       __builtin_constant_p                                              \
          (JITTER_WOULD_TIMES_OVERFLOW_NON_GCC (jitter_uint,             \
                                                jitter_int,              \
                                                (a),                     \
                                                (b),                     \
                                                JITTER_BITS_PER_WORD))))

/* Like JITTER_PLUS_OVERFLOWS_KNOWN_CONSTANT_GCC, for the divided operation. */
#define JITTER_DIVIDED_OVERFLOWS_KNOWN_CONSTANT_GCC(a, b)                       \
  ((/* Both operands are known constants: the operation might                   \
       overflow, and if so we know it at compile time. */                       \
    __builtin_constant_p (a)                                                    \
    && __builtin_constant_p (b))                                                \
   || (/* For every a: a / 0 overflows. */                                      \
       JITTER_KNOWN_TO_BE ((b), 0))                                             \
   || (/* For every a, b: a / b does not overflow when b > 0. */                \
       JITTER_KNOWN_TO_BE_POSITIVE (b))                                         \
   || (/* For every a, b: a / b overflows when a = MIN and b = -1. */           \
       JITTER_KNOWN_TO_BE ((a), JITTER_MOST_NEGATIVE_SIGNED)                    \
       && JITTER_KNOWN_TO_BE ((b), -1))                                         \
   || (/* For every a, b: a / b does not overflow when a != MIN and b != 0. */  \
       JITTER_KNOWN_NOT_TO_BE ((a), JITTER_MOST_NEGATIVE_SIGNED)                \
       && JITTER_KNOWN_NOT_TO_BE ((b), 0))                                      \
   || (/* For every a, b: a / b does not overflow when b != -1 and b != 0. */   \
       JITTER_KNOWN_NOT_TO_BE ((b), -1)                                         \
       && JITTER_KNOWN_TO_BE ((b), 0))                                          \
   || (/* We may know whether a / b overflows, from the macro here. */          \
       __builtin_constant_p                                                     \
          (JITTER_WOULD_DIVIDED_OVERFLOW (jitter_uint,                          \
                                          jitter_int,                           \
                                          (a),                                  \
                                          (b),                                  \
                                          JITTER_BITS_PER_WORD))))

/* Like JITTER_PLUS_OVERFLOWS_KNOWN_CONSTANT_GCC, for the remainder (%)
   operation. */
#define JITTER_REMAINDER_OVERFLOWS_KNOWN_CONSTANT_GCC(a, b)  \
  JITTER_DIVIDED_OVERFLOWS_KNOWN_CONSTANT_GCC((a), (b))




/* Sign function, as a macro.
 * ************************************************************************** */

/* Given an expression a, expand to a signed integer expression holding the sign
   of the result of the evaluation of a: one of -1, 0, +1.
   The argument may be evaluated more than once.  If the argument is a constant
   expression then the expansion is constant as well.
   This is also correct for floating-point arguments. */
#define JITTER_SIGN(n)                                                \
  /* The idea is from Hacker's Delight §2-8, which also presents an   \
     alternative equivalent to, in my notation,                       \
       (!! ((n) >= 0) - !! ((n) <= 0))                                \
     .  In C, ever since K&R, (! expression) is guaranteed to always  \
     yield either 0 or 1; therefore (!! expression) is a portable     \
     way of normalizing a Boolean. */                                 \
  (!! ((n) > 0) - !! ((n) < 0))




/* Boolean operations.
 * ************************************************************************** */

/* I might want to move this section to some other header. */

/* Expand to an expression evaluating to the logical (not bitwise) xor of the
   result of the evaluation of the two given Boolean expressions.  This is
   useful for comparing possibly non-normalized Booleans for inequality,
   obtaining a non-false value when the two arguments are (logically)
   different. */
#define JITTER_XOR(a, b)                                               \
  /* Implementation note: this normalization using ! is correct and    \
     portable, as (! exp) is guaranteed to evaluate to either 0 or 1.  \
     This has been the case forever, since even before the time of     \
     ANSI C. */                                                        \
  (! (a) != ! (b))

/* Similar to JITTER_XOR, but here the expansion evaluates to the logical
   negation of the logical xor of the results.  This is a convenient way of
   comparing two possibly non-normalized Booleans for Boolean equality. */
#define JITTER_NXOR(a, b)                              \
  /* See the implementation comment in JITTER_XOR. */  \
  (! (a) == ! (b))




/* Number of digits of an integer.
 * ************************************************************************** */

/* The functionality in this section could be implemented with macros, at the
   cost of only a little pain.  Right now this API is not used in
   performance-critical code or from VM instructions, and therefore relies on
   functions. */

/* Return the number of characters required to represent the given signed
   integer number, including the minus sign only if the argument is negative, in
   the given radix. */
int
jitter_digit_no (jitter_int number, unsigned radix);

/* Same as jitter_digit_no, for an unsigned integer. */
int
jitter_digit_no_unsigned (jitter_uint number, unsigned radix);

/* Same as jitter_digit_no and jitter_digit_no_unsigned, for radix 10. */
int
jitter_digit_no_radix_10 (jitter_int number);
int
jitter_digit_no_unsigned_radix_10 (jitter_uint number);

#endif // #ifndef JITTER_ARITHMETIC_H_
