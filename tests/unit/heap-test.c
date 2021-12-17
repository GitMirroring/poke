/* Jitter: heap unity test.

   Copyright (C) 2018, 2019 Luca Saiu
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

/* Expand to nothing if we don't have mmap. */
#include <jitter/jitter.h>
#ifdef JITTER_HAVE_MMAP_ANONYMOUS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
//#include <stdbool.h> // FIXME: remove

#include <jitter/jitter-fatal.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-heap.h>
#include <jitter/jitter-heap-debug.h>

__attribute__ ((unused))
void*
test_alloc (struct jitter_heap_block *block, size_t byte_no)
{
  void *p = jitter_heap_allocate_from_block (block, byte_no);
  printf ("# ALLOCed %i bytes: got %p\n", (int) byte_no, p);
  return p;
}

__attribute__ ((unused))
void
test_free (struct jitter_heap_block *block, void *object)
{
  jitter_heap_free_from_block (block, object);
  printf ("# FREEd %p\n", object);
}

__attribute__ ((unused))
void*
test_realloc (struct jitter_heap_block *block, void *old_object,
              size_t new_byte_no)
{
  void *p = jitter_heap_reallocate_from_block (block, old_object, new_byte_no);
  printf ("# REALLOCed %p to %i bytes: got %p\n", old_object, (int) new_byte_no, p);
  return p;
}

#define CHECK_THING(thing_name, pointer)               \
  do                                                   \
    {                                                  \
      if (jitter_heap_debug_ ## thing_name (pointer))  \
        exit (EXIT_FAILURE);                           \
    }                                                  \
  while (false)

#define CHECK_BLOCK(block_pointer)    \
  CHECK_THING (block, block_pointer)
#define CHECK_HEAP(heap_pointer)    \
  CHECK_THING (heap, heap_pointer)

static size_t
mmap_page_size;

static unsigned long mmap_calls = 0;
static unsigned long munmap_calls = 0;

static void *
make_block (size_t size_in_bytes)
{
  void *res;
  //printf ("Make block: getting %liB\n", (long) size_in_bytes);
  //res = aligned_alloc (size_in_bytes, size_in_bytes);
  //res = aligned_alloc (size_in_bytes, size_in_bytes);
  size_in_bytes = JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO (size_in_bytes,
                                                        mmap_page_size);
  res = mmap (NULL,
              size_in_bytes,
              PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS
#ifdef HUGE
              | MAP_HUGETLB
#endif // #ifdef HUGE
              ,
              -1,
              0);
  if (res == MAP_FAILED)
    jitter_fatal ("mmap failed");
  mmap_calls ++;
  if ((((jitter_uint) res) & (jitter_uint) (mmap_page_size - 1)) != 0)
    jitter_fatal ("aligned allocation (size %liB) got an unaligned result",
                  (long) size_in_bytes);
  //printf ("Made a block at %p\n", res);
  return res;
}

static void
destroy_block (void *block, size_t size_in_bytes)
{
  //free (block);
  munmap (block, size_in_bytes);
  munmap_calls ++;
}

void
test_heap (void)
{
  mmap_page_size
#ifdef HUGE
    = 2 * 1024 * 1024L;
#else
    = sysconf (_SC_PAGE_SIZE);
#endif
  /* #define OBJECT_NO 10 //1024 */
/* #define OPERATION_NO (1000 * 1000 * 100) */
/* #define MAX_SIZE 3800 //3850 //4000 //5000// 512 //2048 //512 */
#define OBJECT_NO 1000 //1024
#define OPERATION_NO (1000 * 1000 * 100)
#define MAX_SIZE 1000//100 //3850 //4000 //5000// 512 //2048 //512
  //void **pointers __attribute__ ((unused)) = malloc (sizeof (int) * OBJECT_NO);//jitter_xmalloc (sizeof (int) * OBJECT_NO);
  void * pointers [OBJECT_NO];
  printf ("pointers is at %p\n", pointers);
  //void *pointers [OBJECT_NO];
  int i __attribute__ ((unused));
  int j __attribute__ ((unused));
  for (i = 0; i < OBJECT_NO; i ++)
    pointers [i] = NULL;

#if 0
#define jitter_heap_allocate(heap, size) \
  malloc (size)
#define jitter_heap_free(heap, pointer) \
  free (pointer)
#define jitter_heap_reallocate(heap, pointer, size) \
  realloc (pointer, size)
#elif 0
#define jitter_heap_allocate(heap, size) \
  (void*)((jitter_uint) 1)
#define jitter_heap_free(heap, pointer) \
  NULL
#define jitter_heap_reallocate(heap, pointer, size) \
  (void*)((jitter_uint) 1)
#endif

  struct jitter_heap h;
  jitter_heap_initialize (& h, make_block, destroy_block,
                          mmap_page_size,
                          destroy_block,
                          1024 * 1024 //1024 
                          );
  for (i = 0; i < OPERATION_NO; i ++)
    {
      //printf ("operation %i\n", i);
      int operation = rand () % 3;
      int index = rand () % OBJECT_NO;
      size_t size
        = rand () % MAX_SIZE
        //= 160
        ;
      switch (operation)
        {
        case 0: // alloc
          if (pointers [index] != NULL)
            jitter_heap_free (& h, pointers [index]);
          pointers [index] = jitter_heap_allocate (& h, size);
          break;
        case 1: // free
          if (pointers [index] != NULL)
            {
              jitter_heap_free (& h, pointers [index]);
              pointers [index] = NULL;
            }
          break;
        case 2: // realloc
          if (pointers [index] != NULL)
            pointers [index] = jitter_heap_reallocate (& h, pointers [index], size);
          else
            pointers [index] = jitter_heap_allocate (& h, size);
          break;
        }
    }

  CHECK_HEAP (& h);
  jitter_heap_finalize (& h);
  //free (pointers);
}

int
main (void)
{
  test_heap ();
  printf ("Still alive at the end.  mmap calls: %lu, mmunmap calls: %lu\n",
          mmap_calls, munmap_calls);
  return EXIT_SUCCESS;
}

#else // we don't have JITTER_HAVE_MMAP_ANONYMOUS defined.
#include <stdio.h>

int
main (void)
{
  printf ("This configuration has no mmap.  Nothing to test\n");
  return 0;
}

#endif // #ifdef JITTER_HAVE_MMAP_ANONYMOUS
