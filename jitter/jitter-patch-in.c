/* Jitter: patch-in functionality.

   Copyright (C) 2017 Luca Saiu
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


#include <jitter/jitter-patch-in.h>


/* Do nothing if not using fast branches.
 * ************************************************************************** */

/* This whole source file expands to nothing if patch-ins are not supported in
   this configuration.  The CPP inclusion above suffices to make the CPP
   definition of JITTER_HAVE_PATCH_IN visible, if it exists. */

#ifdef JITTER_HAVE_PATCH_IN




/* Include headers.
 * ************************************************************************** */

#include <stdbool.h>

#include <jitter/jitter.h>
#include <jitter/jitter-malloc.h>




/* Patch-in efficient data structures.
 * ************************************************************************** */

struct patch_in_table_entry *
jitter_make_patch_in_table (const struct jitter_patch_in_descriptor *descs,
                            size_t desc_no,
                            size_t specialized_instruction_no)
{
  /* Make an array of empty dynamic buffers. */
  struct jitter_dynamic_buffer *dbs
    = jitter_xmalloc (sizeof (struct jitter_dynamic_buffer)
                      * specialized_instruction_no);
  int i;
  for (i = 0; i < specialized_instruction_no; i ++)
    jitter_dynamic_buffer_initialize_with_allocated_size (dbs + i, 0);

  /* Scan the unordered descriptors, adding a pointer to each element in the
     appropriate place. */
  for (i = 0; i < desc_no; i ++)
    {
      const struct jitter_patch_in_descriptor *desc = descs + i;
      int opcode = desc->specialized_instruction_opcode;
      jitter_dynamic_buffer_push
         (dbs + opcode,
          & desc,
          sizeof (const struct jitter_patch_in_descriptor *));
    }

  /* Now build the result from the dynamic buffers, destroying them in the
     process. */
  struct patch_in_table_entry *res
    = jitter_xmalloc (sizeof (struct patch_in_table_entry)
                      * specialized_instruction_no);
  for (i = 0; i < specialized_instruction_no; i ++)
    {
      struct jitter_dynamic_buffer *db = dbs + i;
      struct patch_in_table_entry *entry = res + i;
      entry->descriptor_no
        = (db->used_size / sizeof (const struct jitter_patch_in_descriptor *));
      if (entry->descriptor_no == 0)
        {
          /* Don't bother extracting an array of no elements, to be copied.  We
             can save a little memory by releasing the useless empty dynamic
             buffer immediately, and using a NULL pointer in the result
             instead. */
          entry->descriptors = NULL;
          jitter_dynamic_buffer_finalize (db);
        }
      else
        /* Here I reuse the malloc-allocated memory for the dynamic buffer in
           the result, and don't need to finalize the dynamic buffer. */
        entry->descriptors = jitter_dynamic_buffer_extract_trimmed (db);
    }
  free (dbs);

  /* We have built the result data structure. */
  return res;
}

void
jitter_destroy_patch_in_table (struct patch_in_table_entry *table,
                               size_t specialized_instruction_no)
{
  int i;
  for (i = 0; i < specialized_instruction_no; i ++)
    {
      /* When there are no entries I use NULL pointers instead of a
         malloc-allocated array; therefore I have to call free only for
         specialized instructions having at least one patch-in descriptor. */
      struct patch_in_table_entry *entry = table + i;
      if (entry->descriptor_no != 0)
        free (entry->descriptors);
    }
  free (table);
}




/* Patch-in debugging.
 * ************************************************************************** */

/* Write user-readable textual information from the pointed patch-in to the
   pointed stream, prepending the given prefix string to each line. */
void
jitter_dump_patch_in_descriptor_with_prefix
   (FILE *f,
    const char *prefix,
    const struct jitter_patch_in_descriptor *p)
{
  fprintf (f, "%sopcode: %lu\n", prefix, (unsigned long) p->specialized_instruction_opcode);
  fprintf (f, "%soffset: %lu\n", prefix, (unsigned long) p->offset);
  fprintf (f, "%slength: %luB\n", prefix, (unsigned long) p->length);
  fprintf (f, "%scase: %lu\n", prefix, (unsigned long) p->patch_in_case);
  fprintf (f, "%sresidual index: %lu\n", prefix, (unsigned long) p->residual_index);
  fprintf (f, "%scase-dependend word 1: %lx\n", prefix,
           (unsigned long) p->case_dependent_word_1_uint);
  fprintf (f, "%scase-dependend word 2: %lx\n", prefix,
           (unsigned long) p->case_dependent_word_2_uint);
  fprintf (f, "%scase-dependend word 3: %lx\n", prefix,
           (unsigned long) p->case_dependent_word_3_uint);
}

/* Like jitter_dump_patch_in_descriptor_internal with an empty prefix.  This is
   meant for the user. */
void
jitter_dump_patch_in_descriptor (FILE *f,
                                 const struct jitter_patch_in_descriptor *p)
{
  jitter_dump_patch_in_descriptor_with_prefix (f, "", p);
}

/* Given an initial pointer to the patch-in descriptor array and the number of
   its elements, print textual information about each patch-in to the pointed
   stream. */
void
jitter_dump_patch_in_descriptors
   (FILE *f,
    const struct jitter_patch_in_descriptor descriptors[],
    size_t descriptor_no)
{
  fprintf (f, "descriptor_no is %lu\n", (unsigned long) descriptor_no);
  int i;
  for (i = 0; i < descriptor_no; i ++)
    {
      const struct jitter_patch_in_descriptor *p = descriptors + i;
      fprintf (f, "The %i-th descriptor is at %p:\n", i, p);
      jitter_dump_patch_in_descriptor_with_prefix (f, "    ", p);
    }
}

#endif // #ifdef JITTER_HAVE_PATCH_IN
