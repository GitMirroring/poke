/* Jitter: defective VM instructions.

   Copyright (C) 2018, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of Jitter.

   Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#include <jitter/jitter.h>
#include <jitter/jitter-defect.h>

#include <stdio.h>
#include <stdbool.h>

#include <jitter/jitter-fatal.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-print.h>
#include <jitter/jitter-vm.h>


/* Defect printing, defined unconditionally.
 * ************************************************************************** */

/* Begin using a class in the given print context, where the class name is
   formed by the concatenation of the lower-case prefix for the VM of the
   pointed executable routine, concatenated to a dash, concatenated
   to the given suffix.
   For example, if the mutable routine r belonged to a VM named "foo",
     jitter_disassemble_begin_class (ctx, r, "label")
   would open a class in the context ctx named "foo-label". */
__attribute__ ((unused))
static void
jitter_defect_begin_class (jitter_print_context ctx,
                           const struct jitter_vm *vm,
                           const char *suffix)
{
  char *prefix = vm->configuration->lower_case_prefix;
  size_t size = strlen (prefix) + 1 + strlen (suffix) + 1;
  char *buffer = jitter_xmalloc (size);
  sprintf (buffer, "%s-%s", prefix, suffix);
  jitter_print_begin_class (ctx, buffer);
  free (buffer);
}

void
jitter_defect_print_summary (jitter_print_context cx,
                             const struct jitter_vm *vm)
{
  int defect_no = vm->defect_no;
  int defective_instruction_no = vm->defective_specialized_instruction_no;
  int call_related_defective_instruction_no
    = vm->defective_call_related_specialized_instruction_no;
  int replacement_no = vm->replacement_specialized_instruction_no;

#define JITTER_BEGIN_WARNING_                         \
  if (defect_no > 0)                                  \
    {                                                 \
      jitter_print_end_class (cx);                    \
      jitter_defect_begin_class (cx, vm, "warning");  \
    }
#define JITTER_END_WARNING_                           \
  if (defect_no > 0)                                  \
    {                                                 \
      jitter_print_end_class (cx);                    \
      jitter_defect_begin_class (cx, vm, "comment");  \
    }

  jitter_defect_begin_class (cx, vm, "comment");

  JITTER_BEGIN_WARNING_;
  jitter_print_int (cx, 10, defect_no);
  JITTER_END_WARNING_;
  jitter_print_char_star (cx, " defects in ");
  JITTER_BEGIN_WARNING_;
  jitter_print_int (cx, 10, defective_instruction_no);
  JITTER_END_WARNING_;
  jitter_print_char_star (cx, " specialized instructions (of which ");
  JITTER_BEGIN_WARNING_;
  jitter_print_int (cx, 10, call_related_defective_instruction_no);
  JITTER_END_WARNING_;
  jitter_print_char_star (cx, " call-related), ");
  JITTER_BEGIN_WARNING_;
  jitter_print_int (cx, 10, replacement_no);
  JITTER_END_WARNING_;
  jitter_print_char_star (cx, " replacements");

#if defined (JITTER_DEFECT_REPLACEMENT_NEVER)
  if (defect_no > 0)
    {
      jitter_print_char_star (cx, " (");
      JITTER_BEGIN_WARNING_;
      jitter_print_char_star (cx, "INCORRECT CODE");
      JITTER_END_WARNING_;
      jitter_print_char_star (cx, "!)");
    }
#endif // #if defined (JITTER_DEFECT_REPLACEMENT_NEVER)

#if ! defined (JITTER_HAVE_DEFECT_REPLACEMENT)
  jitter_print_char_star (cx, " (defects cannot happen in this configuration)");
#endif // #if ! defined (JITTER_HAVE_DEFECT_REPLACEMENT)

  jitter_print_char_star (cx, ".");

  jitter_print_end_class (cx);
  jitter_print_char (cx, '\n');

#undef JITTER_BEGIN_WARNING_
#undef JITTER_END_WARNING_
}


/* Conditional expansion: begin.
 * ************************************************************************** */

/* Expand to nothing if we are not using defect replacement.  It is harmless to
   unconditionally keep the declarations in the header. */
#if defined (JITTER_HAVE_DEFECT_REPLACEMENT)




/* Defect efficient data structures.
 * ************************************************************************** */

void
jitter_fill_replacement_table
   (jitter_uint *replacement_table,
    struct jitter_vm *vm,
    const jitter_uint *worst_case_replacement_table,
    const jitter_uint *call_related_specialized_instruction_ids,
    jitter_uint call_related_specialized_instruction_id_no,
    const bool *specialized_instruction_call_relateds,
    const struct jitter_defect_descriptor *descs,
    size_t desc_no,
    jitter_int correct_displacement)
{
  size_t specialized_instruction_no = vm->specialized_instruction_no;

  /* Temporarily use the replacement table we are filling to store Booleans:
     false for non-defective instructions, true for defective instructions.
     These Booleans will be replaced with opcodes in the final pass. */

  /* First pass: mark every specialized instruction as non-defective by
     default. */
  int i;
  for (i = 0; i < specialized_instruction_no; i ++)
    replacement_table [i] = false;

  /* Second pass, over the defect descriptor array (not the replacement table):
     scan defect descriptors, and for every defect found mark the associated
     specialized instruction as to be replaced.  Of course one defect is enough
     to make a specialized instruction defective, even if the same specialized
     instruction has other descriptors not encoding any defect. */
  int defect_no = 0;
  int defective_no = 0;
  int call_related_defective_no = 0;
  for (i = 0; i < desc_no; i ++)
    if (__builtin_expect (descs [i].displacement != correct_displacement,
                          false))
      {
        fprintf (stderr, "DEBUG: displacement is %li instaed of %li\n", (long) descs [i].displacement, (long) correct_displacement);
        bool call_related
          = specialized_instruction_call_relateds [descs[i].specialized_opcode];
        /* Count the defect, and mark its instruction as to be replaced.  If
           this is the first defect we find in the instruction also update the
           defective instruction counter.
           Keep a separate count of call-related defective instructions. */
        defect_no ++;
        if (! replacement_table [descs [i].specialized_opcode])
          {
            defective_no ++;
            if (call_related)
              call_related_defective_no ++;
          }
        replacement_table [descs [i].specialized_opcode] = true;
        fprintf (stderr,
                 "The specialized instruction %s is defective.\n",
                 vm->specialized_instruction_names
                    [descs [i].specialized_opcode]);
      }

  /* Third pass: if any call-related instruction has been found to be defective
     then set them all to be replaced. */
  if (call_related_defective_no > 0)
    {
      fprintf (stderr, "At least one call-related instruction is defective.  Marking them all as to be replaced:\n");
      for (i = 0; i < call_related_specialized_instruction_id_no; i ++)
        {
          fprintf (stderr, "* Marking %s as to be replaced.\n", vm->specialized_instruction_names [call_related_specialized_instruction_ids [i]]);
          replacement_table [call_related_specialized_instruction_ids [i]] = true;
        }
    }

#if defined (JITTER_DEFECT_REPLACEMENT_NEVER)
  /* Remove every true value in the replacement table, which now contains
     Booleans: this will prevent any replacement.
     This is dangerous, and of course should never be used in production. */
  for (i = 0; i < specialized_instruction_no; i ++)
    replacement_table [i] = false;
#endif // #if defined (JITTER_DEFECT_REPLACEMENT_NEVER)

#if defined (JITTER_DEFECT_REPLACEMENT_ALL_POSSIBLE)
  /* This stress test is useful to check how defect replacements behave,
     but should not be used in production; I have not tested its performance
     impact, but that could be severe. */
  /* Mark every instruction which has a replacement as to be replaced; do not
     update the counts, because the extra replacements set up here do not mean
     that we have actually found new defects. */
  for (i = 0; i < specialized_instruction_no; i ++)
    if (worst_case_replacement_table [i] != i)
      replacement_table [i] = true;
#endif // #if defined (JITTER_DEFECT_REPLACEMENT_ALL_POSSIBLE)

  /* Fourth and final pass, over the replacement table: replace Booleans with
     specialized opcodes.  The specialized opcode will be equal to the index for
     non-defective instruction, and the replacement specialized opcode for
     defective specialized instructions.
     Count the replacements we are performing. */
  int replacement_no = 0;
  int missing_replacement_no = 0;
  for (i = 0; i < specialized_instruction_no; i ++)
    if (__builtin_expect (replacement_table [i],
                          false))
      {
        /* If an instruction which is not supposed to be defective (for example
           a replacement) is in fact defective, fail fatally and refuse to run.
           This happens when the fake dispatcher label in assembly is different
           from the fake dispatcher label in C.  Here we print the name of the
           instruction involved -- in such a case there will be many... */
        if (worst_case_replacement_table [i] == i)
          {
            fprintf (stderr,
                     "ERROR: specialized instruction %s (opcode %i) "
                     "is defective but has no replacement\n",
                     vm->specialized_instruction_names [i], i);
            missing_replacement_no ++;
          }
        replacement_table [i] = worst_case_replacement_table [i];
        replacement_no ++;
      }
    else
      /* The specialised instruction with opcode i is not defective: its
         "replacement" is itself. */
      replacement_table [i] = i;

  /* ...If an instruction which is not supposed to be defective (for example a
     replacement) is in fact defective, fail fatally and refuse to run -- unless
     JITTER_DEFECT_REPLACEMENT_NEVER is defined, in which case this is expected
     behaviour; anyway, at least print a warning in that case as well. */
#if defined (JITTER_DEFECT_REPLACEMENT_NEVER)
  fprintf (stderr,
           "ERROR: at least one defective instruction has no replacement: this "
           "code is subtly incorrect and MUST NOT BE USED IN PRODUCTION.\n"
           "Proceeding anyway, since Jitter has been configured with "
           "--disable-defect-replacement .");
#else
  if (missing_replacement_no > 0) //////////////////////////////////////////////////
    jitter_fatal ("at least one defective instruction has no replacement: "
                  "failing now instead of running with subtle bugs");
#endif

  /* Update defect counts in the VM struct. */
  vm->defect_no = defect_no;
  vm->defective_specialized_instruction_no = defective_no;
  vm->defective_call_related_specialized_instruction_no
    = call_related_defective_no;
  vm->replacement_specialized_instruction_no = replacement_no;

#if 0
  fprintf (stderr, "%i defects, %i defective instructions, %i replacements\n",
           defect_no, defective_no, replacement_no);
#endif
}




/* Defect debugging.
 * ************************************************************************** */

void
jitter_dump_replacement_table (FILE *f,
                               const jitter_uint *replacement_table,
                               const struct jitter_vm *vm)
{
  size_t specialized_instruction_no = vm->specialized_instruction_no;
  const char * const * specialized_instruction_names
    = vm->specialized_instruction_names;

  int defective_count = 0;
  jitter_uint i;
  for (i = 0; i < specialized_instruction_no; i ++)
    {
      jitter_uint replacement_opcode = replacement_table [i];
      if (replacement_opcode != i)
        {
          fprintf (f, "%s %i -> %s %i\n",
                   specialized_instruction_names [i], (int) i,
                   specialized_instruction_names [replacement_opcode],
                   (int) replacement_opcode);
          defective_count ++;
        }
    }
  if (defective_count > 0)
    fprintf (f, "Replaced %i specialized instructions.\n", defective_count);
}




/* Conditional expansion: end.
 * ************************************************************************** */

/* End of the part to be only expanded when patch-ins are in use. */
#endif // #if defined (JITTER_HAVE_DEFECT_REPLACEMENT)
