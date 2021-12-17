/* JitterLisp: heap allocation header.

   Copyright (C) 2017, 2018 Luca Saiu
   Updated in 2021 by Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with GNU Jitter under the same license.

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


#ifndef JITTERLISP_ALLOCATION_H_
#define JITTERLISP_ALLOCATION_H_

#include <stdlib.h>

#include <jitter/jitter-stack.h>

#include "jitterlisp-sexpression.h"
#include "jitterlisp-ast.h"


/* Alignment.
 * ************************************************************************** */

/* In order to make sure that the allocated pointers respect the minimum
   alignment of (1 << JITTERLISP_TAG_BIT_NO) even when using my own allocator,
   I need to use a constant expression more complex than a simple sizeof to
   represent the required object size.

   Here it is crucial to compute at compile time rather than at run time as far
   as possible: this is why we don't realign an updated allocation pointer, but
   rather assume that the allocation pointer always remains correctly aligned,
   and compute an object size which keeps alignment into account.  Object sizes
   are compile-time constants in most cases. */
#define JITTERLISP_ALIGNMENT_BIT_MASK  \
  JITTER_BIT_MASK(JITTERLISP_INITIAL_POINTER_ZERO_BIT_NO)

/* Expand to the smallest multiple of (1 << JITTERLISP_TAG_BIT_NO) which is
   greater than or equal to the given size. */
#define JITTERLISP_ALIGNED_SIZE(_jitterlisp_original_size)        \
  (((_jitterlisp_original_size) + JITTERLISP_ALIGNMENT_BIT_MASK)  \
   & ~ JITTERLISP_ALIGNMENT_BIT_MASK)

/* Given a type name compute its "aligned sizeof", which is to say the size
   of an object of the given type or the smallest multiple of the alignment
   size which is large enough to accommodate an object. */
#define JITTERLISP_ALIGNED_SIZEOF(_jitterlisp_original_type)   \
  JITTERLISP_ALIGNED_SIZE(sizeof (_jitterlisp_original_type))




/* Cons allocation.
 * ************************************************************************** */

/* Expand to an rvalue of type struct jitterlisp_cons * whose evaluation points
   to a just allocated and uninitialized cons. */
#define JITTERLISP_CONS_MAKE_UNINITIALIZED_UNENCODED()                         \
  /* FIXME: this works, but of course the implementation is temporary. */      \
  ((struct jitterlisp_cons*)                                                   \
   (jitterlisp_allocate (JITTERLISP_ALIGNED_SIZEOF(struct jitterlisp_cons))))




/* Symbol allocation.
 * ************************************************************************** */

/* Uninterned symbols are allocated like any garbage-collected data structures;
   interned symbols have to work differently, since their identity depends on
   whether another symbol with the same name is already interned.

   Interned symbols are currently *not* garbage-collected. */




/* Uninterned symbol allocation.
 * ************************************************************************** */

/* Expand to an rvalue of type struct jitterlisp_symbol * whose evaluation
   points to a just-allocated and uninitialized symbol, not meant to be
   interned.  Notice that the name_or_NULL field must be set to NULL for an
   uninterned symbol, but this macro does not do that. */
#define JITTERLISP_SYMBOL_UNINTERNED_MAKE_UNINITIALIZED_UNENCODED()             \
  /* FIXME: this works, but of course the implementation is temporary. */       \
  ((struct jitterlisp_symbol*)                                                  \
   (jitterlisp_allocate (JITTERLISP_ALIGNED_SIZEOF(struct jitterlisp_symbol))))

/* Return a pointer to a fresh unencoded uninterned symbol, already
   initialized. */
struct jitterlisp_symbol *
jitterlisp_symbol_make_uninterned (void)
  __attribute__ ((__returns_nonnull__));




/* Interned symbol allocation.
 * ************************************************************************** */

/* Return a symbol with the given name, which must be non-NULL, either interning
   a new (correctly initialized) object if no other symbol with the same name is
   already interned, or returning the previously interned symbol with the same
   name otherwise.
   The returned symbol is allocated with malloc.  Right now there is no
   facility to garbage-collect interned symbols, which live as long as the
   symbol table lives, until finalization.
   The string pointed by the argument is not shared with the symbol and the
   caller may destroy it after this function returns. */
struct jitterlisp_symbol *
jitterlisp_symbol_make_interned (const char *name)
  __attribute__ ((__nonnull__ (1), __returns_nonnull__));




/* Closure allocation.
 * ************************************************************************** */

/* Expand to an rvalue of type struct jitterlisp_closure * whose evaluation
   points to a just allocated and uninitialized closure. */
#define JITTERLISP_CLOSURE_MAKE_UNINITIALIZED_UNENCODED()                  \
  /* FIXME: this works, but of course the implementation is temporary. */  \
  ((struct jitterlisp_closure*)                                            \
   (jitterlisp_allocate (JITTERLISP_ALIGNED_SIZEOF(                        \
                            struct jitterlisp_closure))))




/* Vector allocation.
 * ************************************************************************** */

/* Expand to an rvalue of type struct jitterlisp_vector * whose evaluation
   points to a just allocated and uninitialized vector.  The elements are
   *not* initialized, nor allocated. */
#define JITTERLISP_VECTOR_MAKE_UNINITIALIZED_UNENCODED()  \
  ((struct jitterlisp_vector*)                            \
   (jitterlisp_allocate (JITTERLISP_ALIGNED_SIZEOF(       \
                            struct jitterlisp_vector))))





/* Non-primitive macro allocation.
 * ************************************************************************** */

/* Expand to an rvalue of type struct jitterlisp_interpreted_closure * whose
   evaluation points to a just allocated and uninitialized interpreted
   closure, to be used for a non-primitive macro. */
#define JITTERLISP_NON_PRIMITIVE_MACRO_MAKE_UNINITIALIZED_UNENCODED()      \
  /* FIXME: this works, but of course the implementation is temporary. */  \
  ((struct jitterlisp_interpreted_closure*)                                \
   (jitterlisp_allocate (JITTERLISP_ALIGNED_SIZEOF(                        \
                            struct jitterlisp_interpreted_closure))))





/* AST (low-level) allocation.
 * ************************************************************************** */

/* High-level functions for allocating already initialized ASTs are provided in
   jitterlisp-ast.h . */

/* Expand to an rvalue of type struct jitterlisp_ast * whose evaluation points
   to a just allocated AST with the given number of subs, completely
   uninitialized. */
#define JITTERLISP_AST_MAKE_UNINITIALIZED_UNENCODED(_jitterlisp_sub_no)  \
  ((struct jitterlisp_ast*)                                              \
   (jitterlisp_allocate                                                  \
       (JITTERLISP_ALIGNED_SIZE(sizeof (struct jitterlisp_ast)           \
                                + (sizeof (jitterlisp_object)            \
                                   * (_jitterlisp_sub_no))))))




/* Fallback allocation.
 * ************************************************************************** */

/* This is a slow fallback allocation facility, for objects whose size is only
   known at run time.  For commonly occurring heap-allocated objects, such as
   conses, some other facility is, based on CPP macros. */

/* Return an unencoded pointer to a buffer of uninitialized memory with the
   given size.

   The size must be a multiple of the minimum required alignment in bytes (see
   the "Alignment" section above), but this is not checked for: calling this
   with an incorrectly aligned size may lead to subtle bugs. */
char *
jitterlisp_allocate (size_t size_in_bytes)
  __attribute__ ((__returns_nonnull__, __malloc__));




/* GC root registration.
 * ************************************************************************** */

/* GC roots must be registered, using Boehm GC, when they come from somewhere
   else than stack, register or global space.  This means malloc, mmap and
   firends.  Registering roots has no effect when littering.

   GC roots are handled under a strict LIFO policy. */

/* Register a root, providing a pointer to an array of Lisp objects and the
   array size (allowed to be 1 for single objects). */
void
jitterlisp_push_gc_root (jitterlisp_object *object_pointer,
                         size_t element_no)
  __attribute__ ((__nonnull__ (1)));

/* A convenience function registering the memory of the pointed stack backing
   as a GC root. */
void
jitterlisp_push_stack_backing_as_gc_root (struct jitter_stack_backing *sb)
  __attribute__ ((__nonnull__ (1)));


/* Unregister the last how_many roots which have been registered.
   The how_many parameter indicates the number of *roots* to be popped, not the
   number of Lisp objects or bytes.
   Notice that it is not necessary to call this at finalization time, as
   finalization implicitly pops every remaining root.*/
void
jitterlisp_pop_gc_roots (size_t how_many);

/* Pop a single GC root.  This is equivalent to calling jitterlisp_pop_gc_roots
   with an argument of 1. */
void
jitterlisp_pop_gc_root (void);




/* Forced collection.
 * ************************************************************************** */

/* Perform a GC, or do nothing if littering. */
void
jitterlisp_gc (void);




/* Interned symbol list.
 * ************************************************************************** */

/* Return a fresh list containing all the interned symbols which currently
   exist, in an unspecified order. */
jitterlisp_object
jitterlisp_interned_symbols (void);




/* Not for the user: initialization and finalization of the memory subsystem.
 * ************************************************************************** */

/* The functions here are not for the user to call directly.  The user is
   supposed to initialize and finalize every JitterLisp subsystem by calling
   jitterlisp_initialize and jitterlisp_finalize , which in their turn will call
   these functions in the appropriate order. */

/* Initialize the memory subsystem.  It's forbidden to heap-allocate any Lisp
   object until this function has been called. */
void
jitterlisp_memory_initialize (void);

/* Finalize the memory subsystem.  It's forbidden to heap-allocate any Lisp
   object after this function has been called, until
   jitterlisp_memory_initialize is called again. */
void
jitterlisp_memory_finalize (void);


#endif // #ifndef JITTERLISP_ALLOCATION_H_
