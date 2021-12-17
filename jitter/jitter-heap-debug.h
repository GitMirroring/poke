/* Jitter: debugging functions for jitter-heap: header.

   Copyright (C) 2018 Luca Saiu
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


#ifndef JITTER_HEAP_DEBUG_H_
#define JITTER_HEAP_DEBUG_H_

#include <jitter/jitter.h>
#include <jitter/jitter-heap.h>


/* Introduction.
 * ************************************************************************** */

/* This header contains prototypes for functions useful for debugging the heap
   functionality in jitter/jitter-heap.[ch] .  The debugging functionality is
   kept in a separate compilation unit, to avoid linking it with every Jittery
   VM. */




/* Heap debugging.
 * ************************************************************************** */

/* Print debugging information about the pointed block.  Return 0 if no error
   was found, nonzero otherwise. */
int
jitter_heap_debug_block (struct jitter_heap_block *b);

/* Print debugging information about the pointed heap.  Return 0 if no error
   was found, nonzero otherwise. */
int
jitter_heap_debug_heap (struct jitter_heap *h);

#endif // #ifndef JITTER_HEAP_DEBUG_H_
