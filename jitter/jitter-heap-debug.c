/* Jitter: debugging functions for jitter-heap.

   Copyright (C) 2018 Luca Saiu
   Updated in 2020 by Luca Saiu
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


#include <stdio.h>

#include <jitter/jitter.h>
#include <jitter/jitter-heap.h>
#include <jitter/jitter-heap-debug.h>


/* Heap debugging.
 * ************************************************************************** */

/* Given a tag return the corresponging textual tag name, as a statically
   allocated string. */
static char *
jitter_heap_tag_name (enum jitter_heap_thing_tag tag)
{
  switch (tag)
    {
    case jitter_heap_thing_tag_hole:
      return "Hole";
    case jitter_heap_thing_tag_object:
      return "Object";
    case jitter_heap_thing_tag_terminator:
      return "Terminator";
    default:
      return "INVALID";
    }
}

int
jitter_heap_debug_block (struct jitter_heap_block *p)
{
  /* I'll return success by default.  This variable will change if I find any
     error. */
  int res = 0;

  struct jitter_heap_thing *h = & p->left_terminator;
  int terminator_counter = 0;
  printf ("* Block with header at   %p allocated space at %p, %luB:\n", p,
          p->allocated_space, (unsigned long) p->allocated_space_size_in_bytes);
  char *past_allocated_space
    = p->allocated_space + p->allocated_space_size_in_bytes;
  struct jitter_heap_thing *previous_thing = NULL;
  while (terminator_counter < 2)
    {
      if (((char *) h) >= past_allocated_space)
        {
          printf ("  - INVALID: past allocated space for block at %p\n", h);
          res = 1;
          break;
        }

      enum jitter_heap_thing_tag tag = JITTER_HEAP_GET_TAG (h);
      void *untagged_thing_on_the_left
        = JITTER_HEAP_UNTAG_POINTER (h->thing_on_the_left);
      size_t payload_size = h->payload_size_in_bytes;

      printf ("  - %-10s: header %p, payload %p: %luB, total %luB\n",
              jitter_heap_tag_name (tag), h, JITTER_HEAP_THING_TO_PAYLOAD (h),
              (unsigned long) payload_size,
              ((unsigned long) (payload_size + JITTER_HEAP_HEADER_OVERHEAD))
              );
      unsigned misalignment;
      if ((misalignment = (jitter_uint) h % JITTER_HEAP_ALIGNMENT) != 0)
        {
          printf ("  ! HEADER MISALIGNED by %uB\n", misalignment);
          res = 1;
        }
      if ((misalignment = (jitter_uint) JITTER_HEAP_THING_TO_PAYLOAD (h)
           % JITTER_HEAP_ALIGNMENT) != 0)
        {
          printf ("  ! PAYLOAD MISALIGNED by %uB\n", misalignment);
          res = 1;
        }

      if (untagged_thing_on_the_left != previous_thing)
        {
          printf ("  ! INVALID THING ON THE LEFT:  %p\n", untagged_thing_on_the_left);
          res = 1;
        }
      else if (previous_thing != NULL
               && (JITTER_HEAP_GET_TAG (previous_thing)
                   == jitter_heap_thing_tag_hole)
               && tag == jitter_heap_thing_tag_hole)
        {
          printf ("  ! INVALID: ADJACENT HOLES\n");
          res = 1;
        }

      if (tag == jitter_heap_thing_tag_hole)
        {
          printf ("    . previous %p, next %p\n",
                  h->hole_links.previous,  h->hole_links.next);
        }

      if (tag == jitter_heap_thing_tag_terminator)
        terminator_counter ++;
      previous_thing = h;
      h = JITTER_HEAP_THING_ON_THE_RIGHT_OF (h);
    }
  /* Count holes, first-to-last and then last-to-first, to check that the list
     is well formed. */
  struct jitter_heap_thing *hole;
  hole = p->hole_list.first;
  size_t first_to_last_size = 0;
  while (hole != NULL)
    {
      hole = hole->hole_links.next;
      first_to_last_size ++;
    }
  hole = p->hole_list.last;
  size_t last_to_first_size = 0;
  while (hole != NULL)
    {
      hole = hole->hole_links.previous;
      last_to_first_size ++;
    }

  /* Check that the sizes match. */
  if (first_to_last_size != last_to_first_size)
    {
      printf ("! INVALID HOLE LIST (f2l size %lu, l2f size %lu)\n",
              (unsigned long) first_to_last_size,
              (unsigned long) last_to_first_size);
      printf ("Terminators and holes, first to last:\n");
      hole = p->hole_list.first;
      while (hole != NULL)
        {
          printf ("  - Header at %p, payload at %p\n", hole,
                  JITTER_HEAP_THING_TO_PAYLOAD (hole));
          hole = hole->hole_links.next;
        }
      printf ("Terminators and holes, last to first:\n");
      hole = p->hole_list.last;
      while (hole != NULL)
        {
          printf ("  - Header at %p, payload at %p\n", hole,
                  JITTER_HEAP_THING_TO_PAYLOAD (hole));
          hole = hole->hole_links.previous;
        }
      res = 1;
    }

  /* Print potentially distracting general data only if there is a problem. */
  if (res)
  {
    printf ("Alignment:                  %liB\n", (long) JITTER_HEAP_ALIGNMENT);
    printf ("Header overhead:            %liB\n", (long) JITTER_HEAP_HEADER_OVERHEAD);
    printf ("Minimum payload thing size: %liB\n", (long) JITTER_HEAP_MINIMUM_THING_SIZE);
    printf ("Minimum total thing size:   %liB\n", (long) JITTER_HEAP_MINIMUM_PAYLOAD_SIZE);
  }

  if (res)
    printf ("INVALID BLOCK.\n");
  return res;
}

int
jitter_heap_debug_heap (struct jitter_heap *h)
{
  /* Assume success by default. */
  int res = 0;

  printf ("Heap at %p\n", h);

  /* Check that the first block in the list is the default block. */
  struct jitter_heap_block *b = h->block_list.first;
  if (h->default_block != b)
    {
      printf ("! INVALID: default_block %p different from first block %p\n",
              h->default_block, b);
      res = 1;
    }

  /* Walk blocks first-to-last, debugging each of them. */
  size_t first_to_last_size = 0;
  while (b != NULL)
    {
      jitter_heap_debug_block (b);
      b = b->block_links.next;
      first_to_last_size ++;
    }

  /* Walk blocks last-to-first, just to check that their number is the same both
     ways. */
  size_t last_to_first_size = 0;
  b = h->block_list.last;
  while (b != NULL)
    {
      b = b->block_links.previous;
      last_to_first_size ++;
    }
  if (first_to_last_size != last_to_first_size)
    {
      printf ("! INVALID: f2l size %li different from l2f size %li\n",
              (long) first_to_last_size, (long) last_to_first_size);
      res = 1;
    }
  else
    printf ("The heap %p has %li blocks\n", h, (long) first_to_last_size);

  return res;
}
