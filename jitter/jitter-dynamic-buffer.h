/* Jitter: dynamic buffer data structure header.

   Copyright (C) 2017, 2018, 2020 Luca Saiu
   Updated in 2019 by Luca Saiu
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


#ifndef JITTER_DYNAMIC_BUFFER_H_
#define JITTER_DYNAMIC_BUFFER_H_

#include <stdlib.h>
#include <stdbool.h>
#include "jitter.h"

/* A dynamic buffer is a structure holding a dynamically-allocated contiguous
   region of memory, resized and moved as needed.  The memory region can grow to
   contain user data added and removed in a LIFO fashion.  Any operation
   reserving space (but not operations just releasing space) may move the memory
   area, which would invalidate pointers to addresses within the dynamic memory
   region, but it is safe to point within the dynamic memory region after no
   reservation occurs.  At that point a pointer to the beginning of the dynamic
   memory region can be obtained.

   The buffer can hold homogenous data, working in practice as an array, or it
   can hold elements of varying size, under the control of the user.  The size
   of each element is *not* stored within the data structure, and the user has
   to keep track of such information if needed.

   The dynamically memory region is allocated with malloc , which entails some
   alignment restrictions.  No such constraint applies to individual elements,
   which are allocated as demanded by the user without any added padding. */


/* Dynamic buffer public data structures.
 * ************************************************************************** */

/* The dynamic buffer data structure.  The content should be treated as
   opaque. */
struct jitter_dynamic_buffer
{
  /* The currently allocated space, in chars. */
  size_t allocated_size;

  /* The currently used space, in chars. */
  size_t used_size;

  /* A pointer to the beginning of the dynamically-allocated memory region.  It
     is convenient to keep this as a char pointer, to make pointer arithmetic
     easier. */
  char *region;
};





/* Initialization and finalization.
 * ************************************************************************** */

/* Initialize the pointed structure to a dynamic buffer containing zero used
   bytes.  Use a the given value for the initial allocated size. */
void
jitter_dynamic_buffer_initialize_with_allocated_size
   (struct jitter_dynamic_buffer *da, size_t initial_allocated_size)
  __attribute__ ((nonnull (1)));

/* Initialize the pointed structure to a dynamic buffer containing zero used
   bytes.  Use a reasonable default value for the initial allocated size. */
void
jitter_dynamic_buffer_initialize (struct jitter_dynamic_buffer *db)
  __attribute__ ((nonnull (1)));

/* Finalize the pointed structure releasing its dynamically-allocated memory,
   without freeing the structure itself. */
void
jitter_dynamic_buffer_finalize (struct jitter_dynamic_buffer *db)
  __attribute__ ((nonnull (1)));

/* Functions to also allocate and deallocate are probably not needed.  If they
   are at some point add them here and call them jitter_dynamically_array_make
   and jitter_dynamically_array_destroy . */




/* Reserving or releasing memory.
 * ************************************************************************** */

/* Reserve memory of an element of the given size within the dynamic region of
   the pointed dynamic array, without filling the space.  Return a pointer to
   the beginning of the reserved part, which will remain valid as long as no
   memory is reserved for the same dynamic buffer. */
void *
jitter_dynamic_buffer_reserve (struct jitter_dynamic_buffer *db,
                               size_t chars_to_reserve)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Reserve memory in the pointed dynamic buffer, initializing the space with a
   copy of the given element; return a pointer to the beginning of the copy in
   the dynamic region.  This is equivalent to a call to
   jitter_dynamic_buffer_reserve followed by a call to memcpy filling the
   reserved space. */
void *
jitter_dynamic_buffer_push (struct jitter_dynamic_buffer *db,
                            const void *new_element,
                            size_t new_element_size_in_chars)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Like jitter_dynamic_buffer_push , but return a pointer to constant memory. */
const void *
jitter_dynamic_buffer_push_const (struct jitter_dynamic_buffer *db,
                                  const void *new_element,
                                  size_t new_element_size_in_chars)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Release the given amount of memory from the end of the dynamic region of the
   pointed dynamic buffer, without overwriting the freed space.  Return a
   pointer to the beginning of the freed area. */
void*
jitter_dynamic_buffer_pop (struct jitter_dynamic_buffer *db,
                           size_t chars_to_release)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Like jitter_dynamic_buffer_pop, but return a pointer to constant memory. */
const void*
jitter_dynamic_buffer_pop_const (struct jitter_dynamic_buffer *db,
                                 size_t chars_to_release)
  __attribute__ ((returns_nonnull, nonnull (1)));

// FIXME: rationalise name.  This does not return a pointer to char !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/* Return a pointer to the first unused byte in the dynamic memory region. */
void*
jitter_dynamic_buffer_first_unused_char (const struct jitter_dynamic_buffer *db)
  __attribute__ ((returns_nonnull, nonnull (1), pure));

// FIXME: rationalise name.  This does not return a pointer to char !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
/* Like jitter_dynamic_first_unused_char , but return a pointer to constant
   memory. */
const void*
jitter_dynamic_buffer_first_unused_char_const
   (const struct jitter_dynamic_buffer *db)
  __attribute__ ((returns_nonnull, nonnull (1), pure));

/* Return the current size of the used part of the dynamic region, in chars.
   The allocated size might be larger. */
size_t
jitter_dynamic_buffer_size (const struct jitter_dynamic_buffer *db)
  __attribute__ ((nonnull (1), pure));

/* Release part of the allocated but not currently used memory for the pointed
   dynamic buffer, only leaving wiggle_byte_no bytes allocated but not used
   after the currently used data.  Do nothing if the buffer already has less
   than wiggle_byte_no bytes available.
   This does not change the stored data in the used part.
   Rationale: This function is intended to be called periodically after the
   dynamic buffer has been pushed and popped to many times, which may have left
   the allocated memory much larger than needed because of some temporary peak
   of usage.  The wiggle arguments serves to avoid another resize in a very
   short time, when the buffer is expected to grow again. */
void
jitter_dynamic_buffer_compact (struct jitter_dynamic_buffer *db,
                               size_t wiggle_byte_no)
  __attribute__ ((nonnull (1)));




/* Conversion to an ordinary pointer.
 * ************************************************************************** */

/* Return the address of the beginning of the dynamically-allocated region.  The
   address will remain valid as long as dynamic buffer space is not allocated or
   relased, and as long as the dynamic buffer is not finalized. */
void*
jitter_dynamic_buffer_to_pointer (const struct jitter_dynamic_buffer *db)
  __attribute__ ((returns_nonnull, nonnull (1), pure));

/* Like jitter_dynamic_buffer_to_pointer , but return a pointer to constant
   memory. */
const void*
jitter_dynamic_buffer_to_const_pointer (const struct jitter_dynamic_buffer *db)
  __attribute__ ((returns_nonnull, nonnull (1), pure));




/* Extraction.
 * ************************************************************************** */

/* "Extracting" data from a dynamic buffer means returning the malloc-allocated
   buffer contained in the dynamic buffer struct, thus making it invalid.

   The finalization API recognizes these invalid buffers, and avoids freeing the
   extracted data which is (supposedly) held elsewhere, and needs to be finalized
   by the user with free . */

/* Return the malloc-allocated data held in the pointed dynamic buffer, which is
   then no longer usable.  It is the caller's responsibility to free the data.
   Notice that the data will have size db->allocated_size , which may be larger
   than db->used_size . */
void *
jitter_dynamic_buffer_extract (struct jitter_dynamic_buffer *db)
  __attribute__ ((nonnull (1), returns_nonnull));

/* Like jitter_dynamic_buffer_extract , but trim the returned buffer to
   db->used_size using realloc. */
void *
jitter_dynamic_buffer_extract_trimmed (struct jitter_dynamic_buffer *db)
  __attribute__ ((nonnull (1), returns_nonnull));




/* Macro API.
 * ************************************************************************** */

/* Some of the functality above is critical for efficiency in some conditions,
   for example where a dynamic buffer is used as a stack in a tight loop.
   This is a implementation of some the same functionality provided above,
   where at least the common fast path will be inlined. */

/* Just like jitter_dynamic_buffer_to_const_pointer .  Expand to an expression,
   possibly evaluating its argument multiple time.  */
#define JITTER_DYNAMIC_BUFFER_TO_CONST_POINTER(jitter_dynamic_buffer_p)  \
  ((const char*)                                                         \
   ((jitter_dynamic_buffer_p)->region))

/* Just like JITTER_DYNAMIC_BUFFER_TO_CONST_POINTER , but expand to a void *
   expression instead of a char * expression to allow for implicit type
   conversion in user code. */
#define JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER(jitter_dynamic_buffer_p)  \
  ((const void*)                                                              \
   (JITTER_DYNAMIC_BUFFER_TO_CONST_POINTER (jitter_dynamic_buffer_p)))

/* Just like jitter_dynamic_buffer_first_unused_char_const .  Expand to an
   expression, possibly evaluating its argument multiple time. */
#define JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST(jitter_dynamic_buffer_p)  \
  ((const void*)                                                                \
   ((jitter_dynamic_buffer_p)->region                                           \
    + (jitter_dynamic_buffer_p)->used_size))

/* Expand to an expression evaluating to the used size in bytes of the dynamic
   buffer pointed by the result of the given expression.  
   The argument may be evaluated more than once. */
#define JITTER_DYNAMIC_BUFFER_USED_SIZE(_jitter_dynamic_buffer_p)  \
  ((_jitter_dynamic_buffer_p)->used_size)

/* Expand to a statement changing the used size in bytes of the dynamic buffer
   pointed by the result of the evaluation of _jitter_dynamic_buffer to be the
   result of _jitter_dynamic_buffer_new_used_size_in_bytes .  
   This never performs a reallocation, and therefore is only safe to use to
   shrink a buffer, and not to grow it.
   The arguments may be evaluated more than once. */
#define JITTER_DYNAMIC_BUFFER_REDUCE_USED_SIZE(             \
   _jitter_dynamic_buffer_p,                                \
   _jitter_dynamic_buffer_new_used_size_in_bytes)           \
  do                                                        \
    {                                                       \
      (_jitter_dynamic_buffer_p)->used_size                 \
        = (_jitter_dynamic_buffer_new_used_size_in_bytes);  \
    }                                                       \
  while (false)

/* Expand to a statemant which:
   - evaluates the expression _jitter_dynamic_buffer_p_exp , which must have
   type struct jitter_dynamic_buffer * , and the expression
   _jitter_dynamic_buffer_new_element_value_p , which must have as type a
   pointer to _jitter_dynamic_buffer_new_element_type ;
   - then pushes a copy of the object pointed by the result of
   _jitter_dynamic_buffer_new_element_value_p into the dynamic buffer pointed by
   the result of _jitter_dynamic_buffer_p_exp .
   The expansion involves no function calls in the common fast path for which
   it is optimised. */
#define JITTER_DYNAMIC_BUFFER_PUSH(_jitter_dynamic_buffer_p_exp,                \
                                   _jitter_dynamic_buffer_new_element_type,     \
                                   _jitter_dynamic_buffer_new_element_value_p)  \
  do                                                                            \
    {                                                                           \
      struct jitter_dynamic_buffer *_jitter_dynamic_buffer_p                    \
        = (_jitter_dynamic_buffer_p_exp);                                       \
      size_t _jitter_dynamic_buffer_new_element_size                            \
        = sizeof (_jitter_dynamic_buffer_new_element_type);                     \
      size_t _jitter_dynamic_buffer_p_old_allocated_size                        \
        = _jitter_dynamic_buffer_p->allocated_size;                             \
      size_t _jitter_dynamic_buffer_p_old_used_size                             \
        = _jitter_dynamic_buffer_p->used_size;                                  \
      size_t _jitter_dynamic_buffer_p_new_used_size                             \
        = (_jitter_dynamic_buffer_p_old_used_size                               \
           + _jitter_dynamic_buffer_new_element_size);                          \
      if (__builtin_expect (_jitter_dynamic_buffer_p_new_used_size              \
                            > _jitter_dynamic_buffer_p_old_allocated_size,      \
                            false))                                             \
        jitter_dynamic_buffer_reserve                                           \
           (_jitter_dynamic_buffer_p,                                           \
            _jitter_dynamic_buffer_new_element_size);                           \
      else                                                                      \
        _jitter_dynamic_buffer_p->used_size +=                                  \
          _jitter_dynamic_buffer_new_element_size;                              \
      * ((_jitter_dynamic_buffer_new_element_type *)                            \
         ((char *) _jitter_dynamic_buffer_p->region                             \
          + _jitter_dynamic_buffer_p_old_used_size))                            \
         = * (_jitter_dynamic_buffer_new_element_value_p);                      \
    }                                                                           \
  while (false)

#endif // #ifndef JITTER_DYNAMIC_BUFFER_H_
