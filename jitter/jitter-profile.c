/* Jitter: profiling subsystem.

   Copyright (C) 2020 Luca Saiu
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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#include <jitter/jitter.h>

#include <stdlib.h>
#include <string.h>

#include <jitter/jitter-arithmetic.h>
#include <jitter/jitter-human-prefix.h>
#include <jitter/jitter-profile.h>
#include <jitter/jitter-malloc.h>




/* Runtime data structures.
 * ************************************************************************** */

void
jitter_profile_runtime_initialize (const struct jitter_vm *vm,
                                   struct jitter_profile_runtime *prd)
{
  struct jitter_count_profile_runtime *count_rdp
    = & prd->count_profile_runtime;
  struct jitter_sample_profile_runtime *sample_rdp
    = & prd->sample_profile_runtime;
  if (vm->configuration->instrumentation & jitter_vm_instrumentation_count)
    count_rdp->counts
      = jitter_xmalloc (sizeof (uint64_t) * vm->specialized_instruction_no);
  else
    count_rdp->counts = NULL; /* Out of defensiveness. */

  if (vm->configuration->instrumentation & jitter_vm_instrumentation_sample)
    sample_rdp->counts
      = jitter_xmalloc (sizeof (uint32_t) * vm->specialized_instruction_no);
  else
    sample_rdp->counts = NULL; /* Out of defensiveness. */

  /* Set all the counts to zero. */
  jitter_profile_runtime_clear (vm, prd);
}

void
jitter_profile_runtime_finalize (const struct jitter_vm *vm,
                                 struct jitter_profile_runtime *prd)
{
  struct jitter_count_profile_runtime *count_rdp
    = & prd->count_profile_runtime;
  struct jitter_sample_profile_runtime *sample_rdp
    = & prd->sample_profile_runtime;
  if (vm->configuration->instrumentation & jitter_vm_instrumentation_count)
    free (count_rdp->counts);
  if (vm->configuration->instrumentation & jitter_vm_instrumentation_sample)
    free (sample_rdp->counts);
}

struct jitter_profile_runtime *
jitter_profile_runtime_make (const struct jitter_vm *vm)
{
  struct jitter_profile_runtime *res
    = jitter_xmalloc (sizeof (struct jitter_profile_runtime));
  jitter_profile_runtime_initialize (vm, res);
  return res;
}

void
jitter_profile_runtime_destroy (const struct jitter_vm *vm,
                                struct jitter_profile_runtime *pd)
{
  jitter_profile_runtime_finalize (vm, pd);
  free (pd);
}

void
jitter_profile_runtime_clear (const struct jitter_vm *vm,
                              struct jitter_profile_runtime *prd)
{
  struct jitter_count_profile_runtime *count_rdp
    = & prd->count_profile_runtime;
  struct jitter_sample_profile_runtime *sample_rdp
    = & prd->sample_profile_runtime;
  int i;
  if (vm->configuration->instrumentation & jitter_vm_instrumentation_count)
    for (i = 0; i < vm->specialized_instruction_no; i ++)
      count_rdp->counts [i] = 0;
  if (vm->configuration->instrumentation & jitter_vm_instrumentation_sample)
    for (i = 0; i < vm->specialized_instruction_no; i ++)
      sample_rdp->counts [i] = 0;
  sample_rdp->current_specialized_instruction_opcode
    = -1 /* Out of defensiveness. */;
  sample_rdp->sample_no = 0;
}

void
jitter_profile_runtime_merge_from (const struct jitter_vm *vm,
                                   struct jitter_profile_runtime *to,
                                   const struct jitter_profile_runtime *from)
{
  struct jitter_count_profile_runtime *to_count_rdp
    = & to->count_profile_runtime;
  struct jitter_sample_profile_runtime *to_sample_rdp
    = & to->sample_profile_runtime;
  const struct jitter_count_profile_runtime *from_count_rdp
    = & from->count_profile_runtime;
  const struct jitter_sample_profile_runtime *from_sample_rdp
    = & from->sample_profile_runtime;
  int i;
  if (vm->configuration->instrumentation & jitter_vm_instrumentation_count)
    for (i = 0; i < vm->specialized_instruction_no; i ++)
      to_count_rdp->counts [i]
        += from_count_rdp->counts [i];
  if (vm->configuration->instrumentation & jitter_vm_instrumentation_sample)
    for (i = 0; i < vm->specialized_instruction_no; i ++)
      to_sample_rdp->counts [i]
        += from_sample_rdp->counts [i];
  to_sample_rdp->current_specialized_instruction_opcode
    = -1 /* Out of defensiveness. */;
  to_sample_rdp->sample_no
    += from_sample_rdp->sample_no;
}




/* Processed data structures.
 * ************************************************************************** */

/* A comparison function between struct jitter_profile_item objects, for
   qsort. */
static int
jitter_profile_item_compare (const void *pa, const void *pb)
{
  const struct jitter_profile_item *ia = pa;
  const struct jitter_profile_item *ib = pb;

  /* Notice that we sort in descending order.
     It would be nice to be able to just return the result of a subtraction, but
     here there is concrete danger of affecting the sign bit by truncating a
     wider number into an int. */
  if (ia->total_run_time_in_seconds > ib->total_run_time_in_seconds)
    return -1;
  else if (ia->total_run_time_in_seconds < ib->total_run_time_in_seconds)
    return 1;
  else
    {
      /* Secondary sort criterion */
      if (ia->execution_count > ib->execution_count)
        return -1;
      else if (ia->execution_count < ib->execution_count)
        return 1;
      else
        /* Tertiary sort criterion */
        return strcmp (ia->name, ib->name);
    }
}

/* Sort the item of the given profile as per the comment before the data
   structure. */
static void
jitter_profile_sort (const struct jitter_vm *vm, struct jitter_profile *p)
{
  qsort (p->items, p->item_no, sizeof (struct jitter_profile_item),
         jitter_profile_item_compare);
}

/* Return a fresh well-initialised but empty profile. */
static struct jitter_profile *
jitter_profile_make_empty (void)
{
  struct jitter_profile *res = jitter_xmalloc (sizeof (struct jitter_profile));
  res->items = NULL;
  res->item_no = 0;
  return res;
}

/* Common code factoring jitter_profile_specialized_from_runtime and
   jitter_profile_unspecialized_from_runtime . */
static struct jitter_profile *
jitter_profile_from_runtime (const struct jitter_vm *vm,
                             const struct jitter_profile_runtime *rp,
                             bool specialized)
{
  /* If the runtime profile was NULL, which means that the VM was not configured
     for profiling, return an empty profile as the result. */
  if (rp == NULL)
    return jitter_profile_make_empty ();

  /* The structure in struct jitter_profile_runtime is not really appropriate
     for unspecialised instructions.  In the case of unspecialised instructions
     we will build a structure of the same shape, but with information about
     each specialisation of each unspecialised instruction joined together. */
  struct jitter_profile_runtime *origin;
  size_t in_element_no;
  if (specialized)
    {
      in_element_no = vm->specialized_instruction_no;
      origin = (struct jitter_profile_runtime *) rp;
    }
  else
    {
      in_element_no
        = vm->meta_instruction_no + 1 /* One more element for the case of
                                         specialised instructions with no
                                         matching unspecialised version. */;
      /* Make a new profile runtime.  Its content will be blank, and we will
         rely on that by *adding* to the current value of each element, rather
         than simply setting.  Its indices will be unspecialised instruction
         opcodes *plus one*; the index 0 is for specialised instructions with
         no unspecialised counterpart, which are recorded as having opcode -1
         in vm->specialized_instruction_to_unspecialized_instruction . */
      origin = jitter_profile_runtime_make (vm);
      int specialized_i;
      for (specialized_i = 0;
           specialized_i < vm->specialized_instruction_no;
           specialized_i ++)
        {
          int unspecidalized_opcode
            = vm->specialized_instruction_to_unspecialized_instruction
                 [specialized_i];
          int index = unspecidalized_opcode + 1 /* See the comment above. */;
          if (vm->configuration->instrumentation
              & jitter_vm_instrumentation_count)
            origin->count_profile_runtime.counts [index]
              += rp->count_profile_runtime.counts [specialized_i];
          if (vm->configuration->instrumentation
              & jitter_vm_instrumentation_sample)
          origin->sample_profile_runtime.counts [index]
            += rp->sample_profile_runtime.counts [specialized_i];
        }
    }
  struct jitter_count_profile_runtime *count_rdp
    = & origin->count_profile_runtime;
  struct jitter_sample_profile_runtime *sample_rdp
    = & origin->sample_profile_runtime;

  struct jitter_profile *res = jitter_xmalloc (sizeof (struct jitter_profile));
  res->items
    = jitter_xmalloc (/* It is harmless to allocate more space than we need. */
                      vm->specialized_instruction_no
                      * sizeof (struct jitter_profile_item));

  int ini, outi = 0;
  for (ini = 0; ini < in_element_no; ini ++)
    {
      bool interesting = false;
      if (vm->configuration->instrumentation & jitter_vm_instrumentation_count
          && count_rdp->counts [ini] > 0)
        interesting = true;
      if (vm->configuration->instrumentation & jitter_vm_instrumentation_sample
          && sample_rdp->counts [ini] > 0)
        interesting = true;
      if (interesting)
        {
          const char *name;
          if (specialized)
            name = vm->specialized_instruction_names [ini];
          else if (ini == 0)
            name = "<specialized only>";
          else
            name = vm->meta_instructions [ini - 1].name;
          res->items [outi].name = name;
          if (vm->configuration->instrumentation
              & jitter_vm_instrumentation_count)
            res->items [outi].execution_count = count_rdp->counts [ini];
          else
            res->items [outi].execution_count = 0;
          if (vm->configuration->instrumentation
              & jitter_vm_instrumentation_sample)
            res->items [outi].total_run_time_in_seconds
              = (sample_rdp->counts [ini]
                 * (JITTER_PROFILE_SAMPLE_PERIOD_IN_MILLISECONDS / 1000.0));
          else
            res->items [outi].total_run_time_in_seconds = 0;
          outi ++;
        }
    }
  res->item_no = outi;

  /* If we used a temporary structure for unspecialised instructions we can
     dispose of it now. */
  if (! specialized)
    jitter_profile_runtime_destroy (vm, origin);

  /* Sort the output structure as per the specification in the header. */
  jitter_profile_sort (vm, res);
  return res;
}

struct jitter_profile *
jitter_profile_specialized_from_runtime (const struct jitter_vm *vm,
                                         const struct jitter_profile_runtime *rp)
{
  return jitter_profile_from_runtime (vm, rp, true);
}

struct jitter_profile *
jitter_profile_unspecialized_from_runtime (const struct jitter_vm *vm,
                                           const struct jitter_profile_runtime
                                           *rp)
{
  return jitter_profile_from_runtime (vm, rp, false);
}

void
jitter_profile_destroy (struct jitter_profile *p)
{
  free (p->items);
  free (p);
}




/* Printing.
 * ************************************************************************** */

/* Like jitter_mutable_routine_begin_class. */
static void
jitter_profile_begin_class (jitter_print_context ctx,
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

/* Print a warning saying that the VM is not instrumented for profiling. */
static void
jitter_profile_print_uninstrumented (jitter_print_context ct,
                                     const struct jitter_vm *vm)
{
  jitter_profile_begin_class (ct, vm, "warning");
  jitter_print_char_star (ct, "[Profiling instrumentation not enabled for ");
  jitter_print_char_star (ct, vm->configuration->lower_case_prefix);
  jitter_print_char_star (ct, "; you may want to recompile with at least one ");
  jitter_print_char_star (ct, "of the macros ");
  jitter_print_char_star (ct, vm->configuration->upper_case_prefix);
  jitter_print_char_star (ct, "_PROFILE_COUNT and ");
  jitter_print_char_star (ct, vm->configuration->upper_case_prefix);
  jitter_print_char_star (ct, "_PROFILE_SAMPLE defined.]\n");
  jitter_print_end_class (ct);
}

/* The common logic factoring jitter_maximal_name_length_in and
   jitter_maximal_count_width_in . */
#define JITTER_MAXIMUM_IN_ITEMS(the_items, expression, the_element_no)   \
  do                                                                     \
    {                                                                    \
      const struct jitter_profile_item *_jitter_items = (the_items);     \
      size_t _jitter_element_no = (the_element_no);                      \
      size_t _jitter_candidate_maximal_length = 0;                       \
      int _jitter_i;                                                     \
      for (_jitter_i = 0; _jitter_i < _jitter_element_no; _jitter_i ++)  \
        {                                                                \
          size_t _jitter_length = (expression);                          \
          if (_jitter_length > _jitter_candidate_maximal_length)         \
            _jitter_candidate_maximal_length = _jitter_length;           \
        }                                                                \
      return _jitter_candidate_maximal_length;                           \
    }                                                                    \
  while (false)

/* Return the maximum name length in the pointed array of struct
   jitter_profile_item values. */
static
size_t jitter_maximal_name_length_in (const struct jitter_profile_item *items,
                                      size_t element_no)
{
  JITTER_MAXIMUM_IN_ITEMS (items,
                           strlen (_jitter_items [_jitter_i].name),
                           element_no);
}

/* Return the maximum count width in the pointed array of struct
   jitter_profile_item values. */
static
size_t jitter_maximal_count_width_in (const struct jitter_profile_item *items,
                                      size_t element_no)
{
  JITTER_MAXIMUM_IN_ITEMS (items,
                           jitter_digit_no_unsigned_radix_10
                              (_jitter_items [_jitter_i].execution_count),
                           element_no);
}

void
jitter_profile_runtime_print_specialized (jitter_print_context ct,
                                          const struct jitter_vm *vm,
                                          const struct jitter_profile_runtime
                                          *prd)
{
  struct jitter_profile *p = jitter_profile_specialized_from_runtime (vm, prd);
  jitter_profile_print (ct, vm, p);
  jitter_profile_destroy (p);
}

void
jitter_profile_runtime_print_unspecialized (jitter_print_context ct,
                                            const struct jitter_vm *vm,
                                            const struct jitter_profile_runtime
                                            *prd)
{
  struct jitter_profile *p = jitter_profile_unspecialized_from_runtime (vm, prd);
  jitter_profile_print (ct, vm, p);
  jitter_profile_destroy (p);
}

void
jitter_profile_print (jitter_print_context ct,
                      const struct jitter_vm *vm,
                      const struct jitter_profile *p)
{
  /* If the code was not instrumented print a warning and do nothing else. */
  if (vm->configuration->instrumentation == jitter_vm_instrumentation_none)
    {
      jitter_profile_print_uninstrumented (ct, vm);
      return;
    }

  size_t element_no = p->item_no;
  struct jitter_profile_item *items = p->items;

  uint64_t count_sum = 0;
  double time_sum = 0;
  int i;
  for (i = 0; i < element_no; i ++)
    {
      count_sum += items [i].execution_count;
      time_sum += items [i].total_run_time_in_seconds;
    }
  size_t maximal_name_length = jitter_maximal_name_length_in (items, element_no);
  size_t maximal_count_width = jitter_maximal_count_width_in (items, element_no);
  for (i = 0; i < element_no; i ++)
    {
      struct jitter_profile_item *item = items + i;

      jitter_profile_begin_class (ct, vm, "profile-instruction");
      jitter_print_char_star (ct, item->name);
      jitter_print_end_class (ct);

      /* The printed ratio may be a run time ratio, available if sampling, of
         otherwise an execution count ratio. */
      double ratio;
      if (vm->configuration->instrumentation & jitter_vm_instrumentation_sample)
        ratio = item->total_run_time_in_seconds / time_sum;
      else
        ratio = item->execution_count / count_sum;

      /* Print enough additional spaces to get to the width of the longest
         instruction name, plus one space for separation, plus enough additional
         spaces so that, adding the count width, we get to the maximum width. */
      size_t name_length = strlen (item->name);
      size_t count_width = jitter_digit_no_radix_10 (item->execution_count);
      int j;
      for (j = name_length;
           j < maximal_name_length + 1 + maximal_count_width - count_width;
           j ++)
        jitter_print_char (ct, ' ');
      jitter_print_char (ct, ' ');

      /* Print the execution count, if available. */
      if (vm->configuration->instrumentation & jitter_vm_instrumentation_count)
        {
          jitter_profile_begin_class (ct, vm, "profile-execution-count");
          jitter_print_ulong_long (ct, 10, item->execution_count);
          jitter_print_end_class (ct);
          jitter_print_char (ct, ' ');
        }

      /* Print the run time if available and greater than zero; if the execution
         count is also available print the average execution time, otherwise the
         total. */
      if (item->total_run_time_in_seconds > 0)
        {
          /* Print the number, aligned to the right. */
          double run_time_ = item->total_run_time_in_seconds;
          if (vm->configuration->instrumentation
              & jitter_vm_instrumentation_count)
            /* This is safe: it is impossible, if an instruction was executed at
               least once (its run time is positive) and we have execution
               counters available, which are exact, for the count to be zero. */
            run_time_ /= item->execution_count;
          JITTER_HUMAN_READABLE_ (run_time, run_time_, false);
          char run_time_text [100];
          sprintf (run_time_text, "%8.3f%1ss", run_time, run_time_prefix);
          jitter_profile_begin_class (ct, vm, "profile-run-time");
          jitter_print_char_star (ct, run_time_text);
          jitter_print_end_class (ct);
          jitter_print_char (ct, ' ');

          /* Print the ratio as a percentage, aligned to the right. */
          char percentage [10];
          sprintf (percentage, "%5.1f%%", ratio * 100);
          jitter_profile_begin_class (ct, vm, "profile-time-ratio");
          jitter_print_end_class (ct);
          jitter_print_char_star (ct, percentage);
        }
      jitter_print_char (ct, '\n');
    }
}
