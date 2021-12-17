/* Jitter: allocated heap memory blocks.

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


#include <jitter/jitter-aligned-block.h>
#include <jitter/jitter-bitwise.h>
#include <jitter/jitter-fatal.h>

/* Include the right header, according to the selected implementation. */
#if defined (JITTER_ALIGNED_BLOCK_USE_MMAP)
# include <unistd.h>   /* For sysconf . */
# include <sys/mman.h> /* For mmap and munmap . */
#elif defined (JITTER_ALIGNED_BLOCK_USE_ALIGNED_ALLOC)
# include <stdlib.h>
#elif defined (JITTER_ALIGNED_BLOCK_USE_POSIX_MEMALIGN)
# include <stdlib.h>
#elif defined (JITTER_ALIGNED_BLOCK_USE_FALLBACK)
# include <jitter/jitter-malloc.h>
#else
# error "no aligned block implementation defined.  This should never happen."
#endif




/* Aligned block allocation and destruction.
 * ************************************************************************** */

void *
jitter_aligned_block_make (jitter_aligned_block_id *id,
                           size_t alignment_in_bytes, size_t size_in_bytes)
{
  void *res;

  /* The mmap and malloc implementations share the strategy of allocating a
     larger block guaranteed to contain an aligned block inside. */
#if defined (JITTER_ALIGNED_BLOCK_USE_MMAP)         \
    || defined (JITTER_ALIGNED_BLOCK_USE_FALLBACK)
  /* Allocate a larger buffer which is guaranteed to contain an aligned buffer
     of the required size as a sub-buffer.  Keep a pointer to the initial
     buffer, in order to be able to free it later, in the block id. */
  size_t allocated_size_in_bytes;
  if (size_in_bytes < alignment_in_bytes)
    allocated_size_in_bytes = alignment_in_bytes * 2;
  else
    allocated_size_in_bytes = size_in_bytes * 2;
  void *initial_pointer;
#endif

#if defined (JITTER_ALIGNED_BLOCK_USE_MMAP)
  /* Allocate a larger block. */
  initial_pointer = mmap (NULL,
                          allocated_size_in_bytes,
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS,
                          -1,
                          0);
  if (initial_pointer == NULL)
    jitter_fatal ("mmap failed");
  /* Find the aligned part of the mapping, which is the only part we are
     interested in. */
  res = ((void *)
         JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO ((jitter_uint) initial_pointer,
                                               alignment_in_bytes));
  id->initial_map = res;
  id->mapping_length_in_bytes = size_in_bytes;
  /* Unmap the misaligned part (which means: aligned to a double memory page,
     but not to the required alignment) at the beginning and the end.  This also
     checks that the block alignment is a multiple of double the page size, as
     munmap fails otherwise. */
  void *misaligned_before = initial_pointer;
  size_t misaligned_before_length
    = (char *) res - (char *) initial_pointer;
  void *misaligned_after = (char *) res + size_in_bytes;
  size_t misaligned_after_length
    = (((char *) initial_pointer + allocated_size_in_bytes)
       - (char *) misaligned_after);
  if (misaligned_before_length > 0)
    if (munmap (misaligned_before, misaligned_before_length) != 0)
      jitter_fatal ("munmap failed (%li B not an even multiple of the page "
                    "size?)", (long) alignment_in_bytes);
  if (misaligned_after_length > 0)
    if (munmap (misaligned_after, misaligned_after_length) != 0)
      jitter_fatal ("munmap failed (%li B not an even multiple of the page "
                    "size?)", (long) alignment_in_bytes);
#elif defined (JITTER_ALIGNED_BLOCK_USE_ALIGNED_ALLOC)
  /* According to the specification aligned_alloc requires that the size be a
     multiple of the alignment -- thanks to Bruno Haible for letting me notice.
     This solution is therefore, in theory (in practice alignment_in_bytes will
     usually be the same as size_in_bytes), wasteful. */
  size_in_bytes = JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO (size_in_bytes,
                                                        alignment_in_bytes);
  res = aligned_alloc (alignment_in_bytes, size_in_bytes);
  if (res == NULL)
    jitter_fatal ("aligned_alloc failed");
  id->aligned_alloced_buffer = res;
#elif defined (JITTER_ALIGNED_BLOCK_USE_POSIX_MEMALIGN)
  if (posix_memalign (& res, alignment_in_bytes, size_in_bytes) != 0)
    jitter_fatal ("posix_memalign failed");
  id->posix_memaligned_buffer = res;
#elif defined (JITTER_ALIGNED_BLOCK_USE_FALLBACK)
  initial_pointer = jitter_xmalloc (allocated_size_in_bytes);
  res = ((void *)
         JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO ((jitter_uint) initial_pointer,
                                               alignment_in_bytes));
  id->initial_pointer = initial_pointer;
#else
# error "no aligned block implementation defined.  This should never happen."
#endif
  return res;
}

void
jitter_aligned_block_destroy (jitter_aligned_block_id id)
{
#if defined (JITTER_ALIGNED_BLOCK_USE_MMAP)
  munmap (id.initial_map, id.mapping_length_in_bytes);
#elif defined (JITTER_ALIGNED_BLOCK_USE_ALIGNED_ALLOC)
  free (id.aligned_alloced_buffer);
#elif defined (JITTER_ALIGNED_BLOCK_USE_POSIX_MEMALIGN)
  free (id.posix_memaligned_buffer);
#elif defined (JITTER_ALIGNED_BLOCK_USE_FALLBACK)
  free (id.initial_pointer);
#else
# error "no aligned block implementation defined.  This should never happen."
#endif
}
