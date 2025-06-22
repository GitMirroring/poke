/* Jitter garbage collector header.

   Copyright (C) 2020, 2022, 2024 Luca Saiu
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


#ifndef JITTER_GC_H_
#define JITTER_GC_H_

#include <jitter/jitter.h>
#include <jitter/jitter-bitwise.h>

#include <stdbool.h>

#include <jitter/jitter-aligned-block.h>
#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-word-set.h>
#include <jitter/jitter-list.h>
#include <jitter/jitter-time.h>




/* Configuration parameters.
 * ************************************************************************** */

/* WARNING: Do not *ever* define or customise any of these macros out of this
   file, in .c or .h files including this header.
   The result would be very confusing runtime behaviour, with the mutator and
   the collector having incompatible definitions of structs, sizes and
   offsets. */

/* If JITTER_GC_STRESS is defined the default definition of tuning configuration
   parameters below is overridden with values that stress both the garbage
   collector and the mutator, intended to find bugs.  The performance impact
   is severe and this should not be used in production. */
// #define JITTER_GC_STRESS

// #define JITTER_GC_ENABLE_SHARING  1  // this should obviously be machine-generated or moved to jitter-config.h

/* How many steps to have in the young generation, not counting the nursery.
   This can be any natural number including zero. */
#define JITTER_GC_NON_NURSERY_STEP_NO    0

/* If JITTER_GC_DEBUG is defined the code is compiled with sanity checks
   intended to catch many bugs in the collector itself, in shape functions
   supplied by the user, and in user data encoding.
   These checks do not make the garbage collector unusable but are still
   expensive, and therefore must be explicitly enabled for debugging.
   Testing with debugging enabled is highly recommended for new systems and
   after changes involving the heap. */
// #define JITTER_GC_DEBUG 1

/* If JITTER_GC_EXPENSIVE_STATISTICS is defined then also keep statistics which
   may be expensive to update, negatively affecting the total run time. */
#define JITTER_GC_EXPENSIVE_STATISTICS

/* If defined write a full log of every action performed at garbage collection
   time, which is useful for debugging.  The log has a clean format but is too
   expensive and intrusive for being compiled in by default. */
#define JITTER_GC_LOG 1

/* The stream where log messages are printed.  This is only used if
   JITTER_GC_LOG is defined. */
// #define JITTER_GC_LOG_STREAM stdout
#define JITTER_GC_LOG_STREAM stderr




/* Tuning parameters.  FIXME: move ///////////////////////////////////////////////////////////////
 * ************************************************************************** */

/* These unit-prefix macros are useful to be used as constant factors when
   defining other macros expressing memory sizes. */
#define JITTER_GC_KIBI  ((jitter_long_long) 1024)
#define JITTER_GC_MEBI  ((jitter_long_long) (JITTER_GC_KIBI * 1024.))
#define JITTER_GC_GIBI  ((jitter_long_long) (JITTER_GC_MEBI * 1024.))
#define JITTER_GC_TEBI  ((jitter_long_long) (JITTER_GC_GIBI * 1024.))
#define JITTER_GC_PEBI  ((jitter_long_long) (JITTER_GC_TEBI * 1024.))
#define JITTER_GC_EXBI  ((jitter_long_long) (JITTER_GC_PEBI * 1024.))
/* Bigger prefixes are too large to work with in 64-bit integers. */

//#define JITTER_GC_INITIAL_NURSERY_SIZE_IN_BYTES           (.125 * 1024 * 1024.)//(2 * 1024 * 1024.)//(1 * 1024 * 1024.) //(1 * 1024 * 1024.) // JITTER_GC_DEFAULT_MAXIMUM_NURSERY_SIZE_IN_BYTES

/* Check that the word size is reasonable. */
#if JITTER_WORD_BIT_NO == 16
# error "16-bit archtiectures are not supported"
#elif (JITTER_WORD_BIT_NO == 32 || JITTER_WORD_BIT_NO == 64)
  /* Good, nothing to do. */
#else
# error "JITTER_WORD_BIT_NO is not 16, 32 or 64"
#endif
/* From now on we can assume (JITTER_WORD_BIT_NO == 32 \
                              || JITTER_WORD_BIT_NO == 64) */

/* In practice only whole blocks can be assigned to a space, therefore all these
   sizes will be automatically rounded up to multiples of the block payload
   size. */
#define JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES   JITTER_GC_BLOCK_SIZE_IN_BYTES

#if JITTER_WORD_BIT_NO == 32
# define JITTER_GC_DEFAULT_MAXIMUM_NURSERY_SIZE_IN_BYTES (512 * JITTER_GC_KIBI)
#elif JITTER_WORD_BIT_NO == 64
# define JITTER_GC_DEFAULT_MAXIMUM_NURSERY_SIZE_IN_BYTES (128  * JITTER_GC_MEBI)//(64  * JITTER_GC_MEBI)
#else
# error "invalid JITTER_WORD_BIT_NO"
#endif

#define JITTER_GC_INITIAL_NURSERY_SIZE_IN_BYTES           JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES  //((JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES + JITTER_GC_DEFAULT_MAXIMUM_NURSERY_SIZE_IN_BYTES) / 2)

/* When young generation [or nursery space? FIXME: check] survival drops below
   JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE the space size is decreased by
   multiplying it by JITTER_GC_DEFAULT_NURSERY_SHRINKAGE_RATIO; when the same
   survival rises above JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE the space
   size is increased by multiplying it by JITTER_GC_DEFAULT_NURSERY_GROWTH_RATIO
   . */
#define JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE       0.01//.001
#define JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE      .2//.15
#define JITTER_GC_DEFAULT_NURSERY_GROWTH_RATIO            4
#define JITTER_GC_DEFAULT_NURSERY_SHRINKAGE_RATIO         .75

// #define JITTER_GC_DEFAULT_MINIMUM_OLDSPACE_SIZE_IN_BYTES  0
#if JITTER_WORD_BIT_NO == 32
# define JITTER_GC_DEFAULT_MINIMUM_OLDSPACE_SIZE_IN_BYTES  (1. * JITTER_GC_MEBI)
#elif JITTER_WORD_BIT_NO == 64
# define JITTER_GC_DEFAULT_MINIMUM_OLDSPACE_SIZE_IN_BYTES  (64. * JITTER_GC_MEBI)
#else
# error "invalid JITTER_WORD_BIT_NO"
#endif

#if JITTER_WORD_BIT_NO == 32
# define JITTER_GC_DEFAULT_MAXIMUM_OLDSPACE_SIZE_IN_BYTES (512 * JITTER_GC_KIBI) //(1 * JITTER_GC_MEBI)//(64 * JITTER_GC_MEBI)
#elif JITTER_WORD_BIT_NO == 64
# define JITTER_GC_DEFAULT_MAXIMUM_OLDSPACE_SIZE_IN_BYTES (16 * JITTER_GC_MEBI)//(256 * JITTER_GC_MEBI)//(1 * JITTER_GC_GIBI)
#else
# error "invalid JITTER_WORD_BIT_NO"
#endif

#define JITTER_GC_DEFAULT_TARGET_MAJOR_SURVIVAL_RATE      .1

/* The block size should not be smaller than 128kiB: see the comment in
   jitter-aligned-block.h .  This limit is necessary for modern machines such as
   amd64 as of the 2020s; smaller configurations with smaller pages may have
   smaller limits, but Jitter is not practical anyway for machines having less
   than at least a few megabytes worth of RAM. */
#if JITTER_WORD_BIT_NO == 32
# define JITTER_GC_BLOCK_SIZE_IN_BYTES                    (128 * JITTER_GC_KIBI)
#elif JITTER_WORD_BIT_NO == 64
# define JITTER_GC_BLOCK_SIZE_IN_BYTES                    (1 * JITTER_GC_MEBI)
#else
# error "invalid JITTER_WORD_BIT_NO"
#endif

/////////
#undef JITTER_GC_BLOCK_SIZE_IN_BYTES
#define JITTER_GC_BLOCK_SIZE_IN_BYTES (128 * JITTER_GC_KIBI)
//#define JITTER_GC_BLOCK_SIZE_IN_BYTES (128 * JITTER_GC_MEBI)
//#define JITTER_GC_BLOCK_SIZE_IN_BYTES (4 * JITTER_GC_GIBI)
//#define JITTER_GC_BLOCK_SIZE_IN_BYTES (16 * JITTER_GC_GIBI)
//#define JITTER_GC_BLOCK_SIZE_IN_BYTES (1 * JITTER_GC_MEBI)
//#define JITTER_GC_BLOCK_SIZE_IN_BYTES (8 * JITTER_GC_KIBI)
//#define JITTER_GC_BLOCK_SIZE_IN_BYTES (512 * JITTER_GC_KIBI)
//#define JITTER_GC_BLOCK_SIZE_IN_BYTES (8 * JITTER_GC_KIBI)//(128 * JITTER_GC_KIBI) //(8 * JITTER_GC_KIBI)

/* How many survival ratio measurements to keep for computing a survival
   estimate based on recent history.
   A value of 1 means that the each survival rate is estimated to be the same as
   the previous one; with 2, the latest two measurements are considered to
   compute the next estimate.  The estimate is always biased by giving more
   weight to more recent measurements.
   This must be a non-zero natural. */
#define JITTER_GC_SURVIVAL_RATIO_NO  4

/* When computing the survival ratio by a weighted average, as explained in the
   comment above, give this much weight to the most recent measurement.  The
   remaining weight will be used for the total of every remaining elements.
   This biased computation goes on: out of the total of the remaining weight, a
   ratio equal to JITTER_GC_RECENT_BIAS goes to the next most recent
   measurement, with the remaininder being divided in the same way among the
   rest.
   A value of 0.5 means that the most recent element has the same weight of
   every other element combined.  A value of 2./3 mens that the most recent
   element will have a weight of 2/3, twice the weight of all the other elements
   combined.
   This value must be in [0.5, 1).  It is easy to see that the sum of all the
   weights is always exactly 1.
   Higher values make the heap resize more quickly in response to usage changes;
   lower values make the change slower and smoother. */
#define JITTER_GC_RECENT_BIAS       .5//.75//.5//(3./4)

/* How many words the smallest possible heap object can take. */
#define JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS    2


/* Stress mode.
 * ************************************************************************** */

/* Defining macro JITTER_GC_STRESS overrides several other settings with
   alternative definitions.

   The alternative definitions settings are useful to stress-test the garbage
   collector itself or a mutator.  They are certainly not recommended for
   performance. */
#if defined (JITTER_GC_STRESS)
# undef  JITTER_GC_DEBUG
# define JITTER_GC_DEBUG  1
# undef  JITTER_GC_EXPENSIVE_STATISTICS
# define JITTER_GC_EXPENSIVE_STATISTICS 1
# undef  JITTER_GC_BLOCK_SIZE_IN_BYTES
# define JITTER_GC_BLOCK_SIZE_IN_BYTES  (4 * JITTER_GC_KIBI)
# undef  JITTER_GC_NON_NURSERY_STEP_NO
# define JITTER_GC_NON_NURSERY_STEP_NO  3
# undef  JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES
# define JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES  \
  (1 * JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES)
# undef  JITTER_GC_DEFAULT_MAXIMUM_NURSERY_SIZE_IN_BYTES
# define JITTER_GC_DEFAULT_MAXIMUM_NURSERY_SIZE_IN_BYTES  \
  JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES
# undef  JITTER_GC_DEFAULT_MINIMUM_OLDSPACE_SIZE_IN_BYTES
# define JITTER_GC_DEFAULT_MINIMUM_OLDSPACE_SIZE_IN_BYTES  (1 * JITTER_GC_MEBI)
# undef  JITTER_GC_DEFAULT_MAXIMUM_OLDSPACE_SIZE_IN_BYTES
# define JITTER_GC_DEFAULT_MAXIMUM_OLDSPACE_SIZE_IN_BYTES  (32 * JITTER_GC_MEBI)
# undef  JITTER_GC_DEFAULT_TARGET_MAJOR_SURVIVAL_RATE
# define JITTER_GC_DEFAULT_TARGET_MAJOR_SURVIVAL_RATE  .99
#endif // #if defined (JITTER_GC_STRESS)




/* Debugging.
 * ************************************************************************** */

#if defined (JITTER_GC_DEBUG)
# define _JITTER_GC_ASSERT(expression)                         \
    do                                                         \
      {                                                        \
        if (! (expression))                                    \
          jitter_fatal ("assertion violated: " # expression);  \
      }                                                        \
    while (false)
#else  /* ! defined (JITTER_GC_DEBUG) */
# define _JITTER_GC_ASSERT(expression)  \
    do { /* nothing */ } while (false)
#endif /* #if defined (JITTER_GC_DEBUG) */




/* Object size, alignment and padding.
 * ************************************************************************** */

/* How many bytes the smallest possible heap object can take.  Every heap object
   must have a size which is an exact multiple of this, counting any header as
   well. */
#define JITTER_GC_MINIMUM_OBJECT_SIZE_IN_BYTES  \
  (sizeof (jitter_gc_tagged_object)             \
   * JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS)

/* Expand to an expression evaluating to the given number rounded up so as to be
   a multiple of the minimum object size.
   The argument must evaluate to a strictly positive integer.
   This may evaluate the argument
   multiple times, but if the argument is a constant expression then the
   expansion is also a constant expression.  If size is not constant its
   handling should compile to two instructions: sum-of-constant,
   bitwise-and-of-constant. */
#define JITTER_GC_ROUND_SIZE_UP(size)                                           \
  JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO ((size),                                 \
                                        JITTER_GC_MINIMUM_OBJECT_SIZE_IN_BYTES)

/* Headered objects begin with a struct whose first element is a word-sized type
   code.  The following assumes that the offset of the type code field within
   the header is zero.
   This assumption is in fact portable at least since C99, probably earlier:
      C99 §6.7.2.1.13: "Within a structure object, the non-bit-field members and
      the units in which bit-fields reside have addresses that increase in the
      order in which they are declared. A pointer to a structure object,
      suitably converted, points to its initial member (or if that member is a
      bit-field, then to the unit in which it resides), and vice versa. There
      may be unnamed padding within a structure object, but not at its
      beginning."
   In a C11 draft I can find the same identical text at §6.7.2.1.15. */

/* Given the type of an object header (which must include the type code as its
   first field, word-sized) expand to a constant expression which is the offset
   in bytes from the beginning of the header to the beginning of the first field
   after the header.  This frees the user from the need to reason about what
   padding is required for each configuration. */
#define JITTER_GC_POST_HEADER_OFFSET(header_struct_name)        \
  /* The next tagged object after the header must begin at a    \
     valid tagged-object address boundary. */                   \
  JITTER_GC_ROUND_SIZE_UP (sizeof (struct header_struct_name))

/* Given the type of a shape header (which must be a struct including the type
   code as its first field, word-sized) and an expression evaluating to a
   pointer to such a header, expand to an expression evaluating to a pointer to
   the first tagged object past the header, skipping any padding space.  The
   expansion has type jitter_gc_tagged_object * . */
#define JITTER_GC_HEADERED_ELEMENTS(header_struct_name, header_pointer)  \
  ((jitter_gc_tagged_object *)                                           \
   ((char*) (header_pointer)                                             \
    + JITTER_GC_POST_HEADER_OFFSET (header_struct_name)))

/* Given an expression evaluating to a non-negative number of tagged objects,
   expand to an expression evaluating to their total size in bytes, rounded up
   in order to be allocatable as a single heap object.  This is intended for
   computing the size of an headerless object to be allocated (or of an already
   existing headerless object), given the number of its elements.
   This expands to a constant expression if the argument is a constant
   expression.
   Users interested in allocating headerless objects should also use the macro
   JITTER_GC_PAD_HEADERLESS_OBJECT , defined below. */
#define JITTER_GC_HEADERLESS_SIZE_IN_BYTES(tagged_element_no)   \
  JITTER_GC_ROUND_SIZE_UP ((tagged_element_no)                  \
                           * sizeof (jitter_gc_tagged_object))

/* This is similar to JITTER_GC_HEADERLESS_SIZE_IN_BYTES but applies to headered
   objects.  Expand to an expression evaluating to the object allocated size,
   also counting the space taken by a header of the given type at the beginning
   plus the required padding after the header, in addition to the given number
   of tagged elements and the required padding after them.  The header type must
   be a struct with a first field, word-sized, containing the type code.  Notice
   that tagged_element_no is allowed to be zero, which is useful for headered
   objects whose only content is their header. */
#define JITTER_GC_HEADERED_SIZE_IN_BYTES(header_struct_name,             \
                                         tagged_element_no)              \
  /* There is no need to explicitly round up the result of the sum: the  \
     sum has two addends which are both already multiples of the common  \
     factor JITTER_GC_MINIMUM_OBJECT_SIZE_IN_BYTES . */                  \
  (JITTER_GC_POST_HEADER_OFFSET (header_struct_name)                     \
   + JITTER_GC_HEADERLESS_SIZE_IN_BYTES (tagged_element_no))

/* A "header-only" shape has a header, but no tagged elements after it.  Since
   header-only shapes are common in practice this macro is useful as a
   one-argument alternative to JITTER_GC_HEADERED_SIZE_IN_BYTES . */
#define JITTER_GC_HEADER_ONLY_SIZE_IN_BYTES(header_struct_name)  \
  JITTER_GC_HEADERED_SIZE_IN_BYTES(header_struct_name, 0)

/* Expand to the integer logarithm of the argument, which is assumed to be be a
   power of two. */
#define JITTER_GC_LG(n)        \
  ( ((n) == (1LU << 0))  ?  0  \
  : ((n) == (1LU << 1))  ?  1  \
  : ((n) == (1LU << 2))  ?  2  \
  : ((n) == (1LU << 3))  ?  3  \
  : ((n) == (1LU << 4))  ?  4  \
  : ((n) == (1LU << 5))  ?  5  \
  : ((n) == (1LU << 6))  ?  6  \
  : ((n) == (1LU << 7))  ?  7  \
  : ((n) == (1LU << 8))  ?  8  \
  : ((n) == (1LU << 9))  ?  9  \
  : ((n) == (1LU << 10)) ? 10  \
  : ((n) == (1LU << 11)) ? 11  \
  : ((n) == (1LU << 12)) ? 12  \
  : ((n) == (1LU << 13)) ? 13  \
  : ((n) == (1LU << 14)) ? 14  \
  : ((n) == (1LU << 15)) ? 15  \
  : ((n) == (1LU << 16)) ? 16  \
  : ((n) == (1LU << 17)) ? 17  \
  : ((n) == (1LU << 18)) ? 18  \
  : ((n) == (1LU << 19)) ? 19  \
  : ((n) == (1LU << 20)) ? 20  \
  : ((n) == (1LU << 21)) ? 21  \
  : ((n) == (1LU << 22)) ? 22  \
  : ((n) == (1LU << 23)) ? 23  \
  : ((n) == (1LU << 24)) ? 24  \
  : ((n) == (1LU << 25)) ? 25  \
  : ((n) == (1LU << 26)) ? 26  \
  : ((n) == (1LU << 27)) ? 27  \
  : ((n) == (1LU << 28)) ? 28  \
  : ((n) == (1LU << 29)) ? 29  \
  : ((n) == (1LU << 30)) ? 30  \
  : ((n) == (1LU << 31)) ? 31  \
  : /* wrong argument. */ 999)

/* Given a tagged pointer to a boxed object it is always possible, by masking
   the low bits off with a bitwise and operation, to obtain a non-tagged
   pointer, independently from the specific tag being used for each shape.
   (Notice that non-tagged objects are allowed not to resepect this rule).
   This macro defines how many low bits bits we require to mask off to always
   convert from tagged pointer to non-tagged pointer.  The expansion is a
   constant expression. */
#define JITTER_GC_BOXED_TAG_BIT_NO   \
  /* A number of bits equal to
       lg (sizeof (void *))
     would already be guaranteed if allocated objects were just word aligned;
     but since objects are in fact alinged to
     JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS words we get an additional
       lg (JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS)
     bits.  All of those are usable for tags in boxed objects. */ \
  (JITTER_LG_BYTES_PER_WORD \
   + JITTER_GC_LG(JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS))

/* This macro expands to the bit mask selecting (with an & operation) the tag
   bits in a boxed object pointer.  The expansion is a constant expression. */
#define JITTER_GC_BOXED_TAG_BIT_MASK                                   \
  ((((jitter_gc_tagged_object) 1) << JITTER_GC_BOXED_TAG_BIT_NO) - 1)

/* This macro expands to the bit mask selecting (with an & operation) the
   non-tag bits in a boxed object pointer; in other words the mask converting
   a tagged pointer to a non-tagged pointer. */
#define JITTER_GC_BOXED_NON_TAG_BIT_MASK                      \
  (~ (jitter_gc_tagged_object) JITTER_GC_BOXED_TAG_BIT_MASK)




/* Fundamental type definitions.
 * ************************************************************************** */

/* A pointer within some space or to the byte just past it.  This type is used
   for generic pointers which are often summed to a size in bytes, and for which
   casts would prove cumbersome. */   // FIXME: do not use in other cases /////////////////////
typedef char *
jitter_gc_heap_pointer;

/* The type of a generic tagged object, boxed or unboxed.  It is convenient to
   represent tagged objects as unsigned integers rather than as pointers, since
   the first operations performed on them is usually some bit manipulation;
   the result can then be cast.  Defining jitter_gc_tagged_object as a pointer
   would require one more useless cast from pointer to unsigned before the
   bit manipulation. */
typedef jitter_uint
jitter_gc_tagged_object;




/* Heap object shapes and tagging.
 * ************************************************************************** */

// FIXME: comment.
enum jitter_gc_collection_kind
  {
    /* Not actually a collection kind. */
    jitter_gc_collection_kind_ssb_flush,

    jitter_gc_collection_kind_minor,
    jitter_gc_collection_kind_major,
    jitter_gc_collection_kind_global,

    /* Move roots and every object reachable from the roots to own shared space.
       This may leave broken hearts in the young and/or old space, to be fixed
       in another minor or major collection. */
    jitter_gc_collection_kind_share // FIXME: use.
  };

/* Forward declarations. */
struct jitter_gc_heap;
struct jitter_gc_heaplet;

/* The function types defined below match the type of functions to be supplied
   by the user, which describe the shapes of every object used in the
   garbage-collected heap. */

/* Given a tagged object return non-false iff the object has this shape.  This
   function is free to examine only the tag or to look at the header as well: it
   is never called on objects which have been overwritten with broken hearts. */
typedef bool (* jitter_gc_object_has_shape_f) (jitter_gc_tagged_object);

/* Given an untagged pointer to an object return its tagged version.  This only
   makes sense for boxed shapes. */
typedef jitter_gc_tagged_object (* jitter_gc_object_encode_f) (void *);

/* Given a tagged object of this shape return its size in bytes. */
typedef size_t (* jitter_gc_object_size_in_bytes_f) (jitter_gc_tagged_object);

/* Given a word in the heap, return non-false iff the word is a type code
   for an object of this shape.  A type code field, when used, occupies the
   first position in an object header.  It does not need to be the same value
   for every object of the same shape (Rationale examples: the colour in a
   Red-Black tree node or the balance in an AVL tree node might be encoded in
   the type code for a single shape).
   For headerless shapes the pointer to this function will be NULL. */
typedef bool (* jitter_gc_is_type_code_f) (jitter_gc_tagged_object);

/* Given an initial untagged pointer of an object of this shape (in a fromspace)
   and a target untagged pointer, copy the object without updating tagged fields
   (a function of type jitter_gc_object_update_fields_f will take care of that
   later); set the word pointed by copied to the tagged address of the new copy.
   Return the size of the new copy in bytes, which is allowed to be less
   than or equal to the original size.  This function is allowed to change the
   shape of the copied object.
   Rationale: changing the shape of certain objects is an important optimisation
   in lazy functional language where already evaluated closures are replaced
   with their result.  Unfortunately cdr-coding appears difficult to implement
   with this design, without breaking the abstraction between the garbage
   collector and user code in a complex way. */
typedef size_t (* jitter_gc_object_copy_f) (struct jitter_gc_heaplet *a,
                                            jitter_gc_tagged_object *tagged_to_p,
                                            void *from, void *to)  // FIXME: do I want void * ? ///////////////////////////////
  __attribute__ ((nonnull (1), nonnull (2), nonnull (3)));

/* Given the address of an object of this shape (as already copied in tospace)
   and a heaplet, update its tagged fields by calling jitter_gc_handle_word
   on each of their addresses, passing the pointed heaplet; also update any
   other internal field whose value may depend on the new address.
   Return the size of the object in bytes.
   For shapes with no headers the pointer to this function will be NULL: if
   an object can not be identified with a header then every word of it will be
   scanned as an individual tagged object.  */
typedef size_t (* jitter_gc_object_update_fields_f)
   (struct jitter_gc_heaplet *a,
    void *untagged_initial_pointer)
  __attribute__ ((nonnull (1), nonnull (2)));

/* A function finalising a finalisable object.  This same type is used for both
   quick and complete-object finalisers.
   The function receives an untagged initial pointer to the object, from which
   the user will be able to extract any required information.
   - In case of complete-object finalisation the object is guaranteed to be
     “complete” at finalisation time, in the sense that every object it refers
     through its fields will be available and alive when this function runs.
   - In the case of quick finalisation there is no such guarantee: with quick
     finalisation only the “C” fields (such as file descriptors, socket,
     pointers to malloc-allocated memory and the like) will be guaranteed to be
     correct, but the object tagged fields may be invalid or overwritten by
     broken hearts.
   Notice that the function might receive a NULL heaplet pointer, in case the
   object being finalised belongs to the shared space of a heap for which no
   heaplet exists any longer.  On the other hand the heap pointer is guaranteed
   to be non-NULL.
   This function must not allocate GC-heap memory, since it runs at finalisation
   time.  It is possible to mutate alive objects (without allocating from the GC
   heap), or to set non-GC-heap fields.  In the case of complete finalisation
   one might want, for example, to link finalised (resurrected) objects into a
   list or queue, for the mutator to handle asynchronously.
   There is no guarantee of the order in which objects are finalised.  In
   particular it is not necessarily the case that, if an object A points to an
   object B, B is finalised before A. */
typedef void (*jitter_gc_object_finalize_f) (struct jitter_gc_heap *h,
                                             struct jitter_gc_heaplet *a,
                                             void *untagged_pointer);

/* The kind of object finalisation required by a shape. */
enum jitter_gc_shape_finalization_kind
  {
    /* Objects of this shape are not finalisable.  This is the most efficient
       solution in both space and time and should always be preferred when
       applicable. */
    jitter_gc_shape_finalization_kind_none,

    /* Objects of this shape require finalisation, but finalisation does not
       access the object tagged-object fields: in other words it is possible to
       finalise an object without first resurrecting every object pointed by
       it.
       Many finalisers only need to call a C function freeing some resource
       such as a file descriptor or malloc-allocated memory.  For such cases
       quick finalisation is sufficient. */
    jitter_gc_shape_finalization_kind_quick,

    /* Objects of this shape require finalisation, and the finaliser may
       access every field of the object including its tagged pointers: before
       the finaliser is called the object, along with all of its pointed object,
       will in general need to be resurrected.
       This is the least efficient but most general case. */
    jitter_gc_shape_finalization_kind_complete_object
  };

/* A descriptor for one of the possible tagged shapes of an heap object. */
struct jitter_gc_shape
{
  /* A user-readable name for the shape.  This pointer refers internally
     managed memory. */
  char *name;

  /* The kind of object finalisation required by this shape. */
  enum jitter_gc_shape_finalization_kind finalization_kind;

  /* The following fields are function pointers, operating as described by the
     comments associated to the function types above, with respect to this
     shape. */
  jitter_gc_object_has_shape_f object_has_shape;
  jitter_gc_object_encode_f object_encode;  /* non-NULL, as the shape is boxed. */
  jitter_gc_object_size_in_bytes_f object_size_in_bytes;
  jitter_gc_is_type_code_f is_type_code; /* This is allowed to be NULL. */
  jitter_gc_object_copy_f object_copy;
  jitter_gc_object_update_fields_f object_update_fields;
  jitter_gc_object_finalize_f finalize; /* No explicit distinction between quick
                                           and complete-object here. */
};

/* A dynamic array of shape descriptors of type struct jitter_gc_shape.  This
   is scanned in order of insertion when collecting. */
typedef struct jitter_dynamic_buffer
jitter_gc_shapes;

/* A table describing every supported shape. */
struct jitter_gc_shape_table
{
  /* A unique object repeatedly copied to fill dead space, if JITTER_GC_DEBUG is
     defined.  Only useful for debugging.  In case no such object exists any
     valid unboxed object can be used. */
  jitter_gc_tagged_object invalid_object;

  /* A unique object repeatedly copied to fill allocated space in heap objects,
     before they are initialised, if JITTER_GC_DEBUG is defined.  Only useful
     for debugging.  In case no such object exists any valid unboxed object can
     be used. */
  jitter_gc_tagged_object uninitialized_object;

  /* The broken-heart type code, which must be distinguishable from any other
     type code and from unboxed objects. */
  jitter_gc_tagged_object broken_heart_type_code;

  /* Return non-false iff the argument is unboxed.  Unboxed objects are copied
     as they are, without modification. */
  jitter_gc_object_has_shape_f is_unboxed;

  /* A descriptor for every boxed shape.  This is a dynamic array of struct
     jitter_gc_shape elements, consulted at collection time in the same order
     they were inserted. */
  jitter_gc_shapes shapes;

  /* A descriptor for every boxed shape with a header.  This is a dynamic array
     of struct jitter_gc_shape elements, consulted at collection time in the
     same order they were inserted.  The elements are a subset of the elements
     in shapes. */
  jitter_gc_shapes headerful_shapes;

  /* Like headerful_shapes, but hold the shapes with a finaliser, be it quick
     or complete-object. */
  jitter_gc_shapes finalizable_shapes;

  /* Like headerful_shapes, but hold the shapes with a quick finaliser. */
  jitter_gc_shapes quickly_finalizable_shapes;

  /* Like headerful_shapes, but hold the shapes with a complete-object
     finaliser. */
  jitter_gc_shapes complete_object_finalizable_shapes;

  /* Non-false iff the table contains at least one complete-object finalisable
     shape. */
  int /* bool */ has_complete_object_finalizable;
};

// FIXME: comment ////////////////////////////////////////////////////////
void
jitter_gc_shape_table_initialize (struct jitter_gc_shape_table *shape_table_p,
                                  jitter_gc_tagged_object invalid_object,
                                  jitter_gc_tagged_object uninitialized_object,
                                  jitter_gc_tagged_object broken_heart_type_code,
                                  jitter_gc_object_has_shape_f is_unboxed)
  __attribute__ ((nonnull (1),
                  nonnull (5)));

// FIXME: comment ////////////////////////////////////////////////////////
void
jitter_gc_shape_table_finalize (struct jitter_gc_shape_table *shape_table_p)
  __attribute__ ((nonnull (1)));

// FIXME: comment ////////////////////////////////////////////////////////
struct jitter_gc_shape_table *
jitter_gc_shape_table_make (jitter_gc_tagged_object invalid_object,
                            jitter_gc_tagged_object uninitialized_object,
                            jitter_gc_tagged_object broken_heart_type_code,
                            jitter_gc_object_has_shape_f is_unboxed)
  __attribute__ ((nonnull (4),
                  returns_nonnull));

// FIXME: comment ////////////////////////////////////////////////////////
void
jitter_gc_shape_table_destroy (struct jitter_gc_shape_table *shape_table_p)
  __attribute__ ((nonnull (1)));

// FIXME: comment ////////////////////////////////////////////////////////
void
jitter_gc_shape_add_headerless
   (struct jitter_gc_shape_table *shape_table_p,
    const char *name, // memory copied by the function
    jitter_gc_object_has_shape_f object_has_shape,
    jitter_gc_object_encode_f object_encode,
    jitter_gc_object_size_in_bytes_f object_size_in_bytes,
    jitter_gc_object_copy_f object_copy)
  __attribute__ ((nonnull (1), nonnull (2), nonnull (3), nonnull (4),
                  nonnull (5)));

void
jitter_gc_shape_add_headered_non_finalizable
   (struct jitter_gc_shape_table *shape_table_p,
    const char *name, // memory copied by the function
    jitter_gc_object_has_shape_f object_has_shape,
    jitter_gc_object_encode_f object_encode,
    jitter_gc_object_size_in_bytes_f object_size_in_bytes,
    jitter_gc_is_type_code_f is_type_code,
    jitter_gc_object_copy_f object_copy,
    jitter_gc_object_update_fields_f object_update_fields)
  __attribute__ ((nonnull (1), nonnull (2), nonnull (3), nonnull (4),
                  nonnull (5), nonnull (6), nonnull (7)));

void
jitter_gc_shape_add_headered_quickly_finalizable
   (struct jitter_gc_shape_table *shape_table_p,
    const char *name, // memory copied by the function
    jitter_gc_object_has_shape_f object_has_shape,
    jitter_gc_object_encode_f object_encode,
    jitter_gc_object_size_in_bytes_f object_size_in_bytes,
    jitter_gc_is_type_code_f is_type_code,
    jitter_gc_object_copy_f object_copy,
    jitter_gc_object_update_fields_f object_update_fields,
    jitter_gc_object_finalize_f finaliser)
  __attribute__ ((nonnull (1), nonnull (2), nonnull (3), nonnull (4),
                  nonnull (5), nonnull (6), nonnull (7), nonnull (8)));

void
jitter_gc_shape_add_headered_complete_object_finalizable
   (struct jitter_gc_shape_table *shape_table_p,
    const char *name, // memory copied by the function
    jitter_gc_object_has_shape_f object_has_shape,
    jitter_gc_object_encode_f object_encode,
    jitter_gc_object_size_in_bytes_f object_size_in_bytes,
    jitter_gc_is_type_code_f is_type_code,
    jitter_gc_object_copy_f object_copy,
    jitter_gc_object_update_fields_f object_update_fields,
    jitter_gc_object_finalize_f finaliser)
  __attribute__ ((nonnull (1), nonnull (2), nonnull (3), nonnull (4),
                  nonnull (5), nonnull (6), nonnull (7), nonnull (8)));




/* Data structures.  FIXME: organise sections differently. ///////////////////////////////
 * ************************************************************************** */

/* This enumeration type also serves to index generation_spaces , an array of
   pointers to struct jitter_space structures within struct jitter_heaplet ,
   thanks to jitter_gc_generation_index_offset.

   The offset is useful because the one performance-critical case is
   jitter_gc_generation_shared , which is compared against in
   JITTER_GC_WRITE_SHARE_BARRIER: comparing against zero is faster then against
   other values in many architectures.

   The relative order of these cases is important: smaller values in number
   means younger objects in the generation sense.  The “oldest” possible
   objects are unboxed, which are never destroyed at all and objects from any
   other generation can point to without restrictions. */
enum jitter_gc_generation
  {
    /* Young-generation objects live in the nursery and in the ageing spaces.
       They can point to objects of any generation in the same heaplet, and to
       shared objects, without restrictions.  */
    jitter_gc_generation_young = -2,

    /* Old-generation objects live in the old space.  They can point to other
       old objects in the same heaplet and to shared objects, without
       restrictions.
       Old objects pointing to young objects must belong to the remembered set,
       since they are used as roots in minor collections.  The SSB (Sequential
       Store Buffer) is a fast auxiliary data structure used to defer precise
       updates to the remembered set. */
    jitter_gc_generation_old = -1,

    /* Shared-generation objects can only point to other shared objects,
       including shared objects from other heaplets of the same heap.
       In the case of the shared generation the distinction between heaplets
       in the same heap only serves to enable concurrent allocation.  However
       the restriction of the shared generation being closed under the points-to
       relation is heavy: it means that, whenever a shared object is updated to
       point to a non-shared object, the non-shared object must first be made
       shared. */
    jitter_gc_generation_shared = 0,

    /* Space which can old objects but is currently not used.  Since it is
       expensive to ask the operating system for new blocks we usually keep
       some ready. */
    jitter_gc_generation_unused = 1,

    /* At this time the only immportal objects are unboxed.  Since they do not
       live in the heap they are never collected, they do not move, and they
       can be stored anywhere.
       This is a “trivial” generation in the sense that there is no space with
       blocks containing objects of this generation. */
    jitter_gc_generation_immortal = 2,

    /* These aliases are useful for loops on generations: */
    jitter_gc_generation_first = jitter_gc_generation_young,
    jitter_gc_generation_last = jitter_gc_generation_unused,
    jitter_gc_generation_past_last = jitter_gc_generation_last + 1
  };

/* When using this enumerate as an index one should add this value, so that
   the first element (at jitter_gc_generation_young) ends up first. */
#define JITTER_GC_GENERATION_INDEX_OFFSET (- (int) (jitter_gc_generation_young))

/* How many non-trivial generations (by which I mean, generation actually
   having spaces with allocated objects) there are. */
#define JITTER_GC_NON_TRIVIAL_GENERATION_NO  \
  ((int) jitter_gc_generation_past_last - (int) jitter_gc_generation_first)



/* Expand to an expression evaluating to a Boolean, non-false iff the first
   argument evaluates to a generation strictly younger than the generation
   the second argument evaluates to. */
#define JITTER_GC_GENERATION_YOUNGER(a, b)  \
  ((a) < (b))

/* Like JITTER_GC_GENERATION_YOUNGER, but evaluate to non-false if the first
   argument evaluates to a generation strictly older (instead of younger) than
   the generation the second argument evaluates to. */
#define JITTER_GC_GENERATION_OLDER(a, b)  \
  JITTER_GC_GENERATION_YOUNGER((b), (a))

/* Forward-declaration. */
struct jitter_gc_space;

/* A block is a contiguous buffer containing a header followed by a payload
   containing heap objects, allocated by pounter bumping; each space contains
   and essentially consists of a linked list of blocks.  Any initialised block
   belongs to exactly one space, at all times.  It is legitimate for a space not
   currently in use to contain no blocks, at mutation or collection time.
   Each blocks contains a copy of or a ponter to its own runtime strucutres;
   exactly one block at a time will be active for allocation at mutation time;
   during collection one block per involved space will be used for allocation,
   and another for scanning.
   Each block header is allocated at the very beginning of its aligned block,
   inside the buffer itself.  The payload is not pointed: it follows the header
   and possibly some padding, at an offset of
   JITTER_GC_BLOCK_HEADER_ROUNDED_SIZE_IN_BYTES bytes from the beginning.  The
   fact that blocks are aligned in memory makes easy and efficient to obtain a
   pointer to the header of the block containing any given boxed object, even
   tagged, with a simple mask operation. */
struct jitter_gc_block
{
  /* The generation this space belongs to.  This information would also be
     accessible from the space field, but is repeated here in order to speed
     up the lookup process at collection time and avoid one indirection. */
  enum jitter_gc_generation generation;

  /* A pointer to the first byte past the last allocated object within the
     block.  This is only set to a sensible value when the block is put out of
     use, and only used at collection time.  For a block being used as the
     current allocation block of a space this must be NULL: see the comment
     at the beginning of the body of jitter_gc_scan_next . */
  jitter_gc_heap_pointer used_limit;

  /* The opaque data structure required for freeing an aligned block. */
  jitter_aligned_block_id block_id;

  /* A pointer to the space this block belongs to. */
  struct jitter_gc_space *space;

  /* Links to the previous and next block in the current space. */
  struct jitter_list_links links;
};

#define JITTER_GC_BLOCK_HEADER_ROUNDED_SIZE_IN_BYTES         \
  JITTER_GC_ROUND_SIZE_UP (sizeof (struct jitter_gc_block))

#define JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES       \
  (JITTER_GC_BLOCK_SIZE_IN_BYTES                    \
   - JITTER_GC_BLOCK_HEADER_ROUNDED_SIZE_IN_BYTES)

#define JITTER_GC_BLOCK_BIT_MASK                         \
  (~ ((jitter_uint) JITTER_GC_BLOCK_SIZE_IN_BYTES - 1))

/*
// FIXME: comment.///////////////////////////////////////////////////////////////////
  The argument may evaluate to either a tagged object or a pointer: the macro
  works with either, since the argument is cast to a numeric type and its low
  bits are cleared, before casting again to a block pointer. */
#define JITTER_GC_BOXED_TO_BLOCK(object)                 \
  ((struct jitter_gc_block *)                            \
   ((jitter_uint) (object) & JITTER_GC_BLOCK_BIT_MASK))

/* Like JITTER_GC_BOXED_TO_BLOCK, except that the expansion evaluates to a
   pointer to the space containing the block to which the object belongs.  The
   expansion has type struct jitter_gc_space * . */
#define JITTER_GC_BOXED_TO_SPACE(object)     \
  (JITTER_GC_BOXED_TO_BLOCK(object)->space)




/* Object finalisation.
 * ************************************************************************** */

/* This entire structure is opaque from the point of view of the user, who
   should only initialise and update it via JITTER_GC_FINALIZABLE_INITIALIZE and
   JITTER_GC_FINALIZABLE_COPY , defined below.
   This structure *must* be the second element of the finalisable object header,
   coming immediately after the type code.  Finalisable objects *must* be
   headered, therefore they always have a type code. */
struct jitter_gc_finalization_data
{
  /* This links the struct jitter_gc_finalization_data within the finalizables
     list in its space. */
  struct jitter_list_links links;

  /* Non-false if we need to run the finaliser for this object when it is proven
     to be dead.  Unless this field is changed explicitly by a finalisation
     function run under full-object finalisation each object will be finalised
     only once, even if resurrected.  With quick finalisation finalisable
     objects are always finalised once and never resurrected. */
  char /*bool*/ need_to_run_finalizer;
};

# define JITTER_GC_FINALIZABLE_SET_TO_BE_FINALIZED(finalizable_data_p, value)  \
  do                                                                           \
    {                                                                          \
      (finalizable_data_p)->need_to_run_finalizer = (value);                   \
    }                                                                          \
  while (false)

/* A more opaque name for struct jitter_gc_finalization_data , intended for the
   finalization field in user object header structs. */
typedef struct jitter_gc_finalization_data
jitter_gc_finalization_data;

/* JITTER_GC_FINALIZATION_DATA_OFFSET is the offset of the
   jitter_gc_finalization_data field inside a finalisable object header.  The
   jitter_gc_finalization_data field *must* be at this offset in the struct,
   coming as its second field right after the type code.  The type
   struct jitter_gc_example_header is used here to compute the offset; in
   practice I would expect it to be be the size of a word, 8 bytes on 64-bit
   machines and 4 bytes on 32-bit machines, with no padding
   Pedantry apart the question of the offset being predicatable in every user
   struct is non-trivial.  When the fields are correctly ordered the offset is
   indeed predictable in standard C because:
   - the first field has known padding (zero)
   - the offsets for each coming member is increasing
   - the offset of each coming member *depends on its type*
   From iso-iec-9899:2023 (working draft 2022-08), § 6.7.2.1:
     16  Each non-bit-field member of a structure or union object is aligned in
         an implementation-defined manner appropriate to its type.
     17  Within a structure object, the non-bit-field members and the units in
         which bit-fields reside have addresses that increase in the order in
         which they are declared. A pointer to a structure object, suitably
         converted, points to its initial member (or if that member is a
         bit-field, then to the unit in which it resides), and vice versa.
         There may be unnamed padding within a structure object, but not at its
         beginning.
   Therefore, even if there actually is padding, the padding is always in the
   same amount and this offset is indeed the same for every user header. */
struct jitter_gc_example_header
{
  jitter_gc_tagged_object type_code;
  jitter_gc_finalization_data finalization_data;
};
#define JITTER_GC_FINALIZATION_DATA_OFFSET                         \
  (offsetof (struct jitter_gc_example_header, finalization_data))

/* Given an expression evaluating to a pointer to a finalization field of type
   jitter_gc_finalization_data, expand to an expression evaluating to an
   untagged initial pointer to its object. */
#define _JITTER_GC_FINALIZABLE_FINALIZATION_FIELD_TO_UNTAGGED(field_p)  \
  ((jitter_gc_tagged_object *)                                          \
   ((char *) (field_p) - JITTER_GC_FINALIZATION_DATA_OFFSET))

/* A list linking a set of finalisable objects together.  Such lists are used
   for different purposes at different stages; see the comments for struct
   jitter_gc_finalization_data and for enum jitter_gc_finalizable_place .
   One desirable property of this structure, important for efficiency, is that
   a list can be destructively emptied with its content being appended or
   prepended to another in O(1) time.  This avoids an initialisation cost
   proportional to the number of objects at the beginning of collections. */
struct jitter_gc_finalizable_list
{
  /* This of struct jitter_gc_finalization_data fields; each field belongs to
     an object, with objects allowed to have different shapes from each other
     as long as they all have the struct jitter_gc_finalization_data field at
     the same offset from the beginning: see JITTER_GC_FINALIZATION_DATA_OFFSET
     above. */
  struct jitter_list_header header;
};

/* Given a finalisable object header struct name, the name of its finalisation
   data field and an untagged initial pointer to an object header, expand to an
   expression evaluating to a pointer to the finalisation data field for the
   pointed object. */
#define _JITTER_GC_FINALIZABLE_FINALIZATION_DATA_FIELD_P(  \
   object_header_struct_name,                              \
   finalization_data_field_name,                           \
   untagged_p)                                             \
  (& (((struct object_header_struct_name *) (untagged_p))  \
         ->finalization_data_field_name))

/* Initialise the finalisation data of a finalisable object.  FIXME: comment ???????????????????????????????? */
#define JITTER_GC_FINALIZABLE_INITIALIZE(object_header_struct_name,             \
                                         finalization_data_field_name,          \
                                         untagged)                              \
  do                                                                            \
    {                                                                           \
      /* Sanity check: fail if the finalization data object does not come at
         the correct offset, right after the type code.  Having the user
         violate this assumption would lead to all kinds of subtle bugs, so
         this is worth checking.  The check will be optimised away by the
         compiler in case there is nothing wrong. */                            \
      size_t _jitter_gc_offset = offsetof (struct object_header_struct_name,    \
                                           finalization_data_field_name);       \
      if (_jitter_gc_offset != JITTER_GC_FINALIZATION_DATA_OFFSET)              \
        jitter_fatal ("struct " # object_header_struct_name " does not have "   \
                      "its jitter_gc_finalization_data field in second place, " \
                      "immediately following the type code.  Please fix that"); \
/* printf("sizeof (struct " # object_header_struct_name") == %i\n", (int) sizeof (struct object_header_struct_name)); */\
      /* Find the space the object belongs to. */                               \
      char *_jitter_gc_untagged = (char *) (untagged);                          \
      /* Initialise finalisation data for the new object: */                    \
      struct jitter_gc_finalization_data *_jitter_gc_field_p                    \
        = _JITTER_GC_FINALIZABLE_FINALIZATION_DATA_FIELD_P                      \
             (object_header_struct_name, finalization_data_field_name,          \
              _jitter_gc_untagged);                                             \
      /* Link the object to the list of finalisable objects accessible at
         mutation time in its space. */                                         \
      struct jitter_gc_space *_jitter_gc_s                                      \
        = JITTER_GC_BOXED_TO_SPACE (_jitter_gc_untagged);                       \
      JITTER_LIST_LINK_LAST (jitter_gc_finalization_data, links,                \
                             & _jitter_gc_s->finalizables.header, \
                             _jitter_gc_field_p);                               \
      /* The object's finaliser has never been run and therefore will need to
         run, at least for the first time. */                                   \
      _jitter_gc_field_p->need_to_run_finalizer = true;                         \
jitter_gc_log_i (0, "initialising finalisable %s at %p\n", \
                 # object_header_struct_name, \
                 _jitter_gc_untagged);        \
    }                                                                           \
  while (false)

/* Update the finalisation data of an object that is being copied.  This must
   be called from the shape copy function.  FIXME: comment ?????????????????????????????????????? */
#define JITTER_GC_FINALIZABLE_COPY(object_header_struct_name,                  \
                                   finalization_data_field_name,               \
                                   heaplet_p,                                  \
                                   untagged_from,                              \
                                   untagged_to)                                \
  do                                                                           \
    {                                                                          \
/*      struct jitter_gc_heaplet *_jitter_gc_a = (heaplet_p);*/                    \
      /* Find the (to)space now containing the object, which has already been  \
         copied.  It is perfectly acceptable to find this from a pointer to a  \
         header field rather than an initial tagged or untagged pointer: each  \
         object header is still fully contained within its aligned block. */   \
      char *_jitter_gc_untagged_from = (untagged_from);                        \
      char *_jitter_gc_untagged_to = (untagged_to);                            \
      struct jitter_gc_finalization_data *_jitter_gc_from_field_p              \
        = _JITTER_GC_FINALIZABLE_FINALIZATION_DATA_FIELD_P                     \
             (object_header_struct_name, finalization_data_field_name,         \
              _jitter_gc_untagged_from);                                       \
      struct jitter_gc_finalization_data *_jitter_gc_to_field_p                \
        = _JITTER_GC_FINALIZABLE_FINALIZATION_DATA_FIELD_P                     \
             (object_header_struct_name, finalization_data_field_name,         \
              _jitter_gc_untagged_to);                                         \
      struct jitter_gc_block *_jitter_gc_from_b                                \
        = JITTER_GC_BOXED_TO_BLOCK (_jitter_gc_from_field_p);                  \
      struct jitter_gc_space *_jitter_gc_from_s = _jitter_gc_from_b->space;    \
      struct jitter_gc_block *_jitter_gc_to_b                                  \
        = JITTER_GC_BOXED_TO_BLOCK (_jitter_gc_to_field_p);                    \
      struct jitter_gc_space *_jitter_gc_to_s = _jitter_gc_to_b->space;        \
      _JITTER_GC_ASSERT (_jitter_gc_from_s != _jitter_gc_to_s);                \
      /* Unlink the old object (field) from the candidate_dead_finalizables    \
         list in the heaplet, since the object has been found to be alive;     \
         link the new object (field) to the finalizables list
         in the tospace.  Notice that it is important to unlink the original
         field, and not its copy: objects are compared by identity in
         JITTER_LIST_UNLINK . */                                               \
      JITTER_LIST_UNLINK (jitter_gc_finalization_data, links,                  \
                          & _jitter_gc_from_s->finalizables.header,            \
                          _jitter_gc_from_field_p);                            \
      JITTER_LIST_LINK_LAST (jitter_gc_finalization_data, links,               \
                             & _jitter_gc_to_s->finalizables.header,           \
                             _jitter_gc_to_field_p);                           \
jitter_gc_log_i (8, "copying finalisable %s\n", \
                 # object_header_struct_name);  \
      /* The other struct jitter_gc_finalization_data fields do not change:
         the kind of lists this object belongs to is the same, and whether we
         need to finalise it or not does not change either. */                 \
/*printf ("Moving finalizable object " # object_header_struct_name             \
        " from %s to %s\n", \
        jitter_gc_generation_to_string (JITTER_GC_TAGGED_BOXED_TO_GENERATION (_jitter_gc_from_field_p)), \
        jitter_gc_generation_to_string (JITTER_GC_TAGGED_BOXED_TO_GENERATION (_jitter_gc_to_field_p)) \
        );*/                                                                     \
    }                                                                          \
  while (false)




/* Spaces.
 * ************************************************************************** */

/* A space is a collection of blocks containing objects.
   Multiple spaces exist at the same time: copying garbage collection
   requires, for each mutation space ("fromspace") one secondary space
   ("tospace") to exist at least at collection time.
   Different spaces also segregate objects by generation; each generation may
   consist of multiple spaces, and each heap object belongs to exactly one
   space, therefore to exactly one generation.
   Multiple execution threads can allocate concurrently without requiring
   synchronization, by using a different fromspace each, each belonging to
   a different thread-local heaplet.

   Each space is associated to a descriptor containing information about the
   space.  The space from which the mutator allocates will *not* normally access
   and update the descriptor at mutation time: only the very few fields marked
   as "RUNTIME FIELD" below will be needed, ideally to be kept as a working copy
   in reserved registers, thread-local variables or some other efficient storage
   resources crossing the scopes of multiple C functions and user assembly code.

   The descriptor contains a copy of these variables not guaranteed to be kept
   up to date at mutation time: avoiding memory accesses is crucial for
   performance. */
struct jitter_gc_space
{
  /* Iff non-false then at least one object has been scavenged from this space
     into destination_space.  This is cleared at the beginning of each
     collection, and set while scavenging.  It is useful for sharing, in order
     to know what kind of collection, if any, is needed in order to find every
     pointer to an object that is being shared.
     Remark: this field could conceptually belong to a generation object rather
     than to a space, but keeping a generation field up-to-date would require
     one more derefence per scavenged object; this solution instead requires
     just a store to a structure to which we already hold a pointer.
     Checking this field once per space in a given generation is fast enough,
     and not remotely as critical. */
  jitter_int scavenged_from;

  /* RUNTIME FIELD: the address of the next object to be allocated within the
     current block.  This is guaranteed to point within the memory interval
       [beginning, limit)
     when the space is being used for allocation. */
  jitter_gc_heap_pointer allocation_pointer;

  /* RUNTIME FIELD: a pointer to the first byte past the allocation space.
     Memory beginning at limit either falls out of the block boundary or is
     already used for the SSB. */
  char *limit;

  /* The scan pointer is only used at collection time when a space plays the
     role of tospace, for keeping track of what objects still need to be traced.
     With Cheney's algorithm the tracing work list is a contiguous buffer used
     as a queue, starting at scan_pointer and ending at allocation_pointer.  The
     scan_pointer is *not* a runtime field, since it is of no interest to the
     user who wants to allocate objects.  In particular it does not need to be
     kept in a reserved hardware register at mutation time for good performance,
     differently from runtime fields.
     When in use this points within the current scan block or at its limit. */
  jitter_gc_heap_pointer scan_pointer;

  /* The generation this space belongs to. */
  enum jitter_gc_generation generation;

  /* A pointer to the space alive objects from this space should be copied to,
     or NULL if the object is not to be copied in the current collection.
     This is only used at collection time, and set up at the beginning of a
     collection, in a different way according to the collection kind. */
  struct jitter_gc_space *destination_space;

  /* A linked list containing the blocks for this space.  Each element has type
     struct jitter_gc_block and is linked via its field named links. */
  struct jitter_list_header blocks;

  /* A pointer to the one block in this space that is currently in use for
     allocation, or NULL if no such block exists.  When this field is not NULL
     its value must be an element of blocks. */
  struct jitter_gc_block *allocation_block;

  /* A pointer to the one block in this space that is currently in use for
     scanning.  This is only used for tospaces at collection time. */
  struct jitter_gc_block *scan_block;

  /* The total allocated size in bytes for this space, equal to the sum of the
     payload size for every block.  This is updated when blocks are added to or
     removed from the space. */
  size_t allocated_size_in_bytes;

  /* The total used size in bytes for this space.  This is only updated when the
     allocation block changes, and explicitly during collection; of course it
     would be too expensive to increment this field at every allocation. */
  size_t used_size_in_bytes;

  /* Two flags telling whether to remove even the last block when cleaning the
     space, respectively before and after a collection; if false, one block is
     left so that allocation can proceed without any emptiness check.  It is
     necessary to have two distinct flags, since the same space may be cleaned
     both at the beginning and at an end of a collection, with different
     completeness requirements.  These are only initialised when blocks are
     set up to be cleaned; the flags are not used in other cases. */
  bool clean_completely_before;
  bool clean_completely_after;

  /* A list of finalisable objects contained within this space, in any of its
     blocks. */
  struct jitter_gc_finalizable_list finalizables;

  /* A field containing a copy of the list above in a previous state, only used
     at collection time.  By convention this is kept empty when not in use. */
  struct jitter_gc_finalizable_list finalizables_copy;


  /* The space name as a string, malloc-allocated.  Only used for debugging. */
  char *name;
};




/* Statistics.
 * ************************************************************************** */

enum jitter_gc_point_
  {
    jitter_gc_point_before_any_collection,
    jitter_gc_point_after_any_collection,

    jitter_gc_point_before_minor,
    jitter_gc_point_after_minor,
    jitter_gc_point_before_major,
    jitter_gc_point_after_major,

    jitter_gc_point_before_global,
    jitter_gc_point_after_global,

    jitter_gc_point_before_finalization,
    jitter_gc_point_after_finalization,

    jitter_gc_point_before_share,
    jitter_gc_point_after_share,

    jitter_gc_point_ssb_flush,
    jitter_gc_point_block_change,
  };


struct jitter_gc_statistics_snapshot
{
//  enum jitter_gc_action action;
  bool beginning;

  jitter_uint mutator_heaplet_used_byte_no;
  jitter_uint mutator_heaplet_allocated_byte_no;

  jitter_uint fromspace_used_byte_no;
  jitter_uint fromspace_allocated_byte_no;

  jitter_uint tospace_used_byte_no;
  jitter_uint tospace_allocated_byte_no;

  jitter_uint remembered_set_size;

  jitter_uint ssb_length;

  jitter_uint root_no;

  jitter_uint finalized_object_no;

  jitter_point_in_time time;
};

struct jitter_gc_statistics_difference
{
  jitter_int promoted_byte_no;
  jitter_int surviving_non_promoted_byte_no;

  jitter_int remembered_set_size_change;

  size_t used_fromspace_byte_no;
  size_t allocated_fromspace_byte_no;
  size_t used_tospace_byte_no;
  size_t allocated_tospace_byte_no;
  jitter_point_in_time time;
};

// FIXME: just an idea, very much unfinished
struct jitter_gc_action_statistics
{
  /* State at the beginning of the operation. */
  size_t used_fromspace_byte_no_at_beginning;
  size_t used_tospace_byte_no_at_beginning;
  jitter_point_in_time start_time;

  /* State at the end of the operation. */
  size_t used_tospace_byte_no_at_end;
  size_t used_fromspace_byte_no_at_end;
  /* No need to store the end time: elapsed_time is faster to compute
     directly. */

  /* Computed data. */
  double elapsed_time;
  size_t promoted_byte_no;
  size_t surviving_not_promoted_byte_no;
  size_t killed_byte_no;
};

// FIXME: just an idea, very much unfinished
struct jitter_gc_statistics
{
  unsigned long action_no;

  size_t promoted_byte_no;
  size_t surviving_not_promoted_byte_no;
  size_t used_tospace_bytes_at_beginning;
  size_t used_tospace_bytes_at_end;
  size_t used_fromspace_bytes_at_end;
};




/* Synchronisation primitives.
 * ************************************************************************** */

/* The way synchronisation data structures are actually defined is
   system-dependent but we can make it general enough here in the header by
   referring to data structures via pointers.
   The types as specified here, based as they are on generic pointers, make
   it possible to use efficient implementations based on mutexes and condition
   variables, or semaphores, or some other platform-specific alternative. */
typedef void* jitter_gc_heap_synchronization_entity;
typedef void* jitter_gc_heaplet_synchronization_entity;;




/* Heaplets.
 * ************************************************************************** */

/* A temporary_root set is a container of root descriptors of type struct
   jitter_gc_root , which can be updated LIFO style. */
typedef struct jitter_dynamic_buffer
jitter_gc_temporary_root_set;

/* A safe upper limit to the number of spaces used in a given heaplet.  This is
   used as an array size. */
#define JITTER_HEAPLET_SPACE_NO_UPPER_BOUND                 \
  /* This upper bound is correct, but currently not tight.  \
     It is not critical to performance. */                  \
  (2 * JITTER_GC_NON_NURSERY_STEP_NO + 5)

/* Each heaplet is in one of these states. */
enum jitter_gc_heaplet_used_state
{
  /* The heaplet is in use, either mutating or collecting in a non-shared way,
     and is therefore unavailable for shared collection. */
  jitter_gc_heaplet_used_state_in_use,

  /* The heaplet is not in use and available for shared collection; after the
     shared collection is over the thread should be woken up, so that it can
     take control of the heaplet again.  */
  jitter_gc_heaplet_used_state_to_be_woken_up,

  /* The heaplet is not in use and available for shared collection; after the
     shared collection is over the thread should be ????DONE NOTHING TO???? */
  jitter_gc_heaplet_used_state_not_to_be_woken_up,

  /* The heaplet is owned by a thread performing the global collection. */
  jitter_gc_heaplet_used_state_collecting,

  /* An invalid state, used to catch forgotten initialisations. */
  jitter_gc_heaplet_used_state_invalid
};

/* A heaplet data structure contains enough information to be able to allocate
   and collect.
   In order to allocate memory a user needs a pointer to a heaplet and the
   runtime fields (see the comments in struct jitter_gc_space) for the space
   currently in use for allocation.
   Heaplets are useful to provide a thread-local abstraction for allocation,
   hiding some intricate data structures, both internal to the heaplet and
   shared. */
struct jitter_gc_heaplet
{
  /* The heap this heaplet belongs to. */
  struct jitter_gc_heap *heap;

  /* The current state of this heaplet within the heap; see the comments inside
     the enum definition for more information. */
  enum jitter_gc_heaplet_used_state used_state;

  /* A semaphore, condition variable or analogous structure for the thread using
     this heaplet to block on and to be woken up on, respectively before and
     after a shared collection. */
  jitter_gc_heaplet_synchronization_entity semaphore;

  /* This links this heaplet to other heaplets from the same heap within either
     its heap->heaplets_in_use list or its heap->heaplets_not_in_use list. */
  struct jitter_list_links links;

  /* True iff collection is enabled (see the header comment for
     jitter_gc_disable_collection).  Collection is enabled by default. */
  int /*bool*/ collection_enabled;

  /* Two fields usable as runtime fields, in case the user does not want to
     reserve registers but has the heaplet available; this is particularly
     convenient from C, rather than from VM code.
     Of course reserving registers is more efficient.
     These fields are initialised to an invalid value for defensiveness when
     the heaplet is made, and then never read or written by this code: they
     are provided for the user's convenience. */
  jitter_gc_heap_pointer convenience_runtime_allocation_pointer;
  char *convenience_runtime_limit;

  /* A pointer to the shape table for this heap.  This pointer is kept here for
     performance reasons to avoid an indirection, but is identical to the same
     field in the heap data structure. */
  const struct jitter_gc_shape_table *shape_table;

  /* A space containing allocated but unused blocks, available to be repurposed.
     For efficiency's sake the space and generation fields of blocks within this
     space are not kept up to date; doing that would require processing every
     block separately in a loop when clearing spaces, instead of unlinking and
     relinking entire lists; the space allocated size, however, is kept up to
     date -- still, without iterating over each block.  The space used size is
     always zero.  The allocation, limit and scan pointers are not used. */
  struct jitter_gc_space unused_space;

  /* The nursary space, where new objects are made at mutation time.  The
     nursery belongs to the young generation.
     There is no need for a reserve space for the nursery: see the comment
     below. */
  struct jitter_gc_space nursery;

  /* During minor collections:
     - surviving nursery objects get moved into the first ageing space (or to
       the old space when JITTER_GC_NON_NURSERY_STEP_NO is zero);
     - surviving objects from the i-th ageing space get moved to the (i+1)-th
       ageing space, if such a target space exists;
     - survivors from the last ageing space get moved to the old space.
     The fields hereare the JITTER_GC_NON_NURSERY_STEP_NO ageing spaces, along
     with a reserve space for each of them.  After each collection each reserve
     space becomes an ageing space, and vice-versa.
     All of these spaces belong to the young generation.
     JITTER_GC_NON_NURSERY_STEP_NO is allowed to be zero. */
  struct jitter_gc_space steps_a [JITTER_GC_NON_NURSERY_STEP_NO];
  struct jitter_gc_space steps_b [JITTER_GC_NON_NURSERY_STEP_NO];

  /* Pointers keeping track of the current roles of steps_a and steps_b. */
  struct jitter_gc_space* young_ageing_spaces [JITTER_GC_NON_NURSERY_STEP_NO];
  struct jitter_gc_space* young_reserves [JITTER_GC_NON_NURSERY_STEP_NO];

  /* Spaces for the old generation: one of them is the old allocation space, the
     other the old reserve space.  After each major collection the two roles are
     swapped. */
  struct jitter_gc_space oldspace_a;
  struct jitter_gc_space oldspace_b;

  /* Pointers keeping track of the roles of oldspace_a and oldspace_b . */
  struct jitter_gc_space *oldspace;
  struct jitter_gc_space *old_reserve;

#if defined (JITTER_GC_ENABLE_SHARING)
  /* The shared space is closed under the relation "points to": in other words
     no shared-space object is allowed to point to a non-shared-space object.
     This space represents the portion of the shared space currently owned by
     this heaplet, where by "owned" we simply mean that no other heaplet is
     allowed to make new objects in the blocks from this space; the owning
     heaplet is allowed to make objects in the shared space without any
     synchronisation.
     This space belongs to the shared generation.
     There is no reserve space for this space in this heaplet: shared spaces
     are only collected during shared collections, and the reserve space is part
     of the heap. */
  struct jitter_gc_space shared_space_own;
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* An array of pointers to every space in this heaplet, along with a pointer
     to the first unused element in the array. */
  struct jitter_gc_space* spaces [JITTER_HEAPLET_SPACE_NO_UPPER_BOUND];
  struct jitter_gc_space **spaces_limit;

  /* Like above, for each generation: each generation has its own space array
     and limit.  This is designed to support an enum jitter_gc_generation object
     as index. */
  struct jitter_gc_space*
  generation_spaces [JITTER_GC_NON_TRIVIAL_GENERATION_NO]
                    [JITTER_HEAPLET_SPACE_NO_UPPER_BOUND];
  struct jitter_gc_space **generation_spaces_limit
     [JITTER_GC_NON_TRIVIAL_GENERATION_NO];

  /* An array of pointers to spaces which are origin spaces (which is to say,
     are being copied from) during the current collection, along with a pointer
     to the first unused element in the array.  Only used at collection time. */
  struct jitter_gc_space* fromspaces [JITTER_HEAPLET_SPACE_NO_UPPER_BOUND];
  struct jitter_gc_space **fromspaces_limit;

  /* An array of pointers to spaces (and used limit) which are destination
     spaces, which is to say, are being copied into during the current
     collection.  Only used at collection time. */
  struct jitter_gc_space* tospaces [JITTER_HEAPLET_SPACE_NO_UPPER_BOUND];
  struct jitter_gc_space **tospaces_limit;

  /* An array of pointers to spaces (and used limit) which are to be emptied at
     the beginning of a collection.  Only used at collection time. */
  struct jitter_gc_space* spaces_to_clean_before [JITTER_HEAPLET_SPACE_NO_UPPER_BOUND];
  struct jitter_gc_space **spaces_to_clean_before_limit;

  /* An array of pointers to spaces (and used limit) which are to be emptied at
     the end of a collection.  Only used at collection time. */
  struct jitter_gc_space* spaces_to_clean_after [JITTER_HEAPLET_SPACE_NO_UPPER_BOUND];
  struct jitter_gc_space **spaces_to_clean_after_limit;

  /* A linked list containing the global roots for this heaplet.  Each element
     has type struct jitter_gc_global_root and is linked via its field named
     "links". */
  struct jitter_list_header global_roots;

  /* The temporary root set for this heaplet. */
  jitter_gc_temporary_root_set temporary_root_set;

  /* The object being shared, only useful when sharing is enabled, are also held
     in a stack data structure. */
#if defined (JITTER_GC_ENABLE_SHARING)
  jitter_gc_temporary_root_set objects_being_shared;
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* Pre-collection and post-collection hooks.  The lists link together elements
     of type struct jitter_gc_hook via their fields named "links". */
  struct jitter_list_header pre_collection_hooks;
  struct jitter_list_header post_collection_hooks;

  /* Same idea as pre-collection and post-collection hooks, but applied to SSB
     flushes.  Mostly useful for debugging and feedback. */
  struct jitter_list_header pre_ssb_flush_hooks;
  struct jitter_list_header post_ssb_flush_hooks;

  /* A set of tagged boxed oldspace objects which may point to youngspace
     objects, updated by flushing the SSB which is updated by the write barrier.
     These objects are considered roots in minor collections.  Major collections
     empty the set. */
  struct jitter_word_set remembered_set;

  /* Collection tuning parameters. */
  double minimum_nursery_size_in_bytes;
  double maximum_nursery_size_in_bytes;
  double low_nursery_survival_rate;
  double high_nursery_survival_rate;
  double nursery_growth_ratio;
  double nursery_shrinkage_ratio;
  double minimum_oldspace_size_in_bytes;
  double maximum_oldspace_size_in_bytes;
  double target_major_survival_rate;

  /* Automatically computed sizing parameters.  A collection, major or minor, is
     triggered when the relevant space reaches this used space threshold; then
     the threshold is recomputed based on survival statistics. */
  double nursery_used_size_threshold_in_bytes;
  double oldspace_used_size_threshold_in_bytes;

  /* Collection statistics. */
  jitter_point_in_time time_at_the_end_of_initialization;
  jitter_point_in_time collection_start_time;
  unsigned long collection_no;
  unsigned long minor_collection_no;
  unsigned long major_collection_no;
  unsigned long share_no;
  /* These array fields hold the most recent survival rates.  The index is
     updated before each write, so that the array field at the current index
     holds the most recent value. */
  unsigned int latest_nursery_survival_ratio_index;
  double latest_nursery_survival_ratios [JITTER_GC_SURVIVAL_RATIO_NO];
  unsigned int latest_major_survival_ratio_index;
  double latest_major_survival_ratios [JITTER_GC_SURVIVAL_RATIO_NO];
  double total_minor_survival_ratio;
  double last_nursery_survival_ratio;
  double total_nursery_survival_ratio;
  double total_major_survival_ratio;
  double total_collection_time;
  double total_minor_collection_time;
  double total_major_collection_time;
  double total_share_time;
  size_t used_bytes_at_the_beginning_of_this_phase;
  double total_bytes_allocated;
  double total_bytes_copied_minor;
  double total_bytes_copied_major;
  double total_bytes_copied;
  double total_bytes_copied_to_oldspace;
  size_t total_root_size_in_bytes_for_this_collection;
  double total_root_size_in_bytes;
  double total_initial_remembered_set_size;
  double total_final_remembered_set_size;
  double total_ssb_length;
  jitter_uint ssb_flush_no;
  double total_nursery_used_size_in_bytes;
  double total_old_space_used_size_in_bytes;
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  jitter_point_in_time *ssb_flush_begin_time;
  double total_ssb_flush_time;
  jitter_point_in_time *finalization_begin_time;
  double total_finalization_time;
  double total_finalized_object_no;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)

  /* Debugging data structures. */
#if defined (JITTER_GC_DEBUG)
  /* A pointer set used to guarantee that no root is registered more than
     once by mistake. */
  struct jitter_word_set unique_root_table;

  /* Non-false iff the valid copy of runtime fields is currently held in the
     heaplet.  This should never be set explicitly by the user: when
     JITTER_GC_DEBUG is defined this field is kept up to date and its correct
     value is asserted at some critical points -- in particular before changing
     it. */
  bool runtime_fields_owned;
#endif // #if defined (JITTER_GC_DEBUG)
};

/* See the field comment runtime_fields_owned in struct jitter_gc_heaplet
   above. */
#if defined (JITTER_GC_DEBUG)
#define JITTER_GC_DEBUG_INVALIDATE_POINTER(lvalue)  \
  do                                                \
    {                                               \
      (lvalue) = NULL;                              \
    }                                               \
  while (false)
#define JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_OWNED(h)                  \
  do                                                                    \
    {                                                                   \
      if (! (h)->runtime_fields_owned)                                  \
        jitter_fatal ("%s : runtime fields should be OWNED by the "     \
                      "heaplet at this point, but they are not",        \
                      __func__);                                        \
    }                                                                   \
  while (false)
#define JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_NOT_OWNED(h)              \
  do                                                                    \
    {                                                                   \
      if ((h)->runtime_fields_owned)                                    \
        jitter_fatal ("%s : runtime fields should NOT be owned by "     \
                      "the heaplet at this point, but they are",        \
                      __func__);                                        \
    }                                                                   \
  while (false)
#define JITTER_GC_DEBUG_SET_RUNTIME_FIELDS_OWNED(h)                     \
  do                                                                    \
    {                                                                   \
      JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_NOT_OWNED(h);               \
      (h)->runtime_fields_owned = true;                                 \
    }                                                                   \
  while (false)
#define JITTER_GC_DEBUG_SET_RUNTIME_FIELDS_NOT_OWNED(h)                 \
  do                                                                    \
    {                                                                   \
      JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_OWNED(h);                   \
      (h)->runtime_fields_owned = false;                                \
    }                                                                   \
  while (false)
#else // ! defined (JITTER_GC_DEBUG)
# define JITTER_GC_DEBUG_INVALIDATE_POINTER(lvalue)          do {} while (false)
# define JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_OWNED(h)      do {} while (false)
# define JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_NOT_OWNED(h)  do {} while (false)
# define JITTER_GC_DEBUG_SET_RUNTIME_FIELDS_OWNED(h)         do {} while (false)
# define JITTER_GC_DEBUG_SET_RUNTIME_FIELDS_NOT_OWNED(h)     do {} while (false)
#endif // #if defined (JITTER_GC_DEBUG)




/* Heaps.
 * ************************************************************************** */

/* At any given moment the heap contains a "request". */
enum jitter_gc_request
{
  /* No operation has been requested. */
  jitter_gc_request_no_request,

  /* A global collection has been requested. */
  jitter_gc_request_global_gc
};

/* A heap data structure conceptually "holds" the entire heap includings its
   meta-data such as the shape table, and allows access to the heaplets.  The
   system is design to rely on heaplets alone in the common cases, referring
   this global heap structure as little as possible: since this structure is
   shared among threads heap operations, differently from heaplet operations,
   require synchronisation.*/
struct jitter_gc_heap
{
  /* The shape table, of course shared by every heaplet. */
  const struct jitter_gc_shape_table *shape_table;

  /* A lock used to protect shared structures from concurrent access. */
  jitter_gc_heap_synchronization_entity lock;

  /* The current pending request. */
  volatile enum jitter_gc_request request;

  /* A space containing allocated but unused blocks, available to be
     repurposed and not currently belonging to any heaplet.  About which fields
     are initialised, see the comment inside struct jitter_gc_heaplet about
     the unused_space field. */
  struct jitter_gc_space unused_space;

#if defined(JITTER_GC_ENABLE_SHARING)
  /* A space containing used shared not owned by any heaplet. */
  struct jitter_gc_space shared_space;
#endif /* #if defined(JITTER_GC_ENABLE_SHARING) */

  /* Two linked lists containing the heaplets making up this heap.  Each
     element has type struct jitter_gc_heaplet and is linked via its field
     named "links".
     At any given time each heaplet from this heap must be in one of the two
     lists, but not both: the lists are disjoint. */
  struct jitter_list_header heaplets_in_use;
  struct jitter_list_header heaplets_not_in_use;
};




/* Heap initialisation and finalisation.
 * ************************************************************************** */

/* Initialize the pointed heap using the pointed shape table.  Fail fatally if
   allocation fails. */
void
jitter_gc_heap_initialize (struct jitter_gc_heap *h,
                           struct jitter_gc_shape_table *shape_tablep)
  __attribute__ ((nonnull (1, 2)));

/* Finalize the pointed heap.  Fail fatally if any heaplet for it still
   exists. */
void
jitter_gc_heap_finalize (struct jitter_gc_heap *h)
  __attribute__ ((nonnull (1)));

/* Return a pointer to a freshly allocated and initialized heap using the pointed
   shape table.  Fail fatally if allocation fails. */
struct jitter_gc_heap *
jitter_gc_heap_make (struct jitter_gc_shape_table *shape_tablep)
  __attribute__ ((nonnull (1),
                  returns_nonnull));

/* Finalize the pointed heap and also release the memory occupied by its struct.
   Fail fatally if any heaplet for the heap still exists.*/
void
jitter_gc_heap_destroy (struct jitter_gc_heap *h)
  __attribute__ ((nonnull (1)));




/* Heaplet initialisation and finalisation.
 * ************************************************************************** */

/* Initialize the pointed heaplet, allocating two fresh spaces of the given
   size for the pointed object shapes.  Fail fatally if allocation fails. */
void
jitter_gc_heaplet_initialize (struct jitter_gc_heaplet *a,
                              struct jitter_gc_heap *h)
  __attribute__ ((nonnull (1, 2)));

/* Finalize the pointed heaplet, freeing resources for the two spaces. */
void
jitter_gc_heaplet_finalize (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1)));

/* Return a freshly allocated and initialized heaplet containing two fresh
   spaces each having the given size.  Fail fatally if allocation fails. */
struct jitter_gc_heaplet *
jitter_gc_heaplet_make (struct jitter_gc_heap *h)
  __attribute__ ((nonnull (1),
                  returns_nonnull));;

/* Finalize the pointed heaplet and also release the memory occupied by
   its struct. */
void
jitter_gc_heaplet_destroy (struct jitter_gc_heaplet *heapletp)
  __attribute__ ((nonnull (1)));




/* Garbage collection roots.
 * ************************************************************************** */

/* A root_descriptor is a data structure containing an initial untagged pointer
   to a buffer holding valid data which are allowed to be tagged pointers to
   heap space, along with the buffer size. */
struct jitter_gc_root
{
  /* The beginning of the root in memory. */
  jitter_gc_tagged_object *buffer;

  /* How many bytes the root takes. */
  size_t buffer_size_in_bytes;
};

/* Handle a root pointer for the given heaplet.  This function may be called
   from a pre-collection hook: see jitter_gc_register_pre_hook below. */
void
jitter_gc_handle_root_pointer (struct jitter_gc_heaplet *a,
                               jitter_gc_tagged_object *p)
  __attribute__ ((nonnull (1), nonnull (2)));




/* Global root registration and deregistration.
 * ************************************************************************** */

/* A "global" root is registered with the expectation that it will remain in use
   in the long term; a good use case is roots in global variables.  These roots
   are relatively expensive to register and deregister, but have the advantage
   of supporting deregistration in any order, not necessarily following a LIFO
   policy.  Compare with "temporary roots" below. */

/* A pointer to be treated as an opaque handle for a global root, to be used at
   deregistration time. */
typedef struct jitter_gc_global_root *
jitter_gc_global_root;

/* A global root structure. */
struct jitter_gc_global_root
{
  /* The actual root data. */
  struct jitter_gc_root root;

  /* Links within a heaplet-wide doubly-linked list of global roots. */
  struct jitter_list_links links;
};

/* Register the pointed buffer with the given size in bytes as a global root for
   the pointed heaplet.  Return a root handle to be used at deregistration. */
jitter_gc_global_root
jitter_gc_register_global_root (struct jitter_gc_heaplet *a,
                                void *buffer,
                                size_t buffer_size_in_bytes)
  __attribute__ ((nonnull (1, 2), returns_nonnull));

/* Like jitter_gc_register_global_root , with a one-word root. */
jitter_gc_global_root
jitter_gc_register_global_root_1 (struct jitter_gc_heaplet *a,
                                  void *buffer)
  __attribute__ ((nonnull (1, 2), returns_nonnull));

/* Deregister the given global root from the pointed heaplet, freeing up the
   root resources.  Notice that every global root for a heaplet is also
   automatically deregistered when the heaplet is destroyed. */
void
jitter_gc_deregister_global_root (struct jitter_gc_heaplet *a,
                                  jitter_gc_global_root global_root)
  __attribute__ ((nonnull (1, 2)));




/* Temporary root registration.
 * ************************************************************************** */

/* Temporary roots can be handled very efficienctly (see the macro API below)
   but have the disadvantage of requiring deregistration in a strictly LIFO
   order -- possibly deregistering many temporary roots at the same time.
   Temporary roots are convenient for handling automatic variables and function
   arguments in C code. */

/* Push a temporary root starting at the given address and extending for the
   given number of byte to the root set of the pointed heaplet.
   Remark: here the type of buffer is a generic pointer type, more convenient
   for the user to pass without casts compared to the more explicit alternative
   jitter_gc_heap_pointer * . */
void
jitter_gc_push_temporary_root (struct jitter_gc_heaplet *a,
                               void *buffer,
                               size_t buffer_size_in_bytes)
  __attribute__ ((nonnull (1, 2)));

/* Like jitter_gc_push_temporary_root, for the common case of a one-word root.
   The same remark applies. */
void
jitter_gc_push_temporary_root_1 (struct jitter_gc_heaplet *a,
                                 void *buffer)
  __attribute__ ((nonnull (1, 2)));

/* Remove the last temporary root that was registered, and return a copy of it.
   Notice that destroying a heaplet also automatically removes all of its
   roots. */
const struct jitter_gc_root
jitter_gc_pop_temporary_root (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1)));

/* Remove every temporary root for the pointed heaplet.  Notice that
   destroying a heaplet also automatically removes all of its roots. */
void
jitter_gc_remove_all_temporary_roots (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1)));




/* Temporary root registration for C blocks: function API.
 * ************************************************************************** */

/* The functions in this section are intended for registrering and deregistering
   temporary roots associated to the life time of function arguments and
   automatic variables within a C block. */

/* An opaque representation of a temporary root set height, or size.  The
   current height is read before registering temporary roots, and then restored
   at the end of the block.  Restoring the height automatically pops every
   temporary root which was pushed after the corresponding height was read. */
typedef size_t
jitter_gc_temporary_root_set_height;

/* Return the current temporary root set size for the pointed heaplet, so that
   a later call to jitter_gc_reset_temporary_root_set_height may restore it by
   removing every root which was added after this call to
   jitter_gc_get_temporary_root_set_height.
   Remark: the warn_unused_result attribute here is useful to catch some
   mistakes (typically a forgotten use of JITTER_GC_BLOCK_END) but it will
   not prevent *every* error of such kind: when a block has multiple exit
   points GCC will only give the warning if the block is *always* exited
   without having using the result of this function; having just one exit
   where root set heght is correctly restored is unfortunately enough to
   silence the warning, while the user may have forgotten to do the same
   for other exists, which will lead to subtle bugs. */
jitter_gc_temporary_root_set_height
jitter_gc_get_temporary_root_set_height (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1), warn_unused_result));

/* Restore the root set size in the pointed heaplet to the given value,
   which must have been obtained from jitter_gc_get_temporary_root_set_height
   in the same heaplet.  This restores the root set to the state it had
   at the time of the call which returned the height being passed here. */
void
jitter_gc_reset_temporary_root_set_height (struct jitter_gc_heaplet *a,
                                           jitter_gc_temporary_root_set_height h)
  __attribute__ ((nonnull (1)));




/* Temporary root registration for C blocks: macro API.
 * ************************************************************************** */

/* The following three helper macros are an efficient and convenient way of
   using functionality equivalent to jitter_gc_get_temporary_root_set_height and
   jitter_gc_reset_temporary_root_set_height along with
   jitter_gc_push_temporary_root and jitter_gc_push_temporary_root_1 in C
   blocks, hiding block heights to the programmer.  They allow the
   registration and deregistration of temporary roots following a stylised
   pattern, intended to make the code clear and prevent subtle bugs. */

// FIXME: comment ////////////////////////////////////////////////////////
#define JITTER_GC_BLOCK_BEGIN(heaplet)                                      \
  jitter_gc_temporary_root_set_height _jitter_gc_temporary_root_set_height  \
    = (JITTER_DYNAMIC_BUFFER_USED_SIZE                                      \
          (& (heaplet)->temporary_root_set))

// FIXME: comment ////////////////////////////////////////////////////////
#define JITTER_GC_BLOCK_END(heaplet)         \
  JITTER_DYNAMIC_BUFFER_REDUCE_USED_SIZE     \
     (& (heaplet)->temporary_root_set,       \
      _jitter_gc_temporary_root_set_height)

// FIXME: comment ////////////////////////////////////////////////////////
// the pointer must be of type jitter_gc_tagged_object * .
#define JITTER_GC_BLOCK_ROOT(heaplet_p, pointer, object_no)           \
  do                                                                  \
    {                                                                 \
      struct jitter_gc_heaplet *_jitter_gc_heaplet_p = (heaplet_p);   \
      struct jitter_gc_root _jitter_gc_block_root_new_root;           \
      size_t _jitter_gc_block_root_object_no = (object_no);           \
      _jitter_gc_block_root_new_root.buffer = (pointer);              \
      _jitter_gc_block_root_new_root.buffer_size_in_bytes             \
        = sizeof (jitter_gc_tagged_object)                            \
          * _jitter_gc_block_root_object_no;                          \
      JITTER_DYNAMIC_BUFFER_PUSH (& (_jitter_gc_heaplet_p             \
                                        ->temporary_root_set),        \
                                  struct jitter_gc_root,              \
                                  & _jitter_gc_block_root_new_root);  \
/*printf ("QQQQ: root " #pointer " at %p %i objects\n", _jitter_gc_block_root_new_root.buffer, (int) _jitter_gc_block_root_object_no); */ \
    }                                                                 \
  while (false)

// FIXME: comment ////////////////////////////////////////////////////////
// the pointer must be of type jitter_gc_tagged_object * .
#define JITTER_GC_BLOCK_ROOT_1(heaplet_p, pointer)  \
  JITTER_GC_BLOCK_ROOT((heaplet_p), (pointer), 1)

#define JITTER_GC_BLOCK_RETURN_0(heaplet_p)                          \
  do                                                                 \
    {                                                                \
      struct jitter_gc_heaplet *_jitter_gc_heaplet_p = (heaplet_p);  \
      JITTER_GC_BLOCK_END(_jitter_gc_heaplet_p);                     \
      return;                                                        \
    }                                                                \
  while (false)

/* Why JITTER_GC_BLOCK_RETURN_1 needs a result-type argument: a rationale.

   It would be easy to provide a macro JITTER_GC_BLOCK_RETURN_1, expanding to a
   statetement sequence which deregisters temporary roots and then computes an
   expression to be returned.  Here is an example definition:

#define JITTER_GC_BLOCK_RETURN_1(heaplet_p, result)                  \
  do                                                                 \
    {                                                                \
      JITTER_GC_BLOCK_END(heaplet_p);                                \
      // Danger!  Now temporary roots have been deregistered...      \
      return (result); // ...and this might allocate from the heap.  \
    }                                                                \
  while (false)

  This version of JITTER_GC_BLOCK_RETURN_1 is much more subtle than one might
  think and in fact very fragile: in the definition above, in particular, the
  result expression is evaluated after the block has been closed, and therefore
  with the local roots already deregistered: if the result expression requires a
  heap allocation, which is in fact likely given this macro use case, the roots
  would be silently ignored.
  Such bugs are very difficult to catch, even more when the unexpected behaviour
  comes from a predefined macro rather than directly from user code.

  An alternative definition of JITTER_GC_BLOCK_RETURN_1 not suffering from this
  problem is possible in GNU C using either typeof or __auto_type , or in recent
  C++ dialects with the auto type -- but not in standard C before the new 2023
  version (upcoming at the time of writing, Summer 2022).

  ...And so, here is the solution. */
#define JITTER_GC_BLOCK_RETURN_1(heaplet_p, result_type, result)     \
  do                                                                 \
    {                                                                \
      struct jitter_gc_heaplet *_jitter_gc_heaplet_p = (heaplet_p);  \
      result_type _jitter_gc_result = (result);                      \
      JITTER_GC_BLOCK_END(_jitter_gc_heaplet_p);                     \
      return _jitter_gc_result;                                      \
    }                                                                \
  while (false)




/* Garbage collection hooks.
 * ************************************************************************** */

/* Hooks are user-defined functions which are automatically called at the
   beginning or at the end of an actual collection (which means, a collection
   whose kind is not jitter_gc_collection_kind share).
   In particular pre-collection hooks are useful to handle roots from runtime
   data structures such as VM stacks, of which only the actually used part is to
   be scanned. */


/* The type of a pre-collection or post-collection hook function.  The first
   argument is the heaplet which is about to collect or has just collected,
   and the second argument is a pointer to some data structure provided by the
   user at hook registration time.  The second argument is meant to refer to
   dynamic data structures containing roots, such as VM stacks.  The third
   option is the collection kind, which may be useful for debugging or for
   giving feedback to the user. */
typedef void (* jitter_gc_hook_f) (struct jitter_gc_heaplet *,
                                   void *,
                                   enum jitter_gc_collection_kind);

/* A hook descriptor containing a function along with its data. */
struct jitter_gc_hook
{
  /* The hook function. */
  jitter_gc_hook_f function;

  /* Hook data to be passed when calling the function. */
  void *data;

  /* Links within a heaplet-wide doubly-linked list of hook descriptors of
     the same kind -- pre or post. */
  struct jitter_list_links links;
};

/* An opaque descriptor to a registered hook, to be used for deregistration. */
typedef struct jitter_gc_hook*
jitter_gc_hook;

/* Register the given hook at pre-collection, post-collection, pre-SSB-flush or
   post-SSB flush, to be called on the pointed heaplet and on the pointed data,
   in the pointed heaplet.  Hooks are called in the same order they are
   registered, before every collection in the case of pre-collection hooks and
   after each collection in the case of post-collection hooks.  Return a handler
   to be used at deregistration. */
jitter_gc_hook
jitter_gc_hook_register_pre_collection (struct jitter_gc_heaplet *a,
                                        jitter_gc_hook_f hook_function,
                                        void *hook_data)
  __attribute__ ((nonnull (1), nonnull (2), returns_nonnull));
jitter_gc_hook
jitter_gc_hook_register_post_collection (struct jitter_gc_heaplet *a,
                                         jitter_gc_hook_f hook_function,
                                         void *hook_data)
  __attribute__ ((nonnull (1), nonnull (2), returns_nonnull));
jitter_gc_hook
jitter_gc_hook_register_pre_ssb_flush (struct jitter_gc_heaplet *a,
                                       jitter_gc_hook_f hook_function,
                                       void *hook_data)
  __attribute__ ((nonnull (1), nonnull (2), returns_nonnull));
jitter_gc_hook
jitter_gc_hook_register_post_ssb_flush (struct jitter_gc_heaplet *a,
                                        jitter_gc_hook_f hook_function,
                                        void *hook_data)
  __attribute__ ((nonnull (1), nonnull (2), returns_nonnull));


/* Deregister a previously registered hook.  It is an error to pass anything
   that was not returned by jitter_gc_hook_register_*_* , to deregister a hook
   more than once, or to deregister a hook of the wrong kind.
   These functions do *not* deallocate any pointed data passed as the last
   argument at registration time.
   Notice that every hook is automatically deregistered when a heaplet is
   destroyed.*/
void
jitter_gc_hook_deregister_pre_collection (struct jitter_gc_heaplet *a,
                                          jitter_gc_hook hook)
  __attribute__ ((nonnull (1), nonnull (2)));
void
jitter_gc_hook_deregister_post_collection (struct jitter_gc_heaplet *a,
                                           jitter_gc_hook hook)
  __attribute__ ((nonnull (1), nonnull (2)));
void
jitter_gc_hook_deregister_pre_ssb_flush (struct jitter_gc_heaplet *a,
                                         jitter_gc_hook hook)
  __attribute__ ((nonnull (1), nonnull (2)));
void
jitter_gc_hook_deregister_post_ssb_flush (struct jitter_gc_heaplet *a,
                                          jitter_gc_hook hook)
  __attribute__ ((nonnull (1), nonnull (2)));




/* Things to comment.  // FIXME: comment ////////////////////////////////////////////////////////
 * ************************************************************************** */

/* Expand to an expression evaluating to the allocation pointer for the heaplet
   which the evaluation of the argument points to.  The expansion is an l-value,
   but setting it directly is very error-prone: when setting runtime fields the
   user should call jitter_gc_heaplet_update_runtime_fields instead (or, even
   better, the macro JITTER_GC_RUNTIME_TO_HEAPLET), and consider this macro to
   be a read accesor. */
#define JITTER_GC_HEAPLET_ALLOCATION_POINTER(heaplet_p)  \
  ((heaplet_p)->nursery.allocation_pointer)

/* Like JITTER_GC_HEAPLET_ALLOCATION_POINTER but for the limit pointer. */
#define JITTER_GC_HEAPLET_ALLOCATION_LIMIT(heaplet_p)  \
  ((heaplet_p)->nursery.limit)

/* Update the runtime fields of the current fromspace in the pointed heaplet.
   Remark: this operation is the opposite of
   JITTER_GC_HEAPLET_ALLOCATION_POINTER and JITTER_GC_HEAPLET_ALLOCATION_LIMIT ,
   which extract information from the heaplet to be able to update a runtime
   field; this operation takes a changed runtime fields and updates the copy
   within the heaplet.
   Notice that updating the allocation limit means flushing the write-barrier
   SSB, in the copy of the limit pointer within the heaplet; this is intended,
   since keeping the two limit pointers inconistent would be a recipe for very
   subtle bugs.
   The result is the updated limit pointer, which *must* be copied into the
   appropriate runtime field out of the heaplet. */
jitter_gc_heap_pointer
jitter_gc_heaplet_update_runtime_fields
   (struct jitter_gc_heaplet *a,
    jitter_gc_heap_pointer allocation_pointer,
    jitter_gc_heap_pointer allocation_limit)
  __attribute__ ((nonnull (1, 2, 3), warn_unused_result));

/* Flush the write-barrier SSB and update the runtime fields in the pointed
   heaplet, copying from the given runtime fields.
   This is a convenient way of calling jitter_gc_heaplet_update_runtime_fields
   without forgetting to update the limit pointer.  The limit pointer argument
   must be an l-value. */
#define JITTER_GC_RUNTIME_TO_HEAPLET(heaplet,                              \
                                     allocation_pointer_,                  \
                                     allocation_limit_)                    \
  do                                                                       \
    {                                                                      \
      (allocation_limit_)                                                  \
        = jitter_gc_heaplet_update_runtime_fields ((heaplet),              \
                                                   (allocation_pointer_),  \
                                                   (allocation_limit_));   \
      JITTER_GC_DEBUG_SET_RUNTIME_FIELDS_OWNED (heaplet);                  \
    }                                                                      \
  while (false)

/* The converse of JITTER_GC_HEAPLET_TO_RUNTIME: this updates the allocation
   pointer and allocation limit from the heaplet.  Both allocation pointer
   and allocation limit must be l-values.  It does not make sense to call
   this when the write-barrier SSB is not empty. */
#define JITTER_GC_HEAPLET_TO_RUNTIME(heaplet,                                  \
                                     allocation_pointer,                       \
                                     allocation_limit)                         \
  do                                                                           \
    {                                                                          \
      (allocation_pointer) = JITTER_GC_HEAPLET_ALLOCATION_POINTER ((heaplet)); \
      (allocation_limit) = JITTER_GC_HEAPLET_ALLOCATION_LIMIT ((heaplet));     \
      JITTER_GC_DEBUG_SET_RUNTIME_FIELDS_NOT_OWNED (heaplet);                  \
    }                                                                          \
  while (false)

/* This enumerate expresses the kind of action required by the allocation slow
   path.  It is only used as the type for the last argument of
   _jitter_gc_allocate_slow_path .  */
enum jitter_gc_action
  {
    /* Decide what to do using heuristics. */
    jitter_gc_action_default,

    /* Change the allocation block in the nursery replacing it with an empty
       one, making a new block if needed but without ever collecting. */
    jitter_gc_action_block_change,

    /* Force a minor collection. */
    jitter_gc_action_force_minor,

    /* Force a major collection. */
    jitter_gc_action_force_major,

    /* Force a collection; heuristics will decide whether the collection is
       minor or major. */
    jitter_gc_action_force_either,

    /* Force a global collection. */
    jitter_gc_action_force_global,

    /* Perform the share operation, moving non-shared objects (along with all
       the objects they refer) into the shared generation.
[FIXME: check after the implementation that I actually decided to do it like this in the end]
       The share operation also involves a minor or major collection, which is
       executed as part of this operation. */
    jitter_gc_action_share
  };

/* FIXME: comment  //////////////////////////////////////////////////////////////////////////////
   When this is called the write-barrier SSB must be empty and the runtime
   fields within the heaplet must be up to date.  In practice this should only
   ever be called after jitter_gc_heaplet_update_runtime_fields or
   JITTER_GC_RUNTIME_TO_HEAPLET ; since it is so easy to make a mistake I
   consider this function to be internal, and not for the user.
   In order to request an explicit collection the user can call the macro
   JITTER_GC_COLLECT , which is convenient and not error-prone. */
void
_jitter_gc_allocate_slow_path_with_runtime_fields_owned_and_ssb_flushed
   (struct jitter_gc_heaplet *a,
    size_t failed_allocation_size_in_bytes,
    enum jitter_gc_action action)
  __attribute__ ((nonnull (1)));




/* Heap resizing.
 * ************************************************************************** */

/* // FIXME: comment ///////////////////////////////////////////////////////////////////// */
/* size_t */
/* jitter_gc_heap_size (struct jitter_gc_heaplet *a) */
/*   __attribute__ ((nonnull (1), pure)); */

// FIXME: comment /////////////////////////////////////////////////////////////////////
/* Remark: if the new requested size does not suffice to hold currently alive
   data then the new size is automatically increased.
   Rationale: this behaviour more useful than failing. */
void
jitter_gc_resize_heap (struct jitter_gc_heaplet *a,
                       size_t new_size_in_bytes);




/* Allocation.
 * ************************************************************************** */

// FIXME: comment everything /////////////////////////////////////////////////////////////////////


/* If JITTER_GC_DEBUG is defined expand to a statement checking that the given
   argument, the size of an object to allocate in bytes, is a multiple of the
   minimum size, and failing fatally if that is not the case.
   If JITTER_GC_DEBUG is not defined expand to a statement doing nothing. */
#if defined (JITTER_GC_DEBUG)
# define _JITTER_GC_CHECK_NEW_OBJECT_SIZE(size_in_bytes)                      \
    do                                                                        \
      {                                                                       \
        if (JITTER_GC_ROUND_SIZE_UP (size_in_bytes) != size_in_bytes)         \
          jitter_fatal ("trying to allocate %li B, which is not a multiple "  \
                        "of %li B", (long) size_in_bytes,                     \
                        (long) JITTER_GC_MINIMUM_OBJECT_SIZE_IN_BYTES);       \
      }                                                                       \
    while (false)
#else // not debugging
# define _JITTER_GC_CHECK_NEW_OBJECT_SIZE(size_in_bytes)  \
    do { /* Nothing. */ } while (false)
#endif // #if defined (JITTER_GC_DEBUG)



/* FIXME: in the comment: the result is not necessarily interesting.  This
   is meant to be used as an expression, before a comma operator. */
#define _JITTER_GC_TRY_TO_ALLOCATE_EXP(_jitter_gc_allocation_pointer,  \
                                       _jitter_gc_allocation_limit,    \
                                       _jitter_gc_result_lvalue,       \
                                       _jitter_gc_size_in_bytes)       \
  (((_jitter_gc_result_lvalue)                                         \
    = ((jitter_gc_tagged_object *) (_jitter_gc_allocation_pointer))),  \
   ((_jitter_gc_allocation_pointer) += (_jitter_gc_size_in_bytes)))

#define _JITTER_GC_HAS_ALLOCATION_FAILED(_jitter_gc_allocation_pointer,  \
                                         _jitter_gc_allocation_limit)    \
  (__builtin_expect (((_jitter_gc_allocation_pointer)                    \
                      > (_jitter_gc_allocation_limit)),                  \
                     false))

#define _JITTER_GC_IF_ALLOCATE_FAILS_(_jitter_gc_allocation_pointer,      \
                                      _jitter_gc_allocation_limit,        \
                                      _jitter_result_lvalue,              \
                                      _jitter_gc_size_in_bytes)           \
  if (_JITTER_GC_TRY_TO_ALLOCATE_EXP ((_jitter_gc_allocation_pointer),    \
                                      (_jitter_gc_allocation_limit),      \
                                      (_jitter_result_lvalue),            \
                                      (_jitter_gc_size_in_bytes)),        \
      _JITTER_GC_HAS_ALLOCATION_FAILED ((_jitter_gc_allocation_pointer),  \
                                        (_jitter_gc_allocation_limit)))

/* On debug, fill memory for the given heaplet starting from the given
   pointer, for the given number of bytes (which must be a multiple of the word
   size) with the uninitialised value.  If debug is disabled expand to a
   statement doing nothing. */
#if defined (JITTER_GC_DEBUG)
# define _JITTER_GC_SET_UINITIALIZED(_jitter_gc_heaplet_p,                 \
                                     _jitter_gc_new_object_pointer,        \
                                     _jitter_gc_new_object_size_in_bytes)  \
   do                                                                      \
     {                                                                     \
       jitter_gc_tagged_object _jitter_gc_uninitialized                    \
         = (_jitter_gc_heaplet_p)->shape_table->uninitialized_object;      \
       jitter_gc_tagged_object *_jitter_gc_p                               \
         = ((jitter_gc_tagged_object *)                                    \
            (_jitter_gc_new_object_pointer));                              \
       jitter_gc_tagged_object *_jitter_gc_fill_limit                      \
         = ((jitter_gc_tagged_object *)                                    \
            ((char *) _jitter_gc_p                                         \
             + (_jitter_gc_new_object_size_in_bytes)));                    \
       for (; _jitter_gc_p < _jitter_gc_fill_limit; _jitter_gc_p ++)       \
         * _jitter_gc_p = _jitter_gc_uninitialized;                        \
     }                                                                     \
   while (false)
#else // ! defined (JITTER_GC_DEBUG)
# define _JITTER_GC_SET_UINITIALIZED(_jitter_gc_heaplet_p,                 \
                                     _jitter_gc_new_object_pointer,        \
                                     _jitter_gc_new_object_size_in_bytes)  \
    do                                                                     \
      {                                                                    \
        /* Do nothing. */                                                  \
      }                                                                    \
    while (false)
#endif // #if defined (JITTER_GC_DEBUG)

/// FIXME: comment. ///////////////////////////////////////////////////////////////////
#define _JITTER_GC_ALLOCATE(heaplet_p,                                      \
                            allocation_pointer_lvalue,                      \
                            allocation_limit_lvalue,                        \
                            result_lvalue,                                  \
                            size_in_bytes)                                  \
  do                                                                        \
    {                                                                       \
      /* Evaluate r-value arguments, once. */                               \
      struct jitter_gc_heaplet *_jitter_gc_a_heaplet_p = (heaplet_p);       \
      size_t _jitter_gc_a_size_in_bytes = (size_in_bytes);                  \
      /* Check the requested object alignment, when debugging. */           \
      _JITTER_GC_CHECK_NEW_OBJECT_SIZE (_jitter_gc_a_size_in_bytes);        \
      /* Try to allocate by pointer-bumping.  If this fails... */           \
      _JITTER_GC_IF_ALLOCATE_FAILS_ ((allocation_pointer_lvalue),           \
                                     (allocation_limit_lvalue),             \
                                     (result_lvalue),                       \
                                     _jitter_gc_a_size_in_bytes)            \
        {                                                                   \
          /* ...Use the slower alternative, which might involve a garbage
             collection. */                                                 \
          _JITTER_GC_ALLOCATION_UNCOMMON_PATH (_jitter_gc_a_heaplet_p,      \
                                               (allocation_pointer_lvalue), \
                                               (allocation_limit_lvalue),   \
                                               (result_lvalue),             \
                                               _jitter_gc_a_size_in_bytes); \
        }                                                                   \
      /* Fill the allocated buffer with the "uninitialized" value when
         debugging; do nothing more otherwise. */                           \
      _JITTER_GC_SET_UINITIALIZED (_jitter_gc_a_heaplet_p,                  \
                                   (((char *) (allocation_pointer_lvalue))  \
                                    - _jitter_gc_a_size_in_bytes),          \
                                   _jitter_gc_a_size_in_bytes);             \
    }                                                                       \
  while (false)

/* The less-frequently executed part of allocation.  This is executed after
   pointer-bumping failed to allocate the give number of bytes, the allocation
   pointer crossing the limit pointer.  The expansion is a statement reading
   and writing both runtime fields, which must be l-values.  It sets the
   result in the given l-value.  It uses the given heaplet, an r-value.
   Arguments may be evaluated more than once. */
#define _JITTER_GC_ALLOCATION_UNCOMMON_PATH(_jitter_gc_heaplet_p,             \
                                            _jitter_gc_allocation_pointer,    \
                                            _jitter_gc_allocation_limit,      \
                                            _jitter_result_lvalue,            \
                                            _jitter_gc_size_in_bytes)         \
  do                                                                          \
    {                                                                         \
      /* First fix the allocation-pointer, by undoing the update that made
         it overflow. */                                                      \
      (_jitter_gc_allocation_pointer) -= (_jitter_gc_size_in_bytes);          \
      char *_jitter_gc_allocation_pointer_copy                                \
        = (_jitter_gc_allocation_pointer);                                    \
      /* Now flush the write barrier SSB: that alone might free enough space
         for a further attempt to succeed.  An easy way to do that is by
         updating the heaplet runtime fields, which will be needed anyway if
         we are to collect. */                                                \
      JITTER_GC_RUNTIME_TO_HEAPLET ((_jitter_gc_heaplet_p),                   \
                                    (_jitter_gc_allocation_pointer),          \
                                    (_jitter_gc_allocation_limit));           \
      /* Re-acquire the runtime fields, since we are trying to allocate
         once more. */                                                        \
      JITTER_GC_HEAPLET_TO_RUNTIME ((_jitter_gc_heaplet_p),                   \
                                    (_jitter_gc_allocation_pointer),          \
                                    (_jitter_gc_allocation_limit));           \
      /* Try to allocate again.  If this attempt fails as well we really
         need to go through the slow path, and possibly collect. */           \
      _JITTER_GC_IF_ALLOCATE_FAILS_ ((_jitter_gc_allocation_pointer),         \
                                     (_jitter_gc_allocation_limit),           \
                                     (_jitter_result_lvalue),                 \
                                     (_jitter_gc_size_in_bytes))              \
        {                                                                     \
          /* Allocation failed once more: run the slow path, after undoing
             the allocation pointer update.
                 Advanced remark: yes, we do need to undo the update even if
                 now we are working on the heaplet copy and not the runtime
                 copy: the problem is that the runtime fields and the heaplet
                 runtime fields are allowed to physically be the same. */     \
          (_jitter_gc_allocation_pointer)                                     \
            = _jitter_gc_allocation_pointer_copy;                             \
          JITTER_GC_DEBUG_SET_RUNTIME_FIELDS_OWNED ((_jitter_gc_heaplet_p));  \
          _jitter_gc_allocate_slow_path_with_runtime_fields_owned_and_ssb_flushed \
             ((_jitter_gc_heaplet_p),                                         \
              (_jitter_gc_size_in_bytes),                                     \
              jitter_gc_action_default);                             \
          /* Get the new runtime fields from the heaplet. */                  \
          JITTER_GC_HEAPLET_TO_RUNTIME ((_jitter_gc_heaplet_p),               \
                                        (_jitter_gc_allocation_pointer),      \
                                        (_jitter_gc_allocation_limit));       \
          /* The garbage collector has freed at least the requested amount    \
             of memory.  It is now guaranteed that this next attempt will     \
             succeed. */                                                      \
          _JITTER_GC_TRY_TO_ALLOCATE_EXP ((_jitter_gc_allocation_pointer),    \
                                          (_jitter_gc_allocation_limit),      \
                                          (_jitter_result_lvalue),            \
                                          (_jitter_gc_size_in_bytes));        \
        }                                                                     \
    }                                                                         \
  while (false)




/* Padding.
 * ************************************************************************** */

/* FIXME: comment well. ///////////////////////////////////////////////////////////////////
   FIXME: Explain why this is not needed for headered objects (since they are traced by user-specified functions any padding at the end is simply never touched, and can remain uninitialised). */
#define JITTER_GC_PAD_HEADERLESS_OBJECT(untagged_initial_pointer,              \
                                        unpadded_size_in_elements,             \
                                        padding_word)                          \
  do                                                                           \
    {                                                                          \
      jitter_gc_tagged_object _jitter_gc_pad_padding_word                      \
        = (jitter_gc_tagged_object) (padding_word);                            \
      jitter_gc_tagged_object *_jitter_gc_pad_initial_pointer                  \
        = (jitter_gc_tagged_object *) (untagged_initial_pointer);              \
      size_t _jitter_gc_pad_element_no = (unpadded_size_in_elements);          \
      size_t _jitter_gc_pad_unpadded_size_in_bytes                             \
        = _jitter_gc_pad_element_no * sizeof (jitter_gc_tagged_object);        \
      size_t _jitter_gc_pad_padded_size_in_bytes                               \
        = JITTER_GC_ROUND_SIZE_UP (_jitter_gc_pad_unpadded_size_in_bytes);     \
                                                                               \
      jitter_gc_tagged_object *_jitter_gc_pad_p;                               \
      jitter_gc_tagged_object *_jitter_gc_pad_limit                            \
        = (jitter_gc_tagged_object *)((char*) _jitter_gc_pad_initial_pointer   \
                                      + _jitter_gc_pad_padded_size_in_bytes);  \
      for (_jitter_gc_pad_p                                                    \
             = _jitter_gc_pad_initial_pointer + _jitter_gc_pad_element_no;     \
           _jitter_gc_pad_p < _jitter_gc_pad_limit;                            \
           _jitter_gc_pad_p ++)                                                \
        * _jitter_gc_pad_p = _jitter_gc_pad_padding_word;                      \
    }                                                                          \
  while (false)




/* Write barrier.
 * ************************************************************************** */

/* FIXME: comment well. ///////////////////////////////////////////////////////////////////
   The object being stored in the SSB is a tagged object containing some field
   which was updated; notice that the tagged object is stored, and not the
   address of some of its fields.  The object must reside within the heaplet,
   and therefore belong to a space and to a block, which can be checked for its
   generation.

   The result is the new allocation limit.

   Notice that, differently from garbage collecting functions, this function
   gives no guarantee about how much space is freed in the SSB; there might
   simply be no place left.  This function will flush the SSB and register the
   given pointer, but the next attempt of enqueuing an address into the SSB
   might fail again.

   Still it should be noticed that, differently from the case of
   jitter_gc_collect_0, this function does in fact guarantee that the requested
   operation succeeds: only *the next* allocation attempt may fail.  This has
   implications over VM code: after jumping to the slow path of the write
   barrier, control should return *past* the write barrier fast path: that one
   write barrier completed, with success.
   Compare with allocation from VM code, where the slow path has to jump back
   to the fast path to try again after an allocation failed.  [FIXME: is that true for allocation?] */
jitter_gc_heap_pointer
_jitter_gc_ssb_flush_1 (struct jitter_gc_heaplet *a,
                        jitter_gc_heap_pointer allocation_limit,
                        jitter_gc_tagged_object tagged_updated_object)
  __attribute__ ((nonnull (1), nonnull (2),
                  returns_nonnull, warn_unused_result));

/* Like jitter_gc_ssb_flush_1, but do not add any new object to the SSB. */
/* FIXME: remove this and make the function static if not used from macros.  It
   is used internally by jitter_gc_heaplet_update_runtime_fields . */
jitter_gc_heap_pointer
_jitter_gc_ssb_flush_0 (struct jitter_gc_heaplet *a,
                        jitter_gc_heap_pointer allocation_limit)
  __attribute__ ((nonnull (1), nonnull (2),
                  returns_nonnull, warn_unused_result));

/* FIXME: in the comment: the result is not necessarily interesting.  This
   is meant to be used as an expression, before a comma operator.
   Like _JITTER_GC_TRY_TO_ALLOCATE_EXP */
#define _JITTER_GC_SSB_TRY_TO_ENQUEUE_EXP(allocation_limit)   \
  ((allocation_limit)                                         \
   = ((jitter_gc_heap_pointer)                                \
      ((jitter_gc_tagged_object *) (allocation_limit) - 1)))

#define _JITTER_GC_SSB_HAS_ENQUEUING_FAILED(allocation_pointer,   \
                                            allocation_limit)     \
  (__builtin_expect ((allocation_pointer) >= (allocation_limit),  \
                     false))

#define _JITTER_GC_SSB_IF_ENQUEUING_FAILS_(allocation_pointer,    \
                                           allocation_limit)      \
  if (/* First try to update the pointer... */                    \
      _JITTER_GC_SSB_TRY_TO_ENQUEUE_EXP (allocation_limit),       \
      /* Then (after comma) evaluate to a Boolean */              \
      _JITTER_GC_SSB_HAS_ENQUEUING_FAILED ((allocation_pointer),  \
                                           (allocation_limit)))

/* FIXME: is this useful?  Maybe it is conceivable that a want to explicitly
   demand to flush the SSB, just like she can demand an explicit collection. */
#define _JITTER_GC_SSB_FLUSH_0(heaplet,                                  \
                               allocation_limit)                         \
  do                                                                     \
    {                                                                    \
      (allocation_limit) = _jitter_gc_ssb_flush_0 ((heaplet),            \
                                                   (allocation_limit));  \
    }                                                                    \
  while (false)

#define _JITTER_GC_SSB_FLUSH_1(heaplet,                                       \
                               allocation_limit,                              \
                               tagged_updated_object)                         \
  do                                                                          \
    {                                                                         \
      (allocation_limit) = _jitter_gc_ssb_flush_1 ((heaplet),                 \
                                                   (allocation_limit),        \
                                                   (tagged_updated_object));  \
    }                                                                         \
  while (false)

/* This part of the write barrier is everything needed with no sharing.  When
   sharing is enabled the barrier needs to be more complex (and slower), but
   still this functionality will be part of it. */
#define _JITTER_GC_SIMPLE_WRITE_BARRIER(heaplet,                    \
                                        allocation_pointer,         \
                                        allocation_limit,           \
                                        tagged_updated_object,      \
                                        tagged_new_pointed_object)  \
  do                                                                \
    {                                                               \
      _JITTER_GC_SSB_IF_ENQUEUING_FAILS_ ((allocation_pointer),     \
                                          (allocation_limit))       \
        _JITTER_GC_SSB_FLUSH_1 ((heaplet),                          \
                                (allocation_limit),                 \
                                (tagged_updated_object));           \
      else                                                          \
        * (jitter_gc_tagged_object *) (allocation_limit)            \
          = (tagged_updated_object);                                \
    }                                                               \
  while (false)

/* Expand to an expression evaluating to the generation of the given tagged
   object, which is allowed to be boxed or unboxed, but must belong to the
   pointed heaplet. */
#define JITTER_GC_TAGGED_TO_GENERATION(heaplet_p, tagged_object)   \
  /* FIXME: the call into the shape-table function is inefficient, and this
     code is performance-critical because it is used from the write / share
     barrier.  To make it faster I need code generation. */        \
  ((heaplet_p)->shape_table->is_unboxed (tagged_object)            \
   ? jitter_gc_generation_immortal                                 \
   : JITTER_GC_TAGGED_BOXED_TO_GENERATION(tagged_object))

/* Like JITTER_GC_TAGGED_TO_GENERATION, but assume that the object is boxed. */
#define JITTER_GC_TAGGED_BOXED_TO_GENERATION(tagged_object) \
  (JITTER_GC_BOXED_TO_BLOCK(tagged_object)->generation)

/* Given an expression evaluating to a tagged object, expand to an expression
   evaluating to a pointer to the space the tagged object belongs to. */
#define JITTER_GC_TAGGED_BOXED_TO_SPACE(tagged_object) \
  (JITTER_GC_BOXED_TO_BLOCK(tagged_object)->space)

// This version of the write barrier is correct is case of sharing.
#define _JITTER_GC_WRITE_SHARE_BARRIER(heaplet,                                 \
                                       allocation_pointer,                      \
                                       allocation_limit,                        \
                                       tagged_updated_object,                   \
                                       tagged_new_pointed_object)               \
  do                                                                            \
    {                                                                           \
      /* Evaluate expression arguments to avoid multiple evaluations. */        \
      struct jitter_gc_heaplet *_jitter_gc_heaplet = (heaplet);                 \
      jitter_gc_tagged_object _jitter_gc_updated_object =                       \
        (tagged_updated_object);                                                \
      jitter_gc_tagged_object _jitter_gc_new_pointed_object =                   \
        (tagged_new_pointed_object);                                            \
                                                                                \
      /* We have to distinguish the share-barrier case as quickly as possible.
         Luckily it is not so difficult: we just have to check the generation of
         the object which is being updated -- updated, therefore guaranteed to
         be boxed.  Only if the object being updated is shared we need to go
         through the slow path.  The slow path contains conditionals and may
         still end up being trivial, in the case of a shared object pointing to
         another shared object or an immortal object . */                       \
      enum jitter_gc_generation _jitter_gc_updated_generation                   \
        = JITTER_GC_TAGGED_BOXED_TO_GENERATION (_jitter_gc_updated_object);     \
      if (__builtin_expect ((_jitter_gc_updated_generation                      \
                             == jitter_gc_generation_shared),                   \
                            false))                                             \
        {                                                                       \
          /* The share barrier may involve a GC, so the heaplet's runtime
             structures must be up to date when we enter it. */                 \
          JITTER_GC_RUNTIME_TO_HEAPLET (_jitter_gc_heaplet,                     \
                                        allocation_pointer,                     \
                                        allocation_limit);                      \
          jitter_gc_share_barrier_slow_path (_jitter_gc_heaplet,                \
                                             & _jitter_gc_updated_object,       \
                                             & _jitter_gc_new_pointed_object);  \
          JITTER_GC_HEAPLET_TO_RUNTIME (_jitter_gc_heaplet,                     \
                                        allocation_pointer,                     \
                                        allocation_limit);                      \
          /* Update the pointed object and updated object as passed by the
             caller: we may have moved them. */                                 \
          (tagged_new_pointed_object) = _jitter_gc_new_pointed_object;          \
          (tagged_updated_object) = _jitter_gc_updated_object;                  \
        }                                                                       \
      else                                                                      \
        /* If we arrived here then the object being updated is either young or
           old.  It would be possible to check if the object is young and in
           that case just exit, but instead we decided to run the barrier
           anyway, which is fast.  The updated object generation (young or
           old) will be checked later when we flush the SSB. */                 \
        _JITTER_GC_SIMPLE_WRITE_BARRIER(_jitter_gc_heaplet,                     \
                                        allocation_pointer,                     \
                                        allocation_limit,                       \
                                        _jitter_gc_updated_object,              \
                                        tagged_new_pointed_object);             \
    }                                                                           \
  while (false)

/* The write barrier macro.
   Arguments:
   * heaplet pointer
   * allocation pointer l-value
   * allocation limit l-value
   * tagged_updated_object, r-value
   * tagged_new_pointed_object, r-value

   The write barrier must be executed *before* tagged_updated_object is modified
   to point to tagged_updated_object which is an object possibly younger than
   tagged_updated_object.

   In the sharing case a write barrier can trigger a collection (for simplicity
   I suggest assuming that a write barrier can always trigger a collection):
   roots must be registered correctly while the write barrier runs: in
   particular the new pointed object *may* need to be registered as a root (in
   case the C expression expressing it is used again in the code after the write
   barrier).

   When updating an object A to point to an object B, it is allowed that
   tagged_new_pointed_object is not B, but rather some object C pointing to B.
   The rationale is that one may be updating many different fields of A, using
   objects B1, B2, ... Bn; instead of executing the write barrier n times on
   A and Bi, one can have all the Bi objects inside a (possibly temporary) heap
   structure and then call the barrier only once on A and the heap structure.
   The difference in performance is important in the sharing case, when barriers
   are expensive. */
#if defined (JITTER_GC_ENABLE_SHARING)
  /* Define the actual write barrier as the write-share barrier, expensive but
     correct in this case. */
# define JITTER_GC_WRITE_BARRIER  _JITTER_GC_WRITE_SHARE_BARRIER
#else
  /* Define the actual barrier as the cheaper non-sharing write barrier, which
     is enough for the non-sharing case. */
# define JITTER_GC_WRITE_BARRIER  _JITTER_GC_SIMPLE_WRITE_BARRIER
#endif // #if defined (JITTER_GC_ENABLE_SHARING)




/* Sharing.
 * ************************************************************************** */

// FIXME: comment.

// FIXME: comment.  This assumes that the heaplet owns runtime fields.
void
jitter_gc_share_barrier_slow_path (struct jitter_gc_heaplet *a,
                                   jitter_gc_tagged_object *updated_p,
                                   jitter_gc_tagged_object *new_pointed_p);


// FIXME: comment.
// I think I should change this to not take a pointer and instead return a
//  result, with __attribute__ ((warn_unused_result)).
// This is not for the user: this assumes that he heaplet owns runtime fields.
void
_jitter_gc_share (struct jitter_gc_heaplet *a, jitter_gc_tagged_object *p)
  __attribute__ ((nonnull (1), nonnull (2)));

#if defined(JITTER_GC_ENABLE_SHARING)
# define JITTER_GC_SHARE(heaplet_p,                                          \
                         allocation_pointer,                                 \
                         allocation_limit,                                   \
                         object_p)                                           \
    do                                                                       \
      {                                                                      \
        struct jitter_gc_heaplet *_jitter_gc_share_heaplet_p = (heaplet_p);  \
        jitter_gc_tagged_object *_jitter_gc_share_object_p = (object_p);     \
        JITTER_GC_RUNTIME_TO_HEAPLET (_jitter_gc_share_heaplet_p,            \
                                      (allocation_pointer),                  \
                                      (allocation_limit));                   \
        _jitter_gc_share (_jitter_gc_share_heaplet_p,                        \
                          _jitter_gc_share_object_p);                        \
        JITTER_GC_HEAPLET_TO_RUNTIME (_jitter_gc_share_heaplet_p,            \
                                      (allocation_pointer),                  \
                                      (allocation_limit));                   \
      }                                                                      \
    while (false)
#else /* ! defined(JITTER_GC_ENABLE_SHARING) */
# define JITTER_GC_SHARE(heaplet_p,           \
                         allocation_pointer,  \
                         allocation_limit,    \
                         object_p)            \
    do { /* Nothing. */ } while (false)
#endif /* #if defined(JITTER_GC_ENABLE_SHARING) */


/* Global garbage collection (tentative API).
 * ************************************************************************** */

void
jitter_gc_request_global_collection (struct jitter_gc_heaplet *a) // mostly for tests
  __attribute__ ((nonnull (1)));

void
jitter_gc_global_collection_safe_point_slow_path (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1)));

#define JITTER_GC_GLOBAL_COLLECTION_SAFE_POINT(heaplet_p,                     \
                                               allocation_pointer,            \
                                               allocation_limit)              \
  do                                                                          \
    {                                                                         \
      /* Even under the weakest shared memory model it is correct to read a
         memory location written to by another thread, which might or might
         not be up to date.
         It is harmless to see an older request here (out of the expensive
         critical section) as long as the current value becomes eventually
         visible.
         With this premise, read the current request in the heap which is
         written concurrently, without synchronisation.  If it seems like
         there is a global collection request, enter the slow path and check
         again, this time with the proper synchronisation. */                 \
      struct jitter_gc_heaplet                                                \
        *_jitter_gc_safe_point_heaplet_p = (heaplet_p);                       \
      if (__builtin_expect ((_jitter_gc_safe_point_heaplet_p->heap->request   \
                             == jitter_gc_request_global_gc),                 \
                            false))                                           \
          {                                                                   \
            JITTER_GC_RUNTIME_TO_HEAPLET (_jitter_gc_safe_point_heaplet_p,    \
                                          (allocation_pointer),               \
                                          (allocation_limit));                \
            jitter_gc_global_collection_safe_point_slow_path                  \
              (_jitter_gc_safe_point_heaplet_p);                              \
            JITTER_GC_HEAPLET_TO_RUNTIME (_jitter_gc_safe_point_heaplet_p,    \
                                          (allocation_pointer),               \
                                          (allocation_limit));                \
          }                                                                   \
    }                                                                         \
  while (false)

void
jitter_gc_before_blocking (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1)));

void
jitter_gc_after_blocking (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1)));




/* Temporarily disabling garbage collection.
 * ************************************************************************** */

/* Temporarily disable collection in the pointed heaplet.  While collection is
   disabled allocation never results in a collection of any kind, and even
   received global collection requests from other heaplets are not honored and
   left pending.  Explicit collection requests while collection is disabled
   result in fatal errors.
   Notice that sharing is also forbidden while collection is disabled, since any
   non-trivial sharing involves a collection.  [FIXME: do I want to relax this?]
   Collection is enabled by default. */
void
jitter_gc_disable_collection (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1)));

/* Re-enable garbage collection in the pointed heaplet, in case it was
   disabled. */
void
jitter_gc_enable_collection (struct jitter_gc_heaplet *a)
  __attribute__ ((nonnull (1)));




/* Garbage collection on demand.
 * ************************************************************************** */

/* Perform an explicit garbage collection without using the pointed heaplet
   and the given runtime fields.  Perform the given action.
   This uses and updates the given allocation pointer and allocation limit,
   which must be l-values. */
#define JITTER_GC_COLLECT(_jitter_gc_heaplet_p,                                \
                          _jitter_gc_allocation_pointer,                       \
                          _jitter_gc_allocation_limit,                         \
                          action)                                              \
  do                                                                           \
    {                                                                          \
printf("OK-Z 0\n");\
      struct jitter_gc_heaplet *_jitter_gc_collect_heaplet_p                   \
        = (_jitter_gc_heaplet_p);                                              \
      enum jitter_gc_action _jitter_gc_collect_action = (action);              \
      /* Update the runtime fields in the heaplet, so that the garbage         \
         collector can see their current version; this also flushes the SSB,   \
         which is needed. */                                                   \
printf("OK-Z 100\n");\
      JITTER_GC_RUNTIME_TO_HEAPLET (_jitter_gc_collect_heaplet_p,              \
                                    (_jitter_gc_allocation_pointer),           \
                                    (_jitter_gc_allocation_limit));            \
printf("OK-Z 200\n");\
      /* Collect. */                                                           \
      _jitter_gc_allocate_slow_path_with_runtime_fields_owned_and_ssb_flushed  \
         (_jitter_gc_collect_heaplet_p, 0, _jitter_gc_collect_action);         \
printf("OK-Z 300\n");\
      /* Get new runtime fields from the heaplet, which have now changed. */   \
      JITTER_GC_HEAPLET_TO_RUNTIME (_jitter_gc_collect_heaplet_p,              \
                                    (_jitter_gc_allocation_pointer),           \
                                    (_jitter_gc_allocation_limit));            \
printf("OK-Z 400\n");\
    }                                                                          \
  while (false)

/* Like JITTER_GC_COLLECT, forcing a minor collection. */
#define JITTER_GC_COLLECT_MINOR(heaplet_p,                    \
                                allocation_pointer,           \
                                allocation_limit)             \
  JITTER_GC_COLLECT ((heaplet_p),                             \
                     (allocation_pointer),                    \
                     (allocation_limit),                      \
                     jitter_gc_action_force_minor)

/* Like JITTER_GC_COLLECT, forcing a major collection. */
#define JITTER_GC_COLLECT_MAJOR(heaplet_p,                    \
                                allocation_pointer,           \
                                allocation_limit)             \
  JITTER_GC_COLLECT ((heaplet_p),                             \
                     (allocation_pointer),                    \
                     (allocation_limit),                      \
                     jitter_gc_action_force_major)

/* Like JITTER_GC_COLLECT, forcing a collection -- heuristics will decide
   whether minor or major. */
#define JITTER_GC_COLLECT_EITHER(heaplet_p,                   \
                                 allocation_pointer,          \
                                 allocation_limit)            \
  JITTER_GC_COLLECT ((heaplet_p),                             \
                     (allocation_pointer),                    \
                     (allocation_limit),                      \
                     jitter_gc_action_force_either)

/* Like JITTER_GC_COLLECT, forcing a global collection.  This does
   not handle synchronisation at all, and is not for the user. */
// FIXME: handle synchronisation here, remove, or make private //////////////////////////////
#define JITTER_GC_COLLECT_GLOBAL(heaplet_p,                   \
                                 allocation_pointer,          \
                                 allocation_limit)            \
  JITTER_GC_COLLECT ((heaplet_p),                             \
                     (allocation_pointer),                    \
                     (allocation_limit),                      \
                     jitter_gc_action_force_global)




/* Debugging and logging.
 * ************************************************************************** */

/* Convert enumerate values to a printable string.  These functions all return
   pointers to static data, which the user never needs to release. */

const char *
jitter_gc_shape_finalization_kind_to_string
   (enum jitter_gc_shape_finalization_kind kind);
const char *
jitter_gc_collection_kind_to_string (enum jitter_gc_collection_kind kind);
const char *
jitter_gc_action_to_string (enum jitter_gc_action action);
const char *
jitter_gc_generation_to_string (enum jitter_gc_generation generation);


void
jitter_gc_heaplet_dump (struct jitter_gc_heaplet *a);

/* Print detailed human-readable statistics about the pointed heaplet, including
   timing and memory usage, counting time from the end of the heaplet's
   initialisation to the time of this function's call. */
void
jitter_gc_print_statistics (FILE *f, struct jitter_gc_heaplet *a);




/* Internal functions and macros.
 * ************************************************************************** */

/* The functionality in this section is not meant for the user.
   FIXME: make all of these static unless I really need to use it from other
   compilation units.  Comment what I make static in the implementation file. */

/* If logging is enabled define jitter_gc_log_i as a convenience wrapper around
   fprintf (variadic arguments, starting from the format string) using
   JITTER_GC_LOG_STREAM as the stream.
   If logging is disabled define jitter_gc_log_i so that a call to it expands
   to a statemant which does nothing.
   Before the format string, print the given number of spaces. */
#if defined (JITTER_GC_LOG)
# include <stdio.h>
# define jitter_gc_log_i(indentation, ...)                       \
  do                                                             \
    {                                                            \
      if (! jitter_gc_log_muted)                                 \
        {                                                        \
          int _jitter_gc_log_indentation = (int) (indentation);  \
          int _jitter_gc_log_i;                                  \
          for (_jitter_gc_log_i = 0;                             \
               _jitter_gc_log_i < _jitter_gc_log_indentation;    \
               _jitter_gc_log_i ++)                              \
            fputc (' ', (JITTER_GC_LOG_STREAM));                 \
          fprintf ((JITTER_GC_LOG_STREAM), __VA_ARGS__);         \
          fflush (JITTER_GC_LOG_STREAM);                         \
        }                                                        \
    }                                                            \
  while (false)
#else
# define jitter_gc_log_i(indentation, ...)  \
  do { /* Nothing. */ } while (false)
#endif // #if defined (JITTER_GC_LOG)

/* A call to jitter_gc_log behaves like a call to jitter_gc_log_i with a first
   argument of zero prepended. */
#define jitter_gc_log(...)          \
  jitter_gc_log_i (0, __VA_ARGS__)


// FIXME: I might want a reentrant interface even for this.
extern bool
jitter_gc_log_muted;


/// FIXME: comment.  This is used in functions of type jitter_gc_object_update_fields_f , so it is visible by the user ////////////
void
jitter_gc_handle_word (struct jitter_gc_heaplet *a,
                       jitter_gc_tagged_object *p)
  __attribute__ ((nonnull (1), nonnull (2)));

// FIXME: comment.  This is visible to the user, because of hooks.  ////////////////////////////////////
void
jitter_gc_handle_root_pointer (struct jitter_gc_heaplet *a,
                               jitter_gc_tagged_object *p)
  __attribute__ ((nonnull (1), nonnull (2)));




/* FIXME.
 * ************************************************************************** */

/* Easy or important features still to come:
   - debug: add a defensive version of JITTER_GC_BOXED_TO_BLOCK, using a hash
   - disable and re-enable GC
   - make the number of steps choosable at initialisation time
   - finzalisation: use always-nonempty lists.
   - weak pointers
   - one-shot freeing of recently allocated objects, intended for backtracking
     applications.
   - floating point tagging: move to a separate Jitter compilation unit,
     conditionalise on compiler capabilities and possibly architecture
   Maybe:
   - stack tracing
   - hybrid fixed-style stack + Cheney tracing

   Minor things:
   - Shall I use the C99 bool type?  I was very surprised to find it
     less efficient than int on x86_64, because of the need for
     normalisation.
     Example, with int:
0000000000402faf <jitter_gc_space_has>:
  402faf:       48 39 37                cmp    %rsi,(%rdi)
  402fb2:       77 14                   ja     402fc8 <jitter_gc_space_has+0x19>
  402fb4:       53                      push   %rbx
  402fb5:       48 89 f3                mov    %rsi,%rbx
  402fb8:       e8 ea ff ff ff          callq  402fa7 <jitter_gc_space_allocation_limit>
  402fbd:       48 39 c3                cmp    %rax,%rbx
  402fc0:       5b                      pop    %rbx
  402fc1:       0f 92 c0                setb   %al
  402fc4:       0f b6 c0                movzbl %al,%eax
  402fc7:       c3                      retq   
  402fc8:       31 c0                   xor    %eax,%eax
  402fca:       c3                      retq   

     Same code, with bool:
0000000000402faf <jitter_gc_space_has>:
  402faf:       48 39 37                cmp    %rsi,(%rdi)
  402fb2:       77 17                   ja     402fcb <jitter_gc_space_has+0x1c>
  402fb4:       53                      push   %rbx
  402fb5:       48 89 f3                mov    %rsi,%rbx
  402fb8:       e8 ea ff ff ff          callq  402fa7 <jitter_gc_space_allocation_limit>
  402fbd:       48 39 d8                cmp    %rbx,%rax
  402fc0:       5b                      pop    %rbx
  402fc1:       0f 97 c0                seta   %al
  402fc4:       0f b6 c0                movzbl %al,%eax
  402fc7:       83 e0 01                and    $0x1,%eax
  402fca:       c3                      retq   
  402fcb:       31 c0                   xor    %eax,%eax
  402fcd:       83 e0 01                and    $0x1,%eax
  402fd0:       c3                      retq   
   The function returns an && expression.
   I may want to define jitter_bool, for the aesthetics, as int.
 */

#endif // #ifndef JITTER_GC_H_
