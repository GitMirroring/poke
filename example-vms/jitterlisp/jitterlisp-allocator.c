/* JitterLisp: heap allocation.

   Copyright (C) 2017, 2018, 2021 Luca Saiu
   Updated in 2020 and 2022 by Luca Saiu
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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


/* Include the Gnulib header. */
#include <config.h>

#include "jitterlisp-allocator.h"

#include <string.h>

#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-hash.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-malloc.h>

#include "jitterlisp-printer.h"
#include "jitterlisp-sexpression.h"
#include "jitterlisp-config.h"

/* Bohem's GC: include headers if used in this configuration. */
#ifdef JITTERLISP_BOEHM_GC
# define GC_THREADS 1
# include <gc/gc.h>
#endif // #ifdef JITTERLISP_BOEHM_GC


/* Global register variable hack.
 * ************************************************************************** */

/////////////////////////////////////////////////////
/* FIXME: this whole idea will not work here, with separate compilation.  But I
   can uncomment the #undef below and look at the generated code. */
#undef JITTER_HAVE_ASSEMBLY

#ifdef JITTER_HAVE_ASSEMBLY
  /* We need architecture-specific register names. */
# include <jitter/machine/jitter-machine.h>
#endif // #ifdef JITTER_HAVE_ASSEMBLY

/* Expand to a global variable declaration -- a global *register* variable
   if we have machine support.  This doesn't currently run in a Jittery VM,
   so it doesn't matter if we choose registers which are already reserved
   for other purposes. */
#if defined(JITTERLISP_LITTER) && defined(JITTER_HAVE_ASSEMBLY)
# define JITTERLISP_GLOBAL_REGISTER_VARIABLE_(_jitterlisp_type,            \
                                              _jitterlisp_name,            \
                                              _jitterlisp_register_index)  \
  register _jitterlisp_type _jitterlisp_name                               \
     asm (JITTER_STRINGIFY(                                                \
            JITTER_CONCATENATE_TWO(JITTER_RESIDUAL_REGISTER_,              \
                                   _jitterlisp_register_index)))
#else
# define JITTERLISP_GLOBAL_REGISTER_VARIABLE_(_jitterlisp_type,            \
                                              _jitterlisp_name,            \
                                              _jitterlisp_register_index)  \
  static _jitterlisp_type _jitterlisp_name
#endif // #if defined(JITTERLISP_LITTER) && defined(JITTER_HAVE_ASSEMBLY)




/* Symbol data structures.
 * ************************************************************************** */

/* A string hash mapping each interned symbol name as a C string into a
   pointer to the unique struct jitterlisp_symbol for the symbol. */
static struct jitter_hash_table
jitterlisp_symbol_table;

/* The global counter used for printing uninterned symbols in compact
   notation. */
static jitter_uint
jitterlisp_next_uninterned_symbol_index;




/* Thread registration.
 * ************************************************************************** */

void
jitterlisp_register_current_thread (void)
{
#if defined(JITTERLISP_BOEHM_GC)
  struct GC_stack_base stack_base;
  GC_get_stack_base (& stack_base);
  GC_register_my_thread (& stack_base);
#endif // #if defined(JITTERLISP_BOEHM_GC)
}

void
jitterlisp_unregister_current_thread (void)
{
#if defined(JITTERLISP_BOEHM_GC)
  GC_unregister_my_thread ();
#endif // #if defined(JITTERLISP_BOEHM_GC)
}




/* GC root registration.
 * ************************************************************************** */

/* Data about a root. */
struct jitterlisp_gc_root
{
  /* Initial pointer to the array of Lisp objects. */
  jitterlisp_object *object_pointer;

  /* How many objects there are. */
  size_t object_no;
};

/* I keep roots in a global stack. */
static struct jitter_dynamic_buffer
jitterlisp_gc_root_stack;

/* Initialize the global root stack.  This is only used internally. */
static void
jitterlisp_gc_root_stack_initialize (void)
{
  jitter_dynamic_buffer_initialize (& jitterlisp_gc_root_stack);
}

/* Finalize the global root stack.  This is only used internally. */
static void
jitterlisp_gc_root_stack_finalize (void)
{
  jitter_dynamic_buffer_initialize (& jitterlisp_gc_root_stack);
}

void
jitterlisp_push_gc_root (jitterlisp_object *object_pointer,
                         size_t object_no)
{
  struct jitterlisp_gc_root root;
  root.object_pointer = object_pointer;
  root.object_no = object_no;
  jitter_dynamic_buffer_push (& jitterlisp_gc_root_stack,
                              & root,
                              sizeof (struct jitterlisp_gc_root));
#if   defined(JITTERLISP_LITTER)
  /* Do nothing. */
#elif defined(JITTERLISP_BOEHM_GC)
  GC_add_roots (object_pointer,
                ((char *) object_pointer)
                + sizeof (jitterlisp_object) * object_no);
#else
# error "impossible or unimplemented"
#endif // #if defined(...
}

void
jitterlisp_push_stack_backing_as_gc_root (struct jitter_stack_backing *sb)
{
  /* Perform a cheap sanity check: here we must be speaking of elements
     of the size of list objects. */
  if (sb->element_size_in_bytes != sizeof (jitterlisp_object))
    jitter_fatal ("stack backing not an array of Lisp objects");

  /* Register the full memory for the backing as a root.  The size is given
     in elements, not in chars, as expected here. */
  jitterlisp_push_gc_root ((jitterlisp_object *) sb->memory,
                           sb->element_no);
}

void
jitterlisp_pop_gc_roots (size_t how_many)
{
  struct jitterlisp_gc_root *popped_roots __attribute__ ((unused))
    = jitter_dynamic_buffer_pop (& jitterlisp_gc_root_stack,
                                 sizeof (struct jitterlisp_gc_root) * how_many);
#if defined(JITTERLISP_BOEHM_GC)
  int i;
  for (i = 0; i < how_many; i ++)
    GC_remove_roots (popped_roots [i].object_pointer,
                     ((char *) (popped_roots [i].object_pointer)
                      + (sizeof (jitterlisp_object)
                         * popped_roots [i].object_no)));
#endif // #if defined(JITTERLISP_BOEHM_GC)
}

void
jitterlisp_pop_gc_root (void)
{
  jitterlisp_pop_gc_roots (1);
}

void
jitterlisp_pop_all_gc_roots (void)
{
  size_t root_no
    = (jitter_dynamic_buffer_size (& jitterlisp_gc_root_stack)
       / sizeof (struct jitterlisp_gc_root));
  jitterlisp_pop_gc_roots (root_no);
}




/* Initialization and finalization of the memory subsystem.
 * ************************************************************************** */

/* If littering then we simply use global variables for limits, and large
   infrequently allocated blocks for the heap.  This is obviously suboptimal,
   but this entire littering allocator is a toy; the real Jitter garbage
   collector will be less naïf. */
#ifdef JITTERLISP_LITTER
#define JITTERLISP_LITTER_BLOCK_BYTE_NO (32 * 1024L * 1024L)

/* The lowest heap address. */
static char *litter_heap_beginning;

/* The address of the next heap-allocated buffer. */
JITTERLISP_GLOBAL_REGISTER_VARIABLE_(char *, litter_allocator_pointer, 0);

/* The lowest address *out* of the heap area. */
JITTERLISP_GLOBAL_REGISTER_VARIABLE_(char *, litter_allocator_limit, 1);

///* Temporary testing stuff. */
//JITTERLISP_GLOBAL_REGISTER_VARIABLE_(void *, jitterlisp_error_handler_register, 2);

/* Information about one litter block. */
struct jitterlisp_litter_block
{
  void *beginning;
  size_t size_in_bytes;
};

/* A stack of struct jitter_litter_block objects, to keep track of which
   blocks have been allocated.  Even without releasing the individual
   objects we can free the entire blocks at finalization.
   This is currently non-reentrant. */
static struct jitter_dynamic_buffer
jitterlisp_litter_blocks;

/* The total heap size in bytes, mostly for debugging and logging.  This is kept
   as a separate global for efficiency, but the same information could be
   recovered from jitterlisp_litter_blocks . */
static size_t
jitterlisp_litter_heap_size;

/* Make a large block from which we will allocate, without ever releasing
   individual objects, and add it to the litter block stack.  Set allocation
   pointer, heap beginning and limit to refer to the new block.  Any previous
   existing block is kept as well, and objects from one block are allowed to
   point to objects from another. */
static void
jitterlisp_add_litter_block (size_t block_size_in_bytes)
{
  /* Set up the allocation pointer and its limit. */
  litter_allocator_pointer
    = litter_heap_beginning
    = jitter_xmalloc (block_size_in_bytes);
  litter_allocator_limit = (litter_allocator_pointer + block_size_in_bytes);

  /* For debugging purposes initialise the block: */
  jitterlisp_object *p;
  for (p = (jitterlisp_object *) litter_allocator_pointer;
       p < (jitterlisp_object *) litter_allocator_limit;
       p ++)
    * p = JITTERLISP_UNINITIALIZED_HEAP_VALUE;

  /* Add information about it to the stack. */
  struct jitterlisp_litter_block block;
  block.beginning = litter_heap_beginning;
  block.size_in_bytes = block_size_in_bytes;
  jitter_dynamic_buffer_push (& jitterlisp_litter_blocks,
                              & block,
                              sizeof (struct jitterlisp_litter_block));

  /* Update the total heap size. */
  bool is_this_block_the_first = jitterlisp_litter_heap_size == 0;
  jitterlisp_litter_heap_size += block_size_in_bytes;

#define JITTERLISP_TO_MIB(x) ((x) / 1024.0 / 1024.0)
  /* Log, unless disabled, if the block is not the first (the first block
     allocation is uninteresting). */
  if (jitterlisp_settings.verbose_litter && ! is_this_block_the_first)
    {
      /* Flush whatever still needs to be written on stdout before writing
         this slightly annoying message. */
      fflush (stdout);
      /* Write a very compact mesasge to stderr, and flush. */
      char text [500];
      sprintf (text, "[Litter%luMiB]\n",
               (unsigned long) JITTERLISP_TO_MIB(jitterlisp_litter_heap_size));
      jitterlisp_log_char_star (text);
      fflush (stderr);
    }
#undef JITTERLISP_TO_MIB
}

static void
jitterlisp_destroy_litter_blocks (void)
{
  struct jitterlisp_litter_block *popped_blocks
    = ((struct jitterlisp_litter_block *)
       jitterlisp_litter_blocks.region);
  size_t popped_block_no = (jitterlisp_litter_blocks.used_size
                            / sizeof (struct jitterlisp_litter_block));
  int i;
  for (i = 0; i < popped_block_no; i ++)
    free (popped_blocks [i].beginning);
}
#endif // #if defined(JITTERLISP_LITTER)

void
jitterlisp_memory_initialize (void)
{
  /* Initialize the garbage-collected heap. */
#if   defined(JITTERLISP_LITTER)
  /* Initialize the litter block stack and the total heap size. */
  jitter_dynamic_buffer_initialize (& jitterlisp_litter_blocks);
  jitterlisp_litter_heap_size = 0;

  /* Make the first litter block. */
  jitterlisp_add_litter_block (JITTERLISP_LITTER_BLOCK_BYTE_NO);
  if (jitterlisp_settings.verbose)
    fprintf (stderr, "Made the first litter block\n");

#elif defined(JITTERLISP_BOEHM_GC)
  /* Initialize Boehm's GC.  Technically I would be supposed to call GC_INIT
     from main, but this will work well enough in practice.  Actual allocation
     will start in functions nested more deeply than this one, and therefore
     recognizing the C stack bottom as being at the current height will
     suffice. */
  if (jitterlisp_settings.verbose)
    fprintf (stderr, "Initializing Boehm GC...\n");
  GC_INIT ();
  GC_allow_register_threads ();

/* Sanity check (based on compile-time constants): fail if the minimum alignment
   we require is too big.  GC_MALLOC returns pointers aligned to a double
   machine word, which means that their least significant
   (JITTER_LG_BYTES_PER_WORD + 1) bits are guaranteed to be zero. */
  jitter_uint required_zero_bit_mask = JITTERLISP_ALIGNMENT_BIT_MASK;
  jitter_uint provided_zero_bit_mask
    = JITTER_BIT_MASK(JITTER_LG_BYTES_PER_WORD + 1);
  if (required_zero_bit_mask > provided_zero_bit_mask)
    jitter_fatal ("Alignment requirement not satisfied by GC_MALLOC.  This "
                  "can be fixed by conditionally using GC_memalign , but the "
                  "fix is not implemented");

  if (jitterlisp_settings.verbose)
    fprintf (stderr, "...Initialized Boehm GC...\n");

#else
# error "impossible or unimplemented"
#endif // #if defined(...

  jitterlisp_register_current_thread ();

  jitterlisp_gc_root_stack_initialize ();

  /* Initialize the symbol table and the uninterned symbol counter. */
  jitter_hash_initialize (& jitterlisp_symbol_table);
  jitterlisp_next_uninterned_symbol_index = 0;
}

/* Forward-declaration. */
static void
jitterlisp_destroy_interned_symbol (const union jitter_word w);

void
jitterlisp_memory_finalize (void)
{
  jitterlisp_pop_all_gc_roots ();
  jitterlisp_gc_root_stack_finalize ();

  /* Finalize the symbol table, destroying every interned symbol in the
     process. */
  jitter_string_hash_finalize (& jitterlisp_symbol_table,
                               jitterlisp_destroy_interned_symbol);

  jitterlisp_unregister_current_thread ();

  /* Finalize the garbage-collected heap. */
#if   defined(JITTERLISP_LITTER)
  /* Destroy every litter block referred on the litter block stack. */
  jitterlisp_destroy_litter_blocks ();
  /* Finalize the stack itself. */
  jitter_dynamic_buffer_finalize (& jitterlisp_litter_blocks);
#elif defined(JITTERLISP_BOEHM_GC)
  /* Boehm's GC doesn't seem to have a global finalization function -- not
     speaking of per-object finalization, of course. */
#else
# error "impossible or unimplemented"
#endif // #if defined(...
}




/* Allocation.
 * ************************************************************************** */

char *
jitterlisp_allocate (size_t size_in_bytes)
{
#if   defined(JITTERLISP_LITTER)
  char *res = litter_allocator_pointer;
  litter_allocator_pointer += size_in_bytes;
  if (__builtin_expect (litter_allocator_pointer >= litter_allocator_limit,
                        false))
    {
      size_t new_block_size = JITTERLISP_LITTER_BLOCK_BYTE_NO;
      if (new_block_size < size_in_bytes)
        new_block_size = size_in_bytes;
      jitterlisp_add_litter_block (new_block_size);
      return jitterlisp_allocate (size_in_bytes);
    }
  return res;

#elif defined(JITTERLISP_BOEHM_GC)
  return GC_MALLOC (size_in_bytes);

#else
# error "impossible or unimplemented"

#endif // #if defined(...
}

struct jitterlisp_symbol *
jitterlisp_symbol_make_uninterned (void)
{
  struct jitterlisp_symbol *res
    = JITTERLISP_SYMBOL_UNINTERNED_MAKE_UNINITIALIZED_UNENCODED();
  res->name_or_NULL = NULL;
  res->global_value = JITTERLISP_UNDEFINED;
  /* FIXME: this increment operation should be performed in a critical section
     if I add multi-threading support. */
  res->index = jitterlisp_next_uninterned_symbol_index ++;
  res->global_constant = false;
  return res;
}




/* Symbols and symbol interning.
 * ************************************************************************** */

struct jitterlisp_symbol *
jitterlisp_symbol_make_interned (const char *name)
{
  if (jitter_string_hash_table_has (& jitterlisp_symbol_table,
                                    name))
    return ((struct jitterlisp_symbol *)
            (jitter_string_hash_table_get (& jitterlisp_symbol_table,
                                           name).pointer_to_void));
  else
    {
      struct jitterlisp_symbol *res
        = jitter_xmalloc (sizeof (struct jitterlisp_symbol));
      res->name_or_NULL = jitter_xmalloc (strlen (name) + 1);
      strcpy (res->name_or_NULL, name);
      res->global_value = JITTERLISP_UNDEFINED;
      res->index = 0;
      res->global_constant = false;
      jitterlisp_push_gc_root (& res->global_value, 1);
      union jitter_word w = { .pointer = (void *) res };
      jitter_string_hash_table_add (& jitterlisp_symbol_table, name, w);
      return res;
    }
}

/* This is used in finalization.  Notice that interned symbols are not
   garbage-collected. */
static void
jitterlisp_destroy_interned_symbol (const union jitter_word w)
{
  struct jitterlisp_symbol *symbol
    = ((struct jitterlisp_symbol *) w.pointer);

  /* Destroy the symbol name, if any. */
  if (symbol->name_or_NULL != NULL)
    free (symbol->name_or_NULL);

  /* Destroy the struct as well.  This is correct, since symbols are allocated
     with malloc. */
  free (symbol);
}




/* Force collection.
 * ************************************************************************** */

void
jitterlisp_gc (void)
{
#if   defined(JITTERLISP_LITTER)
  /* Do nothing. */
#elif defined(JITTERLISP_BOEHM_GC)
  GC_gcollect ();
#else
# error "impossible or unimplemented"
#endif // #if defined(...
}




/* Interned symbol list.
 * ************************************************************************** */

/* Add a symbol bound in the symbol table to the pointer Lisp list.
   This function is meant to be used with jitter_hash_for_all_bindings . */
static void
jitterlisp_add_binding_to_list (const union jitter_word key,
                                union jitter_word * value_p,
                                void *extra_datum)
{
  jitterlisp_object * list_pointer = extra_datum;

  /* We don't need to do anything with the symbol name here: we only care about
     the value part of the binding.  Encode the symbol, which is stored
     unencoded in the symbol table. */
  jitterlisp_object symbol = JITTERLISP_SYMBOL_ENCODE(value_p->pointer);

  /* Update the pointed list to contain another cons. */
  * list_pointer = jitterlisp_cons (symbol, * list_pointer);
}

jitterlisp_object
jitterlisp_interned_symbols (void)
{
  /* FIXME: this will require changes if I switch to a precise-pointer-finding
     GC.  Pointer to jitterlisp_objects from automatic C variables need to be
     temporarily marked as roots. */

  /* Add each symbol to a list, and return the list. */
  jitterlisp_object res = JITTERLISP_EMPTY_LIST;
  jitter_hash_for_all_bindings (& jitterlisp_symbol_table,
                                jitterlisp_add_binding_to_list,
                                & res);
  return res;
}
