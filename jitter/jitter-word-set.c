/* Jitter: word set data structure.

   Copyright (C) 2020 Luca Saiu
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


#include <stdio.h>  /* Only for the debugging functions.  I may want to
                       remove those altogether. */

#include <jitter/jitter-word-set.h>

#include <jitter/jitter-fatal.h>
#include <jitter/jitter-malloc.h>




/* Internal functions.
 * ************************************************************************** */

/* Fill the pointed buffer with element_no copies of the unused entry. */
static void
jitter_word_set_empty_buffer (jitter_uint *buffer, size_t element_no)
{
  /* GCC translates this into a memset call when the number of elements is
     not statically known.  I guess it is the right choice. */
  int i;
  for (i = 0; i < element_no; i ++)
    buffer [i] = JITTER_WORD_SET_UNUSED;
}

static jitter_uint *
jitter_word_set_allocate_and_empty_buffer (size_t element_no)
{
  jitter_uint *res = jitter_xmalloc (element_no * sizeof (jitter_uint));
  jitter_word_set_empty_buffer (res, element_no);
  return res;
}

void
jitter_word_set_double (struct jitter_word_set *ps)
{
  size_t old_element_no = ps->allocated_element_no;
  size_t new_element_no = old_element_no * 2;
  jitter_uint *old_buffer = ps->buffer;
  jitter_uint *new_buffer
    = jitter_word_set_allocate_and_empty_buffer (new_element_no);
  ps->mask = (ps->mask << 1) | 1;
  ps->buffer = new_buffer;
  ps->allocated_element_no = new_element_no;
  ps->used_element_limit
    = JITTER_WORD_SET_ELEMENT_NO_TO_LIMIT (new_element_no);
  ps->used_element_no = 0;
  int i;
  for (i = 0; i < old_element_no; i ++)
    {
      jitter_uint p = old_buffer [i];
      if (JITTER_WORD_SET_IS_VALID (p))
        {
          /* Add the element to the new buffer, without using the more
             convenient macros which rely on a word set structure and
             automatically resize the buffer. */
          jitter_uint *pp;
          size_t probeno __attribute__ ((unused));
          JITTER_WORD_SET_BUFFER_SEARCH (new_buffer, ps->mask, p,
                                            false,
                                            probeno, pp);
          * pp = p;
          ps->used_element_no ++;
        }
    }
  free (old_buffer);
  //printf ("New size: %li (new element limit %li)\n", new_element_no, ps->used_element_limit);
}




/* Initialisation and finalisation.
 * ************************************************************************** */

/* Perform sanity checks.  This compiles to nothing in correct configuratins, so
   it is simpler to just call the function from jitter_word_set_initialize
   instead of providing another annoying global initialisation function to the
   user. */
static void
jitter_word_set_sanity_checks (void)
{
  if (JITTER_WORD_SET_INITIAL_ELEMENT_NO < 1)
    jitter_fatal ("JITTER_WORD_SET_INITIAL_ELEMENT_NO is less than 1");
  if (JITTER_WORD_SET_RECIPROCAL_FILL_RATIO <= 1)
    jitter_fatal ("JITTER_WORD_SET_RECIPROCAL_FILL_RATIO is less than or "
                  "equal to 1");
}

void
jitter_word_set_initialize (struct jitter_word_set *ps)
{
  /* Crash on configuration errors. */
  jitter_word_set_sanity_checks ();
  
  size_t element_no = JITTER_WORD_SET_INITIAL_ELEMENT_NO;
  jitter_uint index_mask_plus_one = 2;
  while (index_mask_plus_one < element_no)
    index_mask_plus_one *= 2;
  if (index_mask_plus_one != element_no)
    jitter_fatal ("jitter word set: element no not an even power of two");
  ps->allocated_element_no = element_no;
  ps->used_element_limit
    = JITTER_WORD_SET_ELEMENT_NO_TO_LIMIT (element_no);
  ps->mask = index_mask_plus_one - 1;
  /* I want to mask two different sets of bits:
     - enough bit to cover the number of elements: index_mask_plus_one - 1
     - the low bits, which are always clear for offsets to aligned elements
       and are implicitly shifted in or out when converting between offsets
       and pointers.  Here I want to reason in offsets. */
  ps->mask
    = ((ps->mask << JITTER_LG_BYTES_PER_WORD)
       | JITTER_POINTER_MISALIGNMENT_BITS_MASK);

  ps->used_element_no = 0;
  ps->buffer = jitter_word_set_allocate_and_empty_buffer (element_no);
}

void
jitter_word_set_finalize (struct jitter_word_set *ps)
{
  free (ps->buffer);
}




/* Emptying and rebuilding.
 * ************************************************************************** */

void
jitter_word_set_clear (struct jitter_word_set *ps)
{
  jitter_word_set_empty_buffer (ps->buffer, ps->allocated_element_no);
  ps->used_element_no = 0;
}

void
jitter_word_set_clear_and_minimize (struct jitter_word_set *ps)
{
  jitter_word_set_finalize (ps);
  jitter_word_set_initialize (ps);
}

void
jitter_word_set_clear_and_possibly_minimize (struct jitter_word_set *ps,
                                                bool minimize)
{
  /* Differently from jitter_word_set_rebuild_and_possibly_minimize this
     function does not come out so naturally out of factoring. */
  if (minimize)
    jitter_word_set_clear_and_minimize (ps);
  else
    jitter_word_set_clear (ps);
   
}

/* This factors the common logic of jitter_word_set_rebuild and
   jitter_word_set_rebuild_and_minimize.  It is also useful on its own. */
void
jitter_word_set_rebuild_and_possibly_minimize (struct jitter_word_set *ps,
                                                  bool minimize)
{
  /* Keep a pointer to the old buffer and the number of elements, that we would
     otherwise lose by modifying *ps . */
  jitter_uint *old_buffer = ps->buffer;
  size_t old_allocated_element_no = ps->allocated_element_no;

  if (minimize)
    {
      /* If we are minimising, just re-initialise the structure as if it were
         fresh.  A new buffer will be allocated, and old_buffer will also remain
         valid.  This resets the number of used elements, which is what we
         want. */
      jitter_word_set_initialize (ps);
    }
  else
    {
      /* If we are not minimising, replace the buffer in the struct with a new
         one, and empty the new buffer as well as zeroing the number of
         entries. */
      ps->buffer
        = jitter_xmalloc (ps->allocated_element_no * sizeof (jitter_uint));
      jitter_word_set_clear (ps);
    }

  /* At this point the structure is consistent but empty.  Copy non-unused
     non-deleted entries from the old buffer into it. */
  int i;
  for (i = 0; i < old_allocated_element_no; i ++)
    if (JITTER_WORD_SET_IS_VALID (old_buffer [i]))
      JITTER_WORD_SET_ADD_NEW (ps, old_buffer [i]);

  /* We are done with the old buffer. */
  free (old_buffer);
}

void
jitter_word_set_rebuild (struct jitter_word_set *ps)
{
  jitter_word_set_rebuild_and_possibly_minimize (ps, false);
}

void
jitter_word_set_rebuild_and_minimize (struct jitter_word_set *ps)
{
  jitter_word_set_rebuild_and_possibly_minimize (ps, true);
}




/* Debugging.
 * ************************************************************************** */

/* Some common logic factoring jitter_word_set_print and
   jitter_word_set_print_statistics . */
static void
jitter_word_set_print_possibly_with_statistics
   (struct jitter_word_set *psp, bool statistics)
{
  jitter_uint i;
  long valid_element_no = 0;
  long deleted_element_no = 0;
  long total_element_no = psp->allocated_element_no;
  double total_probe_no = 0;
  long min_probe_no __attribute__ ((unused)) = total_element_no;
  long max_probe_no = 0;
  for (i = 0; i < total_element_no; i ++)
    {
      jitter_uint p = psp->buffer [i];
      if (! statistics)
        printf ("%4li. ", (long) i);
      if (JITTER_WORD_SET_IS_VALID (p))
        {
          long probe_no;
          JITTER_WORD_SET_SET_PROBE_NO (psp, p, probe_no);
          if (! statistics)
            printf ("%-18p   probe no %li\n", (void *) p, probe_no);
          valid_element_no ++;
          total_probe_no += probe_no;
          if (probe_no < min_probe_no) min_probe_no = probe_no;
          if (probe_no > max_probe_no) max_probe_no = probe_no;
        }
      else if (p == JITTER_WORD_SET_UNUSED)
        {
          if (! statistics)
            printf ("unused\n");
        }
      else if (p == JITTER_WORD_SET_DELETED)
        {
          if (! statistics)
            printf ("deleted\n");
          deleted_element_no ++;
        }
      else
        jitter_fatal ("impossible");
    }
  if (statistics)
    {
      if (valid_element_no > 0)
        {
          double fill_ratio = ((valid_element_no + deleted_element_no)
                               / (double) total_element_no);
          double average_probe_no = total_probe_no / valid_element_no;
          printf ("elt(val/del/tot) %6li/%li/%-6li "
                  "fill %4.2f "
                  "probes(avg/max)%7.3f/%7li\n",
                  valid_element_no, deleted_element_no, total_element_no,
                  fill_ratio,
                  average_probe_no, max_probe_no);
        }
      else
        printf ("empty word set: no statistics\n");
    }
}

void
jitter_word_set_print (struct jitter_word_set *psp)
{
  jitter_word_set_print_possibly_with_statistics (psp, false);
}

void
jitter_word_set_print_statistics (struct jitter_word_set *psp)
{
  jitter_word_set_print_possibly_with_statistics (psp, true);
}




/* Scratch: code to disassemble, benchmarks and simulations.
 * ************************************************************************** */

__attribute__ ((noclone, noinline))
bool
jitter_word_set_test0 (jitter_int n)
{
  return ! ! n;
}

__attribute__ ((noclone, noinline))
int /*bool*/
jitter_word_set_test1 (struct jitter_word_set *psp, jitter_uint p)
{
  int /*bool*/ b;
  JITTER_WORD_SET_SET_HAS (psp, p, b);
  return b;
}

__attribute__ ((noclone, noinline))
bool
jitter_word_set_test1b (struct jitter_word_set *psp, jitter_uint p)
{
  bool b;
  JITTER_WORD_SET_SET_HAS (psp, p, b);
  return b;
}

__attribute__ ((noclone, noinline))
void
jitter_word_set_test2 (struct jitter_word_set *psp, jitter_uint p)
{
  JITTER_WORD_SET_ADD_NEW (psp, p);
}

__attribute__ ((noclone, noinline))
void
jitter_word_set_test3 (struct jitter_word_set *psp, jitter_uint p)
{
  JITTER_WORD_SET_ADD_UNIQUE (psp, p);
}

__attribute__ ((noclone, noinline))
void
jitter_word_set_test4 (struct jitter_word_set *psp, jitter_uint p)
{
  JITTER_WORD_SET_REMOVE (psp, p);
}

__attribute__ ((noclone, noinline))
void
jitter_word_set_test5 (struct jitter_word_set *psp, jitter_uint p)
{
  int /*bool*/ b;
  JITTER_WORD_SET_SET_HAS (psp, p, b);
  if (b)
    {
      JITTER_WORD_SET_REMOVE (psp, p);
    }
  else
    {
      JITTER_WORD_SET_REMOVE (psp, (jitter_uint) psp);
    }
}

void
jitter_word_set_test_hash (long random_element_no)
{
  struct jitter_word_set ps;
  jitter_word_set_initialize (& ps);
  long i;
  for (i = 0; i < random_element_no; /* nothing */)
    {
      long remaining_element_no = random_element_no - i;

      //long sequential_element_no = rand () % remaining_element_no + 1;

      long sequential_element_no = (long) remaining_element_no * (2. / 3);
      if (sequential_element_no == 0)
        sequential_element_no = 1;
      
      jitter_uint rn0 = rand ();
#if JITTER_BYTES_PER_WORD == 8
      jitter_uint rn1 = rand ();
      jitter_uint rn = rn0 << 32 | rn1;
#else
      jitter_uint rn = rn0;
#endif
      rn &= ~ (((jitter_uint) 1 << JITTER_LG_BYTES_PER_WORD) - 1);
      long j;
      for (j = 0; j < sequential_element_no; j ++)
        JITTER_WORD_SET_ADD_UNIQUE (& ps, rn + JITTER_BYTES_PER_WORD * j);

      i += sequential_element_no;
    }
  // jitter_word_set_print (& ps);
  printf ("%-10li ", random_element_no);
  jitter_word_set_print_statistics (& ps);
  jitter_word_set_finalize (& ps);
}

void
jitter_word_set_word_set_test (void)
{
  unsigned long table_size;
  for (table_size = 64; table_size < ((unsigned long) 1 << 30); table_size *= 2)
    {
      long element_no
        = (long) ((double) table_size
                  / JITTER_WORD_SET_RECIPROCAL_FILL_RATIO
                  - 1);
      jitter_word_set_test_hash (element_no);
    }
}
