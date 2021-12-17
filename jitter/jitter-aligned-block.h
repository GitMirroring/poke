/* Jitter: allocated heap memory blocks -- header.

   Copyright (C) 2020 Luca Saiu
   Updated in 2021 by Luca Saiu
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


#ifndef JITTER_ALIGNED_BLOCK_H_
#define JITTER_ALIGNED_BLOCK_H_

#include <jitter/jitter.h>
#include <jitter/jitter-config.h>

#include <stdlib.h>




/* Introduction.
 * ************************************************************************** */

/* In several places Jitter needs heap blocks allocated to a relatively large
   power of two; such blocks are freed when no longer needed.  The best
   implementation uses mmap to allocate a larger block followed by up to two
   munmap calls freeing the unneeded parts of the mapping at the beginning and
   at the end; this technique, while heavyweight, has the advantage of
   immediately returning memory to the operating system as soon as a block is
   destroyed.
   Notice that the block alignment must be a multiple of the system page size
   times two; on modern systems any power of two larger than 64KiB should be
   supported as a block size in practice.  Older systems tend to have smaller
   pages and are even less of a problem.  If you know of some exception please
   report it as a bug, providing details.

   On systems lacking mmap but providing posix_memalign the implementation is
   still easy, as the allocated block can be released (to the process, not the
   system) by a call to free.  Despite the general case being slightly more
   wasteful, aligned_alloc can be used as well as another alternative: it
   requires the block size to be a multiple of the alignment, so the
   implementation based on aligned_alloc rounds the demanded size up.

   Finally, on inferior systems lacking all of mmap, posix_memalign and
   aligned_alloc I define an alternative, wasting some space but at least
   allowing to free an allocated buffer.

   This wrapper provides a unified abstraction using the best available
   alternative. */




/* Configuration-dependent definitions.
 * ************************************************************************** */

/* Decide which implementation to use according to feature availability, and
   define exactly one feature macro for the implementation.  Check for features
   in the order of implementation desirability. */
#if defined (JITTER_HAVE_MMAP_ANONYMOUS)
# define JITTER_ALIGNED_BLOCK_USE_MMAP
#elif defined (JITTER_HAVE_POSIX_MEMALIGN)
# define JITTER_ALIGNED_BLOCK_USE_POSIX_MEMALIGN
#elif defined (JITTER_HAVE_ALIGNED_ALLOC)
# define JITTER_ALIGNED_BLOCK_USE_ALIGNED_ALLOC
#else
# define JITTER_ALIGNED_BLOCK_USE_FALLBACK
#endif




/* Aligned buffer destruction data structure.
 * ************************************************************************** */

/* Depending on the underlying implementation an initial pointer to the aligned
   buffer itself might not contain enough information to free the block.
   Aligned blocks should always be destroyed by passing the object of type
   jitter_aligned_block_id , which was set by jitter_aligned_block_make at the
   time of the block allocation.

   A jitter_aligned_block_id object should be treated as opaque.  It can be
   copied and retuend by functions; however the user should not make assumptions
   on its size, which may vary according to the configuration. */
struct jitter_aligned_block
{
#if defined (JITTER_ALIGNED_BLOCK_USE_MMAP)
  /* When calling munmap we have to provide a length as well. */
  void *initial_map;
  size_t mapping_length_in_bytes;
#elif defined (JITTER_ALIGNED_BLOCK_USE_ALIGNED_ALLOC)
  /* This is easy: the initial pointer suffices. */
  void *aligned_alloced_buffer;
#elif defined (JITTER_ALIGNED_BLOCK_USE_POSIX_MEMALIGN)
  /* Again, the initial pointer suffices. */
  void *posix_memaligned_buffer;
#elif defined (JITTER_ALIGNED_BLOCK_USE_FALLBACK)
  /* The fallback implementation relies on malloc.  While ugly at allocation
     time, freeing its easy: one call to free on the initial buffer suffices. */
  void *initial_pointer;
#else
# error "no aligned block implementation defined.  This should never happen."
#endif
};

/* The opaque version of struct jitter_aligned_block.  Notice that the size of
   this type depends on the configuration. */
typedef struct jitter_aligned_block
jitter_aligned_block_id;




/* Aligned block allocation and destruction.
 * ************************************************************************** */

/* Return a freshly allocated aligned buffer of the given size, satisfying the
   given minimum alignment which must be a power of two (not necessarily
   checked).  Set the pointed block id, to be used when freeing.
   Fail fatally if allocation fails. */
void *
jitter_aligned_block_make (jitter_aligned_block_id *id,
                           size_t alignment_in_bytes, size_t size_in_bytes)
  __attribute__ ((__malloc__, __warn_unused_result__, __returns_nonnull__,
                  __nonnull__ (1)));

/* Free the block with the given id.  It is not necessary to supply the block
   pointer. */
void
jitter_aligned_block_destroy (jitter_aligned_block_id id);

#endif // JITTER_ALIGNED_BLOCK_H_
