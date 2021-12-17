/* Jitter: Forth-style stacks with optional TOS optimization: implementation.

   Copyright (C) 2017, 2020, 2021 Luca Saiu
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


#include <jitter/jitter.h>

#include <string.h>
#include <unistd.h>

#if defined (JITTER_HAVE_MMAP_ANONYMOUS)
# include <sys/mman.h>
#endif

#include <jitter/jitter-bitwise.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-stack.h>
#include <jitter/jitter-malloc.h>




/* Guard page support.
 * ************************************************************************** */

/* If this configuration supports the required feature define another local
   feature macro. */
#if defined (JITTER_HAVE_MMAP_ANONYMOUS)                                       \
    && defined (JITTER_HAVE_MPROTECT)                                          \
    && ((defined (JITTER_HAVE_SYSCONF) && defined (JITTER_HAVE__SC_PAGESIZE))  \
        || defined (JITTER_HAVE_GETPAGESIZE))
# define JITTER_HAVE_PAGE_PERMISSIONS 1
#endif

#if defined (JITTER_HAVE_PAGE_PERMISSIONS)

/* A value of 0 means not yet initialised.  It is harmess to initialise this
   concurrently even from multiple threads, as long as word writes are
   atomic. */
static jitter_uint
jitter_saved_page_size = 0;

/* Return the system page size, querying the system at the first request. */
static jitter_uint
jitter_page_size (void)
{
  if (__builtin_expect (jitter_saved_page_size != 0, true))
    return jitter_saved_page_size;

  /* Get the page size by querying the system, which might be expensive. */
  jitter_int res;
#if defined (JITTER_HAVE_GETPAGESIZE)
  res = getpagesize ();
#else /* without getpagesize */
  res = sysconf (_SC_PAGE_SIZE);
#endif
  if (res <= 0)
    jitter_fatal ("failed getting page size");
  if (! JITTER_IS_A_POWER_OF_TWO (res))
    jitter_fatal ("page size not a power of two: this should never happen");

  /* Save the result for the next time. */
  jitter_saved_page_size = res;
  return res;
}
#endif // #if defined (JITTER_HAVE_PAGE_PERMISSIONS)

/* This is the trivial version of jitter_stack_backing_update_and_allocate,
   below, not using guard pages. */
static void
jitter_stack_backing_update_and_allocate_trivial (struct jitter_stack_backing
                                                  *backing)
{
  backing->guard_underflow = false;
  backing->guard_overflow = false;
  backing->memory = jitter_xmalloc (backing->element_size_in_bytes
                                    * backing->element_no);
}

/* Destroy the content for the pointed backing, in the trivial case not using
   mmap. */
static void
jitter_stack_backing_destroy_content_trivial (struct jitter_stack_backing
                                              *backing)
{
  free (backing->memory);
}

/* Given a pointer to a stack backing update its parameters keeping into account
   guard pages, if available and needed here; this may change the underflow
   fields and the element_no fields.
   Allocate memory for the stack content. */
#if defined (JITTER_HAVE_PAGE_PERMISSIONS)
static void
jitter_stack_backing_update_and_allocate (struct jitter_stack_backing *backing)
{
  if (! backing->guard_underflow && ! backing->guard_overflow)
    jitter_stack_backing_update_and_allocate_trivial (backing);
  else
    {
      /* Round the size up to a multiple of a page, if we need guard pages and
         can support them.  This also changes the number of elements. */
      jitter_int page_size = jitter_page_size ();
      jitter_int non_guard_size_in_bytes
        = JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO ((backing->element_size_in_bytes
                                                 * backing->element_no),
                                                page_size);
      backing->element_no
        = non_guard_size_in_bytes / backing->element_size_in_bytes;

      /* Compute the allocated size in bytes. */
      jitter_int size_in_bytes = non_guard_size_in_bytes;
      if (backing->guard_underflow)
        size_in_bytes += page_size;
      if (backing->guard_overflow)
        size_in_bytes += page_size;
      backing->mmapped_memory_size = size_in_bytes;

      /* Allocate. */
      backing->memory = mmap (NULL,
                              size_in_bytes,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS,
                              -1,
                              0);
      if (backing->memory == MAP_FAILED)
        jitter_fatal ("could not mmap stack memory");

      /* Set permissions for, which is to say remove every permission from,
         guard pages.  Also record guard page information in the backing. */
      backing->page_length_in_bytes = page_size;
      if (backing->guard_underflow)
        {
          backing->underflow_page_beginning = backing->memory;
          mprotect (backing->underflow_page_beginning, page_size, 0);
        }
      if (backing->guard_overflow)
        {
          backing->overflow_page_beginning
            = backing->memory + size_in_bytes - page_size;
          mprotect (backing->overflow_page_beginning, page_size, 0);
        }

      /* If there is an underflow guard page set the memory pointer to begin
         right after it. */
      if (backing->guard_underflow)
        backing->memory += page_size;
    }
}
#else
static void
jitter_stack_backing_update_and_allocate (struct jitter_stack_backing *backing)
{
  jitter_stack_backing_update_and_allocate_trivial (backing);
}
#endif // #if defined (JITTER_HAVE_PAGE_PERMISSIONS)

/* Destroy the content for the pointed backing. */
#if defined (JITTER_HAVE_PAGE_PERMISSIONS)
static void
jitter_stack_backing_destroy_content (struct jitter_stack_backing *backing)
{
  if (! backing->guard_underflow && ! backing->guard_overflow)
    jitter_stack_backing_destroy_content_trivial (backing);
  else
    {
      char *beginning = backing->memory;
      if (backing->guard_underflow)
        beginning -= backing->page_length_in_bytes;
      munmap (beginning, backing->mmapped_memory_size);
    }
}
#else
static void
jitter_stack_backing_destroy_content (struct jitter_stack_backing *backing)
{
  jitter_stack_backing_destroy_content_trivial (backing);
}
#endif // #if defined (JITTER_HAVE_PAGE_PERMISSIONS)




/* Stack backing initialization, reset, finalization.
 * ************************************************************************** */

void
jitter_stack_reset_backing (struct jitter_stack_backing *backing)
{
  /* If the backing does not require initialisation do nothing. */
  char *initial_element_p_or_NULL = backing->initial_element_copy;
  if (initial_element_p_or_NULL == NULL)
    return;

  /* Fill every element.  Use the number of elements stored in the backing,
     which has already been rounded up to a multiple of the page size where
     needed. */
  jitter_uint element_no = backing->element_no;
  jitter_uint element_size_in_bytes = backing->element_size_in_bytes;
  jitter_uint i;
  for (i = 0; i < element_no; i ++)
    memcpy ((char *) backing->memory + element_size_in_bytes * i,
            initial_element_p_or_NULL,
            element_size_in_bytes);
}

static void
jitter_stack_initialize_backing (struct jitter_stack_backing *backing,
                                 enum jitter_stack_optimization optimization,
                                 jitter_int element_size_in_bytes,
                                 jitter_int element_no,
                                 char *initial_element_p_or_NULL,
                                 bool guard_underflow,
                                 bool guard_overflow)
{
  /* Keep information for freeing and debugging. */
  backing->optimization = optimization;
  backing->element_size_in_bytes = element_size_in_bytes;
  backing->element_no = element_no;
  backing->guard_underflow = guard_underflow;
  backing->guard_overflow = guard_overflow;
  if (initial_element_p_or_NULL == NULL)
    backing->initial_element_copy = NULL;
  else
    {
      backing->initial_element_copy
        = jitter_xmalloc (sizeof (element_size_in_bytes));
      memcpy (backing->initial_element_copy,
              initial_element_p_or_NULL,
              element_size_in_bytes);
    }

  /* Set default values for fields which may be changed if we use mmap. */
  backing->mmapped_memory_size = 0;
  backing->underflow_page_beginning = NULL;
  backing->overflow_page_beginning = NULL;
  backing->page_length_in_bytes = 0;

  /* Update fields keeping into account the available features and whether we
     need guards.  Allocate. */
  jitter_stack_backing_update_and_allocate (backing);

  /* Fill the backing with copies of the initial element, if required. */
  jitter_stack_reset_backing (backing);

#if 0
  if (backing->guard_underflow)
    printf ("Guarded underflow addresses: [%p, %p]\n", backing->underflow_page_beginning, backing->underflow_page_beginning + backing->page_length_in_bytes - 1);
  if (backing->guard_underflow || backing->guard_overflow)
    printf ("Correct addresses:           [%p, %p] (%li B)\n", backing->memory, backing->memory + backing->element_no * backing->element_size_in_bytes - 1, (long) (backing->element_no * backing->element_size_in_bytes));
  if (backing->guard_overflow)
    printf ("Guarded overflow addresses:  [%p, %p]\n", backing->overflow_page_beginning, backing->overflow_page_beginning + backing->page_length_in_bytes - 1);
#endif
}

void
jitter_stack_initialize_tos_backing (struct jitter_stack_backing *backing,
                                     jitter_int element_size_in_bytes,
                                     jitter_int element_no,
                                     char *initial_element_p_or_NULL,
                                     bool guard_underflow,
                                     bool guard_overflow)
{
  jitter_stack_initialize_backing (backing,
                                   jitter_stack_optimization_tos,
                                   element_size_in_bytes,
                                   element_no,
                                   initial_element_p_or_NULL,
                                   guard_underflow,
                                   guard_overflow);
}

void
jitter_stack_initialize_ntos_backing (struct jitter_stack_backing *backing,
                                      jitter_int element_size_in_bytes,
                                      jitter_int element_no,
                                      char *initial_element_p_or_NULL,
                                      bool guard_underflow,
                                      bool guard_overflow)
{
  jitter_stack_initialize_backing (backing,
                                   jitter_stack_optimization_no_tos,
                                   element_size_in_bytes,
                                   element_no,
                                   initial_element_p_or_NULL,
                                   guard_underflow,
                                   guard_overflow);
}

void
jitter_stack_finalize_backing(struct jitter_stack_backing *backing)
{
  /* Release memory. */
  jitter_stack_backing_destroy_content (backing);
  if (backing->initial_element_copy != NULL)
    free (backing->initial_element_copy);

  /* Invalidate the backing content, to catch mistakes more easily. */
  memset (backing, 0xff, sizeof (struct jitter_stack_backing));
}
