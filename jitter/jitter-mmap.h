/* Jitter: mmap abstraction: header.

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
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTER_MMAP_H_
#define JITTER_MMAP_H_

#include <stdlib.h>

#include <jitter/jitter.h>

/* Executable memory allocation: introdution.
 * ************************************************************************** */

/* This uses the memory heap implemented in jitter-heap.[ch] over executable
   memory allocated by mmap; if portability to system without mmap is required
   in the future, the use of mmap itself will be easy to replace with something
   else.

   There is no support yet for W^X systems.

   The API as it stands now is not reentrant with respect to mulithreading.  If
   excutable memory allocation is desired from multiple threads at the same
   time, the user is supposed to synchronize with a mutex. */




/* Executable memory allocation: allocation and release.
 * ************************************************************************** */

/* Return a memory buffer of executable and writable memory, of the given
   size. */
void *
jitter_executable_allocate (size_t size_in_bytes)
  __attribute__ ((__malloc__));

/* Given an initial pointer to an object previously allocated by
   jitter_executable_allocate and possibly already shrunk by this function,
   shrink it in place where possible reducing its size to the new given value,
   assumed (without checking) to be smaller than the current allocated size,
   rounded up as required by the implementation.  Do nothing, without failing,
   if shrinking is not possible. */
void
jitter_executable_shrink_in_place (void *object, size_t new_size_in_bytes)
  __attribute__ ((__nonnull__ (1)));

/* Release the pointed buffer of executable memory returned by a previous call
   to jitter_mmap_allocate_executable . */
void
jitter_executable_deallocate (void *buffer)
  __attribute__ ((__nonnull__ (1)));




/* Executable memory allocation: initialization and finalization.
 * ************************************************************************** */

/* Initialize the executable-memory subsystem. */
void
jitter_initialize_executable (void);

/* Finalize the executable-memory subsystem. */
void
jitter_finalize_executable (void);

#endif // #ifndef JITTER_MMAP_H_
