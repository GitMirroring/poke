/* Jitter: hash table data structure.

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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jitter.h"

#include "jitter-malloc.h"
#include "jitter-fatal.h"
#include "jitter-hash.h"


/* Initial buffer sizes, in elements.
 * ************************************************************************** */

/* How many bindings to have in a bucket at allocation time.  In practice one
   will be used immediately, at bucket allocation; the others are pre-allocated
   to be used in the event of collisions. */
#define INITIAL_BINDING_NO_PER_BUCKET 4

/* How many buckets an empty hash table should have. */
#define INITIAL_BUCKET_NO 65




/* Hash table data structures.
 * ************************************************************************** */

/* A binding is simply a pair associating a key and a value. */
struct jitter_hash_binding
{
  /* The binding key; this should normally be a pointer. */
  union jitter_word key;

  /* The binding value, associated to the key.  This can point to any value.
     Notice that the pointed value is not automatically deallocated. */
  union jitter_word value;
};

struct jitter_hash_bucket
{
  /* How many bindings we can hold before resizing the bucket. */
  size_t allocated_binding_no;

  /* How many bindings we are actually holding. */
  size_t used_binding_no;

  /* A pointer to a malloc-allocated array of struct jitter_hash_binding, of
     allocated_binding_no elements.  Bindings are added to the right, and
     searched from the right to the left; search semantics is therefore LIFO. */
  struct jitter_hash_binding *bindings;
};




/* Hash functions.
 * ************************************************************************** */

/* This is defined in jitter-hash-random-table.c .  There is no need to declare
   it in a header. */
extern const jitter_uint
jitter_hash_random_words [256];

/* The underlying implementation of every hash function here.  Given the
   previous state of the hash function, combine the new char to it and return
   the next state. */
__attribute__ ((always_inline))
static inline jitter_uint
jitter_hash_combine_char (jitter_uint previous, unsigned char new)
{
  jitter_uint state = previous;
  state ^= (state << 1) ^ jitter_hash_random_words [new];
  return state;
}

/* Hash memory starting from the given pointer, reading the given number of
   chars. */
__attribute__ ((always_inline))
static inline jitter_uint
jitter_hash_memory_for (const void *initial_pointer, size_t char_no)
{
  const unsigned char *p = (const unsigned char *) initial_pointer;
  jitter_uint res = 0;
  int i;
  for (i = 0; i < char_no; i ++)
    res = jitter_hash_combine_char (res, p [i]);
  return res;
}

/* Hash memory starting from the given pointer, until the given terminator byte
   is found. */
__attribute__ ((always_inline))
static inline jitter_uint
jitter_hash_memory_until (const void *initial_pointer,
                          unsigned char terminator)
{
  const unsigned char *key = (const unsigned char *) initial_pointer;
  jitter_uint res = 0;
  const unsigned char *p;
  for (p = key; *p != terminator; p ++)
    res = jitter_hash_combine_char (res, * p);
  return res;
}

jitter_uint
jitter_string_hash_function (const union jitter_word key)
{
  return jitter_hash_memory_until (key.pointer_to_char, '\0');
}

jitter_uint
jitter_word_hash_function (const union jitter_word key_as_union)
{
  const jitter_uint key = key_as_union.ufixnum;
  return jitter_hash_memory_for (& key, sizeof (jitter_uint));
}




/* Comparison functions.
 * ************************************************************************** */

bool
jitter_string_hash_key_equal (const union jitter_word key_1,
                              const union jitter_word key_2)
{
  return ! strcmp (key_1.pointer_to_char, key_2.pointer_to_char);
}

bool
jitter_word_hash_key_equal (const union jitter_word key_1,
                            const union jitter_word key_2)
{
  /* Of course this makes no sense if the word size is different from the union
     size.  Do a compile-time sanity check. */
  assert (sizeof (union jitter_word) == sizeof (jitter_int));

  return key_1.fixnum == key_2.fixnum;
}



/* Key and value functions.
 * ************************************************************************** */

void
jitter_do_nothing_on_word (const union jitter_word key)
{
  /* Do nothing. */
}




/* Initialization and finalization.
 * ************************************************************************** */

/* Initialize the pointed hash table. */
static void
jitter_hash_initialize_with_bucket_no (struct jitter_hash_table *t,
                                       size_t bucket_no)
{
  t->bucket_no = bucket_no;
  t->binding_no = 0;
  t->buckets = jitter_xmalloc (sizeof (struct jitter_hash_bucket*)
                               * bucket_no);
  int i;
  for (i = 0; i < bucket_no; i ++)
    t->buckets [i] = NULL;
}

void
jitter_hash_initialize (struct jitter_hash_table *t)
{
  jitter_hash_initialize_with_bucket_no (t, INITIAL_BUCKET_NO);
}

void
jitter_hash_finalize (struct jitter_hash_table *t,
                      jitter_word_function finalize_key,
                      jitter_word_function finalize_value)
{
  int i;
  for (i = 0; i < t->bucket_no; i ++)
    {
      if (t->buckets [i] == NULL)
        continue;

      struct jitter_hash_bucket *bucket = t->buckets [i];
      int j;
      for (j = 0; j < bucket->used_binding_no; j ++)
        {
          struct jitter_hash_binding *binding = bucket->bindings + j;
          if (finalize_key)
            finalize_key (binding->key);
          if (finalize_value)
            finalize_value (binding->value);
        }
      free (bucket->bindings);
      free (bucket);
    }
  free (t->buckets);

  /* For defensiveness, fill the struct with invalid data. */
  memset (t, 0xff, sizeof (struct jitter_hash_table));
}




/* Access.
 * ************************************************************************** */

static bool
jitter_hash_bucket_has (const struct jitter_hash_bucket *b,
                        const union jitter_word key,
                        jitter_hash_key_equal eq)
{
  jitter_uint limit = b->used_binding_no;
  jitter_uint i;
  struct jitter_hash_binding *bindings = b->bindings;
  for (i = 0; i < limit; i ++)
    if (eq (key, bindings [i].key))
      return true;
  return false;
}

static union jitter_word
jitter_hash_bucket_get (const struct jitter_hash_bucket *b,
                        const union jitter_word key,
                        jitter_hash_key_equal eq)
{
  jitter_uint limit = b->used_binding_no;
  jitter_int i;
  struct jitter_hash_binding *bindings = b->bindings;
  for (i = limit - 1; i >= 0; i --)
    if (eq (key, bindings [i].key))
      return bindings [i].value;

  jitter_fatal ("jitter_hash_bucket_get: unbound key");
}

/* Return true iff there was actually one element to remove. */
static bool
jitter_hash_bucket_remove (struct jitter_hash_bucket *b,
                           const union jitter_word key,
                           jitter_word_function key_function,
                           jitter_word_function value_function,
                           jitter_hash_key_equal eq)
{
  jitter_uint limit = b->used_binding_no;
  jitter_int i;
  struct jitter_hash_binding *bindings = b->bindings;
  for (i = limit - 1; i >= 0; i --)
    if (eq (key, bindings [i].key))
      {
        if (key_function)
          key_function (bindings [i].key);
        if (value_function)
          value_function (bindings [i].value);
        memcpy (bindings + i, bindings + i + 1,
                sizeof (struct jitter_hash_binding) * (limit - i - 1));
        b->used_binding_no --;
        return true;
      }
  return false;
}

bool
jitter_hash_table_has (const struct jitter_hash_table *t,
                       const union jitter_word key,
                       jitter_hash_function f,
                       jitter_hash_key_equal eq)
{
  const struct jitter_hash_bucket *b = t->buckets [f (key) % t->bucket_no];
  if (b == NULL)
    return false;
  else
    return jitter_hash_bucket_has (b, key, eq);
}

union jitter_word
jitter_hash_table_get (const struct jitter_hash_table *t,
                       const union jitter_word key,
                       jitter_hash_function f,
                       jitter_hash_key_equal eq)
{
  const struct jitter_hash_bucket *b = t->buckets [f (key) % t->bucket_no];
  if (b == NULL)
    jitter_fatal ("jitter_hash_table_get: unbound key");

  return jitter_hash_bucket_get (b, key, eq);
}

inline static bool
jitter_hash_table_overfull (struct jitter_hash_table *t)
{
  /* A hash should be enlarged when it's over 75% full; assuming no collisions,
     this means that the bindings are at least three fourths of the buckets.
     Actually doing this check in integer arithmetic makes the table look
     overfull a little in advance, which is all for the better. */
  return t->binding_no >= (t->bucket_no * 3 / 4);
}

__attribute__ ((noinline, cold)) static void
jitter_hash_table_enlarge (struct jitter_hash_table *t,
                           jitter_hash_function f)
{
  /* Make a bigger table. */
  size_t new_bucket_no = t->bucket_no * 2 + 1;
  //printf ("enlarging the table: %li to %li (there are %li bindings)\n", (long)t->bucket_no, (long)new_bucket_no, (long)t->binding_no);
  struct jitter_hash_table new_table;
  jitter_hash_initialize_with_bucket_no (& new_table, new_bucket_no);

  /* Copy every binding from the old table to the new.  It's important to scan
     the old buckets left-to-right, so that, even if collisions are different in
     the new table, the elements taking precedence are still on the right in
     each new bucket. */
  int i;
  for (i = 0; i < t->bucket_no; i ++)
    {
      struct jitter_hash_bucket *bucket = t->buckets [i];
      if (bucket == NULL)
        continue;
      struct jitter_hash_binding *bindings = bucket->bindings;
      size_t binding_no = bucket->used_binding_no;
      int j;
      for (j = 0; j < binding_no; j ++)
        jitter_hash_table_add (& new_table,
                               bindings [j].key,
                               bindings [j].value,
                               f);
    }

  /* Finalize the old table, without deallocating the elements (since we share
     them) and move the new table content to the old table.  Now the old table
     has more buckets, and all that survives of the new one is an automatic
     variable which will be deallocated on return. */
  jitter_hash_finalize (t, NULL, NULL);
  memcpy (t, &new_table, sizeof (struct jitter_hash_table));
}

void
jitter_hash_table_add (struct jitter_hash_table *t,
                       const union jitter_word key,
                       const union jitter_word value,
                       jitter_hash_function f)
{
  /* We only enlarge the table (when it's getting too full) on add, and never
     shrink it. */
  if (jitter_hash_table_overfull (t))
    jitter_hash_table_enlarge (t, f);

  t->binding_no ++;
  jitter_uint bucket_index = f (key) % t->bucket_no;

  /* Find the bucket; make it if needed. */
  struct jitter_hash_bucket *b = t->buckets [bucket_index];
  if (b == NULL)
    {
      b = jitter_xmalloc (sizeof (struct jitter_hash_bucket));
      b->allocated_binding_no = INITIAL_BINDING_NO_PER_BUCKET;
      b->used_binding_no = 0;
      b->bindings
        = jitter_xmalloc (sizeof (struct jitter_hash_binding)
                          * INITIAL_BINDING_NO_PER_BUCKET);
      t->buckets [bucket_index] = b;
    }

  /* Find the binding where we need to write within the bucket; make place and
     reallocate if needed. */
  if (b->used_binding_no == b->allocated_binding_no)
    {
      //printf ("enlarging the %i-th bucket: %li to %li\n", (int)bucket_index, (long)b->allocated_binding_no, (long)(b->allocated_binding_no * 2));

      b->bindings = jitter_xrealloc (b->bindings,
                                     sizeof (struct jitter_hash_binding)
                                     * (b->allocated_binding_no *= 2));
    }
  struct jitter_hash_binding *bi = b->bindings + (b->used_binding_no ++);
  bi->key = key;
  bi->value = value;
}

void
jitter_hash_table_remove (struct jitter_hash_table *t,
                          const union jitter_word key,
                          jitter_word_function key_function,
                          jitter_word_function value_function,
                          jitter_hash_function f,
                          jitter_hash_key_equal eq)
{
  struct jitter_hash_bucket *b = t->buckets [f (key) % t->bucket_no];
  if (b == NULL)
    return;
  else
    {
      if (jitter_hash_bucket_remove (b, key, key_function, value_function, eq))
        t->binding_no --;
    }
}




/* Hash iteration.
 * ************************************************************************** */

void
jitter_hash_for_all_bindings (const struct jitter_hash_table *t,
                              jitter_hash_for_all_bindings_function f,
                              void *extra_datum)
{
  /* For each bucket... */
  int i, j;
  for (i = 0; i < t->bucket_no; i ++)
    {
      /* Do nothing if the bucket is NULL. */
      struct jitter_hash_bucket * const bucket = t->buckets [i];
      if (bucket == NULL)
        continue;

      /* For each binding in the bucket... */
      struct jitter_hash_binding * const bindings = bucket->bindings;
      for (j = 0; j < bucket->used_binding_no; j ++)
        /* ... Call the function provided by the user. */
        f (bindings [j].key, bindings [j].value, extra_datum);
    }
}



/* Debugging and tuning.
 * ************************************************************************** */

/* Return the square of the given number. */
static double
square (double x)
{
  return x * x;
}

/* Print information about collisions. */
void
jitter_hash_print_debug_stats (const struct jitter_hash_table *t)
{
  size_t min_bucket_size = t->bucket_no + 1;
  size_t min_nonempty_bucket_size = t->bucket_no + 1;
  size_t max_bucket_size = 0;
  size_t nonempty_bucket_no = 0;
  int i;
  for (i = 0; i < t->bucket_no; i ++)
    {
      struct jitter_hash_bucket *b = t->buckets [i];
      size_t used_size = b != NULL ? b->used_binding_no : 0;
      if (used_size > 0)
        nonempty_bucket_no ++;
      if (used_size > max_bucket_size)
        max_bucket_size = used_size;
      if (used_size < min_bucket_size)
        min_bucket_size = used_size;
      if (used_size > 0 && used_size < min_nonempty_bucket_size)
        min_nonempty_bucket_size = used_size;
    }
  double bucket_size_mean = t->binding_no / (double) t->bucket_no;
  double nonempty_bucket_size_mean
    = t->binding_no / (double) nonempty_bucket_no;

  double bucket_size_variance = 0;
  double nonempty_bucket_size_variance = 0;
  for (i = 0; i < t->bucket_no; i ++)
    {
      struct jitter_hash_bucket *b = t->buckets [i];
      size_t used_size = b != NULL ? b->used_binding_no : 0;
      bucket_size_variance += square (used_size - bucket_size_mean);
      if (used_size > 0)
        nonempty_bucket_size_variance
          += square (used_size - nonempty_bucket_size_mean);
    }
  bucket_size_variance /= t->bucket_no;
  nonempty_bucket_size_variance /= nonempty_bucket_no;

  printf ("Binding no:                      %lu\n",
          (unsigned long) t->binding_no);
  printf ("Fill factor or bucket size mean: %f\n", bucket_size_mean);
  printf ("Bucket no:                       %lu\n",
          (unsigned long) t->bucket_no);
  printf ("Nonempty bucket no:              %lu\n",
          (unsigned long) nonempty_bucket_no);
  printf ("Minimum bucket size:             %lu\n",
          (unsigned long) min_bucket_size);
  printf ("Minimum nonempty bucket size:    %lu\n",
          (unsigned long) min_nonempty_bucket_size);
  printf ("Nonempty bucket size mean:       %f\n", nonempty_bucket_size_mean);
  printf ("Nonempty bucket size variance:   %f\n",
          nonempty_bucket_size_variance);
  printf ("Bucket size variance:            %f\n", bucket_size_variance);
  printf ("Maximum bucket size:             %lu\n",
          (unsigned long) max_bucket_size);
}




/* String hash utility.
 * ************************************************************************** */

static void
jitter_string_hash_key_function (union jitter_word key)
{
  free (key.pointer_to_char);
}

inline static union jitter_word
jitter_string_to_word (const char *s)
{
  union jitter_word w = {.pointer_to_char = (char*) s};
  return w;
}

inline static union jitter_word
jitter_clone_string_into_word (const char *s)
{
  char *copy_of_s = jitter_xmalloc (strlen (s) + 1);
  strcpy (copy_of_s, s);
  union jitter_word w = {.pointer_to_char = (char*) copy_of_s};
  return w;
}

bool
jitter_string_hash_table_has (const struct jitter_hash_table *t,
                              const char *s)
{
  return jitter_hash_table_has (t,
                                jitter_string_to_word (s),
                                jitter_string_hash_function,
                                jitter_string_hash_key_equal);
}

union jitter_word
jitter_string_hash_table_get (const struct jitter_hash_table *t,
                              const char *s)
{
  return jitter_hash_table_get (t,
                                jitter_string_to_word (s),
                                jitter_string_hash_function,
                                jitter_string_hash_key_equal);
}

void
jitter_string_hash_table_add (struct jitter_hash_table *t,
                              const char *s,
                              const union jitter_word value)
{
  jitter_hash_table_add (t,
                         jitter_clone_string_into_word (s),
                         value,
                         jitter_string_hash_function);
}

void
jitter_string_hash_table_remove (struct jitter_hash_table *t,
                                 const char *s,
                                 jitter_word_function value_function)
{
  jitter_hash_table_remove (t,
                            jitter_string_to_word (s),
                            jitter_string_hash_key_function,
                            value_function,
                            jitter_string_hash_function,
                            jitter_string_hash_key_equal);
}

void
jitter_string_hash_finalize (struct jitter_hash_table *t,
                             jitter_word_function finalize_value)
{
  jitter_hash_finalize (t, jitter_string_hash_key_function, finalize_value);
}




/* Word hash utility.
 * ************************************************************************** */

static void
jitter_word_hash_key_function (union jitter_word key)
{
  /* Do nothing: word keys don't use heap allocation. */
}

/* Convert a jitter_int to a union jitter_word. */
inline static union jitter_word
jitter_int_to_word (jitter_int k)
{
  union jitter_word w = {.fixnum = k};
  return w;
}

bool
jitter_word_hash_table_has (const struct jitter_hash_table *t, jitter_int k)
{
  return jitter_hash_table_has (t,
                                jitter_int_to_word (k),
                                jitter_word_hash_function,
                                jitter_word_hash_key_equal);
}

union jitter_word
jitter_word_hash_table_get (const struct jitter_hash_table *t, jitter_int k)
{
  return jitter_hash_table_get (t,
                                jitter_int_to_word (k),
                                jitter_word_hash_function,
                                jitter_word_hash_key_equal);
}

void
jitter_word_hash_table_add (struct jitter_hash_table *t,
                            jitter_int k,
                            const union jitter_word value)
{
  jitter_hash_table_add (t,
                         jitter_int_to_word (k),
                         value,
                         jitter_word_hash_function);
}

void
jitter_word_hash_table_remove (struct jitter_hash_table *t,
                               jitter_int k,
                               jitter_word_function value_function)
{
  jitter_hash_table_remove (t,
                            jitter_int_to_word (k),
                            jitter_word_hash_key_function,
                            value_function,
                            jitter_word_hash_function,
                            jitter_word_hash_key_equal);
}

void
jitter_word_hash_finalize (struct jitter_hash_table *t,
                           jitter_word_function finalize_value)
{
  jitter_hash_finalize (t, jitter_word_hash_key_function, finalize_value);
}
