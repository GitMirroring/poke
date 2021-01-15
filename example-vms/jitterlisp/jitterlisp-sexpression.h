/* JitterLisp: s-expression header.

   Copyright (C) 2017, 2018, 2019, 2020 Luca Saiu
   Updated in 2021 by Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with Jitter under the same license.

   Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTERLISP_SEXPRESSION_H_
#define JITTERLISP_SEXPRESSION_H_


/* Include headers.
 * ************************************************************************** */

#include <stdalign.h>
#include <stdbool.h>

/* We need the jitter_int and jitter_uint types. */
#include <jitter/jitter.h>

/* The macros in this header are based on the Jitter tagging subsystem. */
#include <jitter/jitter-tagging.h>

/* We rely on our CPP general-purpose macros, in particular for token
   concatenation. */
#include <jitter/jitter-cpp.h>




/* About this file.
 * ************************************************************************** */

/* An "s-expression" is a datum containing both a Lisp value and its Lisp type,
   encoded in an efficient way.  The C view of an s-expression is the
   jitterlisp_object type.

   This header provides macro definitions for encoding and decoding
   s-expressions, which is to say for converting from a C object to the tagged
   Lisp representation of the same object, and vice-versa, and for checking
   the Lisp type of an s-expression.

   S-expression allocation and memory handling are *not* covered here: see
   jitterlisp-allocator.h .  Operations on Lisp objects are not defined here
   either: see jitterlisp-operations.h . */




/* Tagged object representation.
 * ************************************************************************** */

/* A JitterLisp object is a tagged object. */
typedef jitter_tagged_object jitterlisp_object;




/* Tagged object representation: conventions.
 * ************************************************************************** */

/* Some operations on tagged objects are more efficient with specific stag
   values, particularly arithmetic and bitwise operations.  When an operation
   below needs to make such assumptions it always does it within CPP
   conditionals checking for the actual tag value.

   Tag values and widths must be kept easy to change in the future, even
   conditionally to accommodate for different hardware. */

/* For every tagged type foo the following macros are defined:
   - the tag size in bits for foos, named JITTERLISP_FOO_TAG_BIT_NO;
   - the tag for foos, named JITTERLISP_FOO_TAG;
   - the untagged C type for foos, named JITTERLISP_FOO_UNTAGGED_TYPE;
   - the macro JITTERLISP_FOO_ENCODE(untagged_exp), expanding to an r-value
     evaluating to the tagged representation of the result of the evaluation
     of untagged_exp as a foo;
   - the macro JITTERLISP_FOO_DECODE(tagged_exp), expanding to an r-value
     evaluating to the untagged representation of the result of the evaluation
     of tagged_exp, as a JITTERLISP_FOO_UNTAGGED_TYPE.
   - the macro JITTERLISP_IS_FOO(tagged_exp), expanding to an r-value which
     evaluates to a C boolean, non-false iff the tagged expression has type
     foo. */




/* Fast-branch-unless helper macro.
 * ************************************************************************** */

/* Fast-branch to the given label if the given object does not have the given
   tag, of the given number of bits.  The arguments may be evaluated more than
   once. */
#define JITTERLISP_BRANCH_FAST_UNLESS_HAS_TAG(jitterlisp_tagged_object,  \
                                              tag,                       \
                                              tag_bit_no,                \
                                              label)                     \
  do                                                                     \
    {                                                                    \
      JITTER_BRANCH_FAST_IF_AND (((jitterlisp_tagged_object)             \
                                  /* xor would work just as well as      \
                                     minus, except that on x86 the       \
                                     lea instruction can simulate a      \
                                     three-operand minus but not a       \
                                     three-operand xor. */               \
                                  - (jitter_uint) (tag)),                \
                                 ((1LU << (tag_bit_no)) - 1),            \
                                 label);                                 \
    }                                                                    \
  while (false)




/* S-expression representation: dummy "anything" type.
 * ************************************************************************** */

/* This dummy tag check succeeds with any object.  It is convenient to have for
   machine-generated code containing a tag-checking macro, which sometimes need
   no actual check. */
#define JITTERLISP_IS_ANYTHING(_jitterlisp_tagged_object)  \
  true

/* Fast-branch to the given label iff the given object is not of "anything"
   type -- which is to say, never fast branch. */
#define JITTERLISP_BRANCH_FAST_UNLESS_ANYTHING(jitterlisp_tagged_object,  \
                                               label)                     \
  do {} while (false)




/* S-expression representation: fixnums.
 * ************************************************************************** */

/* Fixnums are encoded unboxed as two's complement signed integers. */

/* The tag for fixnums.  Notice that a zero tag allows for more efficient sum
   and subtraction operations, and this is exploited in the operation
   definitions. */
#define JITTERLISP_FIXNUM_TAG_BIT_NO    4
#define JITTERLISP_FIXNUM_TAG           0b0000

/* The C type for untagged fixnums.  Notice that fixnums are always signed. */
#define JITTERLISP_FIXNUM_UNTAGGED_TYPE  jitter_int

/* Expand to an r-value evaluating to a C (untagged) boolean which is non-false
   iff the given tagged object evaluates to a fixnum. */
#define JITTERLISP_IS_FIXNUM(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),            \
                 JITTERLISP_FIXNUM_TAG,                  \
                 JITTERLISP_FIXNUM_TAG_BIT_NO)

/* Expand to an r-value evaluating to the encoded representation of the given
   untagged integer expression as a Lisp fixnum. */
#define JITTERLISP_FIXNUM_ENCODE(_jitterlisp_untagged_fixnum)  \
  JITTER_WITH_TAG_SHIFTED_ON((_jitterlisp_untagged_fixnum),    \
                             JITTERLISP_FIXNUM_TAG,            \
                             JITTERLISP_FIXNUM_TAG_BIT_NO)

/* Expand to an r-value evaluating to the untagged jitter_int content of the
   given tagged fixnum.  No type check is performed. */
#define JITTERLISP_FIXNUM_DECODE(_jitterlisp_tagged_fixnum)    \
  ((jitter_int)                                                \
   JITTER_WITH_TAG_ASHIFTED_OFF(_jitterlisp_tagged_fixnum,     \
                                JITTERLISP_FIXNUM_TAG,         \
                                JITTERLISP_FIXNUM_TAG_BIT_NO))

/* Expand to a constant expression evaluating to the number of bits used
   to represent a fixnum, not counting tag bits. */
#define JITTERLISP_FIXNUM_NON_TAG_BIT_NO  \
  JITTER_BITS_PER_WORD - JITTERLISP_FIXNUM_TAG_BIT_NO

/* Expand to a constant expression evaluating to the most (respectively)
   negative or positive fixnum, tagged. */
#define JITTERLISP_FIXNUM_MOST_NEGATIVE                                        \
  JITTERLISP_FIXNUM_ENCODE                                                     \
     (JITTER_MOST_NEGATIVE_SIGNED_IN_BITS (JITTERLISP_FIXNUM_NON_TAG_BIT_NO))
#define JITTERLISP_FIXNUM_MOST_POSITIVE                                        \
  JITTERLISP_FIXNUM_ENCODE                                                     \
     (JITTER_MOST_POSITIVE_SIGNED_IN_BITS (JITTERLISP_FIXNUM_NON_TAG_BIT_NO))

/* Fast-branch to the given label if the given object is not a fixnum.  The
   arguments may be evaluated more than once. */
#define JITTERLISP_BRANCH_FAST_UNLESS_FIXNUM(jitterlisp_tagged_object,  \
                                             label)                     \
  JITTERLISP_BRANCH_FAST_UNLESS_HAS_TAG (jitterlisp_tagged_object,      \
                                         JITTERLISP_FIXNUM_TAG,         \
                                         JITTERLISP_FIXNUM_TAG_BIT_NO,  \
                                         label)




/* S-expression representation: unique and character values.
 * ************************************************************************** */

/* Unique Lisp types populated by a single object such as (), #t and #f only need
   very few bits to represent, so can use a longer stag with a suffix to be shared
   with another type whose elements are relatively few in number.  A good candidate
   for such a type is the character type.  Distinct characters are only about one
   million in Unicode and therefore can be easily represented unboxed along with
   every unique object.

   I prefer to assume a wide fixed-width encoding for characters in memory, such
   as UCF-4; however using single UTF-8 bytes as characters works as well; each
   one will be considered an object of its own.

   This is still quite wasteful in terms of the actual bit configuration used
   in the space of the possible configurations.  Some other type might fit
   here in the future, with just one more stag bit to discriminate. */

/* How to distinguish unique values from characters. */
#define JITTERLISP_UNIQUE_TAG_BIT_NO               5
#define JITTERLISP_UNIQUE_TAG                      0b01000
#define JITTERLISP_CHARACTER_TAG_BIT_NO            5
#define JITTERLISP_CHARACTER_TAG                   0b11000

/* Expand to an r-value evaluating to a C (untagged) boolean which is non-false
   iff the given tagged object evaluates to a unique object. */
#define JITTERLISP_IS_UNIQUE(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),            \
                 JITTERLISP_UNIQUE_TAG,                  \
                 JITTERLISP_UNIQUE_TAG_BIT_NO)

/* Like JITTERLISP_IS_UNIQUE , for characters. */
#define JITTERLISP_IS_CHARACTER(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),               \
                 JITTERLISP_CHARACTER_TAG,                  \
                 JITTERLISP_CHARACTER_TAG_BIT_NO)

/* Encode operation for a character. */
#define JITTERLISP_CHARACTER_ENCODE(_jitterlisp_untagged_character)  \
  JITTER_WITH_TAG_SHIFTED_ON((_jitterlisp_untagged_character),       \
                             JITTERLISP_CHARACTER_TAG,               \
                             JITTERLISP_CHARACTER_TAG_BIT_NO)

/* Encode operation for a unique-object index. */
#define JITTERLISP_UNIQUE_ENCODE(_jitterlisp_unique_index)  \
  JITTER_WITH_TAG_SHIFTED_ON((_jitterlisp_unique_index),    \
                             JITTERLISP_UNIQUE_TAG,         \
                             JITTERLISP_UNIQUE_TAG_BIT_NO)

/* Decode operation for a character. */
#define JITTERLISP_UNIQUE_DECODE(_jitterlisp_tagged_object)   \
  JITTER_WITH_TAG_LSHIFTED_OFF((_jitterlisp_tagged_object),   \
                               JITTERLISP_UNIQUE_TAG,         \
                               JITTERLISP_UNIQUE_TAG_BIT_NO)

/* Decode operation for a unique-object index. */
#define JITTERLISP_CHARACTER_DECODE(_jitterlisp_tagged_object)   \
  JITTER_WITH_TAG_LSHIFTED_OFF((_jitterlisp_tagged_object),      \
                               JITTERLISP_CHARACTER_TAG,         \
                               JITTERLISP_CHARACTER_TAG_BIT_NO)

/* Every unique object has a unique printable name.  The array is meant to be
   indexed by decoded unique values and is defined to have exactly
   JITTERLISP_UNIQUE_OBJECT_NO elements. */
extern const char * const
jitterlisp_unique_object_names [];




/* S-expression representation: specific unique objects.
 * ************************************************************************** */

/* Unique objects are simple unboxed objects, each represented as a shifted
   index.  They are efficient to compare to, but by themselves hold no mutable
   state and no attributes. */

/* How many unique objects there are.  This of course must agree with the
   definitions below. */
#define JITTERLISP_UNIQUE_OBJECT_NO  10

/* Define every unique object.  The names in the definition of
   jitterlisp_unique_object_names must follow this order. */

/* The #f object.  In JitterLisp this is distinct from () and the symbol named
   nil, which is a symbol like any other. */
#define JITTERLISP_FALSE       JITTERLISP_UNIQUE_ENCODE(0)

/* The #t object. */
#define JITTERLISP_TRUE        JITTERLISP_UNIQUE_ENCODE(1)

/* The () object.  In JitterLisp this is distinct from #f and the symbol named
   nil, which is a symbol like any other. */
#define JITTERLISP_EMPTY_LIST  JITTERLISP_UNIQUE_ENCODE(2)

/* The end-of-file or end-of-input object.  This object has no reader syntax. */
#define JITTERLISP_EOF         JITTERLISP_UNIQUE_ENCODE(3)

/* A unique object conventionally used as the result of forms not evaluating to
   any useful result.  No reader syntax. */
#define JITTERLISP_NOTHING     JITTERLISP_UNIQUE_ENCODE(4)

/* A unique object used to represent the global value of globally unbound or
   temporarily undefined variables, for example in the expansion of letrec .
   This is used in the internal representation but should never be the result of
   a correct evaluation.  No reader syntax. */
#define JITTERLISP_UNDEFINED   JITTERLISP_UNIQUE_ENCODE(5)

/* Other unique objects which should never be visible to the user as the result
   of any evaluation, but are useful internally for debugging purposes. */
#define JITTERLISP_UNINITIALIZED_REGISTER_VALUE     JITTERLISP_UNIQUE_ENCODE(6)
#define JITTERLISP_UNINITIALIZED_MAINSTACK_VALUE    JITTERLISP_UNIQUE_ENCODE(7)
#define JITTERLISP_UNINITIALIZED_RETURNSTACK_VALUE  JITTERLISP_UNIQUE_ENCODE(8)
#define JITTERLISP_UNINITIALIZED_HEAP_VALUE         JITTERLISP_UNIQUE_ENCODE(9)

/* Unique object predicates.  Since unique objects are unboxed (and unique)
   these are simple comparisons by identity.  The expansion evaluates to a
   C (untagged) boolean r-value. */
#define JITTERLISP_IS_EMPTY_LIST(_jitterlisp_tagged_object)  \
  ((_jitterlisp_tagged_object) == JITTERLISP_EMPTY_LIST)
#define JITTERLISP_IS_TRUE(_jitterlisp_tagged_object)  \
  ((_jitterlisp_tagged_object) == JITTERLISP_TRUE)
#define JITTERLISP_IS_FALSE(_jitterlisp_tagged_object)  \
  ((_jitterlisp_tagged_object) == JITTERLISP_FALSE)
#define JITTERLISP_IS_EOF(_jitterlisp_tagged_object)  \
  ((_jitterlisp_tagged_object) == JITTERLISP_EOF)
#define JITTERLISP_IS_NOTHING(_jitterlisp_tagged_object)  \
  ((_jitterlisp_tagged_object) == JITTERLISP_NOTHING)
#define JITTERLISP_IS_UNDEFINED(_jitterlisp_tagged_object)  \
  ((_jitterlisp_tagged_object) == JITTERLISP_UNDEFINED)




/* S-expression representation: Booleans.
 * ************************************************************************** */

/* The Boolean constants JITTERLISP_FALSE and JITTERLISP_TRUE are unique values
   like any other, already defined above.  Booleans are not a separate "type"
   in the sense of tags or even extended tags, but it is convenient for the user
   to see them that way.  The following convenience macros provide the
   illusion. */

/* Expand to an r-value (untagged) boolean evaluating to non-false iff the
   given tagged object is #t or #f . */
#define JITTERLISP_IS_BOOLEAN(_jitterlisp_tagged_object)  \
  (JITTERLISP_IS_TRUE(_jitterlisp_tagged_object)          \
   || JITTERLISP_IS_FALSE(_jitterlisp_tagged_object))

/* Expand to an r-value evaluating to a tagged boolean value, true iff the given
   argument evalutes to non-false.

   This is difficult to make always efficient without requiring C's booleans to
   be canonical, but should not be very critical.  The conditional expression
   should always be easy for GCC to optimize away when the encoded value is the
   result of a C comparison accessible to the compiler rather than coming, for
   example, from a function argument. */
#define JITTERLISP_BOOLEAN_ENCODE(_jitterlisp_untagged_bool)          \
  ((_jitterlisp_untagged_bool) ? JITTERLISP_TRUE : JITTERLISP_FALSE)

/* Expand to an r-value evaluating to a C (untagged) boolean value, false
   iff the argument evaluates to the false boolean tagged value.  This is
   the preferred way of checking a JitterLisp object for falsity. */
#define JITTERLISP_BOOLEAN_DECODE(_jitterlisp_tagged_bool)  \
  ((_jitterlisp_tagged_bool) != JITTERLISP_FALSE)




/* S-expression representation: symbols.
 * ************************************************************************** */

/* Symbols are handled in a special way allocation-wise: interned symbols are
   allocated with malloc, and live until the memory subsystem is finalized.
   Uninterned symbols, on the other hand, live on the garbage-collected heap.

   Both symbol types are encoded pointers to a struct jitterlisp_symbol . */
#define JITTERLISP_SYMBOL_TAG_BIT_NO  3
#define JITTERLISP_SYMBOL_TAG         0b001

/* A symbol datum. */
struct jitterlisp_symbol
{
  /* The symbol name as a malloc-allocated string, or NULL if the symbol is not
     interned. */
  char *name_or_NULL;

  /* The value bound to the symbol in the global environment, or
     JITTERLISP_UNDEFINED if there is no global binding.

     FIXME: this makes each symbol a GC root which will require some careful
     testing in the case of interned symbols, as they are malloc-allocated. */
  jitterlisp_object global_value;

  /* A unique index for uninterned symbols, used for the compact printed
     representation.  Not used for interned symbols. */
  jitter_uint index;

  /* Non-false if the global binding for the symbol is constant. */
  bool global_constant;
};

/* Symbol tag checking, encoding and decoding. */
#define JITTERLISP_IS_SYMBOL(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),            \
                 JITTERLISP_SYMBOL_TAG,                  \
                 JITTERLISP_SYMBOL_TAG_BIT_NO)
#define JITTERLISP_SYMBOL_ENCODE(_jitterlisp_untagged_symbol)  \
  JITTER_WITH_TAG_ADDED(_jitterlisp_untagged_symbol,           \
                        JITTERLISP_SYMBOL_TAG,                 \
                        JITTERLISP_SYMBOL_TAG_BIT_NO)
#define JITTERLISP_SYMBOL_DECODE(_jitterlisp_tagged_symbol)     \
  ((struct jitterlisp_symbol *)                                 \
   (JITTER_WITH_TAG_SUBTRACTED((_jitterlisp_tagged_symbol),     \
                               JITTERLISP_SYMBOL_TAG,           \
                               JITTERLISP_SYMBOL_TAG_BIT_NO)))




/* S-expression representation: conses.
 * ************************************************************************** */

/* Conses are represented boxed, with no header.  Accessing the car and cdr
   fields in memory is a frequent operation so it's particularly important to
   decode tagged conses with subtractions, which are often optimizable. */

#define JITTERLISP_CONS_TAG_BIT_NO    3
#define JITTERLISP_CONS_TAG           0b010

/* A cons datum. */
struct jitterlisp_cons
{
  /* The first cons field. */
  jitterlisp_object car;

  /* The second cons field. */
  jitterlisp_object cdr;
};

/* Cons tag checking, encoding and decoding. */
#define JITTERLISP_IS_CONS(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),          \
                 JITTERLISP_CONS_TAG,                  \
                 JITTERLISP_CONS_TAG_BIT_NO)
#define JITTERLISP_CONS_ENCODE(_jitterlisp_untagged_cons)  \
  JITTER_WITH_TAG_ADDED(_jitterlisp_untagged_cons,         \
                        JITTERLISP_CONS_TAG,               \
                        JITTERLISP_CONS_TAG_BIT_NO)
#define JITTERLISP_CONS_DECODE(_jitterlisp_tagged_cons)       \
  ((struct jitterlisp_cons *)                                 \
   (JITTER_WITH_TAG_SUBTRACTED((_jitterlisp_tagged_cons),     \
                               JITTERLISP_CONS_TAG,           \
                               JITTERLISP_CONS_TAG_BIT_NO)))

/* Fast-branch to the given label if the given object is not a cons.  The
   arguments may be evaluated more than once. */
#define JITTERLISP_BRANCH_FAST_UNLESS_CONS(jitterlisp_tagged_object,  \
                                           label)                     \
  JITTERLISP_BRANCH_FAST_UNLESS_HAS_TAG (jitterlisp_tagged_object,    \
                                         JITTERLISP_CONS_TAG,         \
                                         JITTERLISP_CONS_TAG_BIT_NO,  \
                                         label)




/* S-expression representation: boxes.
 * ************************************************************************** */

/* This implementation of boxes is a temporary hack.  Boxed should have their
   own tag, and not just be conses with a particular shape. */

#define JITTERLISP_IS_BOX(_jitterlisp_tagged_object)     \
  (JITTERLISP_IS_CONS(_jitterlisp_tagged_object)         \
   && (JITTERLISP_EXP_C_A_CDR(_jitterlisp_tagged_object) \
       == JITTERLISP_NOTHING))

/* Fast-branch to the given label if the given object is not a box.  The
   arguments may be evaluated more than once. */
#define JITTERLISP_BRANCH_FAST_UNLESS_BOX(jitterlisp_tagged_object,    \
                                          label)                       \
  /* Again, this is obviously a temporary hack for the temporary       \
     representation. */                                                \
  do                                                                   \
    {                                                                  \
      JITTERLISP_BRANCH_FAST_UNLESS_CONS (jitterlisp_tagged_object,    \
                                          label);                      \
      JITTER_BRANCH_FAST_IF_NOTEQUAL (JITTERLISP_EXP_C_A_CDR           \
                                         (jitterlisp_tagged_object),   \
                                      JITTERLISP_NOTHING,              \
                                      label);                          \
    }                                                                  \
  while (false)




/* S-expression representation: closures.
 * ************************************************************************** */

/* Closures are represented boxed, with no header. */

#define JITTERLISP_CLOSURE_TAG_BIT_NO    3
#define JITTERLISP_CLOSURE_TAG           0b011

/* A C type specifying the kind of a closure. */
enum jitterlisp_closure_kind
  {
    /* An interpreted closure, run using the AST interpreter. */
    jitterlisp_closure_type_interpreted,

    /* A compiled closure, run using the Jittery VM. */
    jitterlisp_closure_type_compiled
  };

/* The fields of an intepreted closure object. */
struct jitterlisp_interpreted_closure
{
  /* The non-global environment, as an a-list. */
  jitterlisp_object environment;

  /* The procedure formal arguments as a list of distinct symbols. */
  jitterlisp_object formals;

  /* The procedure body as an AST. */
  jitterlisp_object body;
};

/* The fields of a compiled closure object. */
struct jitterlisp_compiled_closure
{
  /* How many nonlocals there are.  FIXME: remove this unless it's needed
     for garbage collection. */
  jitter_uint nonlocal_no;

  /* Nonlocals as used in compiled code, with no names and with boxed nonlocals
     stored as boxes; NULL if no nonlocals are needed.

     This is currently a list; it should become a vector, ideally with immutable
     size to avoid an indirection, after I properly implement vectors. */
  jitterlisp_object nonlocals;

  /* The non-executable VM routine for the closure code.  FIXME: I may want to
     remove this, or make it optional for disassembling only, after the new
     Jitter API allows me to free this independently from executable_routine. */
  /* This is actually a struct jitterlispvm_mutable_routine * object, but I'm declaring
     this as a generic pointer to avoid cyclical CPP inclusion. */
  void *mutable_routine;

  /* The executable VM routine for the closure code. */
  /* This is actually a struct jitterlispvm_executable_routine * object, but I'm
     declaring this as a generic pointer to avoid cyclical CPP inclusion. */
  void *executable_routine;

  /* The first program point of the VM routine above, always a prolog
     instructions.  This could be extracted as a field from the code itself, but
     it's better to keep a copy here and avoid a memory dereference at call
     time.
     This is actually a jitterlispvm_program_point object, but we declare it as
     a const void * (which is safe: jitterlispvm_program_point is a constant
     pointer type on every dispatching model) to avoid cyclical CPP inclusion
     problems. */
  const void *first_program_point;
};

/* A closure datum. */
struct jitterlisp_closure
{
  /* The kind of this closure. */
  enum jitterlisp_closure_kind kind;

  /* How many arguments this closure takes. */
  jitter_uint in_arity;

  /* The kind determines which field of the anonymous union is actually used.
     Notice that it's allowed, and useful, for a closure to change kind at run
     time without changing its identity. */
  union
  {
    /* An interpreted closure. */
    struct jitterlisp_interpreted_closure interpreted;

    /* An compiled closure. */
    struct jitterlisp_compiled_closure compiled;
  };
};


/* Closure tag checking, encoding and decoding. */
#define JITTERLISP_IS_CLOSURE(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),             \
                 JITTERLISP_CLOSURE_TAG,                  \
                 JITTERLISP_CLOSURE_TAG_BIT_NO)
#define JITTERLISP_CLOSURE_ENCODE(_jitterlisp_untagged_closure)  \
  JITTER_WITH_TAG_ADDED(_jitterlisp_untagged_closure,            \
                        JITTERLISP_CLOSURE_TAG,                  \
                        JITTERLISP_CLOSURE_TAG_BIT_NO)
#define JITTERLISP_CLOSURE_DECODE(_jitterlisp_tagged_closure)    \
  ((struct jitterlisp_closure *)                                 \
   (JITTER_WITH_TAG_SUBTRACTED((_jitterlisp_tagged_closure),     \
                               JITTERLISP_CLOSURE_TAG,           \
                               JITTERLISP_CLOSURE_TAG_BIT_NO)))

/* Closure tag and kind checking. */
#define JITTERLISP_IS_INTERPRETED_CLOSURE(_jitterlisp_tagged_object)  \
  (JITTERLISP_IS_CLOSURE(_jitterlisp_tagged_object)                   \
   && (JITTERLISP_CLOSURE_DECODE(_jitterlisp_tagged_object)->kind     \
       == jitterlisp_closure_type_interpreted))
#define JITTERLISP_IS_COMPILED_CLOSURE(_jitterlisp_tagged_object)  \
  (JITTERLISP_IS_CLOSURE(_jitterlisp_tagged_object)                   \
   && (JITTERLISP_CLOSURE_DECODE(_jitterlisp_tagged_object)->kind     \
       == jitterlisp_closure_type_compiled))




/* S-expression representation: primitives.
 * ************************************************************************** */

/* Primitives are represented boxed. */

#define JITTERLISP_PRIMITIVE_TAG_BIT_NO    3
#define JITTERLISP_PRIMITIVE_TAG           0b100

/* How many arguments a primitive can take, as a maximum. */
#define JITTERLISP_PRIMITIVE_MAX_IN_ARITY   6

/* Primitives are call-by-value (therefore they all behave as procedures: if and
   or , for example, cannot be primitives) and have a fixed in-arity, which in
   interpreted code must be checked at call time.  Primitives, like procedures,
   always return exactly one result, which may be #<nothing> . */

/* A primitive C function takes as its only argument an initial pointer to a C
   array of already evaluated actual arguments, and returns the result which is
   always exactly one.  The primitive function checks the actual argument types
   (when needed), but not their number. */
typedef jitterlisp_object (*jitterlisp_primitive_function)
(const jitterlisp_object *evaluated_actuals);

/* A primitive descriptor, used for both primitive procedures and primitive
   macros.  Primitives descriptors are all global constants and don't live on
   the garbage-collected heap, and don't need to be GC roots as they don't
   point to other Lisp objects.  Still it's important that each structure
   begins at a double word boundary, so that we may tag them.  The C
   specification requires a structure to be aligned to the minimum common
   multiple of the alignment of its members, which is why we add the alignas
   specifier to a field -- it doesn't really matter which one. */
struct jitterlisp_primitive
{
  /* Make the first field, and therefore the whole struct, double-word aligned:
     this ensures that enough low-order zero bits in the address can be
     overwritten by my tag.
     This header is included from files using Gnulib macros, which appear to
     redefine alignas in a way different from just _Alignas ; in order to avoid
     these problems directly use _Alignas. */
  _Alignas (sizeof (jitter_int) * 2)

  /* The primitive Lisp name as a C string. */
  char *name;

  /* How many arguments the primitive takes. */
  jitter_uint in_arity;

  /* Non-false iff the descriptor is for a primitive procedure rather than a
     primitive macro. */
  bool procedure;

  /* A C function implementing the primitive procedure or primitive macro. */
  jitterlisp_primitive_function function;
};

/* Primitive tag checking, encoding and decoding. */
#define JITTERLISP_IS_PRIMITIVE(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),               \
                 JITTERLISP_PRIMITIVE_TAG,                  \
                 JITTERLISP_PRIMITIVE_TAG_BIT_NO)
#define JITTERLISP_PRIMITIVE_ENCODE(_jitterlisp_untagged_primitive)  \
  JITTER_WITH_TAG_ADDED(_jitterlisp_untagged_primitive,              \
                        JITTERLISP_PRIMITIVE_TAG,                    \
                        JITTERLISP_PRIMITIVE_TAG_BIT_NO)
#define JITTERLISP_PRIMITIVE_DECODE(_jitterlisp_tagged_primitive)  \
  ((struct jitterlisp_primitive *)                                 \
   (JITTER_WITH_TAG_SUBTRACTED((_jitterlisp_tagged_primitive),     \
                               JITTERLISP_PRIMITIVE_TAG,           \
                               JITTERLISP_PRIMITIVE_TAG_BIT_NO)))




/* S-expression representation: vectors.
 * ************************************************************************** */

/* Vectors are represented boxed as a header pointing to the actual vector
   elements as a separate heap buffer, not directly accessible by the user. */

#define JITTERLISP_VECTOR_TAG_BIT_NO    3 // disabled: see below.
#define JITTERLISP_VECTOR_TAG           0b100 // disabled: see below.

// FIXME: vectors are currently disabled, until I implement object headers.

/* A vector header. */
struct jitterlisp_vector
{
  /* How many elements there are. */
  jitter_uint element_no;

  /* A pointer to the first element. */
  jitterlisp_object *elements;
};

// FIXME: vectors are currently disabled, until I implement object headers.

/* Vector tag checking, encoding and decoding. */
/* #define JITTERLISP_IS_VECTOR(_jitterlisp_tagged_object)  \ */
/*   JITTER_HAS_TAG((_jitterlisp_tagged_object),        \ */
/*                      JITTERLISP_VECTOR_TAG,             \ */
/*                      JITTERLISP_VECTOR_TAG_BIT_NO) */
#define JITTERLISP_IS_VECTOR(_jitterlisp_tagged_object)  \
  false
#define JITTERLISP_VECTOR_ENCODE(_jitterlisp_untagged_vector)  \
  JITTER_WITH_TAG_ADDED(_jitterlisp_untagged_vector,           \
                        JITTERLISP_VECTOR_TAG,                 \
                        JITTERLISP_VECTOR_TAG_BIT_NO)
#define JITTERLISP_VECTOR_DECODE(_jitterlisp_tagged_vector)     \
  ((struct jitterlisp_vector *)                                 \
   (JITTER_WITH_TAG_SUBTRACTED((_jitterlisp_tagged_vector),     \
                               JITTERLISP_VECTOR_TAG,           \
                               JITTERLISP_VECTOR_TAG_BIT_NO)))




/* [FIXME: tentative] Non-primitive macros.
 * ************************************************************************** */

/* Non-primitive (low-level) macros are implemented exactly like interpreted
   closures using struct jitterlisp_interpreted_closure , with a different
   tag. */

#define JITTERLISP_NON_PRIMITIVE_MACRO_TAG_BIT_NO    3
#define JITTERLISP_NON_PRIMITIVE_MACRO_TAG           0b101


/* Non-primitive-macro tag checking, encoding and decoding. */
#define JITTERLISP_IS_NON_PRIMITIVE_MACRO(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),                         \
                 JITTERLISP_NON_PRIMITIVE_MACRO_TAG,                  \
                 JITTERLISP_NON_PRIMITIVE_MACRO_TAG_BIT_NO)
#define JITTERLISP_NON_PRIMITIVE_MACRO_ENCODE(_jitterlisp_untagged_non_primitive_macro)  \
  JITTER_WITH_TAG_ADDED(_jitterlisp_untagged_non_primitive_macro,                        \
                        JITTERLISP_NON_PRIMITIVE_MACRO_TAG,                              \
                        JITTERLISP_NON_PRIMITIVE_MACRO_TAG_BIT_NO)
#define JITTERLISP_NON_PRIMITIVE_MACRO_DECODE(_jitterlisp_tagged_non_primitive_macro)  \
  ((struct jitterlisp_interpreted_closure *)                                                       \
   (JITTER_WITH_TAG_SUBTRACTED((_jitterlisp_tagged_non_primitive_macro),               \
                               JITTERLISP_NON_PRIMITIVE_MACRO_TAG,                     \
                               JITTERLISP_NON_PRIMITIVE_MACRO_TAG_BIT_NO)))



/* [FIXME: tentative] Primitive macros.
 * ************************************************************************** */

/* Primitive macros are implemented exactly like primitives using struct
   jitterlisp_primitive , with a different tag. */

#define JITTERLISP_PRIMITIVE_MACRO_TAG_BIT_NO    3
#define JITTERLISP_PRIMITIVE_MACRO_TAG           0b110

/* Primitive-macro tag checking, encoding and decoding. */
#define JITTERLISP_IS_PRIMITIVE_MACRO(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG((_jitterlisp_tagged_object),                     \
                 JITTERLISP_PRIMITIVE_MACRO_TAG,                  \
                 JITTERLISP_PRIMITIVE_MACRO_TAG_BIT_NO)
#define JITTERLISP_PRIMITIVE_MACRO_ENCODE(_jitterlisp_untagged_primitive_macro)  \
  JITTER_WITH_TAG_ADDED(_jitterlisp_untagged_primitive_macro,                    \
                        JITTERLISP_PRIMITIVE_MACRO_TAG,                          \
                        JITTERLISP_PRIMITIVE_MACRO_TAG_BIT_NO)
#define JITTERLISP_PRIMITIVE_MACRO_DECODE(_jitterlisp_tagged_primitive_macro)  \
  ((struct jitterlisp_primitive *)                                             \
   (JITTER_WITH_TAG_SUBTRACTED((_jitterlisp_tagged_primitive_macro),           \
                               JITTERLISP_PRIMITIVE_MACRO_TAG,                 \
                               JITTERLISP_PRIMITIVE_MACRO_TAG_BIT_NO)))




/* [FIXME: tentative] Macros.
 * ************************************************************************** */

/* A macro is either a primitive macro or a non-primitive macro. */
#define JITTERLISP_IS_MACRO(_jitterlisp_tagged_object)               \
  (JITTERLISP_IS_PRIMITIVE_MACRO(_jitterlisp_tagged_object)          \
   || JITTERLISP_IS_NON_PRIMITIVE_MACRO(_jitterlisp_tagged_object))




/* S-expression representation: ASTs.
 * ************************************************************************** */

// FIXME: support "extended" types sharing tags at the cost of having a header.

#define JITTERLISP_AST_TAG_BIT_NO    3
#define JITTERLISP_AST_TAG           0b111

/* Just a declaration for the AST data structure.  Its definition is in
   jitterlisp-ast.h . */
struct jitterlisp_ast;

/* AST tag checking, encoding and decoding. */
#define JITTERLISP_IS_AST(_jitterlisp_tagged_object)  \
  JITTER_HAS_TAG(_jitterlisp_tagged_object,           \
                 JITTERLISP_AST_TAG,                  \
                 JITTERLISP_AST_TAG_BIT_NO)
#define JITTERLISP_AST_ENCODE(_jitterlisp_untagged_AST)  \
  JITTER_WITH_TAG_ADDED(_jitterlisp_untagged_AST,        \
                        JITTERLISP_AST_TAG,              \
                        JITTERLISP_AST_TAG_BIT_NO)
#define JITTERLISP_AST_DECODE(_jitterlisp_tagged_AST)        \
  ((struct jitterlisp_ast *)                                 \
   (JITTER_WITH_TAG_SUBTRACTED((_jitterlisp_tagged_AST),     \
                               JITTERLISP_AST_TAG,           \
                               JITTERLISP_AST_TAG_BIT_NO)))




/* Printed representation recursivity.
 * ************************************************************************** */

/* Expand to an r-value evaluating to a C boolean, non-false iff the argument
   evaluates to a Lisp object of a type which may contain other Lisp objects in
   its printed representation.  The argument may be evaluated multiple times.

   Rationale: this is used to avoid circularity checks when printing, for types
   which cannot be recursive. */
#define JITTERLISP_IS_RECURSIVE(_jitterlisp_object)  \
  (JITTERLISP_IS_CONS(_jitterlisp_object)            \
   || JITTERLISP_IS_CLOSURE(_jitterlisp_object)      \
   || JITTERLISP_IS_MACRO(_jitterlisp_object)        \
   || JITTERLISP_IS_VECTOR(_jitterlisp_object)       \
   || JITTERLISP_IS_AST(_jitterlisp_object))




/* Alignment requirement.
 * ************************************************************************** */

/* In order to represent tags we need this number of low-order bits to be
   zero in initial heap pointers. */
#define JITTERLISP_INITIAL_POINTER_ZERO_BIT_NO  3




/* Globally named objects.
 * ************************************************************************** */

/* A few s-expressions, particularly some interned symbols, are important enough
   for performance reasons to be bound to global C variables.  It would be nice
   to make them constant, but this is not possible since most of them require
   heap-allocation.

   This requires them to be GC roots, which will need some work if I switch to a
   moving GC. */
extern jitterlisp_object jitterlisp_else;
extern jitterlisp_object jitterlisp_label;
extern jitterlisp_object jitterlisp_low_level_macro_args;
extern jitterlisp_object jitterlisp_primitive_make_constantb;




/* Not for the user: s-expression initialization and finalization.
 * ************************************************************************** */

/* These functions are called by jitterlisp_initialize and jitterlisp_finalize
   as needed.  They are not for the user to call directly. */

/* Initialize the s-expression subsystem.  This must be called before using any
   other function declared here. */
void
jitterlisp_sexpression_initialize (void);

/* Finalize the s-expression subsystem and free resources.  After this is called
   no other function declared here may be used again, until the subsystem is
   re-initialized with jitterlisp_sexpression_initialize. */
void
jitterlisp_sexpression_finalize (void);

#endif // #ifndef JITTERLISP_SEXPRESSION_H_
