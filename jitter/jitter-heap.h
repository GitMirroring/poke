/* Jitter: memory heap data structure header.

   Copyright (C) 2018, 2019, 2021 Luca Saiu
   Updated in 2020 by Luca Saiu
   Written by Luca Saiu

   This file is part of Jitter.

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


#ifndef JITTER_HEAP_H_
#define JITTER_HEAP_H_

#include <jitter/jitter-config.h> /* For feature macros. */

#if defined (JITTER_HAVE_ALIGNAS)
# include <stdalign.h>  /* For alignas. */
#endif // defined (JITTER_HAVE_ALIGNAS)
#include <stddef.h>    /* For offsetof. */

#include <jitter/jitter.h>
#include <jitter/jitter-bitwise.h>
#include <jitter/jitter-list.h>




/* This is a memory handling facility.
 * ************************************************************************** */

/* This is a heap in the sense of a data structure for implementing dynamic
   memory allocation and freeing of buffers from larger blocks.  A heap in
   this sense is sometimes called a "memory pool".

   This has nothing to do with the tree data structure imposing ordering
   constraints, also called "heap". */

// FIXME: a few words about policies (first-fit in a LIFO-ordered non-segregated
// free list [hole list here], immediate coalescing), single blocks and heaps.
// Wilson's survey citing his own work [p. 68, left column seems to strongly
// favor FIFO over LIFO.  Wilson's arguments seem very reasonable; I am only
// worried about stronger cache effects on current machines, 25 years after
// his publication.
// I can experiment quite easily with FIFO, or conditionalize.]




/* Heap blocks and things.
 * ************************************************************************** */

/* A "block" is a memory interval containing a header, plus space for the
   objects to be allocated.  Every object in a block lives within its memory
   space, without relying on external memory allocation facilities.  The same
   block memory also contains every bookkeeping data structure, again without
   relying on external memory. */

/* An alignment where it is safe for both header structures and payloads to
   begin.
   This *must* be a power of two. */
#define JITTER_HEAP_ALIGNMENT  \
  8 // Eight bytes should be enough for every architecture.

/* Each heap block contains a header and then a sequence of "things", each
   aligned to JITTER_HEAP_ALIGNMENT bytes and all living within the block
   memory.
   Each thing can be:
   - a terminator (a special marker at the beginning and end of each block);
   - a hole (currently unused memory);
   - an object (currently used memory).
   Holes within a single block are linked together in a doubly-linked list,
   always preceded by the left terminator and followed by the right
   terminator.
   Currently, the last freed block is the first in the list (after the
   left terminator).
   Every thing, independently from its tag, contains a payload_size field
   allowing to navigate things within a block left-to-right, ad a
   thing_on_the_left (tagged) pointer, allowing to navigate right-to-left. */

/* A thing header and the beginning of its payload.  Every thing pointer is
   aliged to JITTER_HEAP_ALIGNMENT bytes.  Since every thing header stores a
   pointer to the previous thing (in address order) within the same block and
   its payload size, it is always possible to navigate from one thing to its
   neighbors in both directions; this is useful for hole coalescing.
   This struct is used for things allocated on blocks, and for big objects as
   well. */
struct jitter_heap_thing
{
  /* The first field should have pointer type, so that we can guarantee that if
     the structure is aligned correctly, then no 64-bit pointer will be accessed
     unaligned.  The following fields should get correctly aligned for free. */

  /* A tagged pointer, whose two least significant bits hold a value of type
     enum jitter_heap_thing_tag.  The tag encodes the type of the current thing,
     and not of the thing on the left.
     The pointer itself is aligned to at least JITTER_HEAP_ALIGNMENT bytes, and
     its untagged value can be extracted by masking off the two least
     significant bits or by subtracting a known tag; no shifting is required.
     After untagging this pointer points to the header for the thing immediately
     to the left of the current thing, or is NULL for a left terminator.
     This is NULL, apart from the tag, for big objects. */
  struct jitter_heap_thing *thing_on_the_left;

  /* The payload size, including the anonymous union below and possibly
     extending further; the byte at (payload + payload_size_in_bytes) is right
     past the allocated size of this thing and marks the beginning of the next
     thing on the same block, if any. */
  size_t payload_size_in_bytes;

  /* Which of the following field is used depends on the thing tag. */
  alignas (JITTER_HEAP_ALIGNMENT)
  union
  {
    /* Doubly-linked-list pointers to the previous and next hole in the block.
       This is only used for holes. */
    struct jitter_list_links hole_links;

    /* The beginning of the payload, which can actually extend further than the
       size declared here. */
    char payload [sizeof (struct jitter_list_links)];

    /* /\* This unused field should force the anonymous union to be aligned in a */
    /*    conservative way, so that we can store any reasonable datum as an object */
    /*    payload. *\/ */
    /* double unused; */
  };
};

/* The number of bytes occupied by the initial header in a block, plus padding.
   In other words, this is the offset from the header beginning to the payload
   beginning in each object. */
#define JITTER_HEAP_HEADER_OVERHEAD               \
  (offsetof (struct jitter_heap_thing, payload))  \

/* The size for the smallest possible thing.  The "size of a thing" always
   includes the header as well, and not only the payload. */
#define JITTER_HEAP_MINIMUM_THING_SIZE  \
  (sizeof (struct jitter_heap_thing))

/* The minimum payload (or links) size for a thing in bytes, independently from
   its tag. */
#define JITTER_HEAP_MINIMUM_PAYLOAD_SIZE  \
  (sizeof (struct jitter_heap_thing)      \
   - JITTER_HEAP_HEADER_OVERHEAD)

/* Given an expression evaluating to a pointer, expand to an expression
   evaluting to the same pointer or the next possible smaller value respecting
   JITTER_HEAP_ALIGNMENT. */
#define JITTER_HEAP_ALIGN_LEFT(p)                                    \
  ((void *)                                                          \
   JITTER_PREVIOUS_MULTIPLE_OF_POWER_OF_TWO((jitter_uint) (p),       \
                                            JITTER_HEAP_ALIGNMENT))

/* Like JITTER_HEAP_ALIGN_LEFT, but evaluate to the same pointer or the next
   possible *larger* value. */
#define JITTER_HEAP_ALIGN_RIGHT(p)                               \
  ((void *)                                                      \
   JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO((jitter_uint) (p),       \
                                        JITTER_HEAP_ALIGNMENT))

/* Every used configuration of the tag associated to the pointer field in object
   headers.  The tag is JITTER_HEAP_THING_TAG_BIT_NO bits wide. */
enum jitter_heap_thing_tag
  {
    /* The header is for a hole. */
    jitter_heap_thing_tag_hole =       0,

    /* The header is for an object. */
    jitter_heap_thing_tag_object =     1,

    /* The header is for a block terminator, without distinction between left and
       right. */
    jitter_heap_thing_tag_terminator = 2,

    /* The header for a big object.  Big objects don't live in blocks, but are
       allocated each in its own buffer, controlled by heaps.  See below. */
    jitter_heap_thing_tag_big        = 3
  };

/* How many bits are reserved for a tag within a tagged pointer.  Right now,
   with objects aligned to 8 bytes on every architectures, I could already
   afford 3 tag bits for free, even if 2 suffice.  By simply enforcing a wider
   alignment I could make even more tag bits available. */
#define JITTER_HEAP_THING_TAG_BIT_NO  \
  2

/* A bitmask having 1 bits where the tag is in a pointer, 0 elsewhere. */
#define JITTER_HEAP_THING_TAG_BIT_MASK                       \
  ((jitter_uint) ((1 << JITTER_HEAP_THING_TAG_BIT_NO) - 1))

/* A bitmask having 0 bits where the tag is in a pointer, 1 elsewhere. */
#define JITTER_HEAP_THING_NON_TAG_BIT_MASK  \
  (~ JITTER_HEAP_THING_TAG_BIT_MASK)

/* Given an expression evaluating to an untagged pointer, expand to an
   expression evaluating to the same pointer with the given tag.  The expansion
   may evaluate p multiple times. */
#define JITTER_HEAP_TAG_POINTER(p, tag)            \
  ((struct jitter_heap_thing *)                    \
   (((jitter_uint) (p)) | ((jitter_uint) (tag))))

/* Given an expression evaluating to a tagged pointer and an expression
   evaluating to its tag, assumed to be the same, expand to an expression
   evaluating to the same untagged pointer.  The expansion may evaluate the
   macro arguments multiple times.
   This is the preferred way of untagging a pointer: often the untagging can be
   done at zero costs, as GCC can generate a load instruction with a constant
   offset to account for the quantity to subtract. */
#define JITTER_HEAP_UNTAG_POINTER_KNOWN_TAG(p, tag)  \
  ((struct jitter_heap_thing *)                      \
   (((jitter_uint) (p)) - ((jitter_uint) (tag))))

/* Given an expression evaluating to a tagged pointer, an expression
   evaluating to its current tag assumed to be the the given one, and a third
   expression evaluating to a new tag, expand to an expression evaluating to
   the same pointer tagged with the new tag instead of the current one.
   The expansion may evaluate the macro arguments multiple times. */
#define JITTER_HEAP_RETAG_POINTER(p, old_tag, new_tag)  \
  ((struct jitter_heap_thing *)                         \
   (((jitter_uint) (p))                                 \
    - ((jitter_uint) (old_tag))                         \
    + ((jitter_uint) (new_tag))))

/* Given an expression evaluating to a tagged pointer expand to an expression
   evaluating to the same untagged pointer.  The expansion may evaluate the
   macro arguments multiple times.
   This is usually less efficient than JITTER_HEAP_UNTAG_POINTER_KNOWN_TAG. */
#define JITTER_HEAP_UNTAG_POINTER(p)                            \
  ((struct jitter_heap_thing *)                                 \
   (((jitter_uint) (p)) & JITTER_HEAP_THING_NON_TAG_BIT_MASK))

/* Given an expression evaluating to a tagged pointer expand to an expression
   evaluating to its tag.  The expansion may evaluate the macro argument
   multiple times. */
#define JITTER_HEAP_POINTER_TAG(p)                          \
  ((enum jitter_heap_thing_tag)                             \
   (((jitter_uint) (p)) & JITTER_HEAP_THING_TAG_BIT_MASK))

/* Expand to an expression evaluating to the tag of the thing pointed to by the
   result of the given expression, which should evaluate to an object of type
   struct jitter_heap_thing * . */
#define JITTER_HEAP_GET_TAG(thing)                        \
  (JITTER_HEAP_POINTER_TAG ((thing)->thing_on_the_left))

/* Expand to an expression evaluating to the payload of the result of the given
   expression, which should evaluate to an object of type
   struct jitter_heap_thing * .
   The argument may be evaluated multiple times. */
#define JITTER_HEAP_THING_TO_PAYLOAD(thing)  \
  ((thing)->payload)

/* Expand to an expression evaluating to a the pointer of the thing whose
   initial payload pointer is the result of the given expression.
   The argument may be evaluated multiple times. */
#define JITTER_HEAP_PAYLOAD_TO_THING(p)             \
  ((struct jitter_heap_thing *)                     \
   (((char *) (p)) - JITTER_HEAP_HEADER_OVERHEAD))

/* Expand to an expression evaluating to a pointer to the thing on the left of
   the thing pointed by the result of the given expression, which should
   evaluate to an object of type struct jitter_heap_thing * .
   The argument may be evaluated multiple times. */
#define JITTER_HEAP_THING_ON_THE_LEFT_OF(thing)            \
  (JITTER_HEAP_UNTAG_POINTER((thing)->thing_on_the_left))

/* Expand to an expression evaluating to a pointer to the thing on the right of
   the thing pointed by the result of the given expression, which should
   evaluate to an object of type struct jitter_heap_thing * .
   The argument may be evaluated multiple times. */
#define JITTER_HEAP_THING_ON_THE_RIGHT_OF(thing)     \
  ((struct jitter_heap_thing *)                      \
   (((char *) JITTER_HEAP_THING_TO_PAYLOAD (thing))  \
    + (thing)->payload_size_in_bytes))

/* A "block" is a data structure written at or near the beginning of some
   allocated memory, followed by things living in the same memory space. */
struct jitter_heap_block
{
  /* The first field should have pointer type; see the comment above. */

  /* A pointer to the allocated memory for the block, which is not necessarily
     the same as a pointer to this structure, as there may be alignment
     constraints.  The memory to be freed when the block is released is the one
     referred by this pointer. */
  void *allocated_space;

  /* A doubly linked list containing:
     - the left terminator;
     - all the holes in this block, in an unspecified order;
     - the right terminator.

     The left and right terminators never change after block initialization, and
     therefore the list is "always-nonempty" in the sense of jitter-list.h .
     This assumption makes list update operations much more efficient. */
  struct jitter_list_header hole_list;

  /* How many bytes were allocated, including any bytes wasted by misalignment. */
  size_t allocated_space_size_in_bytes;

  /* Links to the previous and next block within a heap. */
  struct jitter_list_links block_links;

  /* Having the left terminator directly accessible as a field is convenient to
     walk thru the hole list, without dereferencing hole_list->first and then skipping
     the first element every time.
     The right terminator cannot be stored in the same way, as its offset from
     the beginning of the block header depends on the  block size. */
  alignas (JITTER_HEAP_ALIGNMENT)
  struct jitter_heap_thing left_terminator;

  /* There must be no other field after the left terminator: other things get
     allocated in the memory buffer holding this data structure right after the
     left terminator.  */
};




/* Object allocation, deallocation and reallocation from a given block.
 * ************************************************************************** */

/* The functions in this section work on object payload pointers, as seen by the
   user; internal object (and hole) headers are invisible. */

/* Given a heap block, return a pointer to a freshly allocated object within the
   block having the given payload size (or higher, to satisfy alignment
   constraints).  Return NULL if there is no single hole large enough to satisy
   the request.
   The returned pointer is aligned to JITTER_HEAP_ALIGNMENT bytes. */
void *
jitter_heap_allocate_from_block (struct jitter_heap_block *b,
                                 size_t size_in_bytes)
  __attribute__ ((__nonnull__ (1), __malloc__,
                  __alloc_size__ (2),
                  __assume_aligned__ (JITTER_HEAP_ALIGNMENT)));

/* Given a heap block and a pointer to an existing object within it, return a
   pointer to a new object of the given new size having the same content of the
   pointed existing object up to the given new size, and free the existing
   object.  Return NULL if there is no single hole large enough to satisy the
   request.
   The returned pointer may be the same as old_payload or the new object may
   otherwise reuse the same space occupied by the old one, but in the general
   case it is invalid to keep using old_payload after this call returns a
   non-NULL value.
   The returned pointer is aligned to JITTER_HEAP_ALIGNMENT bytes. */
void *
jitter_heap_reallocate_from_block (struct jitter_heap_block *b,
                                   void *old_payload,
                                   size_t new_size_in_bytes)
  __attribute__ ((__nonnull__ (1, 2),
                  __alloc_size__ (3),
                  __assume_aligned__ (JITTER_HEAP_ALIGNMENT),
                  __warn_unused_result__));

/* Given a heap block and an initial pointer to the payload of an object
   allocated on the block, free the object, compacting any holes around it. */
void
jitter_heap_free_from_block (struct jitter_heap_block *p,
                             void *object_on_p)
  __attribute__ ((__nonnull__ (1, 2)));




/* Big objects.
 * ************************************************************************** */

/* Big objects do not live on heap blocks: they are allocated individually, on
   demand, with the primitives supplies by the user in a heap header and
   released immediately as soon as the user frees.  This, when the primitive
   relies on mmap, will let the memory be immediately returned to the operating
   system. */

/* Every big object has a header just like heap things living in blocks, for
   format compatibility.  In order to be able to distinguish big and non-big
   objects at run time big objects have a distinct tag (see the definition of
   enum jitter_heap_thing_tag above).

   Every big object in a heap belongs to a doubly linked list, distinct from the
   list of objects in a block and from the list of blocks.  Since I don't want
   to add two fields in the header of *every* object, wasting space on non-big
   objects as well, big objects have a "big pre-header", which is allocated in
   memory just before the ordinary object header, at a fixed negative offset. */

/* The pre-header for big objects, also containing the object header, structured
   like the header of non-big objects, which in its turn includes the beginning
   of the object payload.
   The pre-header is aligned to JITTER_HEAP_ALIGNMENT bytes, and so are its
   internal header and the payload within the header. */
struct jitter_heap_big
{
  /* Doubly-linked list pointers to the previous and next big objects in
     the same heap. */
  struct jitter_list_links big_links;

  /* The ordinary thing header, containing a jitter_heap_thing_tag_big tag
     and NULL as the thing_on_the_left pointer. */
  alignas (JITTER_HEAP_ALIGNMENT)
  struct jitter_heap_thing thing;

  /* There must be nothing else after the header: the header actually includes
     the beginning of the payload, which must not be interrupted by other fields
     before the rest of the payload. */
};

/* The thing_on_the_left field value for every big object, which is to say a
   NULL pointer tagged with the jitter_heap_thing_tag_big tag. */
#define JITTER_HEAP_BIG_THING_ON_THE_LEFT        \
  ((struct jitter_heap_thing *)                  \
   (((jitter_uint) NULL)                         \
    | (jitter_uint) jitter_heap_thing_tag_big))

/* Given an expression evaluating to an initial pointer to an object payload,
   expand to an expression evaluating to non-false iff the object is big.
   Implementation note: this is faster than using JITTER_HEAP_GET_TAG: I can
   avoid the masking operation by relying on the fact that the thing_on_the_left
   pointer is always (tagged) NULL for big objects. */
#define JITTER_HEAP_IS_PAYLOAD_BIG(payload)                    \
  ((JITTER_HEAP_PAYLOAD_TO_THING(payload)->thing_on_the_left)  \
   == JITTER_HEAP_BIG_THING_ON_THE_LEFT)

/* The overhead of a big pre-header in bytes, or in other words the offset from
   the beginning of the big pre-haeder to the beginning of the thing header.
   The big pre-header overhead doesn't include the thing header size. */
#define JITTER_HEAP_BIG_PRE_HEADER_OVERHEAD   \
  (offsetof (struct jitter_heap_big, thing))

/* The total overhead of big pre-header plus thing header for a big object, or
   in other words the offset from the beginning of the big pre-haeder to the
   beginning of the payload within the thing within the big pre-header. */
#define JITTER_HEAP_BIG_TOTAL_OVERHEAD  \
  (JITTER_HEAP_BIG_PRE_HEADER_OVERHEAD  \
   + JITTER_HEAP_HEADER_OVERHEAD)

/* Given an expression evaluating to an initial pointer to an big object
   payload, expand to an expression evaluating to a pointer to the object
   big pre-header. */
#define JITTER_HEAP_PAYLOAD_TO_BIG(payload)                  \
  ((struct jitter_heap_big *)                                \
   (((char *) (payload)) - JITTER_HEAP_BIG_TOTAL_OVERHEAD))

/* Given an expression evaluating to an initial pointer to an big object
   payload, expand to an expression evaluating to a pointer to the object
   big pre-header. */
#define JITTER_HEAP_BIG_TO_PAYLOAD(bigp)                           \
  ((void *) (((char *) (bigp)) + JITTER_HEAP_BIG_TOTAL_OVERHEAD))

/* A forward-declaration. */
struct jitter_heap;

/* Allocate a fresh big object of the given user payload size from the pointed
   heap.  Return an initial pointer to its payload.
   This function is slightly more efficient than general heap allocation, and
   can be used when the user is sure that the required object will be big. */
void *
jitter_heap_allocate_big (struct jitter_heap *h,
                          size_t user_payload_size_in_bytes)
  __attribute__ ((__nonnull__ (1), __returns_nonnull__));

/* Given a heap pointer and an initial pointer to the payload of a big object
   belonging to it, free the big object.  This doesn't check that the payload
   actually belongs to a suitable object, but should be slightly faster than
   the general freeing function. */
void
jitter_heap_free_big (struct jitter_heap *h, void *big_payload)
  __attribute__ ((__nonnull__ (1)));

/* Reallocation is not supported for big objects.  For the time being I will
   assume that it's a non-critical operation in this case. */




/* Heaps.
 * ************************************************************************** */

/* A "heap" as intended here is a data structure holding an arbitrary number of
   blocks, each block holding objects, along with information about how to make
   and destroy blocks.
   Every block in the heap will have the same length, and will be aligned to
   this length.  This makes it possible, given any object address within the
   block, to find the block with a quick bitwise masking operation.

   A heap is a convenient abstraction to allocate, deallocate and reallocate
   objects from blocks which are automatically made and destroyed as needed; if
   an allocated object is too big to fit in a blog, heap allocation and
   reallocation functions will automatically make a big object instead.

   Of course operations within an existing block are assumed to be more
   efficient than operations altering the number of blocks, which usually
   require syscalls.  Heap operations will attempt to reuse existing blocks as
   far as possible. */

/* Here come some types for functions to be supplied by the user, defining a
   block "kind". */

/* A function allocating fresh memory for a block, taking a size in bytes and
   returning a fresh block of at least the required size, or NULL on allocation
   failure. */
typedef void * (* jitter_heap_primitive_allocate_function)
   (size_t size_in_bytes);

/* A function destroying an existing block, taking the block as returned by the
   appropriate jitter_heap_primitive_allocate_function function and the
   allocated size as it was passed at making time. */
typedef void (* jitter_heap_primitive_free_function)
   (void *allocated_memory, size_t size_in_bytes);

/* A descriptor for heap objects, specifying its allocation primitives.  The
   same descriptor might be used for multiple heaps, but it's relatively
   inconvenient for the user to specify it; therefore structures of this type
   are automatically filled by jitter_heap_initialize , and are effectively
   invisible to the user. */
struct jitter_heap_descriptor
{
  /* A function allocating a fresh block or big object. */
  jitter_heap_primitive_allocate_function make;

  /* A function destroying an entire existing block or big object. */
  jitter_heap_primitive_free_function destroy;

  /* The "natural" alignment in bytes which is always and automatically
     guaranteed by the make allocation primitive, pointed by the field above. */
  size_t make_natural_alignment_in_bytes;

  /* A function deallocating only a *part* for a block allocated by the make
     primitive pointed above.  This can be NULL if no suitable primitive for
     unmapping part of a buffer exists; otherwise the API will be just like an
     munmap with no result. */
  jitter_heap_primitive_free_function unmap_part_or_NULL;

  /* The desired block size in bytes.  This constraint is, in general, respected
     by allocating larger blocks using the make primitive pointed above, and if
     possible unmapping a part of the buffer. */
  size_t block_size_and_alignment_in_bytes;

  /* The block bit mask, having 1 bits for the bits identifying a block and
     0 bits for the bits which are part of an offset within the block. */
  jitter_uint block_bit_mask;

  /* The size in bytes of the smallest payload large enough to belong to a big
     object. */
  size_t block_size_smallest_big_payload_in_bytes;
};

/* A data structure encoding a heap.  The user will initialize a structure
   of this type and use it for allocating and freeing. */
struct jitter_heap
{
  /* A descriptor for this heap.  This small struct is copied rather than
     pointed, to avoid an indirection on time-critical heap operations on
     objects. */
  struct jitter_heap_descriptor descriptor;

  /* The list of all the blocks in this current heap.  This is never empty, but
     the list cannot be considered "always-nonempty" as per jitter-list.h ,
     since this doesn't use terminator elements and the first and last elements
     of the list can change. */
  // FIXME: possibly make this always-nonempty, by adding two dummy (unaligned)
  // blocks as elements within this struct.  In this case the terminators don't
  // need to be in a specific order by address, differently from thing
  // terminators within a block.  Is this critical enough?  Maybe not.
  struct jitter_list_header block_list;

  /* The list of all the big objects in this heap. */
  // FIXME: possibly make this always-nonempty as well.
  struct jitter_list_header big_list;

  /* A pointer to the current block, from which we are allocating by default.
     This is never NULL, and is always equal to the first element of the list;
     however we save indirections by keeping a pointer here as an
     optimization. */
  struct jitter_heap_block *default_block;
};

/* Initialize the pointed heap to use the pointed descriptor elements.  A
   suitable descriptor, stored in the heap, is initialized automatically and
   completed by computing values for the remaining fields from the given
   values.
   The given value for block_size_and_alignment_in_bytes here is a lower bound:
   the actual block size will be increased as needed in order for it to be
   a power of two, and a multiple of make_natural_alignment_in_bytes . */
void
jitter_heap_initialize (struct jitter_heap *h,
                        jitter_heap_primitive_allocate_function make,
                        jitter_heap_primitive_free_function destroy,
                        size_t make_natural_alignment_in_bytes,
                        jitter_heap_primitive_free_function unmap_part_or_NULL,
                        size_t block_size_and_alignment_in_bytes)
  __attribute__ ((__nonnull__ (1, 2, 3)));

/* Finalize the pointed heap, destroying every block it contains. */
void
jitter_heap_finalize (struct jitter_heap *h)
  __attribute__ ((__nonnull__ (1)));




/* Object allocation, deallocation and reallocation from a given heap.
 * ************************************************************************** */

/* These function are similar to their _from_block counterparts, but take a heap
   pointer instead of a block pointer.

   These are the main user functions for working with allocation, reallocation
   and freeing of objects.  The returned payloads may be big objects: the user
   sees no difference.

   Allocation and reallocation never return NULL: failure is fatal. */

/* Given a heap, return a pointer to a freshly allocated object within the
   heap having at least the given payload size (or higher, to satisfy alignment
   constraints).  Fail fatally if allocation is not possible.
   The returned pointer is aligned to JITTER_HEAP_ALIGNMENT bytes. */
void *
jitter_heap_allocate (struct jitter_heap *h, size_t size_in_bytes)
  __attribute__ ((__nonnull__ (1), __malloc__, __returns_nonnull__,
                  __alloc_size__ (2),
                  __assume_aligned__ (JITTER_HEAP_ALIGNMENT)));

/* Given a heap pointer and a pointer to an existing object within it, return a
   pointer to a new object of the given new size having the same content of the
   pointed existing object up to the given new size, and free the existing
   object.  Fail fatally if there is not enough space and creating a new block
   fails.  The returned pointer may be the same as old_payload or the new object
   may otherwise reuse the same space occupied by the old one, but in the
   general case it is invalid to keep using old_payload after this call returns.
   The returned pointer is aligned to JITTER_HEAP_ALIGNMENT bytes. */
void *
jitter_heap_reallocate (struct jitter_heap *h, void *old_payload,
                        size_t new_size_in_bytes)
  __attribute__ ((__nonnull__ (1, 2), __returns_nonnull__, __alloc_size__ (3),
                  __assume_aligned__ (JITTER_HEAP_ALIGNMENT),
                  __warn_unused_result__));

/* Given a heap pointer and a pointer to an existing object within it, free
   space after the object by shrinking it to the given new size in bytes, which
   must not be less than the current object allocated size, rounded up as
   required by the implementation.  If freeing up space is not possible do
   nothing, without failing. */
void
jitter_heap_shrink_in_place (struct jitter_heap *h, void *payload,
                             size_t new_size_in_bytes)
  __attribute__ ((__nonnull__ (1, 2)));

/* Given a heap and an initial pointer to the payload of an object from the
   heap, free the object. */
void
jitter_heap_free (struct jitter_heap *h, void *object_payload)
  __attribute__ ((__nonnull__ (1, 2)));

#endif // #ifndef JITTER_HEAP_H_
