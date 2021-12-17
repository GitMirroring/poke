/* Jitter: dynamic buffer data structure.

   Copyright (C) 2017, 2018, 2019, 2020 Luca Saiu
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


#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "jitter.h"

#include "jitter-malloc.h"
#include "jitter-dynamic-buffer.h"


/* Initial memory size.
 * ************************************************************************** */

/* Buffer sizes are all stored in chars, to make pointer arithmetic easier. */
#define INITIAL_ALLOCATED_SIZE 256




/* Initialization and finalization.
 * ************************************************************************** */

void
jitter_dynamic_buffer_initialize_with_allocated_size
   (struct jitter_dynamic_buffer *da, size_t initial_allocated_size)
{
  /* Allocate a region with the given initial size, keeping track of how much
     memory is available and how much is already used. */
  da->allocated_size = initial_allocated_size;
  da->used_size = 0;
  da->region = jitter_xmalloc (initial_allocated_size);
}

void
jitter_dynamic_buffer_initialize (struct jitter_dynamic_buffer *da)
{
  jitter_dynamic_buffer_initialize_with_allocated_size
    (da, INITIAL_ALLOCATED_SIZE);
}

void
jitter_dynamic_buffer_finalize (struct jitter_dynamic_buffer *da)
{
  /* Do nothing is the region has been extracted. */
  if (da->region == NULL)
    return;

  /* Free the malloc-allocated part. */
  free (da->region);

  /* For defensiveness, fill the struct with invalid data. */
  memset (da, 0xff, sizeof (struct jitter_dynamic_buffer));
}




/* Reserving or releasing memory.
 * ************************************************************************** */

void *
jitter_dynamic_buffer_reserve (struct jitter_dynamic_buffer *db,
                               size_t chars_to_reserve)
{
  /* Remember the offset of the reserved space from the region beginning. */
  size_t offset = db->used_size;

  /* Reserve space, which possibly entails reallocating the region.  This
     potentially changes the region pointer, which is why we can compute the
     result only after this.
     Doubling *and adding one* is reasonable, as it lets us start with a zero
     size as well, without needing any more conditionals. */
  db->used_size += chars_to_reserve;
  if (db->used_size > db->allocated_size)
    db->region = jitter_xrealloc (db->region,
                                  db->allocated_size = db->used_size * 2 + 1);

  /* Now we know where the reserved space begins: it comes offset chars after
     the region, as it is now. */
  return db->region + offset;
}

void *
jitter_dynamic_buffer_push (struct jitter_dynamic_buffer *db,
                            const void *new_element,
                            size_t new_element_size_in_chars)
{
  /* Reserve space, and remember where the new space begins. */
  void *res = jitter_dynamic_buffer_reserve (db, new_element_size_in_chars);

  /* Fill the space. */
  memcpy (res, new_element, new_element_size_in_chars);

  /* Return the beginning of the copy. */
  return res;
}

const void *
jitter_dynamic_buffer_push_const (struct jitter_dynamic_buffer *db,
                                  const void *new_element,
                                  size_t new_element_size_in_chars)
{
  return jitter_dynamic_buffer_push (db, new_element,
                                     new_element_size_in_chars);
}

void*
jitter_dynamic_buffer_pop (struct jitter_dynamic_buffer *db,
                           size_t chars_to_release)
{
  assert (db->used_size >= chars_to_release);
  db->used_size -= chars_to_release;
  return db->region + db->used_size;
}

const void*
jitter_dynamic_buffer_pop_const (struct jitter_dynamic_buffer *db,
                                 size_t chars_to_release)
{
  return jitter_dynamic_buffer_pop (db, chars_to_release);
}

void*
jitter_dynamic_buffer_first_unused_char (const struct jitter_dynamic_buffer *db)
{
  return db->region + db->used_size;
}

const void*
jitter_dynamic_buffer_first_unused_char_const
   (const struct jitter_dynamic_buffer *db)
{
  return jitter_dynamic_buffer_first_unused_char (db);
}

size_t
jitter_dynamic_buffer_size (const struct jitter_dynamic_buffer *db)
{
  return db->used_size;
}

void
jitter_dynamic_buffer_compact (struct jitter_dynamic_buffer *db,
                               size_t wiggle_byte_no)
{
  /* Do nothing if there are less than wiggle_byte_no bytes free. */
  if (db->allocated_size - db->used_size <= wiggle_byte_no)
    return;

  /* If we arrived here then the size is actually shrinking. */
    
  /* Make the allocated size equal to the minimum possible, which is to say the
     number of bytes currently in use, plus the wiggle space.  Then resize the
     buffer.  jitter_xmalloc is intended to also supports a size of zero. */
  db->allocated_size = db->used_size + wiggle_byte_no;
  db->region = jitter_xrealloc (db->region, db->allocated_size);
}




/* Conversion to an ordinary pointer.
 * ************************************************************************** */

void*
jitter_dynamic_buffer_to_pointer (const struct jitter_dynamic_buffer *db)
{
  return db->region;
}

const void*
jitter_dynamic_buffer_to_const_pointer (const struct jitter_dynamic_buffer *db)
{
  return jitter_dynamic_buffer_to_pointer (db);
}




/* Extraction.
 * ************************************************************************** */

void *
jitter_dynamic_buffer_extract (struct jitter_dynamic_buffer *db)
{
  /* Return the pointed buffer, and mark the region is invalidated. */
  void *res = db->region;
  db->region = NULL;
  return res;
}

void *
jitter_dynamic_buffer_extract_trimmed (struct jitter_dynamic_buffer *db)
{
  /* Extract without trimming, then realloc the result. */
  void *untrimmed_res = jitter_dynamic_buffer_extract (db);
  return jitter_xrealloc (untrimmed_res, db->used_size);
}
