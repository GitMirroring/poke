/* Jitter: data tagging header.

   Copyright (C) 2017, 2019 Luca Saiu
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


#ifndef JITTER_TAGGING_H_
#define JITTER_TAGGING_H_

/* We rely on our CPP general-purpose macros, in particular for token
   concatenation. */
#include <jitter/jitter-cpp.h>

/* We need Jitter integer types, and a few bitwise and range operations. */
#include <jitter/jitter.h>
#include <jitter/jitter-bitwise.h>
#include <jitter/jitter-arithmetic.h>




/* Dynamically-typed objects.
 * ************************************************************************** */

/* A "tagged object" or "dynamically-typed object" is a datum containing both a
   value and its type in the dynamic type system, encoded in an efficient way.
   The C view of an tagged object is the jitter_tagged_object type. */

/* A dynamically typed object is encoded, in C, as an unsigned word-sized
   integer.
   Rationale: this way bitwise operations, useful to work on the tag, don't
   require type casts in C. */
typedef jitter_uint
jitter_tagged_object;




/* Bitwise operations.
 * ************************************************************************** */

/* The following operations will be used below, to act on the type part of
   tagged objects. */

/* Expand to an r-value evaluating to non-zero iff the most significant bit
   of the given word is non-zero. */
#define JITTER_MOST_SIGNIFICANT_BIT(_jitter_word)      \
  (((jitter_uint) (_jitter_word))                      \
   & ((jitter_uint) 1) << (JITTER_BITS_PER_WORD - 1))

/* Expand to an r-value evaluating to a bitmask matching the given number of
   least significant bits in a word. */
#define JITTER_BIT_MASK(_jitter_bit_no)          \
  ((((jitter_uint) 1) << (_jitter_bit_no)) - 1)

/* Expand to an r-value evaluating to a bit mask with the given number of
   consecutive 1 bits, the first one starting at the most significant position
   in a word. */
#define JITTER_HIGH_BIT_MASK(_jitter_bit_no)      \
  (JITTER_BIT_MASK(_jitter_bit_no)                \
   << (JITTER_BITS_PER_WORD - (_jitter_bit_no)))

/* Expand to an r-value evaluating to the given word with the given number of
   its least significant bits set to zero.  No side effects.  Rationale: this
   has the advantage of working independently from the original {p,s,}tag but
   might not be the most efficient solution when the original configuration is
   known; in particular these operations is more difficult for GCC to optimize
   away by combining them with arithmetic, including address arithmetic. */
#define JITTER_WITH_BITS_MASKED_OFF(_jitter_word,    \
                                    _jitter_bit_no)  \
  (((jitter_uint) (_jitter_word))                    \
   & ~ JITTER_BIT_MASK(_jitter_bit_no))

/* Expand to an r-value evaluating to the given word with the given number of
   its least significant bits set to the given bits.  No side effects.  This
   does not check that the given new bits fit in the given number of bits.  This
   works by first masking off bits and then applying a bitwise or on the result.
   Rationale: see JITTER_WITH_BITS_MASKED_OFF . */
#define JITTER_WITH_BITS_MASKED_ON(_jitter_word,              \
                                   _jitter_new_bits,          \
                                   _jitter_bit_no)            \
  (JITTER_WITH_BITS_MASKED_OFF(_jitter_word, _jitter_bit_no)  \
   | ((jitter_uint) (_jitter_new_bits)))

/* Expand to an r-value evaluating to the given word arithmetically
   right-shifted by _jitter_bit_no . */
#define JITTER_WITH_BITS_ASHIFTED_OFF(_jitter_word,                 \
                                      _jitter_bit_no)               \
  JITTER_ARITHMETIC_SHIFT_RIGHT (jitter_uint, jitter_int,           \
                                 (_jitter_word), (_jitter_bit_no))

/* Expand to an r-value evaluating to the given word modified by logically
   right-shifting the value by the given number of bits.  No side effects. */
#define JITTER_WITH_BITS_LSHIFTED_OFF(_jitter_word,     \
                                      _jitter_bit_no)   \
  (((jitter_uint) (_jitter_word)) >> (_jitter_bit_no))

/* Expand to an r-value evaluating to the given word modified by left-shifting
   by the given number of bits and or-ing the given new bits to the result.  No
   side effects.  No check is made to ensure that the new bits actually fit in
   the least significant bit area which was filled with zeroes by shifting. */
#define JITTER_WITH_BITS_SHIFTED_ON(_jitter_word,            \
                                    _jitter_new_bits,        \
                                    _jitter_new_bit_no)      \
  ((((jitter_uint) (_jitter_word)) << (_jitter_new_bit_no))  \
   | (_jitter_new_bits))

/* Expand to an r-value evaluating to the given word with the given new bits
   added.
   Rationale: by adding or removing known tag bits this way it is easy for GCC
   to compile a memory accesse from a tagged base with a known offset into a
   single base-plus-constant-offset load or store: the tag bits get combined
   with the memory offset into a compile-time constant.
   The same trick works for sums or subtractions among unshifted tagged fixnums,
   where some intermediate tagging and untagging operations may be avoided by
   merging them with others. */
#define JITTER_WITH_BITS_ADDED(_jitter_word,  \
                               _jitter_bits)  \
  (((jitter_uint) (_jitter_word))             \
   + ((jitter_uint) (_jitter_bits)))

/* Expand to an r-value evaluating to the given word with the given new bits
   subtracted.
   Rationale: see the comment for JITTER_WITH_BITS_ADDED . */
#define JITTER_WITH_BITS_SUBTRACTED(_jitter_word,  \
                                    _jitter_bits)  \
  (((jitter_uint) (_jitter_word))                  \
   - ((jitter_uint) (_jitter_bits)))

/* Expand to an r-value evaluating to the given number of the least significant
   bits in the given word. */
#define JITTER_GET_BITS(_jitter_word,    \
                        _jitter_bit_no)  \
  (((jitter_uint) (_jitter_word))        \
   & JITTER_BIT_MASK(_jitter_bit_no))

/* Expand to an r-value evaluating to non-false iff the given word has the
   given number of its least significant bits equal to the given value. */
#define JITTER_HAS_BITS(_jitter_word,    \
                        _jitter_bits,    \
                        _jitter_bit_no)  \
  (JITTER_GET_BITS(_jitter_word,         \
                   _jitter_bit_no)       \
   == ((jitter_uint) (_jitter_bits)))




/* Tag checking, tagging and untagging.
 * ************************************************************************** */

/* An C object of the appropriate type can be "encoded" into a Jitter object by
   representing it, or some pointer to it or to equivalent information in
   memory, combined with a "tag".  "Decoding" is the opposite process converting
   a Jitter object to a C object, or a pointer to it.

   A tag in this system is in the least significant bits of an object (be it a
   pointer in the case of a boxed object, or the object itself in case of an
   unboxed object); tags may be variable-length as long as it is possible to
   distinguish each different case by looking at a word.  Each case will have
   both a tag configuration and a tag length (in bits).

   Example:
     In a hypothetical system with just three types, integers cons and symbols,
     integers might have the 1-bit tag 0b0, conses the 2-bit tag 0b01 and
     symbols the 2-bit tag 0b11: the integer tag being shorter introduces no
     ambiguity, and makes one more bit available to the object payload.  By
     checking the last three bits on a word we can find its type, with
     conditionals or a table lookup.

   Encoding and decoding are non-destructive operations: they expand to
   expressions evaluating to values, and do not modify the result of their
   operand evaluation.  Memory allocation is a separate operation from encoding
   and decoding; memory operations are defined in jitter-allocator.h , not
   here. */

/* Style/mnemonic convention: these macros have arguments always following
   this order:
   - object;
   - tag;
   - tag-bit-no.
   The general-purpose macros for tagging and untagging objects, meant to be
   used for defining type-specific tagging and untagging, take all of the
   arguments above, in the order above, even if some unneeded arguments may
   never be evaluated.  This makes the code easier to modify. */

/* Expand to a r-value evaluating to a boolean, non-false iff the given word has
   the given tag. */
#define JITTER_HAS_TAG(_jitter_tagged_object,  \
                       _jitter_tag,            \
                       _jitter_tag_bit_no)     \
  (((_jitter_tagged_object)                    \
    & JITTER_BIT_MASK(_jitter_tag_bit_no))     \
   == (_jitter_tag))

/* Expand to an r-value evaluating to the dynamically-typed representation of
   the given object, which must have a type castable to jitter_uint, on which
   the given tag is attached by left-shiting and or-ing. */
#define JITTER_WITH_TAG_SHIFTED_ON(_jitter_untagged_object,  \
                                   _jitter_tag,              \
                                   _jitter_tag_bit_no)       \
  JITTER_WITH_BITS_SHIFTED_ON(                               \
     _jitter_untagged_object,                                \
     _jitter_tag,                                            \
     _jitter_tag_bit_no)

/* Expand to an r-value evaluating to the given object representation modified
   by arithmetically shifting the value right, eliminating tag bits.  No side
   effects. */
#define JITTER_WITH_TAG_ASHIFTED_OFF(_jitter_tagged_object,  \
                                     _jitter_tag,            \
                                     _jitter_tag_bit_no)     \
  JITTER_WITH_BITS_ASHIFTED_OFF(                             \
     _jitter_tagged_object,                                  \
     _jitter_tag_bit_no)

/* Expand to an r-value evaluating to the given object representation modified
   by logically shifting the value right, eliminating tag bits.  No side
   effects. */
#define JITTER_WITH_TAG_LSHIFTED_OFF(_jitter_tagged_object,  \
                                     _jitter_tag,            \
                                     _jitter_tag_bit_no)     \
  JITTER_WITH_BITS_LSHIFTED_OFF(                             \
     _jitter_tagged_object,                                  \
     _jitter_tag_bit_no)

/* Expand to an r-value evaluating to the dynamically-typed representation of
   the given object, which must have a type castable to jitter_uint, on which
   the given tag is attached by masking-shiting and or-ing -- which is to say,
   by overwriting the rightmost _jitter_tag_bit_no bits of the untagged
   representation but without losing any bit of the left.

   Rationale: see the comment before JITTER_WITH_TAG_MASKED_OFF. */
#define JITTER_WITH_TAG_MASKED_ON(_jitter_untagged_object,  \
                                  _jitter_tag,              \
                                  _jitter_tag_bit_no)       \
  JITTER_WITH_BITS_MASKED_ON(                               \
     _jitter_untagged_object,                               \
     _jitter_tag,                                           \
     _jitter_tag_bit_no)

/* Expand to an r-value evaluating to the given object representation modified
   by seeting all the tag bits to zero.  The object payload may or may not need
   to be shifted according to its type, but that operation is not performed by
   the expansion of this macro.  No side effects.
   Rationale: see JITTER_WITH_BITS_MASKED_OFF . */
#define JITTER_WITH_TAG_MASKED_OFF(_jitter_tagged_object,  \
                                   _jitter_tag,            \
                                   _jitter_tag_bit_no)     \
  JITTER_WITH_BITS_MASKED_OFF(                             \
     _jitter_tagged_object,                                \
     _jitter_tag_bit_no)

/* Expand to an r-value evaluating to the dynamically-typed representation of
   the given object, which must have a type castable to jitter_uint, on which
   the given tag is attached by simply adding it -- this assumes that the
   rightmost JITTER_TAG_BIT_NO bits of the untagged representation are zero, and
   does *not* check that it's true. */
#define JITTER_WITH_TAG_ADDED(_jitter_untagged_object,  \
                              _jitter_tag,              \
                              _jitter_tag_bit_no)       \
  JITTER_WITH_BITS_ADDED(_jitter_untagged_object,       \
                         _jitter_tag)

/* Expand to an r-value evaluating to the given object representation modified
   by seeting all the tag bits to zero via a subtraction; this assumes that the
   encoded object has the provided tag, which is *not* checked.  The object
   payload may or may not need to be shifted, according to its type; this macro
   expansion does not do that.  No side effects.

   Rationale: when loading or storing thru tagged pointers, particularly if the
   offset is a compile-time constant, the tag to be subtracted can be combined
   with the offset; this saves one bitwise and instruction. */
#define JITTER_WITH_TAG_SUBTRACTED(_jitter_tagged_object,  \
                                   _jitter_tag,            \
                                   _jitter_tag_bit_no)     \
  JITTER_WITH_BITS_SUBTRACTED(_jitter_tagged_object,       \
                              _jitter_tag)




/* Boxed objects.
 * ************************************************************************** */

/* Assuming the given word evaluates to a boxed tagged object with the given
   tag, expand to an r-value evaluating to an initial pointer to the object
   in-memory representation.  If the type is stage-tagged (see below) the
   pointer will point to the beginning of the header-tagged struct, which is to
   say to the tag header.
   The expression has a char * type and will need to be cast to the appropriate
   type; this macro is meant as a building block for other macros accessing the
   tag header or payload for specific types.  */
#define JITTER_UNBOX_TO_CHAR_STAR(_jitter_word,               \
                                  _jitter_tag,                \
                                  _jitter_tag_bit_no)         \
  ((char *)                                                   \
   (JITTER_WITH_BITS_SUBTRACTED(_jitter_word, _jitter_tag)))

/* Assuming the given word is a boxed object with the given tag, expand to an
   r-value evaluating to an initial pointer to the heap structure, including the
   header tag if any, cast to a pointer to the struct with the given name. */
#define JITTER_UNBOX_TO_STRUCT_STAR(_jitter_word,         \
                                    _jitter_tag,          \
                                    _jitter_tag_bit_no,   \
                                    _jitter_struct_name)  \
  ((struct _jitter_struct_name *)                         \
   (JITTER_UNBOX_TO_CHAR_STAR(_jitter_word,               \
                              _jitter_tag,                \
                              _jitter_tag_bit_no)))




/* The stage tag and header tags.
 * ************************************************************************** */

/* An object tag contains some type information about the object, but not
   necessarily all of it.  If the number of types in the system is too high to
   be encoded using just tag then one tag configuration and length is reserved
   as the "stage tag".  An object whose tag is the stage tag is always boxed,
   and encodes a pointer to a struct whose first field is a word-sized "header
   tag", followed by type-dependent data.  Not all unboxed objects are
   header-tagged: for efficiency reasons some boxed types which are very
   frequently used, such as conses in Lisp, may have a tag which is sufficient
   to identify the object type alone, and in that case the memory representation
   of the object will have no header tag.

   Example:
     A hypothetical system might have fixnums, symbols, the empty list, conses,
     vectors and strings.  Lists are commonly used and their efficiency is
     paramount, so we don't want conses to have a header tag and we want the
     empty list to be unique and unboxed.
     A reasonable tag assignment for this system would be:
     - 2-bit: 0b00 for fixnums, unboxed;
     - 2-bit: 0b01 for the empty list, unboxed;
     - 2-bit: 0b10 for conses, boxed, no header tag;
     - 2-bit: 0b11 as the stage tag.
     Symbols, vectors and strings will be stage-tagged, and their header tag
     will contain three different values in order to distinguish each type from
     the other two.  Notice that symbols can still be compared by identity
     without touching memory: the tagged object points to a struct containing
     the symbol attributes, not to another pointer; if symbols are interned then
     tagged objects for equal symbols will be equal.

   Any object of a header-tagged type is heap-allocated as a struct, of which
   the first field is a header tag named jitter_header_tag , and the second is
   named jitter_payload.  The actual external struct type depends on the
   internal payload type, also a struct, and there may be padding space between
   the header tag and the payload; however we assume that there is no padding
   before the header tag, so that the same header-tag extraction code works for
   any header-tagged type. */


/* Expand to an identifier, conventionally naming the heap-allocated
   non-tagged-header struct for the named type, using the given C disambiguation
   prefix. */
#define JITTER_NON_TAGGED_HEADER_STRUCT_NAME(_jitter_prefix,     \
                                             _jitter_type_name)  \
  JITTER_CONCATENATE_TWO(_jitter_prefix,                         \
                         _jitter_name)

/* Expand to an identifier, conventionally naming the heap-allocated
   tagged-header struct for the named type, using the given C disambiguation
   prefix.  The payload field of this struct, defined by
   JITTER_DEFINE_TAGGED_HEADER_STRUCT_ , will be of a struct type named
   by JITTER_NON_TAGGED_HEADER_STRUCT_NAME . */
#define JITTER_TAGGED_HEADER_STRUCT_NAME(_jitter_prefix,     \
                                         _jitter_type_name)  \
  JITTER_CONCATENATE_THREE(_jitter_prefix,                   \
                           _tagged_,                         \
                           _jitter_type_name)

/* Expand to a struct definition for a tagged-header struct named
     JITTER_TAGGED_HEADER_STRUCT_NAME(_jitter_prefix, _jitter_type_name)
   , containing two fields: a tagged header named jitter_header_tag, and a
   member of an untagged struct named
     JITTER_NON_TAGGED_HEADER_STRUCT_NAME(_jitter_prefix, _jitter_type_name)
   , the member being named jitter_payload . */
#define JITTER_DEFINE_TAGGED_HEADER_STRUCT_(_jitter_prefix, _jitter_type_name)  \
  /* Define a struct containing a header tag and a payload. */                  \
  struct JITTER_TAGGED_HEADER_STRUCT_NAME(_jitter_prefix, _jitter_type_name)    \
  {                                                                             \
    /* The header tag.  The header tag is represented as an untagged */         \
    /* unsigned word-sized integer. */                                          \
    jitter_uint jitter_header_tag;                                              \
                                                                                \
    /* The actual payload, another struct whose definition is not */            \
    /* machine-generated. */                                                    \
    struct JITTER_NON_TAGGED_HEADER_STRUCT_NAME(_jitter_prefix,                 \
                                                _jitter_type_name)              \
    jitter_payload;                                                             \
  };

/* Assumung that _jitter_word evaluates to a stage-tagged object with
   the given stage-tag value and size to a header-tagged object, expand to an
   l-value evaluating to the object header tag. */
#define JITTER_HEADER_TAG(_jitter_word,                        \
                          _jitter_stage_tag,                   \
                          _jitter_stage_tag_bit_no)            \
  (* ((jitter_uint *)                                          \
      (JITTER_UNBOX_TO_CHAR_STAR(_jitter_word,                 \
                                 _jitter_stage_tag,            \
                                 _jitter_stage_tag_bit_no))))

/* Assumung that _jitter_word evaluates to a stage-tagged object with
   the given stage-tag value and size to a header-tagged object, expand to an
   r-value evaluating to the object header tag. */
#define JITTER_GET_HEADER_TAG(_jitter_word,              \
                              _jitter_stage_tag,         \
                              _jitter_stage_tag_bit_no)  \
  JITTER_HEADER_TAG(_jitter_word,                        \
                    _jitter_stage_tag,                   \
                    _jitter_stage_tag_bit_no)

/* Assumung that _jitter_word evaluates to a stage-tagged object with the given
   stage-tag value and size to a header-tagged object, expand to a statement
   setting the object header tag to the given value.  This is useful for
   initializing a just-allocated object. */
#define JITTER_SET_HEADER_TAG(_jitter_word,              \
                              _jitter_stage_tag,         \
                              _jitter_stage_tag_bit_no,  \
                              _jitter_header_tag)        \
  do                                                     \
    {                                                    \
      JITTER_HEADER_TAG(_jitter_word,                    \
                        _jitter_stage_tag,               \
                        _jitter_stage_tag_bit_no)        \
        = (_jitter_header_tag);                          \
    }                                                    \
  while (false)

/* Assuming that the given word evaluates to a stage-tagged object expand
   to an r-value evaluating to a boolean, non-false iff the header tag has
   the given value.
   This dereferences the pointer, and is therefore unsafe if the object is not
   boxed, and may return an incorrect result if the object is boxed but not
   stage-tagged.  JITTER_HAS_HEADER_TAG , below, is a safe version of this which
   also checks the object tag. */
#define JITTER_STAGE_TAGGED_HAS_HEADER_TAG(_jitter_word,              \
                                           _jitter_stage_tag,         \
                                           _jitter_stage_tag_bit_no,  \
                                           _jitter_header_tag)        \
  (JITTER_GET_HEADER_TAG(_jitter_word,                                \
                         _jitter_stage_tag,                           \
                         _jitter_stage_tag_bit_no)                    \
   == (_jitter_header_tag))

/* Expand to an r-value evaluating to a boolean, non-false iff the given word is
   a boxed stage-tagged object with the given stage tag and header tag.
   Notice that several arguments may be evaluated more than once. */
#define JITTER_HAS_HEADER_TAG(_jitter_word,                         \
                              _jitter_stage_tag,                    \
                              _jitter_stage_tag_bit_no,             \
                              _jitter_header_tag)                   \
  (JITTER_HAS_TAG(_jitter_word,                                     \
                  _jitter_stage_tag,                                \
                  _jitter_stage_tag_bit_no)                         \
   && JITTER_STAGE_TAGGED_HAS_HEADER_TAG(_jitter_word,              \
                                         _jitter_stage_tag,         \
                                         _jitter_stage_tag_bit_no,  \
                                         _jitter_header_tag))

/* Expand to an r-value evaluating to an initial pointer to the appropriate
   non-header-tagged structure for the given type, assuming the object is
   actually a non-header-tagged boxed object of the given type. */
#define JITTER_NON_HEADER_TAGGED_PAYLOAD(_jitter_word,                        \
                                         _jitter_tag,                         \
                                         _jitter_tag_bit_no,                  \
                                         _jitter_prefix,                      \
                                         _jitter_type_name)                   \
  JITTER_UNBOX_TO_STRUCT_STAR(                                                \
    _jitter_word,                                                             \
    _jitter_tag,                                                              \
    _jitter_tag_bit_no,                                                       \
    JITTER_NON_TAGGED_HEADER_STRUCT_NAME(_jitter_prefix, _jitter_type_name))

/* Expand to an r-value evaluating to an initial pointer to the appropriate
   payload structure within the header-tagged structure (therefore skipping
   the header tag and any padding after it) for the given type, assuming
   the object is actually a header-tagged object of the given type. */
#define JITTER_HEADER_TAGGED_PAYLOAD(_jitter_word,                             \
                                     _jitter_stage_tag,                        \
                                     _jitter_stage_tag_bit_no,                 \
                                     _jitter_prefix,                           \
                                     _jitter_type_name)                        \
  (& (JITTER_UNBOX_TO_STRUCT_STAR(                                             \
         _jitter_word,                                                         \
         _jitter_stage_tag,                                                    \
         _jitter_stage_tag_bit_no,                                             \
         JITTER_TAGGED_HEADER_STRUCT_NAME(_jitter_prefix, _jitter_type_name))  \
            ->jitter_payload))

/* FIXME: possibly:
     define an initialization macro working on a given type, deciding itself
     whether to use header-tagging or not.  The macro should take the type name
     as a parameter and use the macro below:

     There should probably be at least one macro for each type specifying if the
     type is:
     - unboxed
     - header-tagged boxed
     - non-header-tagged boxed
*/

/* FIXME: definitely:
     define for each type:
     - a macro expanding to the full struct name for a given boxed type,
       be it header-tagged or not;
     - a macro either initializing the header tag of one such structure if
       a header tag exists, or doing nothing otherwise.
 */




/* Scratch.
 * ************************************************************************** */

#define JITTER_TAGGING_SHAPE_UNBOXED_SHIFTED          0
#define JITTER_TAGGING_SHAPE_UNBOXED_UNSHIFTED        1
#define JITTER_TAGGING_SHAPE_BOXED_HEADER_TAGGED      2
#define JITTER_TAGGING_SHAPE_BOXED_NON_HEADER_TAGGED  3

#define JITTER_



#endif // #ifndef JITTER_TAGGING_H_
