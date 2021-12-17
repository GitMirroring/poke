/* Jitter: Forth-style stacks with optional TOS optimization: header.

   Copyright (C) 2017, 2018, 2019, 2020, 2021 Luca Saiu
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


#ifndef JITTER_STACK_H_
#define JITTER_STACK_H_

#include <stdlib.h> /* for size_t . */
#include <stdbool.h>

#include <jitter/jitter.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-cpp.h>


/* Stack backing data structures.
 * ************************************************************************** */

/* A stack @dfn{backing} points to the beginning of the memory buffer used to
   implement the stack, along with some debugging meta-information.

   The stack runtime data structure is more compact, and suitable to be stored
   efficiently in registers; in particular the runtime structure contains no
   pointer to the *beginning* of the memory or no information about the current
   stack height.  Only the runtime stack structure, and not the backing, is
   manipulated at run time after initialization. */

/* A stack may be either TOS-optimized or not. */
enum jitter_stack_optimization
  {
    /* The stack is TOS-optimized: the top of the stack is held in a separate
       field of the struct, hopefully allocated to a machine register in order
       to reduce memory accesses. */
    jitter_stack_optimization_tos,

    /* The stack is not TOS-optimized: the top element is held in memory just
       like any other.  This makes some operations slower (and some marginally
       faster), but has the advantage of freeing up one machine register. */
    jitter_stack_optimization_no_tos
  };

/* The stack @dfn{backing} is a data structure containing some information about
   it such as the number of elements and whether the stack is TOS-optimized or
   not; it also holds a pointer to the *beginning* of the memory used to back
   elements, which makes initialization and finalization easier. */

/* The stack backing. */
struct jitter_stack_backing
{
  /* The stack optimization, either TOS-optimized or not. */
  enum jitter_stack_optimization optimization;

  /* How many bytes each element takes. */
  jitter_uint element_size_in_bytes;

  /* How many elements are allocated for this stack, not including the TOS
     element in the case of TOS optimization.  This is always at least as
     large as the number of elements requested at initialisation, but may
     be larger in order to accommodate guard pages which must begin at page
     boundaries. */
  jitter_uint element_no;

  /* How many bytes were allocated with mmap.  Only used if mmap is available
     and guard pages are used for this backing. */
  jitter_uint mmapped_memory_size;
  
  /* A local malloc-allocated copy of the initial element, if any; NULL if there
     is no initial element. */
  char *initial_element_copy;
  
  /* A Boolean, non-false iff the backing memory contains a guard page for,
     respectively, underflow and overflow. */
  bool guard_underflow;
  bool guard_overflow;

  /* These fields, recording the beginning of the underflow and overflow guard
     pages and the page length in bytes, are useful for debugging or for
     trapping underflow and overflow, for example using GNU libsigsegv. */
  char *underflow_page_beginning;
  char *overflow_page_beginning;
  size_t page_length_in_bytes;

  /* A pointer to the beginning of the memory holding the stack elements.
     When stack elements are pushed or popped a pointer moves in the stack
     data structure, but this field keeps pointing the the beginning of the
     allocated space, making initialization and finalization easier.  The
     memory is heap-allocated, and its beginning is aligned like the result of
     malloc .
     This points to the beginning of usable memory, past the underflow guard
     page if present. */
  char *memory;
};

/* These operations should be not critical for performance on any reasonable
   application, and therefore their implementation can safely involve non-inline
   C functions. */

/* Initialise the pointed stack backing, allocating space from the heap for the
   given number of elements, each of the given size.  Also store implementation
   data in the backing, in order to make finalisation possible and to enable
   debugging.
   If initial_element_p_or_NULL is non-NULL initialise every stack element with
   a copy of the given object.
   If guard_underflow is non-false, add an underflow guard page (in
   configurations where this is possible; ignore the option otherwise) before
   the beginning of usable memory.
   The same for guard_overflow, adding a page after the end of usable memory. */
void
jitter_stack_initialize_tos_backing (struct jitter_stack_backing *backing,
                                     jitter_int element_size_in_bytes,
                                     jitter_int element_no,
                                     char *initial_element_p_or_NULL,
                                     bool guard_underflow,
                                     bool guard_overflow)
  __attribute__ ((nonnull (1)));

/* Like jitter_stack_initialize_tos_backing, for a non-TOS stack. */
void
jitter_stack_initialize_ntos_backing (struct jitter_stack_backing *backing,
                                      jitter_int element_size_in_bytes,
                                      jitter_int element_no,
                                      char *initial_element_p_or_NULL,
                                      bool guard_underflow,
                                      bool guard_overflow)
  __attribute__ ((nonnull (1)));

/* Reset an existing stack backing which has already been initialised and
   possibly used, without re-allocating its resources.  This will re-initialise
   stack elements where needed, and do nothign else. */
void
jitter_stack_reset_backing (struct jitter_stack_backing *backing)
  __attribute__ ((nonnull (1)));

/* Finalize the pointed stack backing, releasing memory. */
void
jitter_stack_finalize_backing (struct jitter_stack_backing *backing)
  __attribute__ ((nonnull (1)));




/* Stack data structure definitions.
 * ************************************************************************** */

/* Since the stack element type is a parameter and adding one indirection level
   would be unacceptable for performance, we cannot use a C struct to define a
   TOS-optimized stack; we would need something like C++ templates.  Anyway it
   is quite easy to treat two separate variables as "the stack". */

/* The @var{stack_container} argument is meant to be part of an expression
   containing the stack variables, for example "mystruct." .  Of course it is
   allowed to be empty. */

/* Expand to the variable name or struct field holding the stack top, as an
   l-value. */
#define JITTER_STACK_TOS_TOP_NAME(type, stack_container, name)       \
  stack_container                                                    \
  JITTER_CONCATENATE_THREE(jitter_tos_optimized_stack_, name, _top)

/* Expand to the the variable name or struct field holding the stack
   under-top-pointer, as an l-value */
#define JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name)  \
  stack_container                                                             \
  JITTER_CONCATENATE_THREE(jitter_tos_optimized_stack_, name, _under_top_p)
#define JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name)  \
  stack_container                                                             \
  JITTER_CONCATENATE_THREE(jitter_non_tos_optimized_stack_, name, _top_p)

/* Declare a stack.  This exapnds to one or more non-extern C variable
   declarations, suitable for automatic variables or struct members. */
#define JITTER_STACK_TOS_DECLARATION(type, name)                         \
  type JITTER_STACK_TOS_TOP_NAME(type, , name);                          \
  type * restrict JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, , name)
#define JITTER_STACK_NTOS_DECLARATION(type, name)                        \
  type * restrict JITTER_STACK_NTOS_TOP_POINTER_NAME(type, , name)

/* Expand to a statement initializing a stack from a backing. */
#define JITTER_STACK_TOS_INITIALIZE(type, stack_container, name, backing)   \
  do                                                                        \
    {                                                                       \
      /* Initialize the under-top pointer to point to one element below     \
         the beginning of the backing for an "empty" stack, so that the     \
         first element to be pushed will increment the pointer to the       \
         first valid position, and store the (unspecified) initial TOS      \
         there.  This is as empty as a TOS-optimized stack can get.  */     \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name)  \
        = ((type *) ((backing).memory)) - 1;                                \
      /* Initialise the TOS.  The backing contains a pointer to a copy of   \
         the initial element chosen by the user, non-NULL iff any was       \
         specified. */                                                      \
      if ((backing).initial_element_copy != NULL)                           \
        JITTER_STACK_TOS_TOP_NAME(type, stack_container, name)              \
          = * (type *) ((backing).initial_element_copy);                    \
    }                                                                       \
  while (false)
#define JITTER_STACK_NTOS_INITIALIZE(type, stack_container, name, backing)  \
  do                                                                        \
    {                                                                       \
      /* Initialize the top pointer to point to one element below           \
         the beginning of the backing for an empty stack, so that the       \
         first element to be pushed will increment the pointer to the       \
         first valid position. */                                           \
      JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name)       \
        = ((type *) ((backing).memory)) - 1;                                \
    }                                                                       \
  while (false)

/* There is no finalization facility for stacks: stacks are meant to be either
   automatic variables or struct fields, so their storage is not heap-allocated.
   Stack *backings* have their own finalization function. */

/* FIXME: comment.

   Stacks are meant to be used in the style of Forth, for accessing the top
   elements only and not as arrays.

   Stacks do not perform any overflow, underflow or TOS-type checking; their
   operations are implemented as macros and are meant to be fast, but inherently
   unsafe.

   Stacks do *not* point to their backing: since every field of stack structures
   is meant to be allocated to a register it's important not to waste space. */




/* Fundamental stack operations.
 * ************************************************************************** */

/* FIXME: comment.

   The @var{stack} argument must be an l-value expression without side effects,
   of type struct jitter_stack_tos ; it is meant to be an automatic variable.

   By using GCC extensions it would be easy to have macros expanding to
   side-effecting expressions, altering the stack and evaluating to the new top;
   but that feels very difficult to do in a portable macro evaluating the stack
   only once, and here avoiding function overhead is paramount. */

/* Expand to an expression returning the top element of the given stack.
   Undefined behavior on empty stack.  The expression this macro expands to is
   an l-value. */
#define JITTER_STACK_TOS_TOP(type, stack_container, name)  \
  JITTER_STACK_TOS_TOP_NAME(type, stack_container, name)
#define JITTER_STACK_NTOS_TOP(type, stack_container, name) \
  (* JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name))

/* Expand to an expression returning the under-top element of the given stack.
   Undefined behavior on empty stack.  The expression this macro expands to is
   an l-value. */
#define JITTER_STACK_TOS_UNDER_TOP(type, stack_container, name)  \
  (* JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name))
#define JITTER_STACK_NTOS_UNDER_TOP(type, stack_container, name)  \
  (JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name) [-1])

/* Expand to an expression returning the under-under-top element of the given
   stack.  Undefined behavior on underflow.  The expression this macro expands
   to is an l-value. */
#define JITTER_STACK_TOS_UNDER_UNDER_TOP(type, stack_container, name)          \
  (JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name) [-1])
#define JITTER_STACK_NTOS_UNDER_UNDER_TOP(type, stack_container, name)  \
  (JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name) [-2])

/* Expand to an expression evaluating to the element distance positions below
   the top of the given stack.  Undefined behavior on underflow.  The expression
   this macro expands to is an r-value.
   The distance should be a constant expression for good performance. */
#define JITTER_STACK_TOS_AT_DEPTH(type, stack_container, name, distance)        \
  ((distance == 0)                                                              \
   ? (JITTER_STACK_TOS_TOP(type, stack_container, name))                        \
   : JITTER_STACK_TOS_AT_NONZERO_DEPTH(type, stack_container, name, distance))
#define JITTER_STACK_NTOS_AT_DEPTH(type, stack_container, name, distance)    \
  JITTER_STACK_NTOS_AT_NONZERO_DEPTH(type, stack_container, name, distance)

/* Like JITTER_STACK_TOS_AT_DEPTH and JITTER_STACK_NTOS_AT_DEPTH, but assume
   that the distance is strictly positive.  This expands to better code when
   the distance is known to be non-zero but its actual value is not known. */
#define JITTER_STACK_TOS_AT_NONZERO_DEPTH(type, stack_container, name,   \
                                          distance)                      \
  (JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name)  \
     [1 - (int) (distance)])
#define JITTER_STACK_NTOS_AT_NONZERO_DEPTH(type, stack_container, name,  \
                                           distance)                     \
  (JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name)       \
     [- (int) (distance)])

/* Expand to a statement updating an element the given stack at the given
   distance from the top (0 being the top, 1 being the undertop, and so on); the
   new element will be set to the result of the expansion of the given new element.
   Undefined behavior on underflow.
   The distance should be a constant expression for good performance. */
#define JITTER_STACK_TOS_SET_AT_DEPTH(type, stack_container, name, distance,  \
                                      new_element)                            \
  do                                                                          \
    {                                                                         \
      /* Keeping the distance in an unsigned variable might give a useful     \
         warning in some cases where the distance as supplied by the user is  \
         incorrect.  However the final array index computed based on the      \
         distance can be negative, so the expression evaluating it must not   \
         involve unsigned operands. */                                        \
      const unsigned _jitter_set_at_depth_distance_u = (distance);            \
      const int _jitter_set_at_depth_distance =                               \
        _jitter_set_at_depth_distance_u;                                      \
      if (_jitter_set_at_depth_distance == 0)                                 \
        JITTER_STACK_TOS_TOP_NAME(type, stack_container, name)                \
          = (new_element);                                                    \
      else                                                                    \
        JITTER_STACK_TOS_SET_AT_NONZERO_DEPTH(type, stack_container, name,    \
                                              _jitter_set_at_depth_distance,  \
                                              (new_element));                 \
    }                                                                         \
  while (false)
#define JITTER_STACK_NTOS_SET_AT_DEPTH(type, stack_container, name, distance,  \
                                       new_element)                            \
  do                                                                           \
    {                                                                          \
      (JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name)         \
          [- (int) (distance)]) = (new_element);                               \
    }                                                                          \
  while (false)

/* Like JITTER_STACK_TOS_SET_AT_DEPTH and JITTER_STACK_NTOS_SET_AT_DEPTH, but
   assume that the distance is strictly positive.  This expands to better code
   when the distance is known to be non-zero but its actual value is not
   known. */
#define JITTER_STACK_TOS_SET_AT_NONZERO_DEPTH(type, stack_container, name,   \
                                              distance, new_element)         \
  do                                                                         \
    {                                                                        \
      (JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name)  \
          [1 - (int) (distance)]) = (new_element);                           \
    }                                                                        \
  while (false)
#define JITTER_STACK_NTOS_SET_AT_NONZERO_DEPTH(type, stack_container, name,  \
                                               distance, new_element)        \
  JITTER_STACK_NTOS_SET_AT_DEPTH(type, stack_container, name, distance,      \
                                 new_element)

/* Expand to a statement pushing an unspecified object on top of the given
   stack.  Undefined behavior on overflow.  There is no result.  This operation
   is more efficient than JITTER_STACK_*_PUSH , and is preferred when the top
   element is about to be replaced by another operation before it is first
   used. */
#define JITTER_STACK_TOS_PUSH_UNSPECIFIED(type, stack_container, name)  \
  do                                                                    \
    {                                                                   \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container,    \
                                              name) [1]                 \
        = JITTER_STACK_TOS_TOP_NAME(type, stack_container, name);       \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container,    \
                                              name) ++;                 \
      /* JITTER_STACK_TOS_TOP_NAME(type, stack_container, name) is */   \
      /* left with its previous value, which happens to be equal to */  \
      /* the new under-top -- but this might change in the future. */   \
    }                                                                   \
  while (false)
/* #define JITTER_STACK_TOS_PUSH_UNSPECIFIED(type, stack_container, name)      \ */
/*   do                                                                        \ */
/*     {                                                                       \ */
/*       * (++ JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container,  \ */
/*                                                     name))                  \ */
/*         = JITTER_STACK_TOS_TOP_NAME(type, stack_container, name);           \ */
/*       /\* JITTER_STACK_TOS_TOP_NAME(type, stack_container, name) is *\/       \ */
/*       /\* left with its previous value, which happens to be equal to the *\/  \ */
/*       /\* new under-top -- but this might change in the future. *\/           \ */
/*    }                                                                        \ */
/*   while (false) */
/* For some reason this version generates one instruction too many on MIPS,
   and x86_64, but not on PowerPC:
   MIPS:
# 0x436a30: stackpushunspecified (12 bytes):
7676b04c 26e20004       addiu $2,$23,4
7676b050 aefe0004       sw $30,4($23)
7676b054 0040b825       or $23,$2,$0
   x86_64:
# 0xef01f0: stackpushunspecified (13 bytes):
00007fd95b54c045 4d 8d 44 24 08 leaq 0x8(%r12),%r8
00007fd95b54c04a 49 89 6c 24 08 movq %rbp,0x8(%r12)
00007fd95b54c04f 4d 89 c4       movq %r8,%r12
   PowerPC:
# 0x10033d18: stackpushunspecified (8 bytes):
f678c040 92 af 00 04    stw r21,4(r15)
f678c044 39 ef 00 04    addi r15,r15,4
  This seems to be the same problem affecting the commented-out version of
  JITTER_STACK_TOS_DROP .  I might be forgetting a restrict qualifier
  somewhere.  But why is PowerPC immune to it?  By the way, the uncommented
  definition above is better on PowerPC as well, as it uses a post-incrementing
  store-word-and-update instruction.  Why is this version different?
  Tested on GCC 8.0.0 20170430 , the same on every architecture. */
#define JITTER_STACK_NTOS_PUSH_UNSPECIFIED(type, stack_container, name)    \
  do                                                                       \
    {                                                                      \
      JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name) ++;  \
    }                                                                      \
  while (false)

/* Expand to a statement pushing the given new element on top of the given
   stack; the new element expression is evaluated only once.  Undefined behavior
   on overflow.  There is no result. */
#define JITTER_STACK_TOS_PUSH(type, stack_container, name, new_element)  \
  do                                                                     \
    {                                                                    \
      const type _jitter_stack_new_element_temp = (new_element);         \
      JITTER_STACK_TOS_PUSH_UNSPECIFIED(type, stack_container, name);    \
      JITTER_STACK_TOS_TOP_NAME(type, stack_container, name)             \
        = _jitter_stack_new_element_temp;                                \
    }                                                                    \
  while (false)
#define JITTER_STACK_NTOS_PUSH(type, stack_container, name, new_element)  \
  do                                                                      \
    {                                                                     \
      const type _jitter_stack_new_element_temp = (new_element);          \
      (JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name))   \
         [1] = _jitter_stack_new_element_temp;                            \
      JITTER_STACK_NTOS_PUSH_UNSPECIFIED(type, stack_container, name);    \
    }                                                                     \
  while (false)

/* Expand to a statement pushing the given new element *under* the top of the
   given stack, moving the current top one position up.
   The stack effect of
     underpush b
   is
     ( a -- b a )
   The new element expression is evaluated only once.  Undefined behavior on
   overflow.  There is no result.
   Rationale:  underpush b  has the same behavious of  push b; swap  but is
   much more efficient (in fact, even more efficient than push alone) with a
   TOS-optimised stack.  The non-TOS-optimised case will not be particularly
   efficient. */
#define JITTER_STACK_TOS_UNDER_PUSH(type, stack_container, name, new_element)  \
  do                                                                           \
    {                                                                          \
      const type _jitter_stack_new_element_temp = (new_element);               \
      JITTER_STACK_TOS_UNDER_PUSH_UNSPECIFIED(type, stack_container, name);    \
      JITTER_STACK_TOS_UNDER_TOP(type, stack_container, name)                  \
        = _jitter_stack_new_element_temp;                                      \
    }                                                                          \
  while (false)
#define JITTER_STACK_NTOS_UNDER_PUSH(type, stack_container, name, new_element)  \
  do                                                                            \
    {                                                                           \
      const type _jitter_stack_new_element_temp = (new_element);                \
      JITTER_STACK_NTOS_UNDER_PUSH_UNSPECIFIED(type, stack_container, name);    \
      JITTER_STACK_NTOS_UNDER_TOP(type, stack_container, name)                  \
        = _jitter_stack_new_element_temp;                                       \
    }                                                                           \
  while (false)

/* Like underpush, but the value of the new element (right below the top after
   the macro execution) is undefined.  The stack effect
   of
     underpush-unspecified
   is
     ( a -- ? a )
   .  The TOS case is very efficient. */
#define JITTER_STACK_TOS_UNDER_PUSH_UNSPECIFIED(type, stack_container, name)    \
  do                                                                            \
    {                                                                           \
      /* Increment the under-top pointer; the top remains as it is; the new     \
         element below the top is left unitialised. */                          \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name) ++;  \
    }                                                                           \
  while (false)
#define JITTER_STACK_NTOS_UNDER_PUSH_UNSPECIFIED(type, stack_container, name)  \
  do                                                                           \
    {                                                                          \
      /* Much more painful than the TOS case: load old top, increment, store   \
         new top; and the store part depends on the load part. */              \
      const type _jitter_stack_old_top_temp                                    \
        = * JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name);   \
      JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name) ++;      \
      * JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name)        \
       = _jitter_stack_old_top_temp;                                           \
    }                                                                          \
  while (false)


/* A stack height should be treated like an abstract type, only to be used with
   JITTER_STACK_*_HEIGHT and JITTER_STACK_*_SET_HEIGHT .  It represents the
   "height" of a stack, which is to say the distance from its bottom.  Given a
   stack it is possible to obtain its height---and then to restore the same
   height on the same (and only the same) stack, which restores its complete
   state including the top element.
   It is safe, and supported, to compare two jitter_stack_height objects for
   equality, in order to check whether one given stack has kept the same height
   in different program states; this may be useful as a form of defensive
   programming, perhaps in an assertion.  Heights taken from *distinct* stacks
   will always compare as different.
   Implementation note: the actual saved value is currently a pointer to the
   undertop element for a TOS-optimized stack, and a pointer to the top element
   for a non-TOS-optimized stack. */
typedef void * restrict
jitter_stack_height;

/* Return the height of the given stack as a jitter_stack_height object.  The
   result can be used in a JITTER_STACK_*_SET_HEIGHT operation. */
#define JITTER_STACK_TOS_HEIGHT(type, stack_container, name)                \
  ((const jitter_stack_height)                                              \
   (JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name)))
#define JITTER_STACK_NTOS_HEIGHT(type, stack_container, name)        \
  ((const jitter_stack_height)                                       \
   JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name))

/* Restore the height of a stack to the given value, which must have been saved
   in advance.  A height should always be used to pop elements, never to push
   them back if the user wishes to reconstruct the state saved beefore in a
   consistent and deterministic way. */
#define JITTER_STACK_TOS_SET_HEIGHT(type, stack_container, name, height)        \
  do                                                                            \
    {                                                                           \
      const jitter_stack_height _jitter_new_height = (height);                  \
      const jitter_stack_height _jitter_old_height                              \
        = JITTER_STACK_TOS_HEIGHT(type, stack_container, name);                 \
      if (__builtin_expect (_jitter_old_height != _jitter_new_height,           \
                            true))                                              \
        {                                                                       \
          JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name)  \
            = _jitter_new_height;                                               \
          JITTER_STACK_TOS_TOP_NAME(type, stack_container, name)                \
            = JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container,    \
                                                      name) [1];                \
        }                                                                       \
    }                                                                           \
  while (false)
#define JITTER_STACK_NTOS_SET_HEIGHT(type, stack_container, name, height)  \
  do                                                                       \
    {                                                                      \
      JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name)      \
        = (const jitter_stack_height) (height);                            \
    }                                                                      \
  while (false)




/* Forth-style stack operations.
 * ************************************************************************** */

/* Push another copy of the top element to the stack; this expands to a
   statement, and there is no result.  Undefined behavior on overflow.  This
   operation is called "dup" in Forth. */
#define JITTER_STACK_TOS_DUP(type, stack_container, name)                       \
  do                                                                            \
    {                                                                           \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name) [1]  \
        = JITTER_STACK_TOS_TOP_NAME(type, stack_container, name);               \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name) ++;  \
    }                                                                           \
  while (false)
/* #define JITTER_STACK_TOS_DUP(type, stack_container, name)                   \ */
/*   do                                                                        \ */
/*     {                                                                       \ */
/*       * (++ JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container,  \ */
/*                                                     name))                  \ */
/*         = JITTER_STACK_TOS_TOP_NAME(type, stack_container, name);           \ */
/*     }                                                                       \ */
/*   while (false) */
/* This version generates one instruction too many on MIPS, x86_64 and Aarch64
   but is good on PowerPC, ARM and SH.  Why?
   x86_64:
     # 0x10791f0: stackdup (11 bytes):
     00007fa1f6a3205b 49 8d 7e 08  leaq 0x8(%r14),%rdi
     00007fa1f6a3205f 4d 89 6e 08  movq %r13,0x8(%r14)
     00007fa1f6a32063 49 89 fe     movq %rdi,%r14
   MIPS:
     # 0x434a30: stackdup (12 bytes):
     7676b06c 26af0004     addiu $15,$21,4
     7676b070 aeb70004     sw $23,4($21)
     7676b074 01e0a825     or $21,$15,$0
   Aarch64:
     # 0x42f1f0: stackdup (12 bytes):
     00000040009ac050 91002345     add x5, x26, #0x8
     00000040009ac054 f900075b     str x27, [x26,#8]
     00000040009ac058 aa0503fa     mov x26, x5
   PowerPC:
     # 0x10032d18: stackdup (8 bytes):
     f678c05c 92 11 00 04  stw r16,4(r17)
     f678c060 3a 31 00 04  addi r17,r17,4
   ARM:
     # 0x3ba30: stackdup (8 bytes):
     f6661050 e5876004     str r6, [r7, #4]
     f6661054 e2877004     add r7, r7, #4
   SH:
     # 0x423a30: stackdup (4 bytes):
     f664e034 e1 1b        mov.l r14,@(4,r11)
     f664e036 04 7b        add #4,r11
  The uncommented version is better even on PowerPC and ARM, using a single
  post-incrementing store, like on Aarch64 which goes from three down to one.
  Tested on GCC 8.0.0 20170430 , the same on every architecture. */
#define JITTER_STACK_NTOS_DUP(type, stack_container, name)                     \
  do                                                                           \
    {                                                                          \
      const type _jitter_stack_top_element_copy                                \
        = * (JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name)); \
      * (++ JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name))   \
        = _jitter_stack_top_element_copy;                                      \
    }                                                                          \
  while (false)

/* Remove the top element from the stack; this expands to a statement, and there
   is no result.  Undefined behavior on empty stack.  This operation is called
   "drop" in Forth. */
#define JITTER_STACK_TOS_DROP(type, stack_container, name)             \
  do                                                                   \
    {                                                                  \
      /* Load the under-top element into the top. */                   \
      JITTER_STACK_TOS_TOP_NAME (type, stack_container, name)          \
        = JITTER_STACK_TOS_UNDER_TOP (type, stack_container, name);    \
      /* Decrement the under-top pointer. */                           \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type,                    \
                                              stack_container,         \
                                              name) --;                \
    }                                                                  \
  while (false)
/* #define JITTER_STACK_TOS_DROP(type, stack_container, name)             \ */
/*   do                                                                   \ */
/*     {                                                                  \ */
/*       JITTER_STACK_TOS_TOP_NAME(type, stack_container, name)           \ */
/*         = * (JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type,             \ */
/*                                                      stack_container,  \ */
/*                                                      name) --);        \ */
/*     }                                                                  \ */
/*   while (false) */
/* FIXME: why is this version generating three instructions?
   This yields (MIPS):
      # 0x436a00: stackdrop (12 bytes):
      7676b050 26eefffc addiu $14,$23,-4
      7676b054 8efe0000 lw $30,0($23)
      7676b058 01c0b825 or $23,$14,$0
   or (PowerPC):
      # 0x10033ce8: stackdrop (12 bytes):
      f678c040 38 af ff fc      addi r5,r15,-4
      f678c044 82 af 00 00      lwz r21,0(r15)
      f678c048 7c af 2b 78      mr r15,r5
   or (x86_64):
      # 0x15c11f0: stackdrop (12 bytes):
      00007fc8726cf04a 49 8d 54 24 f8   leaq -0x8(%r12),%rdx
      00007fc8726cf04f 49 8b 2c 24      movq (%r12),%rbp
      00007fc8726cf053 49 89 d4 movq %rdx,%r12
   or (SH -- here one of the four instructions is justified by SH's two-operand
       arithmetic, but the problem of keeping two different pointers is still
       the same):
      # 0x423a30: stackdrop (8 bytes):
      f664e02e b3 63    mov r11,r3
      f664e030 b2 6e    mov.l @r11,r14
      f664e032 fc 73    add #-4,r3
      f664e034 33 6b    mov r3,r11
  The uncommented version above does the right thing, yielding two
  instructions (or just one on Aarch64 and ARM, using a post-decrementing load).
  The under-top pointer is declared restrict , so GCC cannot assume
  that the under-top pointer may alias the top; and in fact the top is
  correctly held in a register, even with this three-instruction version.
  Tested on GCC 8.0.0 20170430 , the same on every architecture. */
#define JITTER_STACK_NTOS_DROP(type, stack_container, name)                \
  do                                                                       \
    {                                                                      \
      JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name) --;  \
    }                                                                      \
  while (false)

/* Push a copy of the under-top element.  Undefined behavior on empty
   stack.  This operation is called "over" in Forth.
   I currently have two variants of the TOS version, one generating
   shorter code and not using temporary registers, but with a
   shorter-distance memory dependency; the other version generates
   one more instruction and uses a register, but behaves better with
   respect to dependencies.  FIXME: benchmark, and possibly choose
   differently according to the configuration. */
#define JITTER_STACK_TOS_OVER_LONGER(type, stack_container, name)   \
  do                                                                \
    {                                                               \
      const type _jitter_stack_over_under_top_temp                  \
        = JITTER_STACK_TOS_UNDER_TOP(type, stack_container, name);  \
      JITTER_STACK_TOS_PUSH(type, stack_container, name,            \
                            _jitter_stack_over_under_top_temp);     \
    }                                                               \
  while (false)
#define JITTER_STACK_TOS_OVER_SHORTER(type, stack_container, name)              \
  do                                                                            \
    {                                                                           \
      /* Name the old top as a temporary.  This does not require a load, but    \
         having a separate temporary read early might help the compiler with    \
         alias analysis. */                                                     \
      const type _jitter_stack_over_top_old                                     \
        = JITTER_STACK_TOS_TOP (type, stack_container, name);                   \
      /* Write the old top into memory, at the over-under-top position, which   \
         means at the top. */                                                   \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME (type, stack_container, name) [1] \
        = _jitter_stack_over_top_old;                                           \
      /* Read the old under-top, which we have not touched, into the top. */    \
      JITTER_STACK_TOS_TOP (type, stack_container, name)                        \
        = JITTER_STACK_TOS_UNDER_TOP (type, stack_container, name);             \
      /* Increment the under-top stack pointer. */                              \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME (type, stack_container, name) ++; \
    }                                                                           \
  while (false)
#define JITTER_STACK_TOS_OVER(type, stack_container, name)                \
  /* FIXME: benchmark and choose either JITTER_STACK_TOS_OVER_SHORTER or  \
     JITTER_STACK_TOS_OVER_LONGER . */                                    \
  JITTER_STACK_TOS_OVER_SHORTER (type, stack_container, name)
#define JITTER_STACK_NTOS_OVER(type, stack_container, name)                     \
  do                                                                            \
    {                                                                           \
      const type _jitter_stack_old_under_top_element_copy                       \
        = JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name) [-1]; \
      * (++ JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name))    \
        = _jitter_stack_old_under_top_element_copy;                             \
    }                                                                           \
  while (false)

/* Insert a copy of the top element as the new under-under-top element, moving
   the under-top and top elements one position up each; after the execution of
   the operation the stack will be one element taller.  Undefined behavior on
   underflow.  This operation is called "tuck" in Forth, and has stack effect
   ( a b -- b a b ). */
#define JITTER_STACK_TOS_TUCK(type, stack_container, name)                      \
  do                                                                            \
    {                                                                           \
      /* Do not disturb the top, which has already the correct value for the    \
         final state.  Instead load a copy of the under-top element, whose      \
         memory content will need to change. */                                 \
      const type _jitter_stack_tuck_under_top_old                               \
        = JITTER_STACK_TOS_UNDER_TOP (type, stack_container, name);             \
      /* Change the stack height.  From now on we can think about stack         \
         elements in the positions they will occupy in the final state. */      \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME (type, stack_container, name) ++; \
      /* We have to change both the under-under-top and the under-top elements, \
         which now are sure to have values different from the ones in the final \
         state. */                                                              \
      JITTER_STACK_TOS_UNDER_UNDER_TOP (type, stack_container, name)            \
        = JITTER_STACK_TOS_TOP (type, stack_container, name);                   \
      JITTER_STACK_TOS_UNDER_TOP (type, stack_container, name)                  \
        = _jitter_stack_tuck_under_top_old;                                     \
    }                                                                           \
  while (false)
#define JITTER_STACK_NTOS_TUCK(type, stack_container, name)                 \
  do                                                                        \
    {                                                                       \
      /* This version will not be as nice and fast as the TOS case.  The    \
         top three elements in the stack all need to change. */             \
      /* Load the current under-top and top. */                             \
      const type _jitter_stack_tuck_under_top_old                           \
        = JITTER_STACK_NTOS_UNDER_TOP (type, stack_container, name);        \
      const type _jitter_stack_tuck_top_old                                 \
        = JITTER_STACK_NTOS_TOP (type, stack_container, name);              \
      /* Change the stack height.  The height will be what we need in the   \
         final state after this. */                                         \
      JITTER_STACK_NTOS_TOP_POINTER_NAME (type, stack_container, name) ++;  \
      /* Store the changed elements. */                                     \
      JITTER_STACK_NTOS_UNDER_UNDER_TOP (type, stack_container, name)       \
        = _jitter_stack_tuck_top_old;                                       \
      JITTER_STACK_NTOS_UNDER_TOP (type, stack_container, name)             \
        = _jitter_stack_tuck_under_top_old;  /* The height has changed. */  \
      JITTER_STACK_NTOS_TOP (type, stack_container, name)                   \
        = _jitter_stack_tuck_top_old;  /* Again, not the same height. */    \
    }                                                                       \
  while (false)

/* Swap the top and under-top element on the stack; this expands to a statement,
   and there is no result.  Undefined behavior on underflow.  This operation is
   called "swap" in Forth. */
#define JITTER_STACK_TOS_SWAP(type, stack_container, name)          \
  do                                                                \
    {                                                               \
      const type _jitter_stack_swap_under_top_temp                  \
        = JITTER_STACK_TOS_UNDER_TOP(type, stack_container, name);  \
      JITTER_STACK_TOS_UNDER_TOP(type, stack_container, name)       \
        = JITTER_STACK_TOS_TOP(type, stack_container, name);        \
      JITTER_STACK_TOS_TOP(type, stack_container, name)             \
        = _jitter_stack_swap_under_top_temp;                        \
    }                                                               \
  while (false)
#define JITTER_STACK_NTOS_SWAP(type, stack_container, name)          \
  do                                                                 \
    {                                                                \
      const type _jitter_stack_swap_top_temp                         \
        = JITTER_STACK_NTOS_TOP(type, stack_container, name);        \
      const type _jitter_stack_swap_under_top_temp                   \
        = JITTER_STACK_NTOS_UNDER_TOP(type, stack_container, name);  \
      JITTER_STACK_NTOS_UNDER_TOP(type, stack_container, name)       \
        = _jitter_stack_swap_top_temp;                               \
      JITTER_STACK_NTOS_TOP(type, stack_container, name)             \
        = _jitter_stack_swap_under_top_temp;                         \
    }                                                                \
  while (false)

/* Swap the under-top and the under-under-top elements on the stack; this
   expands to a statement, and there is no result.  Undefined behavior on
   underflow.  This operation does not exist in Forth, but following its
   naming pattern I would call it "quake"; the metaphore being an earth
   movement right below the surface.
   The stack effect is ( a b c -- b a c ). */
#define JITTER_STACK_TOS_QUAKE(type, stack_container, name)               \
  do                                                                      \
    {                                                                     \
      const type _jitter_stack_quake_under_under_top_old                  \
        = JITTER_STACK_TOS_UNDER_UNDER_TOP(type, stack_container, name);  \
      const type _jitter_stack_quake_under_top_old                        \
        = JITTER_STACK_TOS_UNDER_TOP(type, stack_container, name);        \
      JITTER_STACK_TOS_UNDER_UNDER_TOP(type, stack_container, name)       \
        = _jitter_stack_quake_under_top_old;                              \
      JITTER_STACK_TOS_UNDER_TOP(type, stack_container, name)             \
        = _jitter_stack_quake_under_under_top_old;                        \
    }                                                                     \
  while (false)
#define JITTER_STACK_NTOS_QUAKE(type, stack_container, name)               \
  do                                                                       \
    {                                                                      \
      const type _jitter_stack_quake_under_under_top_old                   \
        = JITTER_STACK_NTOS_UNDER_UNDER_TOP(type, stack_container, name);  \
      const type _jitter_stack_quake_under_top_old                         \
        = JITTER_STACK_NTOS_UNDER_TOP(type, stack_container, name);        \
      JITTER_STACK_NTOS_UNDER_UNDER_TOP(type, stack_container, name)       \
        = _jitter_stack_quake_under_top_old;                               \
      JITTER_STACK_NTOS_UNDER_TOP(type, stack_container, name)             \
        = _jitter_stack_quake_under_under_top_old;                         \
    }                                                                      \
  while (false)

/* Remove the under-top element from the stack, without affecting the top; this
   expands to a statement, and there is no result.  Undefined behavior on an
   empty or one-element stack.  This operation is called "nip" in Forth. */
#define JITTER_STACK_TOS_NIP(type, stack_container, name)                      \
  do                                                                           \
    {                                                                          \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name) --; \
    }                                                                          \
  while (false)
#define JITTER_STACK_NTOS_NIP(type, stack_container, name)                    \
  do                                                                          \
    {                                                                         \
      const type _jitter_stack_nip_top_temp                                   \
        = JITTER_STACK_NTOS_TOP(type, stack_container, name);                 \
      * (-- JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name))  \
        = _jitter_stack_nip_top_temp;                                         \
    }                                                                         \
  while (false)

/* Expand to a statement rearranging the topmost three elements of the stack
   so that their configuration (top on the right) changes from
     a b c
   to
     b c a
   .  Undefined behavior on underflow.  This operation is called "rot" in
   Forth. */
#define JITTER_STACK_TOS_ROT(type, stack_container, name)  \
  JITTER_STACK_TOS_ROLL (type, stack_container, name, 2)
#define JITTER_STACK_NTOS_ROT(type, stack_container, name)  \
  JITTER_STACK_NTOS_ROLL (type, stack_container, name, 2)

/* Expand to a statement rearranging the topmost three elements of the stack
   so that their configuration (top on the right) changes from
     a b c
   to
     c a b
   .  Undefined behavior on underflow.  This operation is called "-rot" in
   Forth. */
#define JITTER_STACK_TOS_MROT(type, stack_container, name)  \
  JITTER_STACK_TOS_MROLL (type, stack_container, name, 2)
#define JITTER_STACK_NTOS_MROT(type, stack_container, name)  \
  JITTER_STACK_NTOS_MROLL (type, stack_container, name, 2)




/* Potentially inefficient stack operations.
 * ************************************************************************** */

/* The stack operations defined in this section may expand to inefficient code
   and are not recommended for production use, unless the arguments are known
   small constants.  Anyway they are convenient for testing. */


/* Expand to a statement rearranging the topmost roll_depth + 1 elements of
   the given stack so that the element originally at depth roll_depth goes to
   top, and every element originally above it is moved down one position to
   make place.
   For example, passing roll_depth with a value of 3 turns
     a b c d e f
   into
     a b d e f c
   .  In Forth the depth argument is taken from the stack.  Here
     JITTER_STACK_TOS_ROLL(type, stack_container, name, N)
   behaves like
     N roll
   in Forth. 
   Just like in Forth, roll is a potentially inefficient operation, as it
   requires O(roll_depth) memory accesses.  In production it should only be
   used with small and constant values of roll_depth . */
#define JITTER_STACK_TOS_ROLL(type, stack_container, name, roll_depth)          \
  do                                                                            \
    {                                                                           \
      /* Use an unsigned variable to get a warning in case the user passes a    \
         negative constant by mistake.  Then convert to int, so that we may     \
         safely compute negative indices from other index operands, as it may   \
         be needed fo termination conditions. */                                \
      unsigned jitter_roll_depthu = (roll_depth);                               \
      int jitter_roll_depth = jitter_roll_depthu;                               \
      if (jitter_roll_depth == 0)                                               \
        break;                                                                  \
      /* Slide values down from depth i to i + 1.  Keep the next value to be    \
         written in the temporary jitter_roll_old , initialized with the top    \
         out of the loop. */                                                    \
      type jitter_roll_old                                                      \
        = JITTER_STACK_TOS_TOP (type, stack_container, name);                   \
      int jitter_roll_depth_i;                                                  \
      for (jitter_roll_depth_i = 1;                                             \
           jitter_roll_depth_i <= jitter_roll_depth;                            \
           jitter_roll_depth_i ++)                                              \
        {                                                                       \
          /* Before overwriting a value, save it: it will become the next       \
             jitter_roll_old .  Then perform the overwrite, and update          \
             jitter_roll_old .  Here I can afford the faster _AT_NONZERO_DEPTH  \
             macros, as the only case dealing with the top is handled out of    \
             the loop. */                                                       \
          const type jitter_roll_to_be_overwritten                              \
            = JITTER_STACK_TOS_AT_NONZERO_DEPTH (type, stack_container, name,   \
                                                 jitter_roll_depth_i);          \
          JITTER_STACK_TOS_SET_AT_NONZERO_DEPTH(type, stack_container, name,    \
                                                jitter_roll_depth_i,            \
                                                jitter_roll_old);               \
          jitter_roll_old = (type) jitter_roll_to_be_overwritten;               \
        }                                                                       \
      /* Now after the loop jitter_roll_old has the old value of the deepest    \
         value.  That goes to the top. */                                       \
      JITTER_STACK_TOS_TOP (type, stack_container, name) = jitter_roll_old;     \
    }                                                                           \
  while (false)
#define JITTER_STACK_NTOS_ROLL(type, stack_container, name, roll_depth)         \
  do                                                                            \
    {                                                                           \
      /* Use an unsigned variable to get a warning in case the user passes a    \
         negative constant by mistake.  Then convert to int, so that we may     \
         safely compute negative indices from other index operands, as it may   \
         be needed fo termination conditions. */                                \
      unsigned jitter_roll_depthu = (roll_depth);                               \
      int jitter_roll_depth = jitter_roll_depthu;                               \
      /* Do nothing if the deepest element to be affected is at depth zero. */  \
      if (jitter_roll_depth == 0)                                               \
        break;                                                                  \
      /* Slide values down from depth i to i + 1.  Keep the next value to be    \
         written in the temporary jitter_roll_old , initialized with the top    \
         out of the loop. */                                                    \
      type jitter_roll_old                                                      \
        = JITTER_STACK_NTOS_TOP (type, stack_container, name);                  \
      int jitter_roll_depth_i;                                                  \
      for (jitter_roll_depth_i = 1;                                             \
           jitter_roll_depth_i <= jitter_roll_depth;                            \
           jitter_roll_depth_i ++)                                              \
        {                                                                       \
          /* Before overwriting a value, save it: it will become the next       \
             jitter_roll_old .  Then perform the overwrite, and update          \
             jitter_roll_old . */                                               \
          const type jitter_roll_to_be_overwritten                              \
            = JITTER_STACK_NTOS_AT_DEPTH (type, stack_container, name,          \
                                          jitter_roll_depth_i);                 \
          JITTER_STACK_NTOS_SET_AT_DEPTH(type, stack_container, name,           \
                                         jitter_roll_depth_i,                   \
                                         jitter_roll_old);                      \
          jitter_roll_old = (type) jitter_roll_to_be_overwritten;               \
        }                                                                       \
      /* Now after the loop jitter_roll_old has the old value of the deepest    \
         value.  That goes to the top. */                                       \
      JITTER_STACK_NTOS_TOP (type, stack_container, name) = jitter_roll_old;    \
    }                                                                           \
  while (false)

/* Expand to a statement performing a stack change similar in spirit to the roll
   operation, but rearranging the stack elements the opposite way: the old top
   becomes the deepest element, and all the elements originally below it are
   moved up by one position to make place.
   For example, given that roll (3) turns
     a b c d e f
   into
     a b d e f c
   , -roll (3) turns
     a b c d e f
   into
     a b f c d e
   .  This "-roll" is not actually a predefined operation in Forth, but has its
   place as a sensible variant of roll.
   This "-roll" is to "roll" like "-rot" is to "rot". */
#define JITTER_STACK_TOS_MROLL(type, stack_container, name, mroll_depth)        \
  do                                                                            \
    {                                                                           \
      /* Use an unsigned variable to get a warning in case the user passes a    \
         negative constant by mistake.  Then convert to int, so that we may     \
         safely compute negative indices from other index operands, as it may   \
         be needed fo termination conditions. */                                \
      unsigned jitter_mroll_depthu = (mroll_depth);                             \
      int jitter_mroll_depth = jitter_mroll_depthu;                             \
      if (jitter_mroll_depth == 0)                                              \
        break;                                                                  \
      /* Slide values up from depth i to i - 1.  Keep the next value to be      \
         written in the temporary jitter_roll_old , initialized with the top    \
         out of the loop.  Compared to roll, I iterate in the opposite          \
         direction, so that here again the old top is the first value to be     \
         read. */                                                               \
      type jitter_mroll_old                                                     \
        = JITTER_STACK_TOS_TOP (type, stack_container, name);                   \
      int jitter_mroll_depth_i;                                                 \
      for (jitter_mroll_depth_i = jitter_mroll_depth;                           \
           jitter_mroll_depth_i > 0; /* Differently from the TOS case this      \
                                        needs to be a strict > . */             \
           jitter_mroll_depth_i --)                                             \
        {                                                                       \
          /* Before overwriting a value, save it: it will become the next       \
             jitter_mroll_old .  Then perform the overwrite, and update         \
             jitter_mroll_old . */                                              \
          const type jitter_mroll_to_be_overwritten                             \
            = JITTER_STACK_TOS_AT_NONZERO_DEPTH (type, stack_container, name,   \
                                                 jitter_mroll_depth_i);         \
          JITTER_STACK_TOS_SET_AT_NONZERO_DEPTH (type, stack_container, name,   \
                                                 jitter_mroll_depth_i,          \
                                                 jitter_mroll_old);             \
          jitter_mroll_old = (type) jitter_mroll_to_be_overwritten;             \
        }                                                                       \
      /* Differently from the non-TOS case, here the final update must be out   \
         of the loop: this is the only case where _AT_NONZERO_DEPTH macros      \
         would not work. */                                                     \
      JITTER_STACK_TOS_TOP (type, stack_container, name) = jitter_mroll_old;    \
    }                                                                           \
  while (false)
#define JITTER_STACK_NTOS_MROLL(type, stack_container, name, mroll_depth)       \
  do                                                                            \
    {                                                                           \
      /* Use an unsigned variable to get a warning in case the user passes a    \
         negative constant by mistake.  Then convert to int, so that we may     \
         safely compute negative indices from other index operands, as it may   \
         be needed fo termination conditions. */                                \
      unsigned jitter_mroll_depthu = (mroll_depth);                             \
      int jitter_mroll_depth = jitter_mroll_depthu;                             \
      /* Do nothing if the deepest element to be affected is at depth zero. */  \
      if (jitter_mroll_depth == 0)                                              \
        break;                                                                  \
      /* Slide values up from depth i to i - 1.  Keep the next value to be      \
         written in the temporary jitter_roll_old , initialized with the top    \
         out of the loop.  Compared to roll, I iterate in the opposite          \
         direction, so that here again the old top is the first value to be     \
         read. */                                                               \
      type jitter_mroll_old                                                     \
        = JITTER_STACK_NTOS_TOP (type, stack_container, name);                  \
      int jitter_mroll_depth_i;                                                 \
      for (jitter_mroll_depth_i = jitter_mroll_depth;                           \
           jitter_mroll_depth_i > 0;  /* Here a guard with >= instead of >, and \
                                         no statement after the loop, would be  \
                                         correct, differenrtly from the TOS     \
                                         case.  But GCC 9 as of early 2019      \
                                         appears to do a better job at          \
                                         unrolling this way, without a load not \
                                         followed by a use of the loaded        \
                                         value. */                              \
           jitter_mroll_depth_i --)                                             \
        {                                                                       \
          /* Before overwriting a value, save it: it will become the next       \
             jitter_mroll_old .  Then perform the overwrite, and update         \
             jitter_mroll_old . */                                              \
          const type jitter_mroll_to_be_overwritten                             \
            = JITTER_STACK_NTOS_AT_DEPTH (type, stack_container, name,          \
                                          jitter_mroll_depth_i);                \
          JITTER_STACK_NTOS_SET_AT_DEPTH (type, stack_container, name,          \
                                          jitter_mroll_depth_i,                 \
                                          jitter_mroll_old);                    \
          jitter_mroll_old = (type) jitter_mroll_to_be_overwritten;             \
        }                                                                       \
      /* Now after the loop jitter_mroll_old has the old value of the deepest   \
         value.  That goes to the top.                                          \
         See the comment above: this would not be needed if the loop guard had  \
         a non-strict comparison. */                                            \
      JITTER_STACK_NTOS_TOP (type, stack_container, name) = jitter_mroll_old;   \
    }                                                                           \
  while (false)

/* Expand to a statement performing a stack change to delete a given number of
   non-top elements at the given depth.
   * The element_no argument is the number of elements to delete.  It must be
     non-negative;
   * the depth argument is the depth of the deepest element to delete.  It must
     be strictly positive.
   For good performance both arguments should be known at compile time.
   It is forbidden for this operation to change the top element: the metaphor of
   "sliding" is a landslide where some below-ground strata collapse, which cause
   a displacement of the surface layer down (potentially along with other layers)
   without changing the surface.
   After the operation is executed the stack will be element_no elements shorter.
   Undefined behavior on:
   * underflow;
   * top element write;
   * any over-the-top element access.
   For example
     slide 1 1
   is equivalent to nip, and has the stack effect
     ( a b -- b )
   .  Then
     slide 2 2
   is equivalent to nip nip, and has the stack effect
     ( a b c -- c )
   and
     slide 1 2
   has the stack effect
     ( b c d -- c d )
   This is a generalization of nip, fast when the arguments are known at compile
   time but potentially dangerous to use because of the possibility of accessing
   the top element or even elements above the top by mistake, which may lead to
   subtly incorrect results. */
#define JITTER_STACK_TOS_SLIDE(type, stack_container, name, element_no, depth)  \
  do                                                                            \
    {                                                                           \
      /* Use unsigned variables to get a warning in case the user passes        \
         negative constants by mistake. */                                      \
      unsigned int jitter_slide_element_no = (element_no);                      \
      unsigned int jitter_slide_depth = (depth);                                \
                                                                                \
      /* Slide values down starting from the deepest one, iterating towards     \
         the top.  Having the jitter_slide_source_depth unsigned might help     \
         catching some user bugs, ifthe value is negative and the compiler      \
         gives a warning. */                                                    \
      unsigned int jitter_slide_source_depth;                                   \
      for (jitter_slide_source_depth                                            \
             = jitter_slide_depth - jitter_slide_element_no;                    \
           /* The top is never modified, and the old under-top element is the   \
              shallowest element that can be read; instead the under-under-top  \
              element is the shallowest to be written. */                       \
           jitter_slide_source_depth != 0; /* !=: stop before the top. */       \
           jitter_slide_source_depth --)                                        \
        {                                                                       \
          unsigned int jitter_slide_target_depth                                \
            = jitter_slide_source_depth + jitter_slide_element_no;              \
          const type jitter_slide_source                                        \
            = JITTER_STACK_TOS_AT_NONZERO_DEPTH (type, stack_container, name,   \
                                                 jitter_slide_source_depth);    \
          JITTER_STACK_TOS_SET_AT_NONZERO_DEPTH (type, stack_container, name,   \
                                                 jitter_slide_target_depth,     \
                                                 jitter_slide_source);          \
        }                                                                       \
      /* Decrement the (under-top) stack pointer. */                            \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME(type, stack_container, name)      \
        -= jitter_slide_element_no;                                             \
      /* Notice that the top element has not been updated.  This TOS version    \
         will *always* leave it as it was, which satisfies the specification    \
         above but will lead to subtle bugs if this operation is used           \
         carelessly.*/                                                          \
    }                                                                           \
  while (false)
#define JITTER_STACK_NTOS_SLIDE(type, stack_container, name, element_no, depth) \
  do                                                                            \
    {                                                                           \
      /* Use unsigned variables to get a warning in case the user passes        \
         negative constants by mistake.  Then convert to signed types and use   \
         the signed versions which are more convenient later; we need a sign    \
         test in the loop guard. */                                             \
      unsigned int jitter_slide_element_nou = (element_no);                     \
      unsigned int jitter_slide_depthu = (depth);                               \
      int jitter_slide_element_no = jitter_slide_element_nou;                   \
      int jitter_slide_depth = jitter_slide_depthu;                             \
                                                                                \
      /* Slide values down starting from the deepest one, iterating towards     \
         the top. */                                                            \
      int jitter_slide_source_depth;                                            \
      for (jitter_slide_source_depth                                            \
             = jitter_slide_depth - jitter_slide_element_no;                    \
           /* In the non-TOS case the top element is the last to be read. */    \
           jitter_slide_source_depth >= 0;                                      \
           jitter_slide_source_depth --)                                        \
        {                                                                       \
          int jitter_slide_target_depth                                         \
            = jitter_slide_source_depth + jitter_slide_element_no;              \
          const type jitter_slide_source                                        \
            = JITTER_STACK_NTOS_AT_DEPTH (type, stack_container, name,          \
                                          jitter_slide_source_depth);           \
          JITTER_STACK_NTOS_SET_AT_NONZERO_DEPTH (type, stack_container, name,  \
                                                  jitter_slide_target_depth,    \
                                                  jitter_slide_source);         \
        }                                                                       \
      /* Decrement the top stack pointer. */                                    \
      JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name)           \
        -= jitter_slide_element_no;                                             \
    }                                                                           \
  while (false)

/* Duplicate a non-top element at the given depth, moving every shallower
   element up by one position.  At the end of the operation the stack becomes
   one position taller.
   Undefined behavior on:
   * underflow;
   * non-positive argument.
   For good performance the argument should be known at compile time.
   bulge 1 is equivalent to over swap, and has effect
     ( a b -- a a b )
   bulge 2 has effect
     ( a b c -- a a b c ) */
#define JITTER_STACK_TOS_BULGE(type, stack_container, name, depth)              \
  do                                                                            \
    {                                                                           \
      /* Use unsigned variables to get a warning in case the user passes a      \
         negative constant by mistake, or reaches a negative depth.  Also use   \
         unsigned variables for depths, so that user errors causing a           \
         wraparound are more catastrophic and therefore easier to see. */       \
      unsigned int jitter_bulge_depth_old = (depth);                            \
      /* The given depth is relative to the old state.  Immediately increment   \
         the (under-top) stack pointer, and only reason about depths in the     \
         new state. */                                                          \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME (type, stack_container, name) ++; \
      unsigned int jitter_bulge_max_source_depth = jitter_bulge_depth_old + 1;  \
      unsigned int jitter_bulge_max_target_depth                                \
        = jitter_bulge_max_source_depth - 1;                                    \
      /* No special handling is needed for the top in the TOS case, as it       \
         already has the correct value. */                                      \
      /* Iterate from shallow to deep positions, up to an update to the top     \
         element, *not included* (this is different from the TOS case).  This   \
         makes elements at the depth mentioned by the user or above but below   \
         the top slide up by one position.                                      \
         Notice the inconsistency in behavior with respect to a zero depth      \
         argument and the non-TOS case below: the TOS case with zero depth      \
         degenerates to push-unspecified rather than dup. */                    \
      unsigned int jitter_bulge_target_depth;                                   \
      for (jitter_bulge_target_depth = 1; /* Different from non-TOS. */         \
           jitter_bulge_target_depth <= jitter_bulge_max_target_depth;          \
           jitter_bulge_target_depth ++)                                        \
        {                                                                       \
          unsigned int jitter_bulge_source_depth                                \
            = jitter_bulge_target_depth + 1;                                    \
          JITTER_STACK_TOS_AT_NONZERO_DEPTH (type, stack_container, name,       \
                                             jitter_bulge_target_depth)         \
            = JITTER_STACK_TOS_AT_NONZERO_DEPTH (type, stack_container, name,   \
                                                 jitter_bulge_source_depth);    \
        }                                                                       \
    }                                                                           \
  while (false)
#define JITTER_STACK_NTOS_BULGE(type, stack_container, name, depth)            \
  do                                                                           \
    {                                                                          \
      /* Use unsigned variables to get a warning in case the user passes a     \
         negative constant by mistake, or reaches a negative depth.  Also use  \
         unsigned variables for depths, so that user errors causing a          \
         wraparound are more catastrophic and therefore easier to see. */      \
      unsigned int jitter_bulge_depth_old = (depth);                           \
      /* The given depth is relative to the old state.  Immediately increment  \
         the stack pointer and only reason about new-state depths. */          \
      JITTER_STACK_NTOS_TOP_POINTER_NAME (type, stack_container, name) ++;     \
      unsigned int jitter_bulge_max_source_depth = jitter_bulge_depth_old + 1; \
      unsigned int jitter_bulge_max_target_depth                               \
        = jitter_bulge_max_source_depth - 1;                                   \
      /* Iterate from shallow to deep positions, up to an update to the top    \
         element, included.  This makes elements at the depth mentioned by     \
         the user or above slide up by one position.                           \
         Notice that specifying a depth of 0, in the non-TOS case, degenerates \
         to dup.  This behavior must *not* be relied upon, as it is not        \
         consistent with the TOS case. */                                      \
      unsigned int jitter_bulge_target_depth;                                  \
      for (jitter_bulge_target_depth = 0;                                      \
           jitter_bulge_target_depth <= jitter_bulge_max_target_depth;         \
           jitter_bulge_target_depth ++)                                       \
        {                                                                      \
          unsigned int jitter_bulge_source_depth                               \
            = jitter_bulge_target_depth + 1;                                   \
          JITTER_STACK_NTOS_AT_DEPTH (type, stack_container, name,             \
                                      jitter_bulge_target_depth)               \
            = JITTER_STACK_NTOS_AT_DEPTH (type, stack_container, name,         \
                                          jitter_bulge_source_depth);          \
        }                                                                      \
    }                                                                          \
  while (false)

/* Exchange the top element with a deeper element, deleting the element_no
   elements in between.
   Undefined behavior on:
   * underflow;
   * negative element_no.
   This is fast when the argument is known at compile time.
   The metaphor is two objects dancing around a central pivot exchanging their
   places, and at the same time squashing the pivot.
   The stack effect for whirl 1 is
     ( a b c -- c a )
   , equivalent to
     ( a b c ) nip ( a c ) swap ( c a ).
   For whirl 2 the effect will be
     ( a b c d -- d a )
   .  The case of whirl 0 degenerates to swap:
     ( a b -- b a ) */
#define JITTER_STACK_TOS_WHIRL(type, stack_container, name, element_no)        \
  do                                                                           \
    {                                                                          \
      /* As long as the argument is a known constant this is just an typical   \
         exchange between a memory location and a register, followed by a      \
         stack increment. */                                                   \
      /* Use an unsigned variable to get a warning in case the user passes     \
         a negative constant by mistake. */                                    \
      unsigned int _jitter_whirl_element_no = (element_no);                    \
      /* The stack depth of the deepest element to be touched, in the old      \
         state. */                                                             \
      unsigned int _jitter_whirl_deepest_depth_old                             \
        = _jitter_whirl_element_no + 1;                                        \
      /* Name the two source elements.  This will only cost one load, since    \
         there is no need to modify the top-element register until near the    \
         end. */                                                               \
      const type _jitter_stack_swirl_deepest_old                               \
        = JITTER_STACK_TOS_AT_NONZERO_DEPTH (type, stack_container, name,      \
                                             _jitter_whirl_deepest_depth_old); \
      const type _jitter_stack_swirl_top_old                                   \
        = JITTER_STACK_TOS_TOP (type, stack_container, name);                  \
      /* Write the two target elements and adjust the stack (under-top)        \
         pointer.  The deepest element is always in memory, at the same        \
         address where the old version of it was.  This will cost one store,   \
         one register copy, plus the register increment. */                    \
      JITTER_STACK_TOS_SET_AT_NONZERO_DEPTH (type, stack_container, name,      \
                                             _jitter_whirl_deepest_depth_old,  \
                                             _jitter_stack_swirl_top_old);     \
      JITTER_STACK_TOS_TOP (type, stack_container, name)                       \
        = _jitter_stack_swirl_deepest_old;                                     \
      JITTER_STACK_TOS_UNDER_TOP_POINTER_NAME (type, stack_container, name)    \
        -= _jitter_whirl_element_no;                                           \
    }                                                                          \
  while (false)
#define JITTER_STACK_NTOS_WHIRL(type, stack_container, name, element_no)    \
  do                                                                        \
    {                                                                       \
      /* This is not properly an exchange between two memory locations:     \
         the new top element will end up at a different address from the    \
         old top element. */                                                \
      /* Use an unsigned variable to get a warning in case the user passes  \
         a negative constant by mistake. */                                 \
      unsigned int _jitter_whirl_element_no = (element_no);                 \
      /* The stack depth of the deepest element to be touched, in the old   \
         state. */                                                          \
      unsigned int _jitter_whirl_deepest_depth_old                          \
        = _jitter_whirl_element_no + 1;                                     \
      /* Read the two source elements from memory. */                       \
      const type _jitter_stack_swirl_deepest_old                            \
        = JITTER_STACK_NTOS_AT_DEPTH (type, stack_container, name,          \
                                      _jitter_whirl_deepest_depth_old);     \
      const type _jitter_stack_swirl_top_old                                \
        = JITTER_STACK_NTOS_TOP (type, stack_container, name);              \
      /* Adjust the stack pointer and write the two target elements, again  \
         in memory. */                                                      \
      JITTER_STACK_NTOS_SET_AT_DEPTH (type, stack_container, name,          \
                                      _jitter_whirl_deepest_depth_old,      \
                                      _jitter_stack_swirl_top_old);         \
      JITTER_STACK_NTOS_TOP_POINTER_NAME (type, stack_container, name)      \
        -= _jitter_whirl_element_no;                                        \
      JITTER_STACK_NTOS_TOP (type, stack_container, name)                   \
        = _jitter_stack_swirl_deepest_old;                                  \
    }                                                                       \
  while (false)

/* Expand to a statement destructively reversing the order of the topmost n
   elements of the given stack. */
#define JITTER_STACK_TOS_REVERSE(type, stack_container, name, element_no)      \
  do                                                                           \
    {                                                                          \
      /* Use an unsigned variable to get a warning in case the user passes a   \
         negative constant by mistake.  Then convert to int, so that we may    \
         safely compute negative indices from other index operands, as it may  \
         be needed fo termination conditions. */                               \
      unsigned jitter_reverse_element_nou = (element_no);                      \
      int jitter_reverse_element_no = jitter_reverse_element_nou;              \
      /* Do nothing unless we have at least two elements to swap.  If we do,   \
         then of course we have to handle the TOS specially, and swap it with  \
         the deepest element. */                                               \
      if (jitter_reverse_element_no < 2)                                       \
        break;                                                                 \
      JITTER_SWAP (type,                                                       \
                   JITTER_STACK_TOS_TOP_NAME(type, stack_container, name),     \
                   JITTER_STACK_TOS_AT_NONZERO_DEPTH                           \
                      (type, stack_container, name,                            \
                       (jitter_reverse_element_no - 1)));                      \
      /* Every other element to swap will be at a non-zero depth.  Have two    \
         indices moving towards one another; keep swapping element pairs       \
         until the two indices meet or cross. */                               \
      int jitter_reverse_up_depth = 1;                                         \
      int jitter_reverse_down_depth = jitter_reverse_element_no - 2;           \
      while (jitter_reverse_down_depth > jitter_reverse_up_depth)              \
        {                                                                      \
          JITTER_SWAP (type,                                                   \
                       JITTER_STACK_TOS_AT_NONZERO_DEPTH                       \
                          (type, stack_container, name,                        \
                           jitter_reverse_up_depth),                           \
                       JITTER_STACK_TOS_AT_NONZERO_DEPTH                       \
                          (type, stack_container, name,                        \
                           jitter_reverse_down_depth));                        \
          jitter_reverse_up_depth ++;                                          \
          jitter_reverse_down_depth --;                                        \
        }                                                                      \
    }                                                                          \
  while (false)
#define JITTER_STACK_NTOS_REVERSE(type, stack_container, name, element_no)      \
  do                                                                            \
    {                                                                           \
      /* Use an unsigned variable to get a warning in case the user passes a    \
         negative constant by mistake.  Then convert to int, so that we may     \
         safely compute negative indices from other index operands, as it may   \
         be needed fo termination conditions. */                                \
      unsigned jitter_reverse_element_nou = (element_no);                       \
      int jitter_reverse_element_no = jitter_reverse_element_nou;               \
      /* Have two depth indices moving into opposite directions.  Keep          \
         swapping elements at the dephts described by the two indices, until    \
         the indices meet or cross. */                                          \
      int jitter_reverse_up_depth = 0;                                          \
      int jitter_reverse_down_depth = jitter_reverse_element_no - 1;            \
      while (jitter_reverse_down_depth > jitter_reverse_up_depth)               \
        {                                                                       \
          JITTER_SWAP (type,                                                    \
                       JITTER_STACK_NTOS_AT_DEPTH (type, stack_container, name, \
                                                   jitter_reverse_up_depth),    \
                       JITTER_STACK_NTOS_AT_DEPTH (type, stack_container, name, \
                                                   jitter_reverse_down_depth)); \
          jitter_reverse_up_depth ++;                                           \
          jitter_reverse_down_depth --;                                         \
        }                                                                       \
    }                                                                           \
  while (false)




/* Generic stack operations with user-specified functions.
 * ************************************************************************** */

/* Compute the given "setter" on the top element of the stack, replacing it
   with the result.
   This expands to a statement, and there is no result.  Undefined behavior
   on an empty or one-element stack.
   The "setter" is supposed to be the name of a two-argument macro expanding
   to a statement which modifies its first argument (an l-value expression)
   using the other (an r-value expression).  The setter body may be
   protected with the usual do .. while (false) idiom.

   Example:
#define SETSUCC(res, a)          do { res.field = a.field + 1; } while (0)
   JITTER_STACK_TOS_UNARY(union my_union, stack, , SETSUCC);
#undef  SETSUCC */
#define JITTER_STACK_TOS_UNARY(type, stack_container, name, setter)  \
  do                                                                 \
    {                                                                \
      {                                                              \
        setter(JITTER_STACK_TOS_TOP(type, stack_container, name),    \
               JITTER_STACK_TOS_TOP(type, stack_container, name));   \
      }                                                              \
    }                                                                \
  while (false)
#define JITTER_STACK_NTOS_UNARY(type, stack_container, name, setter)  \
  do                                                                  \
    {                                                                 \
      {                                                               \
        setter(JITTER_STACK_NTOS_TOP(type, stack_container, name),    \
               JITTER_STACK_NTOS_TOP(type, stack_container, name));   \
      }                                                               \
    }                                                                 \
  while (false)

/* Compute the given "setter" on the under-top and top elements of the stack, in
   that order, replacing both with the result.
   This expands to a statement, and there is no result.  Undefined behavior
   on an empty or one-element stack.
   The "setter" is supposed to be the name of a three-argument macro expanding
   to a statement which modifies its first argument (an l-value expression)
   using the other two (r-value expressions), in order.  The setter body may be
   protected with the usual do .. while (false) idiom, but does not need to be.

   Example:
#define SETPLUS(res, a, b)      do { res.field = a.field + b.field; } while (0)
   JITTER_STACK_TOS_BINARY(union my_union, stack, , SETPLUS);
#undef  SETPLUS */
#define JITTER_STACK_TOS_BINARY(type, stack_container, name, setter)      \
  do                                                                      \
    {                                                                     \
      {                                                                   \
        setter(JITTER_STACK_TOS_TOP(type, stack_container, name),         \
               JITTER_STACK_TOS_UNDER_TOP(type, stack_container, name),   \
               JITTER_STACK_TOS_TOP(type, stack_container, name));        \
      }                                                                   \
      JITTER_STACK_TOS_NIP(type, stack_container, name);                  \
    }                                                                     \
  while (false)
#define JITTER_STACK_NTOS_BINARY(type, stack_container, name, setter)      \
  do                                                                       \
    {                                                                      \
      {                                                                    \
        setter(JITTER_STACK_NTOS_UNDER_TOP(type, stack_container, name),   \
               JITTER_STACK_NTOS_UNDER_TOP(type, stack_container, name),   \
               JITTER_STACK_NTOS_TOP(type, stack_container, name));        \
      }                                                                    \
      JITTER_STACK_NTOS_TOP_POINTER_NAME(type, stack_container, name) --;  \
    }                                                                      \
  while (false)




/* Helper macros, not for the user.
 * ************************************************************************** */

/* Expand to a statement destructively swapping the values of the two given
   l-values, which must have both the given type.  Either a or b may be
   evaluated twice. */
#define JITTER_SWAP(type, a, b)           \
  do                                      \
    {                                     \
      const type _jitter_swap_tmp = (a);  \
      (a) = (b);                          \
      (b) = (type) _jitter_swap_tmp;      \
    }                                     \
  while (false)

#endif // #ifndef JITTER_STACK_H_
