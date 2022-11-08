/* JitterLisp: operations on JitterLisp objects: header.

   Copyright (C) 2017, 2018, 2019, 2020, 2021 Luca Saiu
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


#ifndef JITTERLISP_OPERATIONS_H_
#define JITTERLISP_OPERATIONS_H_

/* Include the Gnulib header. */
#include <config.h>

#include <stdbool.h>

#include "jitterlisp-sexpression.h"
#include "jitterlisp-eval.h"
#include "jitterlisp-macros.h"
#include "jitterlisp-allocator.h"




/* Introduction and conventions.
 * ************************************************************************** */

/* JitterLisp operations are implemented as macros, with the number of function
   calls reduced to a mininum: we want to avoid function calls from VM
   instructions, particularly for common operations; wrapped functions introduce
   a memory indirection per call at run time which is best avoided. */

/* JitterLisp operation macros expand to C *statements*, not C expressions.
   Operations having one or more results are macros with l-values as their
   output arguments, which read objects and write other objects.

   Rationale: it would be possible to use expression by relying on the GNU C
   expression-as-statement extension, but as a Jitter example I want JitterLisp
   to be as portabile as reasonably possible.  I also want to be able to
   conditioanlize definitions with respect to the architecture and in particular
   to the machine word size, which makes boxedness configuration-dependent in
   many cases: what would be easy to define as an expression with a certain
   representation would be difficult or impossible with another.
   The style of having output arguments as l-values to be modified, while not
   the most friendly to the programmer, is well suited to VMs and scales
   well from stacks to registers: notice that a Jitter stack allows easy access
   to its top or undertop as an l-value, just like a register. */


/* The operations defined here perform no type checking: it is assumed, and not
   checked, that every operand has the required tag. */




/* Tag assumptions.
 * ************************************************************************** */

/* This file assumes that fixnums are coded with a zero tag.  The rest of the
   JitterLisp code is conditionalised to also allow less efficient solutions but
   that has become pointless, and will be simplified at some point: the solution
   of a zero tag for fixnums has proved to be the best. */
#if JITTERLISP_FIXNUM_TAG != 0
# error "Tagging fixnums with a tag different from 0 is not supported in this"
# error "code, as of late 2020.  The older code still conditionalised on the"
# error "value of fixnum tags will be simplified at some point: a zero tag for"
# error "fixnums has proved to be the best solution."
#endif




/* Expression operations.
 * ************************************************************************** */

// FIXME: describe the expression-operation idea.

// FIXME: JITTERLISP_EXP_




/* Fixnums-to-fixnum and fixnum-to-fixnum expression operations.
 * ************************************************************************** */

// FIXME: JITTERLISP_EXP_FF_F_

/* These operations take one or more fixnums and compute another fixnum as a
   result.  Right now there is no other numeric type in JitterLisp, but these
   will have to become more complicated in the future. */

/* Expand to an r-value expression evaluating to the tagged fixnum operation
   result having the given tagged fixnum operands, and the given infix C
   operation as the operator (the C operator working on untagged operands). */
#define JITTERLISP_EXP_FF_F_BINARY(_jitterlisp_infix,            \
                                   _jitterlisp_tagged_fixnum_a,  \
                                   _jitterlisp_tagged_fixnum_b)  \
  JITTERLISP_FIXNUM_ENCODE(                                      \
     (JITTERLISP_FIXNUM_DECODE(_jitterlisp_tagged_fixnum_a))     \
     _jitterlisp_infix                                           \
     (JITTERLISP_FIXNUM_DECODE(_jitterlisp_tagged_fixnum_b)))

/* Right now fixnum-to-fixnum operations behave "correctly" on overflow and
   underflow, where correctly means that the result is always tagged correctly
   -- but the result decoded value will be whatever the C operators yielded.
   I'll have to do something more complex, and almost certainly less efficient,
   after introducing bignums. */

/* The plus and minus operations can be defined in a more efficient way than the
   others with respect to tagging, even more if the fixnum tag is zero.
   Signedness does not matter for sum and subtraction on a two's complement
   machine, so we can avoid casting to and from a signed integer. */
#if (JITTERLISP_FIXNUM_TAG) == 0
# define JITTERLISP_EXP_FF_F_PLUS_OR_MINUS(_jitterlisp_infix,            \
                                           _jitterlisp_tagged_fixnum_a,  \
                                           _jitterlisp_tagged_fixnum_b)  \
    /* The infix operation is on unsigned operands, and the result does  \
       not need to be masked: the tag of the result is automatically     \
       correct for a sum or subtraction of two operands with low-order   \
       zeroes. */                                                        \
    (((jitterlisp_object) (_jitterlisp_tagged_fixnum_a))                 \
     _jitterlisp_infix                                                   \
     ((jitterlisp_object) (_jitterlisp_tagged_fixnum_b)))
#else // fixnum tag non-zero
# define JITTERLISP_EXP_FF_F_PLUS_OR_MINUS(_jitterlisp_infix,            \
                                           _jitterlisp_tagged_fixnum_a,  \
                                           _jitterlisp_tagged_fixnum_b)  \
    /* Notice that the infix operation is on unsigned operands. */       \
    JITTER_WITH_TAG_MASKED_ON(                                           \
       ((_jitterlisp_tagged_fixnum_a)                                    \
        _jitterlisp_infix                                                \
        (JITTERLISP_WITH_TAG_SUBTRACTED(                                 \
           (_jitterlisp_tagged_fixnum_b),                                \
           JITTERLISP_FIXNUM_TAG,                                        \
           JITTERLISP_FIXNUM_TAG_BIT_NO))),                              \
       JITTERLISP_FIXNUM_TAG,                                            \
       JITTERLISP_FIXNUM_TAG_BIT_NO)
#endif // #if fixnum tag is zero

/* Multiplication can also be defined in a more efficient way when the fixnum
   tag is zero, relying on two's complement arithmetic.  This is particularly
   useful for multiplications by a constant (in which case the constant should
   be on the right). */
#if (JITTERLISP_FIXNUM_TAG) == 0
# define JITTERLISP_EXP_FF_F_TIMES(_jitterlisp_tagged_fixnum_a,        \
                                   _jitterlisp_tagged_fixnum_b)        \
   ((jitterlisp_object)                                                \
    (((jitter_int) (_jitterlisp_tagged_fixnum_a))                      \
     *                                                                 \
     (((jitter_int)                                                    \
       JITTER_WITH_TAG_ASHIFTED_OFF(_jitterlisp_tagged_fixnum_b,       \
                                    JITTERLISP_FIXNUM_TAG,             \
                                    JITTERLISP_FIXNUM_TAG_BIT_NO)))))
#else // fixnum tag non-zero
# define JITTERLISP_EXP_FF_F_TIMES(_jitterlisp_tagged_fixnum_a,  \
                                   _jitterlisp_tagged_fixnum_b)  \
    JITTERLISP_EXP_FF_F_BINARY(*,                                \
                               _jitterlisp_tagged_fixnum_a,      \
                               _jitterlisp_tagged_fixnum_b)
#endif // #if fixnum tag is zero

/* Division can be defined in a more efficient way when the fixnum tag is zero,
   relying on two's complement arithmetic.  This implementation, compared to the
   naïve one, avoids right-shifting the two arguments. */
#if (JITTERLISP_FIXNUM_TAG) == 0
# define JITTERLISP_EXP_FF_F_DIVIDED(_jitterlisp_tagged_fixnum_a,         \
                                     _jitterlisp_tagged_fixnum_b)         \
    JITTERLISP_FIXNUM_ENCODE((jitter_int) (_jitterlisp_tagged_fixnum_a)   \
                             /                                            \
                             (jitter_int) (_jitterlisp_tagged_fixnum_b))
#else // fixnum tag non-zero
# define JITTERLISP_EXP_FF_F_DIVIDED(_jitterlisp_tagged_fixnum_a,  \
                                     _jitterlisp_tagged_fixnum_b)  \
    JITTERLISP_EXP_FF_F_BINARY(/,                                  \
                               _jitterlisp_tagged_fixnum_a,        \
                               _jitterlisp_tagged_fixnum_b)
#endif // #if fixnum tag is zero

/* Division by two can be defined in a more efficient way when the fixnum tag is
   zero, by relying on two's complement arithmetic.  GCC is not able to
   automatically generate the same better code without this for all
   architectures.  Notice that the conditional expression compiles to
   straight-line code, as GCC is able to translate it into either a logical
   right-shift transferring the sign bit to the least significant bit, or into a
   use of the carry bit. */
#if (JITTERLISP_FIXNUM_TAG) == 0
# ifdef JITTER_HAVE_FAST_MASK_OFF
#   define JITTERLISP_EXP_F_F_2DIVIDED(_jitterlisp_tagged_fixnum_a)  \
      ((JITTER_ARITHMETIC_SHIFT_RIGHT                                \
           (jitter_uint, jitter_int,                                 \
            ((_jitterlisp_tagged_fixnum_a)                           \
             + (((jitter_int) (_jitterlisp_tagged_fixnum_a) < 0)     \
                ? JITTERLISP_FIXNUM_ENCODE (1)                       \
                : 0)),                                               \
            1))                                                      \
       & ~ (((jitter_uint) 1 << JITTERLISP_FIXNUM_TAG_BIT_NO) - 1))
# else /* ! defined (JITTER_HAVE_FAST_MASK_OFF) */
#   define JITTERLISP_EXP_F_F_2DIVIDED(_jitterlisp_tagged_fixnum_a)  \
      ((JITTER_ARITHMETIC_SHIFT_RIGHT                                \
           (jitter_uint, jitter_int,                                 \
            ((_jitterlisp_tagged_fixnum_a)                           \
             + (((jitter_int) (_jitterlisp_tagged_fixnum_a) < 0)     \
                ? JITTERLISP_FIXNUM_ENCODE (1)                       \
                : 0)),                                               \
            JITTERLISP_FIXNUM_TAG_BIT_NO + 1))                       \
       << JITTERLISP_FIXNUM_TAG_BIT_NO)
# endif // #ifdef JITTER_HAVE_FAST_MASK_OFF
#else
# define JITTERLISP_EXP_F_F_2DIVIDED(_jitterlisp_tagged_fixnum_a)    \
    JITTERLISP_FIXNUM_ENCODE                                         \
       (JITTERLISP_FIXNUM_DECODE (_jitterlisp_tagged_fixnum_a) / 2)
#endif // #if fixnum tag is zero

/* Remainder by two, with a signed dividend. */
#if (JITTERLISP_FIXNUM_TAG) == 0
# define JITTERLISP_EXP_F_F_2REMAINDER(_jitterlisp_tagged_fixnum_a)   \
  (/* This addend evaluates to zero for a non-negative argument, or   \
      to the result times -2 for a negative argument, with no         \
      branches. */                                                    \
   (JITTER_IS_NEGATIVE_ALL_ONES (jitter_uint, jitter_int,             \
                                 (_jitterlisp_tagged_fixnum_a))       \
    & (- ((_jitterlisp_tagged_fixnum_a)                               \
          & JITTERLISP_FIXNUM_ENCODE (1)) << 1))                      \
   /* The second addend evaluates to the absolute value of the        \
      result. */                                                      \
   + ((_jitterlisp_tagged_fixnum_a) & JITTERLISP_FIXNUM_ENCODE (1)))
#else
# define JITTERLISP_EXP_F_F_2REMAINDER(_jitterlisp_tagged_fixnum_a)  \
    JITTERLISP_FIXNUM_ENCODE                                         \
      (JITTERLISP_FIXNUM_DECODE (_jitterlisp_tagged_fixnum_a) % 2)
#endif // #if fixnum tag is zero

/* Expression oprations on fixnums. */
#define JITTERLISP_EXP_FF_F_PLUS(_jitterlisp_tagged_fixnum_a,     \
                                 _jitterlisp_tagged_fixnum_b)     \
  JITTERLISP_EXP_FF_F_PLUS_OR_MINUS(+,                            \
                                    _jitterlisp_tagged_fixnum_a,  \
                                    _jitterlisp_tagged_fixnum_b)
#define JITTERLISP_EXP_FF_F_MINUS(_jitterlisp_tagged_fixnum_a,    \
                                  _jitterlisp_tagged_fixnum_b)    \
  JITTERLISP_EXP_FF_F_PLUS_OR_MINUS(-,                            \
                                    _jitterlisp_tagged_fixnum_a,  \
                                    _jitterlisp_tagged_fixnum_b)
/* JITTERLISP_EXP_FF_F_TIMES is defined above. */
/* JITTERLISP_EXP_FF_F_DIVIDED is defined above. */
#define JITTERLISP_EXP_FF_F_REMAINDER(_jitterlisp_tagged_fixnum_a,  \
                                  _jitterlisp_tagged_fixnum_b)      \
  JITTERLISP_EXP_FF_F_BINARY(%,                                     \
                             _jitterlisp_tagged_fixnum_a,           \
                             _jitterlisp_tagged_fixnum_b)
/* Again, I can use a faster implementation when the fixnum tag is zero. */
#if (JITTERLISP_FIXNUM_TAG) == 0
  /* If the fixnum tag is zero I can do an arithmetic negation on the entire
     encoded object, including the tag.  Tag bits will remain set to zero in
     the result. */
#define JITTERLISP_EXP_F_F_MINUS(_jitterlisp_tagged_fixnum_a)  \
  (- (_jitterlisp_tagged_fixnum_a))
#else /* Fallback case: non-zero fixnum tag. */
#define JITTERLISP_EXP_F_F_MINUS(_jitterlisp_tagged_fixnum_a)  \
  JITTERLISP_EXP_FF_F_MINUS(JITTERLISP_FIXNUM_ENCODE(0),       \
                            _jitterlisp_tagged_fixnum_a)
#endif // #if (JITTERLISP_FIXNUM_TAG) == 0




/* Fixnums-to-boolean expression operations.
 * ************************************************************************** */

/* In the case of comparison expression operations on fixnums a solution in the
   spirit of the more efficient solution for plus and minus above works with
   *any* tag.  Notice that the operands must be compared as signed. */
#define JITTERLISP_EXP_FF_B_COMPARISON(_jitterlisp_infix,            \
                                       _jitterlisp_tagged_fixnum_a,  \
                                       _jitterlisp_tagged_fixnum_b)  \
  JITTERLISP_BOOLEAN_ENCODE(((jitter_int)                            \
                             (_jitterlisp_tagged_fixnum_a))          \
                            _jitterlisp_infix                        \
                            ((jitter_int)                            \
                             (_jitterlisp_tagged_fixnum_b)))

/* Boolean operations on fixnum operands. */
#define JITTERLISP_EXP_FF_B_EQUAL(_jitterlisp_tagged_fixnum_a,  \
                                  _jitterlisp_tagged_fixnum_b)  \
  JITTERLISP_EXP_FF_B_COMPARISON(==,                            \
                                 _jitterlisp_tagged_fixnum_a,   \
                                 _jitterlisp_tagged_fixnum_b)
#define JITTERLISP_EXP_FF_B_NOTEQUAL(_jitterlisp_tagged_fixnum_a,  \
                                     _jitterlisp_tagged_fixnum_b)  \
  JITTERLISP_EXP_FF_B_COMPARISON(!=,                               \
                                 _jitterlisp_tagged_fixnum_a,      \
                                 _jitterlisp_tagged_fixnum_b)
#define JITTERLISP_EXP_FF_B_LESS(_jitterlisp_tagged_fixnum_a,  \
                                 _jitterlisp_tagged_fixnum_b)  \
  JITTERLISP_EXP_FF_B_COMPARISON(<,                            \
                                 _jitterlisp_tagged_fixnum_a,  \
                                 _jitterlisp_tagged_fixnum_b)
#define JITTERLISP_EXP_FF_B_NOTLESS(_jitterlisp_tagged_fixnum_a,  \
                                    _jitterlisp_tagged_fixnum_b)  \
  JITTERLISP_EXP_FF_B_COMPARISON(>=,                              \
                                 _jitterlisp_tagged_fixnum_a,     \
                                 _jitterlisp_tagged_fixnum_b)
#define JITTERLISP_EXP_FF_B_GREATER(_jitterlisp_tagged_fixnum_a,  \
                                    _jitterlisp_tagged_fixnum_b)  \
  JITTERLISP_EXP_FF_B_LESS(_jitterlisp_tagged_fixnum_b,           \
                           _jitterlisp_tagged_fixnum_a)
#define JITTERLISP_EXP_FF_B_NOTGREATER(_jitterlisp_tagged_fixnum_a,  \
                                       _jitterlisp_tagged_fixnum_b)  \
  JITTERLISP_EXP_FF_B_NOTLESS(_jitterlisp_tagged_fixnum_b,           \
                              _jitterlisp_tagged_fixnum_a)




/* Cons expression operations.
 * ************************************************************************** */

/* Cons selectors operations, including the ones for composed selectors, are
   always definable as expressions as they don't require heap allocation.  (That
   is not the case for cons construction.) */

/* Expand to an r-value expression evaluating to the tagged car or cdr of the
   given tagged cons operand. */
#define JITTERLISP_EXP_C_A_CAR(_jitterlisp_tagged_cons)   \
  (JITTERLISP_CONS_DECODE(_jitterlisp_tagged_cons)->car)
#define JITTERLISP_EXP_C_A_CDR(_jitterlisp_tagged_cons)   \
  (JITTERLISP_CONS_DECODE(_jitterlisp_tagged_cons)->cdr)

/* Composed cons selectors.  Given a tagged value expand to an expression
   evaluating to its tagged caar, cadr, and so on.  These are compositions of
   JITTERLISP_EXP_C_A_CAR and JITTERLISP_EXP_C_A_CDR . */
/* Length 2. */
#define JITTERLISP_EXP_C_A_CAAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CADR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CDR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDDR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CDR(_jitterlisp_tagged_cons))
/* Length 3. */
#define JITTERLISP_EXP_C_A_CAAAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CAAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CAADR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CADR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CADAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CDAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CADDR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CDDR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDAAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CAAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDADR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CADR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDDAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CDAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDDDR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CDDR(_jitterlisp_tagged_cons))
/* Length 4. */
#define JITTERLISP_EXP_C_A_CAAAAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CAAAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CAAADR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CAADR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CAADAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CADAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CAADDR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CADDR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CADAAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CDAAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CADADR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CDADR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CADDAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CDDAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CADDDR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CAR(JITTERLISP_EXP_C_A_CDDDR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDAAAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CAAAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDAADR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CAADR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDADAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CADAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDADDR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CADDR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDDAAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CDAAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDDADR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CDADR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDDDAR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CDDAR(_jitterlisp_tagged_cons))
#define JITTERLISP_EXP_C_A_CDDDDR(_jitterlisp_tagged_cons)                   \
  JITTERLISP_EXP_C_A_CDR(JITTERLISP_EXP_C_A_CDDDR(_jitterlisp_tagged_cons))


/* Box expression operations.
 * ************************************************************************** */

/* Box selection is definable as an expression as it doesn't require heap
   allocation.  (That is not the case for box construction.) */

// FIXME: this implementation is, of course, temporary.

/* Expand to an r-value expression evaluating to the tagged content of the
   given tagged box operand. */
#define JITTERLISP_EXP_B_A_GET(_jitterlisp_tagged_box)   \
  (JITTERLISP_CONS_DECODE(_jitterlisp_tagged_box)->car)




/* FIXME: move
 * ************************************************************************** */

#define JITTER_BEGIN_  \
  do                   \
    {

#define JITTER_END_    \
    }                  \
  while (false)




/* Fixnums-to-fixnum operations.
 * ************************************************************************** */

#define JITTERLISP_PLUS_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)  \
  JITTER_BEGIN_                                                              \
    (_jitterlisp_out)                                                        \
      = JITTERLISP_EXP_FF_F_PLUS(_jitterlisp_in0, _jitterlisp_in1);          \
  JITTER_END_

#define JITTERLISP_MINUS_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)  \
  JITTER_BEGIN_                                                               \
    (_jitterlisp_out)                                                         \
      = JITTERLISP_EXP_FF_F_MINUS(_jitterlisp_in0, _jitterlisp_in1);          \
  JITTER_END_

#define JITTERLISP_TIMES_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)  \
  JITTER_BEGIN_                                                               \
    (_jitterlisp_out)                                                         \
      = JITTERLISP_EXP_FF_F_TIMES(_jitterlisp_in0, _jitterlisp_in1);          \
  JITTER_END_

#define JITTERLISP_DIVIDED_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1) \
  JITTER_BEGIN_                                                                \
    if (_jitterlisp_in1 == JITTERLISP_FIXNUM_ENCODE(0))                        \
      jitterlisp_error_cloned ("division by zero");                            \
    (_jitterlisp_out)                                                          \
      = JITTERLISP_EXP_FF_F_DIVIDED(_jitterlisp_in0, _jitterlisp_in1);         \
  JITTER_END_
#define JITTERLISP_DIVIDED_UNSAFE_(_jitterlisp_out, _jitterlisp_in0,    \
                                   _jitterlisp_in1)                     \
  JITTER_BEGIN_                                                         \
    (_jitterlisp_out)                                                   \
      = JITTERLISP_EXP_FF_F_DIVIDED(_jitterlisp_in0, _jitterlisp_in1);  \
  JITTER_END_

#define JITTERLISP_QUOTIENT_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1) \
  JITTERLISP_DIVIDED_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)
#define JITTERLISP_QUOTIENT_UNSAFE_(_jitterlisp_out, _jitterlisp_in0,           \
                                    _jitterlisp_in1)                            \
  JITTERLISP_DIVIDED_UNSAFE_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)

#define JITTERLISP_REMAINDER_(_jitterlisp_out, _jitterlisp_in0,           \
                              _jitterlisp_in1)                            \
  JITTER_BEGIN_                                                           \
    if (_jitterlisp_in1 == JITTERLISP_FIXNUM_ENCODE(0))                   \
      jitterlisp_error_cloned ("remainder of division by zero");          \
    (_jitterlisp_out)                                                     \
      = JITTERLISP_EXP_FF_F_REMAINDER(_jitterlisp_in0, _jitterlisp_in1);  \
  JITTER_END_
#define JITTERLISP_REMAINDER_UNSAFE_(_jitterlisp_out, _jitterlisp_in0,    \
                                     _jitterlisp_in1)                     \
  JITTER_BEGIN_                                                           \
    (_jitterlisp_out)                                                     \
      = JITTERLISP_EXP_FF_F_REMAINDER(_jitterlisp_in0, _jitterlisp_in1);  \
  JITTER_END_

#define JITTERLISP_1PLUS_(_jitterlisp_out, _jitterlisp_in0)     \
  JITTER_BEGIN_                                                 \
    (_jitterlisp_out)                                           \
      = JITTERLISP_EXP_FF_F_PLUS(_jitterlisp_in0,               \
                                 JITTERLISP_FIXNUM_ENCODE(1));  \
  JITTER_END_
#define JITTERLISP_1MINUS_(_jitterlisp_out, _jitterlisp_in0)     \
  JITTER_BEGIN_                                                  \
    (_jitterlisp_out)                                            \
      = JITTERLISP_EXP_FF_F_MINUS(_jitterlisp_in0,               \
                                  JITTERLISP_FIXNUM_ENCODE(1));  \
  JITTER_END_
#define JITTERLISP_2TIMES_(_jitterlisp_out, _jitterlisp_in0)     \
  JITTER_BEGIN_                                                  \
    (_jitterlisp_out)                                            \
      = JITTERLISP_EXP_FF_F_TIMES(_jitterlisp_in0,               \
                                  JITTERLISP_FIXNUM_ENCODE(2));  \
  JITTER_END_
#define JITTERLISP_2DIVIDED_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                                 \
    (_jitterlisp_out)                                           \
      = JITTERLISP_EXP_F_F_2DIVIDED(_jitterlisp_in0);           \
  JITTER_END_
#define JITTERLISP_2QUOTIENT_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_2DIVIDED_((_jitterlisp_out), (_jitterlisp_in0))
#define JITTERLISP_2REMAINDER_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                                   \
    (_jitterlisp_out)                                             \
      = JITTERLISP_EXP_F_F_2REMAINDER(_jitterlisp_in0);           \
  JITTER_END_

#define JITTERLISP_NEGATE_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                               \
    (_jitterlisp_out)                                         \
      = JITTERLISP_EXP_F_F_MINUS(_jitterlisp_in0);            \
  JITTER_END_




/* Fixnums-to-boolean operations.
 * ************************************************************************** */

#define JITTERLISP_LESSP_(_jitterlisp_out, _jitterlisp_in0,           \
                          _jitterlisp_in1)                            \
  JITTER_BEGIN_                                                       \
    (_jitterlisp_out)                                                 \
      = JITTERLISP_EXP_FF_B_LESS(_jitterlisp_in0, _jitterlisp_in1);   \
  JITTER_END_
#define JITTERLISP_GREATERP_(_jitterlisp_out, _jitterlisp_in0,          \
                             _jitterlisp_in1)                           \
  JITTERLISP_LESSP_(_jitterlisp_out, _jitterlisp_in1, _jitterlisp_in0)
#define JITTERLISP_NOTLESSP_(_jitterlisp_out, _jitterlisp_in0,          \
                             _jitterlisp_in1)                           \
  JITTER_BEGIN_                                                         \
    (_jitterlisp_out)                                                   \
      = JITTERLISP_EXP_FF_B_NOTLESS(_jitterlisp_in0, _jitterlisp_in1);  \
  JITTER_END_
#define JITTERLISP_NOTGREATERP_(_jitterlisp_out, _jitterlisp_in0,          \
                                _jitterlisp_in1)                           \
  JITTERLISP_NOTLESSP_(_jitterlisp_out, _jitterlisp_in1, _jitterlisp_in0)




/* Boolean operations.
 * ************************************************************************** */

/* Compute a tagged boolean, #t iff the operand is #f. */
#define JITTERLISP_NOT_(_jitterlisp_out, _jitterlisp_in0)                \
  JITTER_BEGIN_                                                          \
    (_jitterlisp_out)                                                    \
      = JITTERLISP_BOOLEAN_ENCODE((_jitterlisp_in0)                      \
                                  == JITTERLISP_BOOLEAN_ENCODE(false));  \
  JITTER_END_

/* Canonicalize a boolean, computing #f if the argument is #f and #t if the
   operand is anything else. */
#define JITTERLISP_BOOLEAN_CANONICALIZE_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                                             \
    (_jitterlisp_out)                                                       \
      = JITTERLISP_BOOLEAN_ENCODE((_jitterlisp_in0)                         \
                                  != JITTERLISP_BOOLEAN_ENCODE(false));     \
  JITTER_END_




/* Cons operations.
 * ************************************************************************** */

// FIXME: comment.

#define JITTERLISP_CONS_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)  \
  JITTER_BEGIN_                                                              \
    struct jitterlisp_cons *_jitterlisp_tmp                                  \
      = JITTERLISP_CONS_MAKE_UNINITIALIZED_UNENCODED();                      \
    _jitterlisp_tmp->car = (_jitterlisp_in0);                                \
    _jitterlisp_tmp->cdr = (_jitterlisp_in1);                                \
    (_jitterlisp_out) = JITTERLISP_CONS_ENCODE(_jitterlisp_tmp);             \
  JITTER_END_

#define JITTERLISP_CAR_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                            \
    struct jitterlisp_cons *_jitterlisp_tmp                \
      = JITTERLISP_CONS_DECODE(_jitterlisp_in0);           \
    (_jitterlisp_out) = _jitterlisp_tmp->car;              \
  JITTER_END_
#define JITTERLISP_CDR_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                            \
    struct jitterlisp_cons *_jitterlisp_tmp                \
      = JITTERLISP_CONS_DECODE(_jitterlisp_in0);           \
    (_jitterlisp_out) = _jitterlisp_tmp->cdr;              \
  JITTER_END_

#define JITTERLISP_SET_CARB_(_jitterlisp_out,      \
                             _jitterlisp_cons,     \
                             _jitterlisp_new_car)  \
  JITTER_BEGIN_                                    \
    struct jitterlisp_cons *_jitterlisp_tmp        \
      = JITTERLISP_CONS_DECODE(_jitterlisp_cons);  \
    _jitterlisp_tmp->car = (_jitterlisp_new_car);  \
    (_jitterlisp_out) = JITTERLISP_NOTHING;        \
  JITTER_END_
#define JITTERLISP_SET_CDRB_(_jitterlisp_out,      \
                             _jitterlisp_cons,     \
                             _jitterlisp_new_cdr)  \
  JITTER_BEGIN_                                    \
    struct jitterlisp_cons *_jitterlisp_tmp        \
      = JITTERLISP_CONS_DECODE(_jitterlisp_cons);  \
    _jitterlisp_tmp->cdr = (_jitterlisp_new_cdr);  \
    (_jitterlisp_out) = JITTERLISP_NOTHING;        \
  JITTER_END_




/* Box operations.
 * ************************************************************************** */

// FIXME: reimplement with the correct box struct.

#define JITTERLISP_BOX_(_jitterlisp_out, _jitterlisp_in0)         \
  JITTER_BEGIN_                                                   \
    struct jitterlisp_cons *_jitterlisp_tmp                       \
      = JITTERLISP_CONS_MAKE_UNINITIALIZED_UNENCODED();           \
    _jitterlisp_tmp->car = (_jitterlisp_in0);                     \
    _jitterlisp_tmp->cdr = JITTERLISP_NOTHING;                    \
    (_jitterlisp_out) = JITTERLISP_CONS_ENCODE(_jitterlisp_tmp);  \
  JITTER_END_

#define JITTERLISP_BOX_GET_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                                \
    jitterlisp_object _jitterlisp_tmp                          \
      = JITTERLISP_EXP_B_A_GET(_jitterlisp_in0);               \
    (_jitterlisp_out) = _jitterlisp_tmp;                       \
  JITTER_END_

#define JITTERLISP_BOX_SETB_(_jitterlisp_out,          \
                             _jitterlisp_box,          \
                             _jitterlisp_new_content)  \
  JITTERLISP_SET_CARB_(_jitterlisp_out,                \
                       _jitterlisp_box,                \
                       _jitterlisp_new_content)




/* Closure operations.
 * ************************************************************************** */

/* Make a fresh interpreted closure with the given fields. */
#define JITTERLISP_CLOSURE_(_jitterlisp_out,                                \
                            _jitterlisp_in0,                                \
                            _jitterlisp_in1,                                \
                            _jitterlisp_in2)                                \
  JITTER_BEGIN_                                                             \
    struct jitterlisp_closure *_jitterlisp_tmp                              \
      = JITTERLISP_CLOSURE_MAKE_UNINITIALIZED_UNENCODED();                  \
    _jitterlisp_tmp->kind = jitterlisp_closure_type_interpreted;            \
    jitterlisp_object _jitterlisp_formals = (_jitterlisp_in1);              \
    _jitterlisp_tmp->interpreted.formals = _jitterlisp_formals;             \
    _jitterlisp_tmp->in_arity = 0;                                          \
    while (! JITTERLISP_IS_EMPTY_LIST(_jitterlisp_formals))                 \
      {                                                                     \
        _jitterlisp_formals = JITTERLISP_EXP_C_A_CDR(_jitterlisp_formals);  \
        _jitterlisp_tmp->in_arity ++;                                       \
      };                                                                    \
    _jitterlisp_tmp->interpreted.environment = (_jitterlisp_in0);           \
    _jitterlisp_tmp->interpreted.body = (_jitterlisp_in2);                  \
    (_jitterlisp_out) = JITTERLISP_CLOSURE_ENCODE(_jitterlisp_tmp);         \
  JITTER_END_

/* Lookup a closure field. */
#define JITTERLISP_CLOSURE_IN_ARITY_(_jitterlisp_out, _jitterlisp_in0)   \
  JITTER_BEGIN_                                                          \
    struct jitterlisp_closure *_jitterlisp_tmp                           \
      = JITTERLISP_CLOSURE_DECODE(_jitterlisp_in0);                      \
    jitter_uint _jitterlisp_in_arity = _jitterlisp_tmp->in_arity;        \
    (_jitterlisp_out) = JITTERLISP_FIXNUM_ENCODE(_jitterlisp_in_arity);  \
  JITTER_END_

/* Lookup a closure field from the union within a closure, knowing its kind. */
#define JITTERLISP_KINDED_CLOSURE_FIELD_(_jitterlisp_out,                 \
                                         _jitterlisp_in0,                 \
                                         _jitterlisp_kind_suffix,         \
                                         _jitterlisp_field_name)          \
  JITTER_BEGIN_                                                           \
    struct jitterlisp_closure *_jitterlisp_tmp                            \
      = JITTERLISP_CLOSURE_DECODE(_jitterlisp_in0);                       \
    (_jitterlisp_out)                                                     \
      = _jitterlisp_tmp->_jitterlisp_kind_suffix._jitterlisp_field_name;  \
  JITTER_END_
#define JITTERLISP_INTERPRETED_CLOSURE_ENVIRONMENT_(_jitterlisp_out,  \
                                                    _jitterlisp_in0)  \
  JITTERLISP_KINDED_CLOSURE_FIELD_(_jitterlisp_out, _jitterlisp_in0,  \
                                   interpreted, environment)
#define JITTERLISP_INTERPRETED_CLOSURE_FORMALS_(_jitterlisp_out,      \
                                                _jitterlisp_in0)      \
  JITTERLISP_KINDED_CLOSURE_FIELD_(_jitterlisp_out, _jitterlisp_in0,  \
                                   interpreted, formals)
#define JITTERLISP_INTERPRETED_CLOSURE_BODY_(_jitterlisp_out,         \
                                             _jitterlisp_in0)         \
  JITTERLISP_KINDED_CLOSURE_FIELD_(_jitterlisp_out, _jitterlisp_in0,  \
                                   interpreted, body)

/* Destructively modify all the modifiable fields in an interpreted closure.  By
   setting them all in the same operation I can guarantee that no Lisp code will
   see a closure partly updated, which would be dangerous in case
   closure-updating code used the same closure.  Notice that the in-arity
   doesn't change: it is assumed to be immutable after initialization. */
#define JITTERLISP_INTERPRETED_CLOSURE_SET_(_jitterlisp_out,       \
                                            _jitterlisp_in0,       \
                                            _jitterlisp_in1,       \
                                            _jitterlisp_in2,       \
                                            _jitterlisp_in3)       \
  JITTER_BEGIN_                                                    \
    struct jitterlisp_closure *_jitterlisp_tmp                     \
      = JITTERLISP_CLOSURE_DECODE(_jitterlisp_in0);                \
    _jitterlisp_tmp->interpreted.environment = (_jitterlisp_in1);  \
    _jitterlisp_tmp->interpreted.formals = (_jitterlisp_in2);      \
    _jitterlisp_tmp->interpreted.body = (_jitterlisp_in3);         \
    (_jitterlisp_out) = JITTERLISP_NOTHING;                        \
  JITTER_END_




/* Symbol operations.
 * ************************************************************************** */

// FIXME: comment.

#define JITTERLISP_GENSYM_(_jitterlisp_out)                           \
  JITTER_BEGIN_                                                       \
    struct jitterlisp_symbol *_jitterlisp_tmp                         \
      = jitterlisp_symbol_make_uninterned ();                         \
    (_jitterlisp_out) = JITTERLISP_SYMBOL_ENCODE(_jitterlisp_tmp);    \
  JITTER_END_

#define JITTERLISP_CONSTANTP_(_jitterlisp_out,                        \
                              _jitterlisp_in0)                        \
  JITTER_BEGIN_                                                       \
    struct jitterlisp_symbol *_jitterlisp_tmp                         \
      = JITTERLISP_SYMBOL_DECODE(_jitterlisp_in0);                    \
    (_jitterlisp_out)                                                 \
      = JITTERLISP_BOOLEAN_ENCODE(_jitterlisp_tmp->global_constant);  \
  JITTER_END_

#define JITTERLISP_MAKE_CONSTANTB_(_jitterlisp_out,                     \
                                   _jitterlisp_in0)                     \
  JITTER_BEGIN_                                                         \
    JITTERLISP_SYMBOL_DECODE(_jitterlisp_in0)->global_constant = true;  \
    (_jitterlisp_out) = JITTERLISP_NOTHING;                             \
  JITTER_END_

#define JITTERLISP_DEFINEDP_(_jitterlisp_out,                \
                             _jitterlisp_in0)                \
  JITTER_BEGIN_                                              \
    struct jitterlisp_symbol *_jitterlisp_tmp                \
      = JITTERLISP_SYMBOL_DECODE(_jitterlisp_in0);           \
    jitterlisp_object _jitterlisp_value                      \
      = _jitterlisp_tmp->global_value;                       \
    (_jitterlisp_out)                                        \
      = JITTERLISP_BOOLEAN_ENCODE(_jitterlisp_value          \
                                  != JITTERLISP_UNDEFINED);  \
  JITTER_END_

#define JITTERLISP_SYMBOL_GLOBAL_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                                      \
    struct jitterlisp_symbol *_jitterlisp_tmp                        \
      = JITTERLISP_SYMBOL_DECODE(_jitterlisp_in0);                   \
    if (_jitterlisp_tmp->global_value == JITTERLISP_UNDEFINED)       \
      jitterlisp_error_cloned ("undefined global");                  \
    (_jitterlisp_out) = _jitterlisp_tmp->global_value;               \
  JITTER_END_

#define JITTERLISP_UNDEFINE_(_jitterlisp_out,              \
                             _jitterlisp_in0)              \
  JITTER_BEGIN_                                            \
    struct jitterlisp_symbol *_jitterlisp_tmp              \
      = JITTERLISP_SYMBOL_DECODE(_jitterlisp_in0);         \
    if (_jitterlisp_tmp->global_constant)                  \
      jitterlisp_error_cloned ("undefining constant");     \
    _jitterlisp_tmp->global_value = JITTERLISP_UNDEFINED;  \
    (_jitterlisp_out) = JITTERLISP_NOTHING;                \
  JITTER_END_

#define JITTERLISP_INTERNED_SYMBOLS_(_jitterlisp_out)      \
  JITTER_BEGIN_                                            \
    (_jitterlisp_out) = jitterlisp_interned_symbols ();    \
  JITTER_END_




/* Vector operations.
 * ************************************************************************** */

// FIXME: comment.

/* Set the l-value _jitterlisp_out to be a new vector with the given
   (fixnum-encoded) number of elements, each initialized to the given encoded
   value. */
#define JITTERLISP_VECTOR_MAKE_(_jitterlisp_out,                       \
                                _jitterlisp_in_elt_no,                 \
                                _jitterlisp_in_initial_elt)            \
  JITTER_BEGIN_                                                        \
    const jitter_uint _jitterlisp_elt_no_untagged                      \
      = JITTERLISP_FIXNUM_DECODE(_jitterlisp_in_elt_no);               \
    const jitterlisp_object _jitterlisp_in_initial_elt_evaluated       \
      = (_jitterlisp_in_initial_elt);                                  \
    jitterlisp_object *_jitterlisp_elts                                \
      = ((jitterlisp_object *)                                         \
         jitterlisp_allocate (                                         \
            JITTERLISP_ALIGNED_SIZE(sizeof (jitterlisp_object)         \
                                    * _jitterlisp_elt_no_untagged)));  \
    int _jitterlisp_i;                                                 \
    for (_jitterlisp_i = 0;                                            \
         _jitterlisp_i < _jitterlisp_elt_no_untagged;                  \
         _jitterlisp_i ++)                                             \
      _jitterlisp_elts [_jitterlisp_i]                                 \
        = _jitterlisp_in_initial_elt_evaluated;                        \
    struct jitterlisp_vector *_jitterlisp_tmp                          \
      = JITTERLISP_VECTOR_MAKE_UNINITIALIZED_UNENCODED();              \
    _jitterlisp_tmp->element_no                                        \
      = JITTERLISP_FIXNUM_ENCODE(_jitterlisp_elt_no_untagged);         \
    _jitterlisp_tmp->elements = _jitterlisp_elts;                      \
    (_jitterlisp_out) = JITTERLISP_VECTOR_ENCODE(_jitterlisp_tmp);     \
  JITTER_END_




/* Non-primitive macro operations.
 * ************************************************************************** */

// FIXME: comment.

#define JITTERLISP_NON_PRIMITIVE_MACRO_(_jitterlisp_out,                        \
                                        _jitterlisp_in0,                        \
                                        _jitterlisp_in1,                        \
                                        _jitterlisp_in2)                        \
  JITTER_BEGIN_                                                                 \
    struct jitterlisp_interpreted_closure *_jitterlisp_tmp                      \
      = JITTERLISP_NON_PRIMITIVE_MACRO_MAKE_UNINITIALIZED_UNENCODED();          \
    _jitterlisp_tmp->environment = _jitterlisp_in0;                             \
    _jitterlisp_tmp->formals = _jitterlisp_in1;                                 \
    _jitterlisp_tmp->body = _jitterlisp_in2;                                    \
    (_jitterlisp_out) = JITTERLISP_NON_PRIMITIVE_MACRO_ENCODE(_jitterlisp_tmp); \
  JITTER_END_




/* Comparison operations.
 * ************************************************************************** */

/* Compute a tagged boolean, #t iff the two given arguments are
   equal-by-identity. */
#define JITTERLISP_EQP_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)  \
  JITTER_BEGIN_                                                             \
    (_jitterlisp_out)                                                       \
      = JITTERLISP_BOOLEAN_ENCODE((_jitterlisp_in0) == (_jitterlisp_in1));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the two given arguments are
   not equal-by-identity. */
#define JITTERLISP_NOT_EQP_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)  \
  JITTER_BEGIN_                                                                 \
    (_jitterlisp_out)                                                           \
      = JITTERLISP_BOOLEAN_ENCODE((_jitterlisp_in0) != (_jitterlisp_in1));      \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given fixnum argument is respectively
   zero, a non-zero, positive, non-positive, negative, non-negative. */
#define JITTERLISP_COMPARE_FIXNUM_UNARY_(_jitterlisp_out, _jitterlisp_in0,  \
                                         _jitterlisp_expression_operator,   \
                                         _jitterlisp_right)                 \
  JITTER_BEGIN_                                                             \
    (_jitterlisp_out) =                                                     \
      _jitterlisp_expression_operator(                                      \
         (_jitterlisp_in0), JITTERLISP_FIXNUM_ENCODE(_jitterlisp_right));   \
  JITTER_END_
#define JITTERLISP_ZEROP_(_jitterlisp_out, _jitterlisp_in0)           \
  JITTERLISP_COMPARE_FIXNUM_UNARY_(_jitterlisp_out, _jitterlisp_in0,  \
                                   JITTERLISP_EXP_FF_B_EQUAL, 0)
#define JITTERLISP_NON_ZEROP_(_jitterlisp_out, _jitterlisp_in0)       \
  JITTERLISP_COMPARE_FIXNUM_UNARY_(_jitterlisp_out, _jitterlisp_in0,  \
                                   JITTERLISP_EXP_FF_B_NOTEQUAL, 0)
#define JITTERLISP_POSITIVEP_(_jitterlisp_out, _jitterlisp_in0)           \
  (_jitterlisp_out)                                                       \
    = JITTER_CONDITIONAL_IF_POSITIVE (jitterlisp_object, jitter_int,      \
                                      _jitterlisp_in0,                    \
                                      JITTERLISP_TRUE, JITTERLISP_FALSE)
#define JITTERLISP_NON_POSITIVEP_(_jitterlisp_out, _jitterlisp_in0)          \
  (_jitterlisp_out)                                                          \
    = JITTER_CONDITIONAL_IF_NONPOSITIVE (jitterlisp_object, jitter_int,      \
                                         _jitterlisp_in0,                    \
                                         JITTERLISP_TRUE, JITTERLISP_FALSE)
#define JITTERLISP_NEGATIVEP_(_jitterlisp_out, _jitterlisp_in0)           \
  (_jitterlisp_out)                                                       \
    = JITTER_CONDITIONAL_IF_NEGATIVE (jitterlisp_object, jitter_int,      \
                                      _jitterlisp_in0,                    \
                                      JITTERLISP_TRUE, JITTERLISP_FALSE)
#define JITTERLISP_NON_NEGATIVEP_(_jitterlisp_out, _jitterlisp_in0)          \
  (_jitterlisp_out)                                                          \
    = JITTER_CONDITIONAL_IF_NONNEGATIVE (jitterlisp_object, jitter_int,      \
                                         _jitterlisp_in0,                    \
                                         JITTERLISP_TRUE, JITTERLISP_FALSE)




/* Type checking operations.
 * ************************************************************************** */

/* Compute a tagged boolean, #t iff the given in-argument is () . */
#define JITTERLISP_NULLP_(_jitterlisp_out, _jitterlisp_in0)                    \
  JITTER_BEGIN_                                                                \
    (_jitterlisp_out)                                                          \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_EMPTY_LIST(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is not () . */
#define JITTERLISP_NON_NULLP_(_jitterlisp_out, _jitterlisp_in0)                 \
  JITTER_BEGIN_                                                                 \
    (_jitterlisp_out)                                                           \
      = JITTERLISP_BOOLEAN_ENCODE(! JITTERLISP_IS_EMPTY_LIST(_jitterlisp_in0)); \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a fixnum . */
#define JITTERLISP_FIXNUMP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                            \
    (_jitterlisp_out)                                                      \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_FIXNUM(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a number . */
#define JITTERLISP_NUMBERP_(_jitterlisp_out, _jitterlisp_in0)             \
  JITTER_BEGIN_                                                           \
    (_jitterlisp_out)                                                     \
      = (JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_FIXNUM(_jitterlisp_in0)  \
       /* || ... There are no other numbers yet. */));                    \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a character . */
#define JITTERLISP_CHARACTERP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                               \
    (_jitterlisp_out)                                                         \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_CHARACTER(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is unique . */
#define JITTERLISP_UNIQUEP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                            \
    (_jitterlisp_out)                                                      \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_UNIQUE(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a boolean . */
#define JITTERLISP_BOOLEANP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                             \
    (_jitterlisp_out)                                                       \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_BOOLEAN(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is the eof object . */
#define JITTERLISP_EOFP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                         \
    (_jitterlisp_out)                                                   \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_EOF(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is the nothing
   object. */
#define JITTERLISP_NOTHINGP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                             \
    (_jitterlisp_out)                                                       \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_NOTHING(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is the undefined
   object. */
#define JITTERLISP_UNDEFINEDP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                               \
    (_jitterlisp_out)                                                         \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_UNDEFINED(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a cons . */
#define JITTERLISP_CONSP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                          \
    (_jitterlisp_out)                                                    \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_CONS(_jitterlisp_in0));  \
  JITTER_END_
/* Compute a tagged boolean, #t iff the given in-argument is not a cons . */
#define JITTERLISP_NON_CONSP_(_jitterlisp_out, _jitterlisp_in0)            \
  JITTER_BEGIN_                                                            \
    (_jitterlisp_out)                                                      \
      = JITTERLISP_BOOLEAN_ENCODE(! JITTERLISP_IS_CONS(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a box . */
#define JITTERLISP_BOXP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                         \
    (_jitterlisp_out)                                                   \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_BOX(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a symbol . */
#define JITTERLISP_SYMBOLP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                            \
    (_jitterlisp_out)                                                      \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_SYMBOL(_jitterlisp_in0));  \
  JITTER_END_
/* Compute a tagged boolean, #t iff the given in-argument is not a symbol . */
#define JITTERLISP_NON_SYMBOLP_(_jitterlisp_out, _jitterlisp_in0)            \
  JITTER_BEGIN_                                                              \
    (_jitterlisp_out)                                                        \
      = JITTERLISP_BOOLEAN_ENCODE(! JITTERLISP_IS_SYMBOL(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a primitive. */
#define JITTERLISP_PRIMITIVEP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                               \
    (_jitterlisp_out)                                                         \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_PRIMITIVE(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is (respectively)
   a closure, an interpreted closure, a compiled closure. */
#define JITTERLISP_CLOSUREP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                             \
    (_jitterlisp_out)                                                       \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_CLOSURE(_jitterlisp_in0));  \
  JITTER_END_
#define JITTERLISP_INTERPRETED_CLOSUREP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                                             \
    (_jitterlisp_out)                                                       \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_INTERPRETED_CLOSURE(        \
           _jitterlisp_in0));  \
  JITTER_END_
#define JITTERLISP_COMPILED_CLOSUREP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTER_BEGIN_                                                          \
    (_jitterlisp_out)                                                    \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_COMPILED_CLOSURE(        \
           _jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a macro; this
   doesn't distinguish between primitive macros and macro closures. */
#define JITTERLISP_MACROP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                           \
    (_jitterlisp_out)                                                     \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_MACRO(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is an AST; this
   doesn't distinguish between primitive asts and ast closures. */
#define JITTERLISP_ASTP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                         \
    (_jitterlisp_out)                                                   \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_AST(_jitterlisp_in0));  \
  JITTER_END_

/* Compute a tagged boolean, #t iff the given in-argument is a vector . */
#define JITTERLISP_VECTORP_(_jitterlisp_out, _jitterlisp_in0)              \
  JITTER_BEGIN_                                                            \
    (_jitterlisp_out)                                                      \
      = JITTERLISP_BOOLEAN_ENCODE(JITTERLISP_IS_VECTOR(_jitterlisp_in0));  \
  JITTER_END_




/* I/O operations.
 * ************************************************************************** */

#define JITTERLISP_DISPLAY_(_jitterlisp_out, _jitterlisp_in0)      \
  JITTER_BEGIN_                                                    \
    jitterlisp_print (jitterlisp_print_context, _jitterlisp_in0);  \
    (_jitterlisp_out) = JITTERLISP_NOTHING;                        \
  JITTER_END_

#define JITTERLISP_CHARACTER_DISPLAY_(_jitterlisp_out, _jitterlisp_in0)   \
  JITTER_BEGIN_                                                           \
    jitter_int _jitterlisp_character                                      \
      = JITTERLISP_CHARACTER_DECODE(_jitterlisp_in0);                     \
    jitter_print_char (jitterlisp_print_context, _jitterlisp_character);  \
    jitter_print_flush (jitterlisp_print_context);                        \
    (_jitterlisp_out) = JITTERLISP_NOTHING;                               \
  JITTER_END_

#define JITTERLISP_CHARACTER_READ_(_jitterlisp_out)              \
  JITTER_BEGIN_                                                  \
    jitter_int _jitterlisp_character = getchar ();               \
    (_jitterlisp_out)                                            \
      = ((_jitterlisp_character == EOF)                          \
         ? JITTERLISP_EOF                                        \
         : JITTERLISP_CHARACTER_ENCODE(_jitterlisp_character));  \
  JITTER_END_

#define JITTERLISP_NEWLINE_(_jitterlisp_out)             \
  JITTER_BEGIN_                                          \
    jitter_print_char (jitterlisp_print_context, '\n');  \
    (_jitterlisp_out) = JITTERLISP_NOTHING;              \
  JITTER_END_

#define JITTERLISP_READ_(_jitterlisp_out)                     \
  JITTER_BEGIN_                                               \
    (_jitterlisp_out) = jitterlisp_read_readline_one ("> ");  \
  JITTER_END_




/* Error handling operations.
 * ************************************************************************** */

#define JITTERLISP_ERROR_(_jitterlisp_out, _jitterlisp_in0)           \
  JITTER_BEGIN_                                                       \
    jitterlisp_print_error_char_star ("Error: ");                     \
    jitterlisp_print_error (_jitterlisp_in0);                         \
    jitterlisp_print_error_char_star ("\n");                          \
    /* This should never be used, but let's initialize it in case */  \
    /* it remains visible because of some bug. */                     \
    (_jitterlisp_out) = JITTERLISP_UNDEFINED;                         \
    /* Error out. */                                                  \
    jitterlisp_error_cloned ("erroring out from Lisp");               \
  JITTER_END_




/* GC operations.
 * ************************************************************************** */

#define JITTERLISP_GC_(_jitterlisp_out)      \
  JITTER_BEGIN_                              \
    jitterlisp_gc ();                        \
    (_jitterlisp_out) = JITTERLISP_NOTHING;  \
  JITTER_END_




/* AST operations.
 * ************************************************************************** */

/* AST case checking. */
#define JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0,             \
                              _jitterlisp_lowercase_case_name)              \
  JITTER_BEGIN_                                                             \
    bool _jitterlisp_right_case                                             \
      = (JITTERLISP_AST_DECODE(_jitterlisp_in0)->case_                      \
         == JITTER_CONCATENATE_TWO(jitterlisp_ast_case_,                    \
                                   _jitterlisp_lowercase_case_name));       \
    (_jitterlisp_out) = JITTERLISP_BOOLEAN_ENCODE(_jitterlisp_right_case);  \
  JITTER_END_
#define JITTERLISP_AST_LITERALP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, literal)
#define JITTERLISP_AST_VARIABLEP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, variable)
#define JITTERLISP_AST_DEFINEP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, define)
#define JITTERLISP_AST_IFP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, if)
#define JITTERLISP_AST_SETBP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, setb)
#define JITTERLISP_AST_WHILEP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, while)
#define JITTERLISP_AST_PRIMITIVEP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, primitive)
#define JITTERLISP_AST_CALLP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, call)
#define JITTERLISP_AST_LAMBDAP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, lambda)
#define JITTERLISP_AST_LETP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, let)
#define JITTERLISP_AST_SEQUENCEP_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_CASEP_(_jitterlisp_out, _jitterlisp_in0, sequence)

/* AST construction. */
#define JITTERLISP_AST_MAKE_(_jitterlisp_lowercase_case_name,     \
                             _jitterlisp_out, ...)                \
  JITTER_BEGIN_                                                   \
    (_jitterlisp_out)                                             \
      = JITTER_CONCATENATE_TWO(jitterlisp_ast_make_,              \
                               _jitterlisp_lowercase_case_name)(  \
           __VA_ARGS__);                                          \
  JITTER_END_
#define JITTERLISP_AST_LITERAL_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_MAKE_(literal, _jitterlisp_out, _jitterlisp_in0)
#define JITTERLISP_AST_VARIABLE_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_MAKE_(variable, _jitterlisp_out, _jitterlisp_in0)
#define JITTERLISP_AST_DEFINE_(_jitterlisp_out, _jitterlisp_in0,  \
                               _jitterlisp_in1)                   \
  JITTERLISP_AST_MAKE_(define, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1)
#define JITTERLISP_AST_IF_(_jitterlisp_out, _jitterlisp_in0,  \
                           _jitterlisp_in1, _jitterlisp_in2)  \
  JITTERLISP_AST_MAKE_(if, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1, _jitterlisp_in2)
#define JITTERLISP_AST_SETB_(_jitterlisp_out, _jitterlisp_in0,  \
                             _jitterlisp_in1)                   \
  JITTERLISP_AST_MAKE_(setb, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1)
#define JITTERLISP_AST_WHILE_(_jitterlisp_out, _jitterlisp_in0,  \
                              _jitterlisp_in1)                   \
  JITTERLISP_AST_MAKE_(while, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1)
#define JITTERLISP_AST_PRIMITIVE_(_jitterlisp_out, _jitterlisp_in0,  \
                                  _jitterlisp_in1)                   \
  JITTERLISP_AST_MAKE_(primitive, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1)
#define JITTERLISP_AST_CALL_(_jitterlisp_out, _jitterlisp_in0,  \
                             _jitterlisp_in1)                   \
  JITTERLISP_AST_MAKE_(call, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1)
#define JITTERLISP_AST_LAMBDA_(_jitterlisp_out, _jitterlisp_in0,  \
                               _jitterlisp_in1)                   \
  JITTERLISP_AST_MAKE_(lambda, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1)
#define JITTERLISP_AST_LET_(_jitterlisp_out, _jitterlisp_in0,  \
                            _jitterlisp_in1, _jitterlisp_in2)  \
  JITTERLISP_AST_MAKE_(let, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1, _jitterlisp_in2)
#define JITTERLISP_AST_SEQUENCE_(_jitterlisp_out, _jitterlisp_in0,  \
                                 _jitterlisp_in1)                   \
  JITTERLISP_AST_MAKE_(sequence, _jitterlisp_out, _jitterlisp_in0,  \
                       _jitterlisp_in1)

/* AST accessors. */
#define JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0,        \
                            _jitterlisp_sub_index,                   \
                            _jitterlisp_lowercase_case_name)         \
  JITTER_BEGIN_                                                      \
    struct jitterlisp_ast *_jitterlisp_decoded_ast                   \
      = JITTERLISP_AST_DECODE(_jitterlisp_in0);                      \
    if (_jitterlisp_decoded_ast->case_                               \
        != JITTER_CONCATENATE_TWO(jitterlisp_ast_case_,              \
                                  _jitterlisp_lowercase_case_name))  \
      jitterlisp_error_cloned                                        \
         ("invalid AST case: non-"                                   \
          JITTER_STRINGIFY(_jitterlisp_lowercase_case_name));        \
    (_jitterlisp_out)                                                \
      = _jitterlisp_decoded_ast->subs[_jitterlisp_sub_index];        \
  JITTER_END_
#define JITTERLISP_AST_GET_OPERANDS_(_jitterlisp_out, _jitterlisp_in0,    \
                                     _jitterlisp_lowercase_case_name)     \
  JITTER_BEGIN_                                                           \
    jitterlisp_object _jitterlisp_in0_value = (_jitterlisp_in0);          \
    if (JITTERLISP_AST_DECODE(_jitterlisp_in0_value)->case_               \
        != JITTER_CONCATENATE_TWO(jitterlisp_ast_case_,                   \
                                  _jitterlisp_lowercase_case_name))       \
      jitterlisp_error_cloned                                             \
         ("invalid AST case: non-"                                        \
          JITTER_STRINGIFY(_jitterlisp_lowercase_case_name));             \
    (_jitterlisp_out) = jitterlisp_ast_operands (_jitterlisp_in0_value);  \
  JITTER_END_
#define JITTERLISP_AST_LITERAL_VALUE_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, literal)
#define JITTERLISP_AST_VARIABLE_NAME_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, variable)
#define JITTERLISP_AST_DEFINE_NAME_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, define)
#define JITTERLISP_AST_DEFINE_BODY_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 1, define)
#define JITTERLISP_AST_IF_CONDITION_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, if)
#define JITTERLISP_AST_IF_THEN_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 1, if)
#define JITTERLISP_AST_IF_ELSE_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 2, if)
#define JITTERLISP_AST_SETB_NAME_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, setb)
#define JITTERLISP_AST_SETB_BODY_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 1, setb)
#define JITTERLISP_AST_WHILE_GUARD_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, while)
#define JITTERLISP_AST_WHILE_BODY_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 1, while)
#define JITTERLISP_AST_PRIMITIVE_OPERATOR_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, primitive)
#define JITTERLISP_AST_PRIMITIVE_OPERANDS_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_OPERANDS_(_jitterlisp_out, _jitterlisp_in0, primitive)
#define JITTERLISP_AST_CALL_OPERATOR_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, call)
#define JITTERLISP_AST_CALL_OPERANDS_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_OPERANDS_(_jitterlisp_out, _jitterlisp_in0, call)
#define JITTERLISP_AST_LAMBDA_FORMALS_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, lambda)
#define JITTERLISP_AST_LAMBDA_BODY_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 1, lambda)
#define JITTERLISP_AST_LET_BOUND_NAME_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, let)
#define JITTERLISP_AST_LET_BOUND_FORM_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 1, let)
#define JITTERLISP_AST_LET_BODY_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 2, let)
#define JITTERLISP_AST_SEQUENCE_FIRST_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 0, sequence)
#define JITTERLISP_AST_SEQUENCE_SECOND_(_jitterlisp_out, _jitterlisp_in0)  \
  JITTERLISP_AST_GET_(_jitterlisp_out, _jitterlisp_in0, 1, sequence)




/* Interpretation operations.
 * ************************************************************************** */

/* Macroexpand the given expression in the given non-global expansion-time
   environment. */
#define JITTERLISP_MACROEXPAND_(_jitterlisp_out, _jitterlisp_in0,  \
                                _jitterlisp_in1)                   \
  JITTER_BEGIN_                                                    \
    _jitterlisp_out = jitterlisp_macroexpand (_jitterlisp_in0,     \
                                              _jitterlisp_in1);    \
  JITTER_END_

/* Eval the given expression in the given non-global environment, using
   the AST interpreter, the VM, or the default engine, respectively. */
#define JITTERLISP_EVAL_INTERPRETER_(_jitterlisp_out, _jitterlisp_in0,       \
                                     _jitterlisp_in1)                        \
  JITTER_BEGIN_                                                              \
    _jitterlisp_out = jitterlisp_eval_interpreter (_jitterlisp_in0,          \
                                                   _jitterlisp_in1);         \
  JITTER_END_
#define JITTERLISP_EVAL_VM_(_jitterlisp_out, _jitterlisp_in0,  \
                            _jitterlisp_in1)                   \
  JITTER_BEGIN_                                                \
    _jitterlisp_out = jitterlisp_eval_vm (_jitterlisp_in0,     \
                                          _jitterlisp_in1);    \
  JITTER_END_
#define JITTERLISP_EVAL_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)  \
  JITTER_BEGIN_                                                              \
    _jitterlisp_out = jitterlisp_eval_interpreter (_jitterlisp_in0,          \
                                                   _jitterlisp_in1);         \
  JITTER_END_

/* Return the result of applying the given operator, already evaluated, to the
   given list of already evaluated operands; use the AST interpreter, the
   Jittery VM or the default engine, respectively. */
#define JITTERLISP_APPLY_INTERPRETER_(_jitterlisp_out, _jitterlisp_in0,       \
                                      _jitterlisp_in1)                        \
  JITTER_BEGIN_                                                               \
    _jitterlisp_out = jitterlisp_apply_interpreter (_jitterlisp_in0,          \
                                                    _jitterlisp_in1);         \
  JITTER_END_
#define JITTERLISP_APPLY_VM_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1) \
  JITTER_BEGIN_                                                                 \
    _jitterlisp_out = jitterlisp_apply_vm (_jitterlisp_in0, _jitterlisp_in1);   \
  JITTER_END_
#define JITTERLISP_APPLY_(_jitterlisp_out, _jitterlisp_in0, _jitterlisp_in1)  \
  JITTER_BEGIN_                                                               \
    _jitterlisp_out = jitterlisp_apply (_jitterlisp_in0, _jitterlisp_in1);    \
  JITTER_END_

/* Return the result of applying the given primitive to the given list of
   already evaluated operands. */
#define JITTERLISP_APPLY_PRIMITIVE_(_jitterlisp_out, _jitterlisp_in0,  \
                                    _jitterlisp_in1)                   \
  JITTER_BEGIN_                                                        \
    _jitterlisp_out = jitterlisp_apply_primitive (_jitterlisp_in0,     \
                                                  _jitterlisp_in1);    \
  JITTER_END_




/* Compilation operations.
 * ************************************************************************** */

/* Call the C part of the code generator, making a closure compiled.  This
   destructively modifies all the fields in a closure, be it interpreted or
   compiled, making it compiled. */
#define JITTERLISP_INTERPRETED_CLOSURE_MAKE_COMPILEDB_(_jitterlisp_out,  \
                                                       _jitterlisp_in0,  \
                                                       _jitterlisp_in1,  \
                                                       _jitterlisp_in2,  \
                                                       _jitterlisp_in3)  \
  JITTER_BEGIN_                                                          \
    struct jitterlisp_closure *_jitterlisp_closure                       \
      = JITTERLISP_CLOSURE_DECODE(_jitterlisp_in0);                      \
    jitter_int _jitterlisp_in_arity                                      \
      = JITTERLISP_FIXNUM_DECODE(_jitterlisp_in1);                       \
    jitterlisp_object _jitterlisp_nonlocals = (_jitterlisp_in2);         \
    jitterlisp_object _jitterlisp_code = (_jitterlisp_in3);              \
    jitterlisp_compile (_jitterlisp_closure, _jitterlisp_in_arity,       \
                        _jitterlisp_nonlocals, _jitterlisp_code);        \
    (_jitterlisp_out) = JITTERLISP_NOTHING;                              \
  JITTER_END_

/* Print VM code from a compiled closure in human-readable form. */
#define JITTERLISP_COMPILED_CLOSURE_PRINT_(_jitterlisp_out,      \
                                           _jitterlisp_in0)      \
  JITTER_BEGIN_                                                  \
    struct jitterlisp_compiled_closure *_jitterlisp_cc           \
      = & JITTERLISP_CLOSURE_DECODE(_jitterlisp_in0)->compiled;  \
    jitterlisp_print_compiled_closure (_jitterlisp_cc);          \
    (_jitterlisp_out) = JITTERLISP_NOTHING;                      \
  JITTER_END_

/* Disassemble native code from a compiled closure. */
#define JITTERLISP_COMPILED_CLOSURE_DISASSEMBLE_(_jitterlisp_out,  \
                                                 _jitterlisp_in0)  \
  JITTER_BEGIN_                                                    \
    struct jitterlisp_compiled_closure *_jitterlisp_cc             \
      = & JITTERLISP_CLOSURE_DECODE(_jitterlisp_in0)->compiled;    \
    jitterlisp_disassemble_compiled_closure (_jitterlisp_cc);      \
    (_jitterlisp_out) = JITTERLISP_NOTHING;                        \
  JITTER_END_




/* Operations fast-branching on overflow, only used from VM code.
 * ************************************************************************** */

/* These operations are all defined just like the ones not checking for overflow
   in the case of unsafe code, ignoring the label argument.
   For safe code they use the Jitter operate-branch-fast-on-overflow macros,
   which are only usable from VM code. */

#if defined (JITTERLISP_UNSAFE)
# define JITTERLISP_PLUS_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0,      \
                                      jitterlisp_in1, label)               \
  JITTERLISP_PLUS_ ((jitterlisp_out), (jitterlisp_in0), (jitterlisp_in1))
# define JITTERLISP_MINUS_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0,      \
                                       jitterlisp_in1, label)               \
  JITTERLISP_MINUS_ ((jitterlisp_out), (jitterlisp_in0), (jitterlisp_in1))
# define JITTERLISP_TIMES_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0,      \
                                       jitterlisp_in1, label)               \
  JITTERLISP_TIMES_ ((jitterlisp_out), (jitterlisp_in0), (jitterlisp_in1))
# define JITTERLISP_DIVIDED_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0,  \
                                         jitterlisp_in1, label)           \
  JITTERLISP_DIVIDED_UNSAFE_ ((jitterlisp_out),                           \
                              (jitterlisp_in0), (jitterlisp_in1))
# define JITTERLISP_QUOTIENT_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0,  \
                                          jitterlisp_in1, label)           \
  JITTERLISP_DIVIDED_UNSAFE_ ((jitterlisp_out),                            \
                              (jitterlisp_in0), (jitterlisp_in1))
# define JITTERLISP_REMAINDER_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0,  \
                                           jitterlisp_in1, label)           \
  JITTERLISP_REMAINDER_UNSAFE_ ((jitterlisp_out),                           \
                                (jitterlisp_in0), (jitterlisp_in1))
# define JITTERLISP_1PLUS_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0, label)  \
  JITTERLISP_1PLUS_ ((jitterlisp_out), (jitterlisp_in0))
# define JITTERLISP_1MINUS_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0, label)  \
  JITTERLISP_1MINUS_ ((jitterlisp_out), (jitterlisp_in0))
# define JITTERLISP_2TIMES_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0, label)  \
  JITTERLISP_2TIMES_ ((jitterlisp_out), (jitterlisp_in0))
/* 2-divided never overflows. */
/* 2-quotient never overflows. */
/* 2-remainder never overflows. */
# define JITTERLISP_NEGATE_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0, label)  \
  JITTERLISP_NEGATE_ ((jitterlisp_out), (jitterlisp_in0))
#else /* safe */
# define JITTERLISP_PLUS_OR_OVERFLOW_(jitterlisp_out,                         \
                                      jitterlisp_in0, jitterlisp_in1, label)  \
    JITTER_BEGIN_                                                             \
      JITTER_PLUS_BRANCH_FAST_IF_OVERFLOW ((jitterlisp_out),                  \
                                           (jitterlisp_in0),                  \
                                           (jitterlisp_in1),                  \
                                           (label));                          \
    JITTER_END_
# define JITTERLISP_MINUS_OR_OVERFLOW_(jitterlisp_out,                         \
                                       jitterlisp_in0, jitterlisp_in1, label)  \
    JITTER_BEGIN_                                                              \
      JITTER_MINUS_BRANCH_FAST_IF_OVERFLOW ((jitterlisp_out),                  \
                                           (jitterlisp_in0),                   \
                                           (jitterlisp_in1),                   \
                                           (label));                           \
    JITTER_END_
# define JITTERLISP_TIMES_OR_OVERFLOW_(jitterlisp_out,                         \
                                       jitterlisp_in0, jitterlisp_in1, label)  \
    JITTER_BEGIN_                                                              \
      jitter_int jitterlisp_in0_decoded                                        \
        = JITTERLISP_FIXNUM_DECODE (jitterlisp_in0);                           \
      JITTER_TIMES_BRANCH_FAST_IF_OVERFLOW ((jitterlisp_out),                  \
                                            jitterlisp_in0_decoded,            \
                                            (jitterlisp_in1),                  \
                                            (label));                          \
    JITTER_END_
# define JITTERLISP_DIVIDED_OR_OVERFLOW_(jitterlisp_out,                       \
                                         jitterlisp_in0, jitterlisp_in1,       \
                                         label)                                \
    JITTER_BEGIN_                                                              \
      jitter_int jitterlisp_out_decoded;                                       \
      jitter_int jitterlisp_in0_result = (jitterlisp_in0);                     \
      jitter_int jitterlisp_in1_result = (jitterlisp_in1);                     \
      if ((jitterlisp_in0_result                                               \
           == (JITTER_MOST_NEGATIVE_SIGNED_IN_BITS (sizeof (jitter_int)        \
                                                    * CHAR_BIT)                \
               & ~ (jitter_int) ((1LU << JITTERLISP_FIXNUM_TAG_BIT_NO) - 1)))  \
          && jitterlisp_in1_result == JITTERLISP_FIXNUM_ENCODE (-1))           \
        JITTER_BRANCH_FAST (label);                                            \
      JITTER_BRANCH_FAST_IF_ZERO (jitterlisp_in1_result, (label));             \
      jitterlisp_out_decoded = (jitterlisp_in0_result                          \
                                / jitterlisp_in1_result);                      \
      (jitterlisp_out) = JITTERLISP_FIXNUM_ENCODE (jitterlisp_out_decoded);    \
    JITTER_END_
# define JITTERLISP_QUOTIENT_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0,  \
                                          jitterlisp_in1, label)           \
    JITTERLISP_DIVIDED_OR_OVERFLOW_ ((jitterlisp_out), (jitterlisp_in0),   \
                                     (jitterlisp_in1), (label))
# define JITTERLISP_REMAINDER_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0,   \
                                           jitterlisp_in1, label)            \
    JITTER_BEGIN_                                                            \
      jitter_int jitterlisp_out_decoded;                                     \
      jitter_int jitterlisp_in0_decoded                                      \
        = JITTERLISP_FIXNUM_DECODE (jitterlisp_in0);                         \
      jitter_int jitterlisp_in1_decoded                                      \
        = JITTERLISP_FIXNUM_DECODE (jitterlisp_in1);                         \
      JITTER_REMAINDER_BRANCH_FAST_IF_OVERFLOW (jitterlisp_out_decoded,      \
                                                jitterlisp_in0_decoded,      \
                                                jitterlisp_in1_decoded,      \
                                                (label));                    \
      (jitterlisp_out) = JITTERLISP_FIXNUM_ENCODE (jitterlisp_out_decoded);  \
    JITTER_END_
# define JITTERLISP_1PLUS_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0, label)  \
    JITTER_BEGIN_                                                              \
      JITTER_PLUS_BRANCH_FAST_IF_OVERFLOW ((jitterlisp_out),                   \
                                           (jitterlisp_in0),                   \
                                           JITTERLISP_FIXNUM_ENCODE (1),       \
                                           (label));                           \
    JITTER_END_
# define JITTERLISP_1MINUS_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0, label)  \
    JITTER_BEGIN_                                                               \
      JITTER_MINUS_BRANCH_FAST_IF_OVERFLOW ((jitterlisp_out),                   \
                                            (jitterlisp_in0),                   \
                                            JITTERLISP_FIXNUM_ENCODE (1),       \
                                            (label));                           \
    JITTER_END_
# define JITTERLISP_2TIMES_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0, label)  \
    JITTERLISP_TIMES_OR_OVERFLOW_ ((jitterlisp_out),                            \
                                   JITTERLISP_FIXNUM_ENCODE (2),                \
                                   (jitterlisp_in0),                            \
                                   (label))
/* 2-divided never overflows. */
/* 2-quotient never overflows. */
/* 2-remainder never overflows. */
# define JITTERLISP_NEGATE_OR_OVERFLOW_(jitterlisp_out, jitterlisp_in0, label)  \
    JITTERLISP_MINUS_OR_OVERFLOW_ (jitterlisp_out,                         \
                                   JITTERLISP_FIXNUM_ENCODE (0), \
                                   jitterlisp_in0, label)
#endif /* safe */




#endif // #ifndef JITTERLISP_OPERATIONS_H_
