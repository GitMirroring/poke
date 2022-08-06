/* Jitter: general-purpose bitwise macro header.

   Copyright (C) 2018, 2019, 2020 Luca Saiu
   Updated in 2022 by Luca Saiu
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


#ifndef JITTER_BITWISE_H_
#define JITTER_BITWISE_H_


#include <limits.h> /* For CHAR_BIT . */

#include <jitter/jitter.h>


/* Introduction.
 * ************************************************************************** */

/* This header contains some general-purpose macros for bitwise operations. */




/* Powers of two.
 * ************************************************************************** */

/* Expand to an expression evaluating to nonzero if the given argument, assumed
   to be nonzero, does *not* evaluate to a power of two.
   This may evaluate the argument multiple times, but is otherwise efficient and
   always branchless. */
#define JITTER_ISNT_NONZERO_A_POWER_OF_TWO(x)  \
  ((x) & ((x) - 1))

/* Expand to an expression evaluating to nonzero if the given argument, assumed
   to be nonzero, evaluates to a power of two.
   This may evaluate the argument multiple times. */
#define JITTER_IS_NONZERO_A_POWER_OF_TWO(x)  \
  (! JITTER_ISNT_NONZERO_A_POWER_OF_TWO(x))

/* Expand to an expression evaluating to nonzero if the given argument evaluates
   to a power of two.
   This may evaluate the argument multiple times. */
#define JITTER_IS_A_POWER_OF_TWO(x)  \
  (((x) != 0) && JITTER_IS_NONZERO_A_POWER_OF_TWO(x))

/* Expand to an expression evaluating to nonzero if the given argument does
   *not* evaluate to a power of two.
   This may evaluate the argument multiple times. */
#define JITTER_ISNT_A_POWER_OF_TWO(x)  \
  (((x) == 0) || JITTER_ISNT_NONZERO_A_POWER_OF_TWO(x))

/* Given a possibly signed x and a power of two p, expand to an expression
   evaluating to the maximum integer less than or equal to x which is a multiple
   of p.
   The signed version relies on two's complement representation.
   This assumes that p is a power of two and may evaluate both arguments
   multiple times.  However, if both arguments are constant expression, the
   expansion is also a constant expression. */
#define JITTER_PREVIOUS_MULTIPLE_OF_POWER_OF_TWO(x, p)  \
  ((x) & ~ ((p) - 1))

/* Given a possibly signed integer x and a power of two p, expand to an
   expression evaluating to the minimum integer greater than or equal to x which
   is a multiple of p.

   This assumes that p is a power of two and may evaluate both arguments
   multiple times.  However, if both arguments are constant expression, the
   expansion is also a constant expression.
   Notice that if p is a compile-time constant and x is in a register then the
   result can be computed in just two fast instructions: sum-of-literal,
   bitwise-and-of-literal.

   I learned this technique from from Hacker's Delight, §3-1.
   Let y be the smallest multiple of p greater than or equal to x, where p
   is a power of two.  Then:
     y = (x + p - 1) & - p     [1]
   and also
     y = x + (- x & (p - 1))   [2]
   I am currently using [1].  [2] can be useful in other contexts, because
   the second addend expresses a displacement from x to y. */
#define JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO(x, p)  \
  (((x) + ((p) - 1)) & ~ ((p) - 1))

/* Given a positive integer a and a positive integer b, expand to an expression
   evaluating to the smallest integer greater than or equal to a which is a
   multiple of b.
   This may evaluate both arguments multiple times.  However, if both arguments
   are constant expression, the expansion is also a constant expression. */
#define JITTER_NEXT_MULTIPLE_OF_POSITIVE(a, b)  \
  (((a) + (b) - 1) / (b) * (b))




/* Type sizes in bits.
 * ************************************************************************** */

/* Given a type expand to a constant expression evaluating to the type size in
   bits.  This is useful for playing with bit masks. */
#define JITTER_SIZEOF_IN_BITS(_jitter_type)  \
  (sizeof (_jitter_type) * CHAR_BIT)

/* Jitter word size in bits. */
#define JITTER_BITS_PER_WORD \
  JITTER_SIZEOF_IN_BITS (jitter_int)




/* Binary field extraction.
 * ************************************************************************** */

/* Expand to an expression evaluating to a field of the given word starting from
   the given 0-based bit index until the given 0-based bit index, both ends
   included.  The 0th bit is considered to be the least significant.
   The result is logically-right-shifted so that the least significant bit of
   the field is the least significant bit of the result.
   The expansion may evaluate the arguments multiple times. */
#define JITTER_WORD_FIELD(word, from_bit_index, to_bit_index)  \
  ((((uint64_t) (word)) >> (from_bit_index))                   \
   & ((1UL << ((to_bit_index) - (from_bit_index) + 1)) - 1))




/* Arithmetic right shifts.
 * ************************************************************************** */

/* Expand to an r-value evaluating to the given word arithmetically
   right-shifted by _jitter_bit_nom , using the given types which must be
   unsigned and signed integer types of the same width.

   There are basically two separate implementations of this, one relying on >>
   sign-extending on signed operands, like GCC does, and another generic but
   slow solution.  Which implementation is used depends on a constant expression
   checking how >> behaves at compile time.  I cannot think of any way of moving
   this logic to configure, as the check would depend on run-time behavior and
   would break under cross-compilation.

   Notice that the seemingly obvious alternative of doing a signed division by
   a power of two does not always compute the correct result with a negative
   operand in two's complement: the rounding direction for signed division is
   not what we need here. */
#define JITTER_ARITHMETIC_SHIFT_RIGHT(_jitter_unsigned_type,        \
                                      _jitter_signed_type,          \
                                      _jitter_word,                 \
                                      _jitter_bit_no)               \
  (JITTER_RIGHT_SHIFT_SIGN_EXTENDS (_jitter_unsigned_type,          \
                                    _jitter_signed_type)            \
   ? JITTER_ARITHMETIC_SHIFT_RIGHT_GCC (_jitter_unsigned_type,      \
                                        _jitter_signed_type,        \
                                        (_jitter_word),             \
                                        (_jitter_bit_no))           \
   : JITTER_ARITHMETIC_SHIFT_RIGHT_GENERIC (_jitter_unsigned_type,  \
                                            _jitter_signed_type,    \
                                            (_jitter_word),         \
                                            (_jitter_bit_no)))

/* Expand to a constant expression, nonzero iff >> sign-extends (at least on an
   argument of size jitter_int , which is what we care about here).
   This is used in the implementation of JITTER_ARITHMETIC_SHIFT_RIGHT . */
#define JITTER_RIGHT_SHIFT_SIGN_EXTENDS(_jitter_unsigned_type,                \
                                        _jitter_signed_type)                  \
  /* Just do on one simple test.  Some ridiculous C compiler might in theory  \
     behave in some different way than actually performing an arithmetic      \
     shift and still give the expected result here, but I do not feel like    \
     being overly pedantic. */                                                \
  ((((_jitter_signed_type) -56) >> 3)                                         \
   == ((_jitter_signed_type) -7))

/* One of the two implementations for JITTER_ARITHMETIC_SHIFT_RIGHT .  This
   definition is more efficient than the alternative but relies on >>
   sign-extending on signed operands like GCC does; the C standards doesn't
   define a behavior in this case. */
#define JITTER_ARITHMETIC_SHIFT_RIGHT_GCC(_jitter_unsigned_type,  \
                                          _jitter_signed_type,    \
                                          _jitter_word,           \
                                          _jitter_bit_no)         \
  ((_jitter_unsigned_type)                                        \
   (((_jitter_signed_type) (_jitter_word))                        \
    >> (_jitter_bit_no)))

/* One of the two implementations for JITTER_ARITHMETIC_SHIFT_RIGHT .  This
   definition does not rely on implementation-defined behavior. */
#define JITTER_ARITHMETIC_SHIFT_RIGHT_GENERIC(_jitter_unsigned_type,  \
                                              _jitter_signed_type,    \
                                              _jitter_word,           \
                                              _jitter_bit_no)         \
  /* The technique comes from from Hacker's Delight, §2. */           \
  ((_jitter_signed_type)                                              \
   ((((_jitter_unsigned_type) (_jitter_word)) >> (_jitter_bit_no))    \
    | - (((_jitter_unsigned_type)                                     \
          (((_jitter_unsigned_type) (_jitter_word))                   \
           >> (JITTER_SIZEOF_IN_BITS (_jitter_unsigned_type) - 1)))   \
         << (JITTER_SIZEOF_IN_BITS (_jitter_unsigned_type) - 1        \
             - (_jitter_bit_no)))))




/* Straight-line sign tests.
 * ************************************************************************** */

/* Expand to an expression which evalues the given word, which must have the given
   signed type; the expansion result, of the given unsigned type, will evaluate to
   either:
   - the unsigned conversion of -1 (all bits set to 1), meaning true, when
     _jitter_word is positive;
   - the value 0, meaning false, when _jitter_word is negative.

   Rationale: this is conceived to materialize a Boolean result, usually to be
   further normalized with bitwise operations like in the macros below.
   Of course using this macro as the conditional or guard in a test or loop in C
   is not useful, as its entire point is to avoid branches. */
#define JITTER_IS_NEGATIVE_ALL_ONES(_jitter_unsigned_type,  \
                                    _jitter_signed_type,    \
                                    _jitter_word)           \
  ((_jitter_unsigned_type)                                  \
   JITTER_ARITHMETIC_SHIFT_RIGHT                            \
     (_jitter_unsigned_type,                                \
      _jitter_signed_type,                                  \
      (_jitter_word),                                       \
      JITTER_SIZEOF_IN_BITS (_jitter_unsigned_type) - 1))

/* Given an integer type in unsigned and singed version and three expressions
   named discriminand, then and else, expand to an expression evaluating to
   then if the discriminand is negative, and to else otherwise.
   The the three expressions, which may be evaluated multiple times in the
   expansion, must have the given type.  The then and else expressions should
   be constant expressions for good performance.
   An important use case for this macro is a VM primitive returning a tagged
   Boolean result, which needs to be materialized.
   Define JITTER_CONDITIONAL_IF_NEGATIVE to use one of the two implementations
   below.  The API is identical and either implementation will work on every
   platform, but performance may differ.  On some machines (for example,
   PowerPC) straight-line code is often a huge win; on others it is just a
   modest improvement, and in a few cases (SH, and possibly x86_64) is
   actually counterproductive. */
#if defined (JITTER_HAVE_FAST_STRAIGHT_LINE_NEGATIVITY)
# define JITTER_CONDITIONAL_IF_NEGATIVE(_jitter_unsigned_type,             \
                                        _jitter_signed_type,               \
                                        _jitter_discriminand,              \
                                        _jitter_then,                      \
                                        _jitter_else)                      \
    JITTER_CONDITIONAL_IF_NEGATIVE_STRAIGHT_LINE (_jitter_unsigned_type,   \
                                                  _jitter_signed_type,     \
                                                  (_jitter_discriminand),  \
                                                  (_jitter_then),          \
                                                  (_jitter_else))
#else /* ! defined (JITTER_HAVE_FAST_STRAIGHT_LINE_NEGATIVITY) */
# define JITTER_CONDITIONAL_IF_NEGATIVE(_jitter_unsigned_type,       \
                                        _jitter_signed_type,         \
                                        _jitter_discriminand,        \
                                        _jitter_then,                \
                                        _jitter_else)                \
    JITTER_CONDITIONAL_IF_NEGATIVE_TRIVIAL (_jitter_unsigned_type,   \
                                            _jitter_signed_type,     \
                                            (_jitter_discriminand),  \
                                            (_jitter_then),          \
                                            (_jitter_else))
#endif // #if defined (JITTER_HAVE_FAST_STRAIGHT_LINE_NEGATIVITY)

/* One of the two implementations of JITTER_CONDITIONAL_IF_NEGATIVE, in this
   case expanding to straight-line code.
   Implementation note:
   The idea is rewriting
     (discriminand < 0) ? then : else
   to an expression of the following shape
     (discriminand < 0) & X ^ Y
   Now, (discriminand < 0) is either -1 or 0, as per
   JITTER_IS_NEGATIVE_ALL_ONES; it follows that (discriminand < 0) & X will be 0
   when discriminand >= 0.  Since the result must be else when discriminand >= 0
   we have that Y can only be else in order to have (0 ^ Y) = else.  At this
   point we can determine the right value for X: the xor operation with Y is
   applied unconditionally, and has the effect of flipping every 1 bit from
   else; but the only place where we can make the result be then is in X.  X
   must be such that xor-ing -1 & X with else gives then .  So, X must set the 1
   bits from then to the *opposite* configuration compared to what we aim for in
   the result, except for the ones which will are 1 in else; the bits from then
   will be flipped once, the ones from else will be flipped twice.
   It is not difficult to see that the solution is:
   * X = ~ (then ^ ~ else)
   * Y = else . */
#define JITTER_CONDITIONAL_IF_NEGATIVE_STRAIGHT_LINE(_jitter_unsigned_type,  \
                                                     _jitter_signed_type,    \
                                                     _jitter_discriminand,   \
                                                     _jitter_then,           \
                                                     _jitter_else)           \
  ((/* This subexpression is discriminand < 0 ? -1 : 0 . */                  \
    (_jitter_unsigned_type)                                                  \
    (JITTER_IS_NEGATIVE_ALL_ONES (_jitter_unsigned_type,                     \
                                  _jitter_signed_type,                       \
                                  (_jitter_discriminand)))                   \
    & (/* X = ~ (then ^ ~ else) . */                                         \
       ~ ((_jitter_unsigned_type) (_jitter_then)                             \
          ^ ~ (_jitter_unsigned_type) (_jitter_else))))                      \
   ^ (/* Y = else . */                                                       \
      (_jitter_unsigned_type) (_jitter_else)))

/* An alternative trivial implementation of JITTER_CONDITIONAL_IF_NEGATIVE , to
   be used instead of JITTER_CONDITIONAL_IF_NEGATIVE_STRAIGHT_LINE on machines
   where arbitrary shifts are expensive or take many instructions. */
#define JITTER_CONDITIONAL_IF_NEGATIVE_TRIVIAL(_jitter_unsigned_type,  \
                                               _jitter_signed_type,    \
                                               _jitter_discriminand,   \
                                               _jitter_then,           \
                                               _jitter_else)           \
  (((_jitter_signed_type) (_jitter_discriminand) < 0)                  \
   ? (_jitter_unsigned_type) (_jitter_then)                            \
   : (_jitter_unsigned_type) (_jitter_else))

/* Like JITTER_CONDITIONAL_IF_NEGATIVE , but expanding to then if the
   discriminand is non-negative. */
#define JITTER_CONDITIONAL_IF_NONNEGATIVE(_jitter_unsigned_type,  \
                                          _jitter_signed_type,    \
                                          _jitter_discriminand,   \
                                          _jitter_then,           \
                                          _jitter_else)           \
  JITTER_CONDITIONAL_IF_NEGATIVE (_jitter_unsigned_type,          \
                                  _jitter_signed_type,            \
                                  (_jitter_discriminand),         \
                                  (_jitter_else),                 \
                                  (_jitter_then))

/* Like JITTER_CONDITIONAL_IF_NEGATIVE , but expanding to then if the
   discriminand is positive. */
#define JITTER_CONDITIONAL_IF_POSITIVE(_jitter_unsigned_type,                   \
                                       _jitter_signed_type,                     \
                                       _jitter_discriminand,                    \
                                       _jitter_then,                            \
                                       _jitter_else)                            \
  /* This cannot be rewritten into a call to JITTER_CONDITIONAL_IF_NEGATIVE.    \
     One could naïvely think of using the equivalence                           \
        d > 0  iff  (- d) < 0                                                   \
     , which would be correct except that the arithmetic negation overflows     \
     when d is the most negative integer.                                       \
     This alternative, more directly working on the sign bit,                   \
        d > 0  iff  (~ d) < 0                                                   \
     , fails when d is zero.                                                    \
     Hacker's Delight §2-12 "Comparison Predicates" gives formulas for x < y    \
     which would be applicable to 0 < y, if they did both not involve an        \
     arithmetic negation of y.                                                  \
     I cannot think of any clever way to optimize this.  If some way exists     \
     then it can be implemented here, in the place of this trivial version. */  \
  (((_jitter_signed_type) (_jitter_discriminand) > 0)                           \
   ? (_jitter_unsigned_type) (_jitter_then)                                     \
   : (_jitter_unsigned_type) (_jitter_else))

/* Like JITTER_CONDITIONAL_IF_NEGATIVE , but expanding to then if the
   discriminand is non-positive. */
#define JITTER_CONDITIONAL_IF_NONPOSITIVE(_jitter_unsigned_type,  \
                                          _jitter_signed_type,    \
                                          _jitter_discriminand,   \
                                          _jitter_then,           \
                                          _jitter_else)           \
  JITTER_CONDITIONAL_IF_POSITIVE (_jitter_unsigned_type,          \
                                  _jitter_signed_type,            \
                                  (_jitter_discriminand),         \
                                  (_jitter_else),                 \
                                  (_jitter_then))




/* Pointers and misalignment.
 * ************************************************************************** */

/* A word-sized bitmask having 1 bits on the right, in the positions which would
   make a pointer to a word-sized datum be misaligned. */
#define JITTER_POINTER_MISALIGNMENT_BITS_MASK            \
  ((((jitter_uint) 1) << JITTER_LG_BYTES_PER_WORD) - 1)

/* A bit mask which is just the one's complement of
   JITTER_POINTER_MISALIGNMENT_BITS_MASK , having the low bits set to 1 instead
   of the high bits. */
#define JITTER_POINTER_NON_MISALIGNMENT_BITS_MASK  \
  (~ JITTER_POINTER_MISALIGNMENT_BITS_MASK)

/* Given an expression evaluating to a pointer or a word-sized bitmask expand to
   an expression evaluating to the given expression converted to a bitmask and
   with the lowest bits masked off so as to form a correct bit configuration for
   a pointer to an aligned word-sized datum.
   If the argument is constant then the expansion is constant.
   This definition is conditional and avoids actually performing the mask
   operation if pointers are guaranteed to be always aligned by the machine
   or the ABI.
   See jitter/jitter-pointer-set.h for the reason why this bizarre macro is
   in fact useful. */
#if JITTER_ALIGNOF_VOID_P_P < JITTER_BYTES_PER_WORD
# define JITTER_UNMISALIGNED_BITMASK(_jitter_uint)                 \
    /* On this configuration there is the risk of actually having  \
       misaligned pointers: mask off the low bits. */              \
    ((jitter_uint) (_jitter_uint) &                                \
     JITTER_POINTER_NON_MISALIGNMENT_BITS_MASK)
#else
# define JITTER_UNMISALIGNED_BITMASK(_jitter_uint)                \
    /* On this configuration pointers are always aligned: do not  \
       mask. */                                                   \
    ((jitter_uint) (_jitter_uint))
#endif

#endif // #ifndef JITTER_BITWISE_H_
