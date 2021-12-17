/* Jitter: VM-independent instruction implementation.

   Copyright (C) 2016, 2017 Luca Saiu
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


#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <jitter/jitter-instruction.h>

#include <jitter/jitter-malloc.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-hash.h>
#include <jitter/jitter-print.h>


/* Meta-instructions.
 * ************************************************************************** */

const struct jitter_meta_instruction*
jitter_lookup_meta_instruction (const struct jitter_hash_table *hash,
                                  const char *name)
{
  if (! jitter_string_hash_table_has (hash, name))
    jitter_fatal ("invalid instruction name \"%s\"", name);

  return jitter_string_hash_table_get (hash, name).pointer_to_void;
}

void
jitter_initialize_meta_instructions
   (struct jitter_hash_table *meta_instruction_string_hash,
    const struct jitter_meta_instruction *meta_instructions,
    size_t meta_instruction_no)
{
  jitter_hash_initialize (meta_instruction_string_hash);

  int i;
  for (i = 0; i < meta_instruction_no; i ++)
    {
      const struct jitter_meta_instruction *mi = meta_instructions + i;
      union jitter_word w = {.pointer_to_void = (void*) mi};
      jitter_string_hash_table_add (meta_instruction_string_hash,
                                    mi->name,
                                    w);
    }
}

void
jitter_finalize_meta_instructions (struct jitter_hash_table *
                                     meta_instruction_string_hash)
{
  jitter_string_hash_finalize (meta_instruction_string_hash, NULL);
}

struct jitter_instruction *
jitter_make_instruction (const struct jitter_meta_instruction * const mi)
{
  struct jitter_instruction *res
    = jitter_xmalloc (sizeof (struct jitter_instruction));
  const int arity = mi->parameter_no;;
  res->meta_instruction = mi;
  if (arity == 0)
    res->parameters = NULL;
  else
    {
      res->parameters
        = jitter_xmalloc (arity * sizeof (struct jitter_parameter*));
      int i;
      for (i = 0; i < arity; i ++)
        res->parameters[i] = jitter_make_instruction_parameter ();
    }
  return res;
}

void
jitter_destroy_instruction (struct jitter_instruction *i)
{
  if (i->parameters != NULL)
    {
      int j;
      size_t arity = i->meta_instruction->parameter_no;
      for (j = 0; j < arity; j ++)
        jitter_destroy_instruction_parameter (i->parameters [j]);
      free (i->parameters);
    }
  free (i);
}




/* Instructions
 * ************************************************************************** */

struct jitter_parameter *
jitter_make_instruction_parameter (void)
{
  struct jitter_parameter *res
    = jitter_xmalloc (sizeof (struct jitter_parameter));

  /* Initialize the whole memory to a fixed pattern, before setting fields.
     This may be important, since we compare parameters with memcmp and
     therefore the memory content must be entirely deterministic, including
     any unused part of the union. */
  memset (res, 0, sizeof (struct jitter_parameter));

  /* Try and prevent distraction mistakes by making the type field invalid. */
  res->type = jitter_parameter_type_uninitialized;

  /* Also make the label invalid. */
  res->label = -1;

  return res;
}

struct jitter_parameter*
jitter_clone_instruction_parameter (const struct jitter_parameter *original)
{
  struct jitter_parameter *res = jitter_make_instruction_parameter ();
  jitter_copy_instruction_parameter (res, original);
  return res;
}

void
jitter_copy_instruction_parameter (struct jitter_parameter *to,
                                   const struct jitter_parameter *from)
{
  /* The parameter data structure doesn't contain any pointer to data allocated
     with malloc that we have to manage explicitly, so this is a straightforward
     copy. */
  memcpy (to, from, sizeof (struct jitter_parameter));
}

void
jitter_destroy_instruction_parameter (struct jitter_parameter *p)
{
  free (p);
}




/* Parameter comparison.
 * ************************************************************************** */

int
jitter_compare_instruction_parameters (const struct jitter_parameter *a,
                                       const struct jitter_parameter *b)
{
  /* Parameters don't point to heap-allocated data as part of their values, so
     we can simply compare their memory contents lexicographically. */
  return memcmp (a, b, sizeof (struct jitter_parameter));
}

bool
jitter_instruction_parameters_equal (const struct jitter_parameter *a,
                                     const struct jitter_parameter *b)
{
  return jitter_compare_instruction_parameters (a, b) == 0;
}




/* VM instruction parameter printing.
 * ************************************************************************** */

void
jitter_default_literal_parameter_printer (jitter_print_context out,
                                          jitter_uint arg)
{
  jitter_print_char_star (out, "0x");
  jitter_print_jitter_uint (out, 16, arg);
}
