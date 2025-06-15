/* pvm-alloc.h - Memory allocator for the PVM.  */

/* Copyright (C) 2019, 2020, 2021, 2022, 2023, 2024, 2025 Jose E.
 * Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PVM_ALLOC_H
#define PVM_ALLOC_H

#include <config.h>

/* This file provides memory allocation services to the PVM code.  */

/* Functions to initialize and finalize the allocator, respectively.
   At finalization time all allocated memory is fred.  No pvm_alloc_*
   services shall be used once finalized, unless pvm_alloc_init is
   invoked again.  */

void pvm_alloc_initialize (void);
void pvm_alloc_finalize (void);

/* Allocate SIZE words and return a pointer to the allocated memory.
   Allocated memory is not automatically deallocated.  One should
   explicitly free that using `pvm_free_uncollectable'.
   On error, return NULL.  */

void *pvm_alloc_uncollectable (size_t size);

void pvm_free_uncollectable (void *ptr);

/* Register/unregister NELEM pointers at POINTER as roots for the
   garbage-collector.  */

void *pvm_alloc_add_gc_roots (void *pointer, size_t nelems);
void pvm_alloc_remove_gc_roots (void *handle);

/* FIXME FIXME FIXME */

void *pvm_gc_register_vm_stack (void *pointer, size_t nelems, void **tos_ptr);
void pvm_gc_deregister_vm_stack (void* handle);

/* Forced collection.  */

void pvm_gc_collect (void);

/* Register/unregister a new thread whose stack that may contain PVM
   values.  This is used for memory management.  */

int pvm_alloc_register_thread (void);
int pvm_alloc_unregister_thread (void);

#endif /* ! PVM_ALLOC_H */
