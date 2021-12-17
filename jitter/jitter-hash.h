/* Jitter: hash table data structure header.

   Copyright (C) 2017, 2018 Luca Saiu
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


#ifndef JITTER_HASH_H_
#define JITTER_HASH_H_

#include <stdlib.h>
#include <stdbool.h>
#include "jitter.h"


/* Hash table public data structures.
 * ************************************************************************** */

/* The hash table data structure.  Notice that the hash function is not stored
   within the hash, even if that would be easy.  Anyway, a hash table should
   always be accessed using the same function: passing different functions for
   different table accesses is almost certainly not what the user wants. */
struct jitter_hash_table
{
  /* How many buckets exist.  Each of them is allowed to be NULL. */
  size_t bucket_no;

  /* How many bindings exist in all buckets. */
  size_t binding_no;

  /* An array of pointers to buckets.  Each element may be either NULL for an
     empty bucket, or a pointer to a malloc-allocated bucket, still allowed to
     be empty. */
  struct jitter_hash_bucket **buckets;
};




/* Hash functions.
 * ************************************************************************** */

/* A hash function takes one key and returns one unsigned integer, not
   restricted to the bucket size. */
typedef jitter_uint (*jitter_hash_function) (const union jitter_word key);

/* A few hash functions are predefined. */

/* A hash function working on string keys.  Strings are of course hashed by
   content, and not by address. */
jitter_uint
jitter_string_hash_function (const union jitter_word key)
  __attribute__ ((pure));

/* A hash function working on word keys, with a word intended in the sense of
   Jitter -- an object of size sizeof (void *).  If the word is a pointer the
   pointed object is ignored. */
jitter_uint
jitter_word_hash_function (const union jitter_word key)
  __attribute__ ((pure));




/* Comparison functions.
 * ************************************************************************** */

/* Return true iff two keys are equal. */
typedef bool (*jitter_hash_key_equal) (const union jitter_word key_1,
                                       const union jitter_word key_2);

/* A comparison function for strings.  Comparison works by content, not by
   address. */
bool
jitter_string_hash_key_equal (const union jitter_word key_1,
                              const union jitter_word key_2)
  __attribute__ ((pure));

/* A comparison function for words, in the Jitter sense.  The convenience
   functions for hash words below expose an API using jitter_int objects as
   keys, but this signature is required for the internal machinery. */
bool
jitter_word_hash_key_equal (const union jitter_word key_1,
                            const union jitter_word key_2)
  __attribute__ ((pure));




/* Key and value functions.
 * ************************************************************************** */

/* A function doing something on a word.  This is convenient for deallocating. */
typedef void (*jitter_word_function) (const union jitter_word);

/* A function doing nothing to a word.  This is convenient for trivial
   finalizers where deallocation is not needed. */
void
jitter_do_nothing_on_word (const union jitter_word key);




/* Initialization and finalization.
 * ************************************************************************** */

/* Initialize the pointed hash table. */
void
jitter_hash_initialize (struct jitter_hash_table *t)
  __attribute__ ((nonnull (1)));

/* Call the given functions, when either is non-NULL, on every key and value in
   an unspecified order then finalize, without de-allocating the main struct,
   the pointed hash table. */
void
jitter_hash_finalize (struct jitter_hash_table *t,
                      jitter_word_function finalize_key,
                      jitter_word_function finalize_value)
  __attribute__ ((nonnull (1)));

/* Functions to also allocate and deallocate are probably not needed.  If they
   are at some point add them here and call them jitter_hash_make and
   jitter_hash_destroy . */




/* Access.
 * ************************************************************************** */

/* Return true iff the pointed hash contains at least one binding for the
   pointed key. */
bool
jitter_hash_table_has (const struct jitter_hash_table *t,
                       const union jitter_word key,
                       jitter_hash_function f,
                       jitter_hash_key_equal eq)
  __attribute__ ((pure, nonnull (1, 3, 4)));

/* Return the value most recently associated to the given key in the pointed
   tabel with the given function.  Fail fatally if no binding for the key
   exists. */
union jitter_word
jitter_hash_table_get (const struct jitter_hash_table *t,
                       const union jitter_word key,
                       jitter_hash_function f,
                       jitter_hash_key_equal eq)
  __attribute__ ((nonnull (1, 3, 4)));

/* Associate the given value to the given key in the pointed table using the
   given function.  This does not remove the previous bindings for the key, if
   any. */
void
jitter_hash_table_add (struct jitter_hash_table *t,
                       const union jitter_word key,
                       const union jitter_word value,
                       jitter_hash_function f)
  __attribute__ ((nonnull (1, 4)));

/* Remove the most recent binding for the given key in the pointed table.  Do
   nothing if no binding for the key exists.  Before removing the binding, call
   the given functions, if not NULL, on the key and the value; the functions are
   a convenient way to deallocate.  It is allowed to have both, neither, or just
   one of them non-NULL. */
void
jitter_hash_table_remove (struct jitter_hash_table *t,
                          const union jitter_word key,
                          jitter_word_function key_function,
                          jitter_word_function value_function,
                          jitter_hash_function f,
                          jitter_hash_key_equal eq)
  __attribute__ ((nonnull (1, 5, 6)));




/* Hash iteration.
 * ************************************************************************** */

/* This is currently very crude, but sufficient for the purpose. */

/* A function performing some work on a hash binding. */
typedef
void (* jitter_hash_for_all_bindings_function)
     (const union jitter_word key,
      const union jitter_word value,
      void *extra_datum);

/* Call the given function once on each binding of the pointed hash table, in
   some unspecified order.  The last argument is passed as is to the worker
   function.
   The function must not change the hash table content. */
void
jitter_hash_for_all_bindings (const struct jitter_hash_table *t,
                              jitter_hash_for_all_bindings_function f,
                              void *extra_datum);




/* Debugging and tuning.
 * ************************************************************************** */

/* Print information about collisions. */
void
jitter_hash_print_debug_stats (const struct jitter_hash_table *t)
  __attribute__ ((cold, nonnull (1)));




/* String hash utility.
 * ************************************************************************** */

/* Specialized versions of the functions above using the string hash
   function.  Keys are strings, automatically copied by add operations;
   copies are freed by remove operations and finalization. */
bool
jitter_string_hash_table_has (const struct jitter_hash_table *t,
                              const char *key)
  __attribute__ ((pure, nonnull (1, 2)));
union jitter_word
jitter_string_hash_table_get (const struct jitter_hash_table *t,
                              const char *key)
  __attribute__ ((pure, nonnull (1, 2)));
void
jitter_string_hash_table_add (struct jitter_hash_table *t,
                              const char *key,
                              const union jitter_word value)
  __attribute__ ((nonnull (1, 2)));
void
jitter_string_hash_table_remove (struct jitter_hash_table *t,
                                 const char *key,
                                 jitter_word_function value_function)
  __attribute__ ((nonnull (1, 2)));
void
jitter_string_hash_finalize (struct jitter_hash_table *t,
                             jitter_word_function finalize_value)
  __attribute__ ((nonnull (1)));




/* Word hash utility.
 * ************************************************************************** */

/* Specialized versions of the functions above using the word hash
   function.  Keys are words, copied as they are without concern for
   any pointed memory, in case they are pointers.  Keys don't require
   heap allocation or deallocation. */
bool
jitter_word_hash_table_has (const struct jitter_hash_table *t,
                            const jitter_int key)
  __attribute__ ((pure, nonnull (1)));
union jitter_word
jitter_word_hash_table_get (const struct jitter_hash_table *t,
                            const jitter_int key)
  __attribute__ ((pure, nonnull (1)));
void
jitter_word_hash_table_add (struct jitter_hash_table *t,
                            const jitter_int key,
                            const union jitter_word value)
  __attribute__ ((nonnull (1)));
void
jitter_word_hash_table_remove (struct jitter_hash_table *t,
                               const jitter_int key,
                               jitter_word_function value_function)
  __attribute__ ((nonnull (1)));
void
jitter_word_hash_finalize (struct jitter_hash_table *t,
                           jitter_word_function finalize_value)
  __attribute__ ((nonnull (1)));

#endif // #ifndef JITTER_HASH_H_
