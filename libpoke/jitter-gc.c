/* Jitter garbage collector.

   Copyright (C) 2020, 2022, 2024 Luca Saiu
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


#include <jitter/jitter.h>

#include <stdbool.h>
#include <assert.h>  // FIXME: remove in the end.  I probably do not want it.
#include <stdlib.h>

// // FIXME: trashy hack.  This is just to be able to reuse my complex tagging macros from JitterLisp
// #define JITTERLISP_LITTER  // To placate sanity checks in JitterLisp headers.  Not littering, of course
// // Even trashier hack.
// #define JITTER_INTERNAL

#include "jitter-gc.h"

#include <jitter/jitter-aligned-block.h>
#include <jitter/jitter-arithmetic.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-word-set.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-human-prefix.h>
#include <jitter/jitter-time.h>

//#include "example-vms/jitterlisp/jitterlisp.h"




/* Forward declarations.
 * ************************************************************************** */

static size_t
jitter_gc_space_used_size_in_bytes (const struct jitter_gc_space *s);
static size_t
jitter_gc_space_allocated_size_in_bytes (const struct jitter_gc_space *s);
static void
jitter_gc_scavenge (struct jitter_gc_heaplet *a);
static void
jitter_gc_update_heuristics (struct jitter_gc_heaplet *a,
                             enum jitter_gc_collection_kind k);
static void
jitter_gc_global_roots_initialize (struct jitter_list_header *grp);
static void
jitter_gc_global_roots_finalize (struct jitter_list_header *grp);
static void
jitter_gc_hooks_initialize (struct jitter_list_header *hs);
static void
jitter_gc_hooks_finalize (struct jitter_list_header *hs);
static size_t
jitter_gc_round_threshold (size_t threshold_in_bytes);
static void
jitter_gc_set_up_spaces (struct jitter_gc_heaplet *a,
                         enum jitter_gc_collection_kind kind);
static void
jitter_gc_clean_spaces_before (struct jitter_gc_heaplet *a);
static void
jitter_gc_reset_for_scanning (struct jitter_gc_heaplet *a);
static void
jitter_gc_join_fromspace_finalizables_into_candidate_dead
   (struct jitter_gc_heaplet *a);
static int /*bool*/
jitter_gc_handle_object_finalization (struct jitter_gc_heaplet *a);
static void
jitter_gc_clean_spaces_after (struct jitter_gc_heaplet *a);
static void
jitter_gc_heaplet_flip (struct jitter_gc_heaplet *a,
                        enum jitter_gc_collection_kind kind);
static void
jitter_gc_global_gc_if_needed_and_unlock (struct jitter_gc_heap *h,
                                          struct jitter_gc_heaplet *a);
static size_t
jitter_gc_list_length (struct jitter_list_header *list);
static size_t
jitter_gc_object_finalize_all_heap_finalizables (struct jitter_gc_heap *h);
static size_t
jitter_gc_object_finalize_all_heaplet_finalizables (struct jitter_gc_heaplet *a);




/* Defensiveness and debugging.
 * ************************************************************************** */

/* Define JITTER_GC_ARE_WE_DEBUGGING as a Boolean constant, so that we can use
   it in C expressions and conditional statements, always optimised by the
   compiler, without CPP conditionals. */
#if defined (JITTER_GC_DEBUG)
# define JITTER_GC_ARE_WE_DEBUGGING  true
#else
# define JITTER_GC_ARE_WE_DEBUGGING  false
#endif /* #if defined (JITTER_GC_DEBUG) */

/* Print the given number of spaces to the log stream. */
__attribute__ ((unused))
static void
jitter_gc_indent (int indentation_level)
{
#if defined (JITTER_GC_LOG)
  /* If I simply used an empty format string, as it would be reasonable here,
     GCC would give a warning:
       warning: zero-length gnu_printf format string [-Wformat-zero-length]
     I am not sure if this warning is a particularly good idea when there are
     no variadic arguments. */
  jitter_gc_log_i (indentation_level, "%s", "");
#endif /* #if defined (JITTER_GC_LOG) */
}


/* Perform sanity checks.  The code compiles to nothing when no constrain is
   violated, so it is reasonable to call this code from any initialisation
   function without providing a global initialisation, at no cost.
   I have chosen to check sanity from heaplet construction, since every
   program using the collector will have to execute it early.  */
static void
jitter_gc_check_sanity (void)
{
  if (JITTER_GC_NON_NURSERY_STEP_NO < 0)
    jitter_fatal ("JITTER_GC_NON_NURSERY_STEP_NO must be non-negative, but is "
                  "now %li", (long) JITTER_GC_NON_NURSERY_STEP_NO);
  if (JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS < 2)
    jitter_fatal ("JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS must be at least 2");
  if (JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS > 31)
    jitter_fatal ("JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS must be less than 32");
  if (! JITTER_IS_A_POWER_OF_TWO (sizeof (jitter_gc_tagged_object)))
    jitter_fatal ("the word size appears to be %li, which is not a power of "
                  "two; this machine is *very* bizarre.  Sorry",
                  (long) sizeof (jitter_gc_tagged_object));
  if (! JITTER_IS_A_POWER_OF_TWO (JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS))
    jitter_fatal ("JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS is %li which is not "
                  "a power of two",
                  (long) JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS);

  if (! JITTER_IS_A_POWER_OF_TWO (JITTER_GC_BLOCK_SIZE_IN_BYTES))
    jitter_fatal ("JITTER_GC_BLOCK_SIZE_IN_BYTES is %li which is not a power "
                  "of two",
                  (long) JITTER_GC_BLOCK_SIZE_IN_BYTES);
  if (JITTER_GC_BLOCK_HEADER_ROUNDED_SIZE_IN_BYTES
      >= JITTER_GC_BLOCK_SIZE_IN_BYTES)
    jitter_fatal ("the block size %li B is too small: there is no place for "
                  "any payload after a %li-B header, rounded up to %li B",
                  (long) JITTER_GC_BLOCK_SIZE_IN_BYTES,
                  (long) sizeof (struct jitter_gc_block),
                  (long) JITTER_GC_BLOCK_HEADER_ROUNDED_SIZE_IN_BYTES);
  if (JITTER_GC_RECENT_BIAS < 0.5 || JITTER_GC_RECENT_BIAS >= 1)
    jitter_fatal ("JITTER_GC_RECENT_BIAS is %f, but valid values are between "
                  "0.5 included and 1 not included",
                  (double) JITTER_GC_RECENT_BIAS);
  /* In fact some of the checks can be performed even before run time, with CPP.
     I am placing them at this point in the source just because they
     conceputally belong here. */

  if (JITTER_GC_DEFAULT_NURSERY_GROWTH_RATIO <= 1)
    jitter_fatal ("JITTER_GC_DEFAULT_NURSERY_GROWTH_RATIO is %f, but valid "
                  "values are strictly greater than 1",
                  (double) JITTER_GC_DEFAULT_NURSERY_GROWTH_RATIO);
  if (JITTER_GC_DEFAULT_NURSERY_SHRINKAGE_RATIO <= 0
      || JITTER_GC_DEFAULT_NURSERY_SHRINKAGE_RATIO >= 1)
    jitter_fatal ("JITTER_GC_DEFAULT_NURSERY_GROWTH_RATIO is %f, but valid "
                  "values are strictly positive and less than 1",
                  (double) JITTER_GC_DEFAULT_NURSERY_GROWTH_RATIO);

  if (JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE
      > JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE)
    jitter_fatal ("JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE is %f "
                  "which is greater than "
                  "JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE which is %f",
                  (double) JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE,
                  (double) JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE);

if (JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE < 0)
    jitter_fatal ("JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE is %f "
                  "which is less than 0",
                  (double) JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE);
if (JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE < 0)
    jitter_fatal ("JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE is %f "
                  "which is less than 0",
                  (double) JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE);
if (JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE > 1)
    jitter_fatal ("JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE is %f "
                  "which is greater than 1",
                  (double) JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE);
if (JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE > 1)
    jitter_fatal ("JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE is %f "
                  "which is greater than 1",
                  (double) JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE);

  if (JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES <= 0)
    jitter_fatal ("JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES is "
                  "non-positive");
  /* FIXME: check that the other tuning parameters are ordered sensibly: a
     maximum should not be smaller than its matching minimum. */
// FIXME: CPP #if ... #error tests, if any.
}

static void
jitter_gc_space_dump (struct jitter_gc_space *s,
                      size_t *total_used,
                      size_t *total_allocated)
{
  size_t used_size_ = jitter_gc_space_used_size_in_bytes (s);
  * total_used += used_size_;
  size_t allocated_size_ = jitter_gc_space_allocated_size_in_bytes (s);
  * total_allocated += allocated_size_;
  JITTER_HUMAN_READABLE_ (used_size, used_size_, true);
  JITTER_HUMAN_READABLE_ (allocated_size, allocated_size_, true);
  printf ("%-10s %8.1f %2sB of %8.1f %2sB",
          s->name,
          used_size, used_size_prefix,
          allocated_size, allocated_size_prefix);
}

void
jitter_gc_heaplet_dump (struct jitter_gc_heaplet *a)
{
  size_t total_used_ = 0;
  size_t total_allocated_ = 0;

  printf (" (Unused:           ");
  jitter_gc_space_dump (& a->unused_space, & total_used_, & total_allocated_);
  printf (")\n");
  printf ("  Nursery:          ");
  jitter_gc_space_dump (& a->nursery, & total_used_, & total_allocated_);
  printf ("\n");
  int i;
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    {
      printf ("  Ageing%2i:         ", i);
      jitter_gc_space_dump (a->young_ageing_spaces [i], & total_used_, & total_allocated_);
      printf ("\n");

      printf ("  Young reserve%2i:  ", i);
      jitter_gc_space_dump (a->young_reserves [i], & total_used_, & total_allocated_);
      printf ("\n");
    }

  printf ("  Oldspace:         ");
  jitter_gc_space_dump (a->oldspace, & total_used_, & total_allocated_);
  printf ("\n");

  printf ("  Oldspace reserve: ");
  jitter_gc_space_dump (a->old_reserve, & total_used_, & total_allocated_);
  printf ("\n");

#if defined (JITTER_GC_ENABLE_SHARING)
  printf ("  Shared own:       ");
  jitter_gc_space_dump (& a->shared_space_own, & total_used_, & total_allocated_);
  printf ("\n");
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  JITTER_HUMAN_READABLE_ (total_used, total_used_, true);
  JITTER_HUMAN_READABLE_ (total_allocated, total_allocated_, true);
  printf ("  TOTAL:            ");
  printf ("%-10s %8.1f %2sB of %8.1f %2sB\n",
          "",
          total_used, total_used_prefix,
          total_allocated, total_allocated_prefix);
}

const char *
jitter_gc_shape_finalization_kind_to_string
   (enum jitter_gc_shape_finalization_kind kind)
{
  switch (kind)
    {
    case jitter_gc_shape_finalization_kind_none:
      return "none";
    case jitter_gc_shape_finalization_kind_quick:
      return "quick";
    case jitter_gc_shape_finalization_kind_complete_object:
      return "complete-object";
    default:
      return "<INVALID FINALISATION KIND>";
    }
}

const char *
jitter_gc_finalizable_place_to_string (enum jitter_gc_finalizable_place list)
{
  switch (list)
    {
    case jitter_gc_finalisable_place_not_to_be_finalized:
      return "not-to-be-finalized";

    case jitter_gc_finalisable_place_to_be_finalized:
      return "to-be-finalized";

    default:
      return "<INVALID FINALISABLE LIST>";
    }
}

const char *
jitter_gc_collection_kind_to_string (enum jitter_gc_collection_kind kind)
{
  switch (kind)
    {
    case jitter_gc_collection_kind_ssb_flush:
      return "flush-ssb";

    case jitter_gc_collection_kind_minor:
      return "minor";
    case jitter_gc_collection_kind_major:
      return "major";
    case jitter_gc_collection_kind_global:
      return "global";

    case jitter_gc_collection_kind_share:
      return "share";

    default:
      //jitter_fatal ("invalid collection kind %i", (int) kind);
      return "<INVALID COLLECTION KIND>";
    }
}

const char *
jitter_gc_action_to_string (enum jitter_gc_action action)
{
  switch (action)
    {
    case jitter_gc_action_default:       return "default";
    case jitter_gc_action_block_change:  return "change-block";
    case jitter_gc_action_force_minor:   return "force-minor";
    case jitter_gc_action_force_major:   return "force-major";
    case jitter_gc_action_force_either:  return "force-either";
    case jitter_gc_action_force_global:  return "force-global";
    case jitter_gc_action_share:         return "share";
    default:                             return "<INVALID ACTION>";
    }
}

const char *
jitter_gc_generation_to_string (enum jitter_gc_generation generation)
{
  switch (generation)
    {
    case jitter_gc_generation_young:     return "young";
    case jitter_gc_generation_old:       return "old";
    case jitter_gc_generation_shared:    return "shared";
    case jitter_gc_generation_unused:    return "unused";
    case jitter_gc_generation_immortal:  return "immortal";
    default:                             return "<INVALID GENERATION>";
    }
}

void
jitter_gc_print_statistics (FILE *f, struct jitter_gc_heaplet *a)
{
  double total_run_time
    = jitter_time_subtract_from_now (a->time_at_the_end_of_initialization);

  double total_collection_time = a->total_collection_time;
  double total_share_time = a->total_share_time;
  double mutator_run_time = total_run_time - total_collection_time;
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  if (a->collection_no > 0)
    {
      /* If we are measuring finalisation time then to not count that as part of
         collection time. */
      total_collection_time -= a->total_finalization_time;

      /* Mutation time as considered above is correct ... */
    }
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  /* ...with this correction. */
  if (a->ssb_flush_no > 0)
    {
      /* If we are measuring SSB flush time as well then do not count it as
         mutation time. */
      mutator_run_time -= a->total_ssb_flush_time;
    }
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  fprintf (f, "Total run time:        %11.6f s                [ %f s ]\n", total_run_time,
           total_run_time);
  fprintf (f, "    Total mutation time:    %11.2f s (%.2f %%) [ %f s ]\n",
           mutator_run_time, mutator_run_time / total_run_time * 100,
           mutator_run_time);
  fprintf (f, "    Total collection time:  %11.2f s (%.2f %%) [ %f s ]\n",
           total_collection_time, total_collection_time / total_run_time * 100,
           total_collection_time);
  if (a->share_no > 0)
    fprintf (f, "    Total share time:       %11.2f s (%.2f %%) [ %f s ]\n",
             total_share_time, total_share_time / total_run_time * 100,
             total_share_time);
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  if (a->ssb_flush_no > 0)
    fprintf (f, "    Total SSB flush time:   %11.2f s (%.2f %%)  [ %f s ]\n",
             a->total_ssb_flush_time,
             a->total_ssb_flush_time / total_run_time * 100,
             a->total_ssb_flush_time);
  if (a->collection_no > 0)
    fprintf (f, "    Total finalisation time:%11.2f s (%.2f %%)  [ %f s ]\n",
             a->total_finalization_time,
             a->total_finalization_time / total_run_time * 100,
             a->total_finalization_time);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  JITTER_HUMAN_READABLE_ (block_size, JITTER_GC_BLOCK_SIZE_IN_BYTES, true);
  JITTER_HUMAN_READABLE_ (block_payload_size,
                          JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES, true);
  fprintf (f, "    Block size:             %11.2f %sB (%.3f %sB payload",
           block_size, block_size_prefix, block_payload_size,
           block_payload_size_prefix);
#if 0
  double block_overhead_ratio
    = 1. - ((double) JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES
            / JITTER_GC_BLOCK_SIZE_IN_BYTES);
  fprintf (f, ", %.5f%% overhead", 100 * block_overhead_ratio);
#endif
  fprintf (f, ")\n");
  JITTER_HUMAN_READABLE_ (total_memory_allocated, a->total_bytes_allocated,
                          true);
  JITTER_HUMAN_READABLE_ (total_memory_copied, a->total_bytes_copied, true);
  fprintf (f, "    Total memory allocated: %11.2f %sB\n",
           total_memory_allocated, total_memory_allocated_prefix);
  fprintf (f, "    Total memory copied:    %11.2f %sB\n",
           total_memory_copied, total_memory_copied_prefix);
  JITTER_HUMAN_READABLE_ (allocation_rate_mutation,
                          a->total_bytes_allocated / mutator_run_time, true);
  fprintf (f, "    Allocation rate (mut.): %11.2f %sB/s\n",
           allocation_rate_mutation, allocation_rate_mutation_prefix);
  if (a->ssb_flush_no > 0)
    {
      double average_ssb_length = a->total_ssb_length / a->ssb_flush_no;
      fprintf (f, "    SSB flushes:            %8lu\n",
               (unsigned long) a->ssb_flush_no);
      fprintf (f, "      SSB length:              %11.3f elt\n",
               average_ssb_length);
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
      JITTER_HUMAN_READABLE_ (average_ssb_flush_time,
                              a->total_ssb_flush_time / a->ssb_flush_no, false);
      fprintf (f, "        SSB flush time:          %11.3f %ss",
               average_ssb_flush_time, average_ssb_flush_time_prefix);
      if (a->total_ssb_length > 0)
        {
          JITTER_HUMAN_READABLE_ (average_ssb_flush_time_per_element,
                                  a->total_ssb_flush_time / a->total_ssb_length, false);
      fprintf (f, " (%.3f %ss/elt)", average_ssb_flush_time_per_element,
               average_ssb_flush_time_per_element_prefix);
        }
      fprintf (f, "\n");
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
   }
  fprintf (f, "    Collected:              %8lu    times\n",
           (unsigned long) a->collection_no);
  if (a->collection_no > 0)
    {
      /* Include finalisation time within the collection pause time.  There is
         no need for this correction with the other pause times below, which are
         measured directly and not meddled with here by subtracting finalisation
         times.  */
      double total_finalization_time = 0;
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
      total_finalization_time = a->total_finalization_time;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
      JITTER_HUMAN_READABLE_ (average_pause_time,
                              ((total_collection_time + total_finalization_time)
                               / a->collection_no),
                              false);
      JITTER_HUMAN_READABLE_ (average_minor_pause_time,
                              a->total_minor_collection_time / a->collection_no,
                              false);
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
      JITTER_HUMAN_READABLE_ (average_root_size,
                              a->total_root_size_in_bytes / a->collection_no,
                              true);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
      JITTER_HUMAN_READABLE_ (average_memory_copied,
                              a->total_bytes_copied / a->collection_no, true);
      JITTER_HUMAN_READABLE_ (copy_rate,
                              a->total_bytes_copied / total_collection_time,
                              true);
      double average_initial_remembered_set_size
        = a->total_initial_remembered_set_size / a->collection_no;
      double average_final_remembered_set_size
        = a->total_final_remembered_set_size / a->collection_no;
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
      fprintf (f, "      Root size:               %11.3f %sB\n",
               average_root_size, average_root_size_prefix);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
      fprintf (f, "        Pause time incl. fin.:   %11.3f %ss\n",
               average_pause_time, average_pause_time_prefix);
      fprintf (f, "        Memory copied:           %11.3f %sB\n",
               average_memory_copied, average_memory_copied_prefix);
      fprintf (f, "        Copy rate:               %11.3f %sB/s\n",
               copy_rate, copy_rate_prefix);
      fprintf (f, "        Remembered set at entry: %11.3f elt\n",
               average_initial_remembered_set_size);
      fprintf (f, "        Remembered set at exit:  %11.3f elt\n",
               average_final_remembered_set_size);
    }
  if (a->minor_collection_no > 0)
    {
      JITTER_HUMAN_READABLE_ (average_minor_pause_time,
                              (a->total_minor_collection_time
                               / a->minor_collection_no),
                              false);
      fprintf (f, "    Minor collections:      %8lu\n",
               (unsigned long) a->minor_collection_no);
      JITTER_HUMAN_READABLE_ (average_nursery_used_size,
                              (a->total_nursery_used_size_in_bytes
                               / a->collection_no),
                              true);
      fprintf (f, "      Nursery:                 %11.3f %sB\n",
               average_nursery_used_size, average_nursery_used_size_prefix);
      if (JITTER_GC_NON_NURSERY_STEP_NO > 0)
        fprintf (f, "      Non-nursery steps:       %9lu steps\n",
                 (unsigned long) JITTER_GC_NON_NURSERY_STEP_NO);
      fprintf (f, "        Pause time incl. fin.:   %11.3f %ss\n",
               average_minor_pause_time, average_minor_pause_time_prefix);
      JITTER_HUMAN_READABLE_ (copy_rate,
                              (a->total_bytes_copied_minor
                               / a->total_minor_collection_time),
                              true);
      fprintf (f, "        Copy rate:               %11.3f %sB/s\n",
               copy_rate, copy_rate_prefix);
#if JITTER_GC_NON_NURSERY_STEP_NO > 0
      double average_nursery_survival_ratio
        = a->total_nursery_survival_ratio / a->minor_collection_no;
#endif
      double average_minor_survival_ratio
        = a->total_minor_survival_ratio / a->minor_collection_no;
#if JITTER_GC_NON_NURSERY_STEP_NO > 0
      fprintf (f, "        Nursery survival ratio:  %11.3f %%\n",
               average_nursery_survival_ratio * 100);
#endif
      fprintf (f, "        Survival ratio:          %11.3f %%\n",
               average_minor_survival_ratio * 100);
      JITTER_HUMAN_READABLE_ (average_memory_copied_to_oldspace,
                              (a->total_bytes_copied_to_oldspace
                               / a->minor_collection_no),
                              true);
      fprintf (f, "        Memory made old:         %11.3f %sB\n",
               average_memory_copied_to_oldspace,
               average_memory_copied_to_oldspace_prefix);
    }
  if (a->major_collection_no > 0)
    {
      JITTER_HUMAN_READABLE_ (average_major_pause_time,
                              (a->total_major_collection_time
                               / a->major_collection_no),
                              false);
      double average_major_survival_ratio
        = a->total_major_survival_ratio / a->major_collection_no;
      fprintf (f, "    Major collections:      %8lu\n",
               (unsigned long) a->major_collection_no);
      JITTER_HUMAN_READABLE_ (average_old_space_used_size,
                              (a->total_old_space_used_size_in_bytes
                               / a->major_collection_no),
                              true);
      fprintf (f, "      Old space used size:     %11.3f %sB\n",
               average_old_space_used_size, average_old_space_used_size_prefix);
      fprintf (f, "        Pause time incl. fin:    %11.3f %ss\n",
               average_major_pause_time, average_major_pause_time_prefix);
      JITTER_HUMAN_READABLE_ (copy_rate,
                              (a->total_bytes_copied_major
                               / a->total_major_collection_time),
                              true);
      fprintf (f, "        Copy rate:               %11.3f %sB/s\n",
               copy_rate, copy_rate_prefix);
      fprintf (f, "        Survival ratio:          %11.3f %%\n",
               average_major_survival_ratio * 100);
    }
  if (a->share_no > 0)
    {
      JITTER_HUMAN_READABLE_ (average_share_pause_time, (a->total_share_time
                                                         / a->share_no),
                              false);
      fprintf (f, "    Shares:                 %8lu\n",
               (unsigned long) a->share_no);
      fprintf (f, "        Pause time incl. fin:    %11.3f %ss\n",
               average_share_pause_time, average_share_pause_time_prefix);
    }
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  if (a->collection_no > 0)
    {
      JITTER_HUMAN_READABLE_ (per_collection_finalization_time,
                              a->total_finalization_time / a->collection_no,
                              false);
      JITTER_HUMAN_READABLE_ (finalized_objects_per_coll,
                              a->total_finalized_object_no / a->collection_no,
                              false);
      fprintf (f, "    Finalisation:             %11.2f %s obj./coll.\n",
               finalized_objects_per_coll, finalized_objects_per_coll_prefix);
      fprintf (f, "        Latency per collection:  %11.3f %ss\n",
               per_collection_finalization_time,
               per_collection_finalization_time_prefix);
      if (a->total_finalized_object_no > 0)
        {
          JITTER_HUMAN_READABLE_ (object_finalization_time,
                                  (a->total_finalization_time
                                   / a->total_finalized_object_no),
                                  false);
          fprintf (f, "        Latency per finalized o.:%11.3f %ss\n",
                   object_finalization_time, object_finalization_time_prefix);
        }
    }
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
}

// FIXME: remove.
static size_t
jitter_gc_list_lengthr (struct jitter_list_header *list)
{
  size_t res = 0;
  struct jitter_gc_finalization_data *field;
  for (field = list->last; field != NULL; field = field->links.previous)
    res ++;
  return res;
}

// FIXME: remove
__attribute__ ((unused))
static void
jitter_gc_list_dump (struct jitter_list_header *list)
{
  size_t length = jitter_gc_list_length (list);
  size_t lengthr = jitter_gc_list_lengthr (list);
  if (length != lengthr)
    jitter_fatal ("length != lengthr");
  struct jitter_gc_finalization_data *field;
  printf ("R [ ");
  for (field = list->first; field != NULL; field = field->links.next)
    {
      printf ("%p, next %p", field, field->links.next);
      printf (" ;  ");
    }
  printf ("]\n");
  printf ("%li [ ", (long)length);
  for (field = list->first; field != NULL; field = field->links.next)
    printf ("%p ", field);
  printf ("]  ");
  printf ("first: %p  ", list->first);
  if (list->first != NULL)
    printf ("second: %p  ",
            ((struct jitter_gc_finalization_data *) list->first)->links.next);
  printf ("last: %p\n", list->last);
  printf ("E header %p\n", list); \
}


/* Synchronisation prototypes.
 * ************************************************************************** */

/* The static functions declared here may be implemented in different ways,
   according to the available synchronisation features which are specific to the
   platform.
   Notice that the actual synchronisation data structures involved are not
   defined here. */

/* Initialize synchronization fields for the pointed heap, allocating memory
   and setting fields as neeed. */
static void
jitter_gc_heap_synchronization_initialize (struct jitter_gc_heap *h);

/* Initialize synchronization fields for the pointed heaplet, allocating memory
   and setting fields as neeed. */
static void
jitter_gc_heaplet_synchronization_initialize (struct jitter_gc_heaplet *a);

/* Release resources for the synchronization fields in the pointed heap; this is
   meant to be used before the heap is destroyed. */
static void
jitter_gc_heap_synchronization_finalize (struct jitter_gc_heap *h);

/* Release resources for the synchronization fields in the pointed heaplet; this
   is meant to be used before the heaplet is destroyed. */
static void
jitter_gc_heaplet_synchronization_finalize (struct jitter_gc_heaplet *a);

/* Acquire the lock on the pointed heap data structure. */
static void
jitter_gc_heap_lock (struct jitter_gc_heap *h);

#if defined (JITTER_GC_ENABLE_SHARING)
/* Release the lock on the pointed heap data structure. */
static void
jitter_gc_heap_unlock (struct jitter_gc_heap *h);

/* Release the lock on the heap, which is assumed to be held by the calling
   thread at call time, and block using the synchronisation structure from the
   pointed heaplet; this can be thought of as a V operation on the heap lock
   followed by a P operation on the heaplet's counting (or even Boolean)
   semaphore initialised to zero. */
static void
jitter_gc_heap_unlock_and_heaplet_block (struct jitter_gc_heap *h,
                                         struct jitter_gc_heaplet *a);

/* Wake up the thread blocked on the synchronisation structure from the pointed
   heaplet; this can be thought of as a V operation on a counting (or even
   Boolean) semaphore which is currently guaranteed to be waiting. */
static void
jitter_gc_heaplet_wake_up (struct jitter_gc_heaplet *a);
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */




/* Synchronisation instances.
 * ************************************************************************** */

#if ! defined (JITTER_GC_ENABLE_SHARING)

/* Synchronisation: trivial case
 * ..................................................... */

/* Sharing is not enabled, so we need no synchronisation primitives at all. */

#define NOOP_FUNCTION_HEAPLET_1(name, struct_type_suffix)       \
  static void name (struct jitter_gc_ ## struct_type_suffix *)  \
  {                                                             \
  }
#define NOOP_FUNCTION_HEAPLET_2(name,                                        \
                                struct_type_suffix_1, struct_type_suffix_2)  \
  static void name (struct jitter_gc_ ## struct_type_suffix_1 *,             \
                    struct jitter_gc_ ## struct_type_suffix_2 *)             \
  {                                                                          \
  }

NOOP_FUNCTION_HEAPLET_1(jitter_gc_heap_synchronization_initialize, heap)
NOOP_FUNCTION_HEAPLET_1(jitter_gc_heap_synchronization_finalize, heap)
NOOP_FUNCTION_HEAPLET_1(jitter_gc_heaplet_synchronization_initialize, heaplet)
NOOP_FUNCTION_HEAPLET_1(jitter_gc_heaplet_synchronization_finalize, heaplet)
NOOP_FUNCTION_HEAPLET_1(jitter_gc_heap_lock, heap)
NOOP_FUNCTION_HEAPLET_2(jitter_gc_global_gc_if_needed_and_unlock, heap, heaplet)

#elif defined(JITTER_HAVE_ISO_C_THREAD_HEADER)                \
    && defined(JITTER_HAVE_ISO_C_THREAD_FUNCTIONS)

/* Synchronisation: ISO C mutexes and condition variables.
 * ..................................................... */

#include <threads.h>

struct jitter_gc_semaphore
{
  /* The ISO C condition variable for a heaplet.  Its mutex is the one from the
     heap. */
  cnd_t condition;

  /* Remember whether we are supposed to be blocked.  This is necessary to
     prevent spurious wakeups. */
  bool blocked;
};

static void
jitter_gc_heap_synchronization_initialize (struct jitter_gc_heap *h)
{
  mtx_t *mutex = jitter_xmalloc (sizeof (mtx_t));
  if (mtx_init (mutex, mtx_plain) != thrd_success)
    jitter_fatal ("mtx_init failed");
  h->lock = mutex;
}

static void
jitter_gc_heaplet_synchronization_initialize (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_semaphore *semaphore
    = jitter_xmalloc (sizeof (struct jitter_gc_semaphore));
  if (cnd_init (& semaphore->condition) != thrd_success)
    jitter_fatal ("cnd_init failed");
  semaphore->blocked = false;
  a->semaphore = semaphore;
}

static void
jitter_gc_heap_synchronization_finalize (struct jitter_gc_heap *h)
{
  mtx_destroy (h->lock);
  free (h->lock);
}

static void
jitter_gc_heaplet_synchronization_finalize (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_semaphore *semaphore = a->semaphore;
  cnd_destroy (& semaphore->condition);
  free (semaphore);
}

static void
jitter_gc_heap_lock (struct jitter_gc_heap *h)
{
  if (__builtin_expect (mtx_lock (h->lock), thrd_success) != thrd_success)
    jitter_fatal ("mtx_lock failed");
}

static void
jitter_gc_heap_unlock (struct jitter_gc_heap *h)
{
  if (__builtin_expect (mtx_unlock (h->lock), thrd_success) != thrd_success)
    jitter_fatal ("mtx_unlock failed");
}

static void
jitter_gc_heap_unlock_and_heaplet_block (struct jitter_gc_heap *h,
                                         struct jitter_gc_heaplet *a)
{
  struct jitter_gc_semaphore *semaphore = a->semaphore;
  semaphore->blocked = true;
  /* I am using a loop to make sure that this works even with spurious
     interrupts, which I think are impossible in this scenario where each
     threads blocks on a separate condition variable. */
  do
    if (__builtin_expect (cnd_wait (& semaphore->condition, h->lock),
                          thrd_success)
        != thrd_success)
      jitter_fatal ("cnd_wait failed");
  while (__builtin_expect (semaphore->blocked, false));
  jitter_gc_heap_unlock (h);
}

static void
jitter_gc_heaplet_wake_up (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_semaphore *semaphore = a->semaphore;
  semaphore->blocked = false;
  if (__builtin_expect (cnd_signal (& semaphore->condition), thrd_success)
      != thrd_success)
    jitter_fatal ("cnd_signal failed");
}

#elif defined(JITTER_HAVE_PTHREAD_HEADER) \
      && defined(JITTER_HAVE_PTHREAD_FUNCTIONS)

/* Synchronisation: POSIX mutexes and condition variables.
 * ..................................................... */

#include <pthread.h>

struct jitter_gc_semaphore
{
  /* The POSIX condition variable for a heaplet.  Its mutex is the one from the
     heap. */
  pthread_cond_t condition;

  /* Remember whether we are supposed to be blocked.  This is necessary to
     prevent spurious wakeups. */
  bool blocked;
};

static void
jitter_gc_heap_synchronization_initialize (struct jitter_gc_heap *h)
{
  pthread_mutex_t *mutex = jitter_xmalloc (sizeof (pthread_mutex_t));
  if (pthread_mutex_init (mutex, NULL) != 0)
    jitter_fatal ("pthread_mutex_init failed");
  h->lock = mutex;
}

static void
jitter_gc_heaplet_synchronization_initialize (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_semaphore *semaphore
    = jitter_xmalloc (sizeof (struct jitter_gc_semaphore));
  if (pthread_cond_init (& semaphore->condition, NULL) != 0)
    jitter_fatal ("pthread_cond_init failed");
  semaphore->blocked = false;
  a->semaphore = semaphore;
}

static void
jitter_gc_heap_synchronization_finalize (struct jitter_gc_heap *h)
{
  if (pthread_mutex_destroy (h->lock) != 0)
    jitter_fatal ("pthread_mutex_destroy failed");
  free (h->lock);
}

static void
jitter_gc_heaplet_synchronization_finalize (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_semaphore *semaphore = a->semaphore;
  if (pthread_cond_destroy (& semaphore->condition) != 0)
    jitter_fatal ("pthread_cond_destroy failed");
  free (semaphore);
}

static void
jitter_gc_heap_lock (struct jitter_gc_heap *h)
{
  if (__builtin_expect (pthread_mutex_lock (h->lock), 0) != 0)
    jitter_fatal ("pthread_mutex_lock failed");
}

static void
jitter_gc_heap_unlock (struct jitter_gc_heap *h)
{
  if (__builtin_expect (pthread_mutex_unlock (h->lock), 0) != 0)
    jitter_fatal ("pthread_mutex_unlock failed");
}

static void
jitter_gc_heap_unlock_and_heaplet_block (struct jitter_gc_heap *h,
                                         struct jitter_gc_heaplet *a)
{
  struct jitter_gc_semaphore *semaphore = a->semaphore;
  semaphore->blocked = true;
  /* I am using a loop to make sure that this works even with spurious
     interrupts, which I think are impossible in this scenario where each
     threads blocks on a separate condition variable. */
  do
    if (__builtin_expect (pthread_cond_wait (& semaphore->condition, h->lock), 0)
        != 0)
      jitter_fatal ("pthread_cond_wait failed");
  while (__builtin_expect (semaphore->blocked, false));
  jitter_gc_heap_unlock (h);
}

static void
jitter_gc_heaplet_wake_up (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_semaphore *semaphore = a->semaphore;
  semaphore->blocked = false;
  if (__builtin_expect (pthread_cond_signal (& semaphore->condition), 0) != 0)
    jitter_fatal ("pthread_cond_signal failed");
}

#elif defined(JITTER_HAVE_POSIX_SEMAPHORE_HEADER) \
      && defined(JITTER_HAVE_POSIX_SEMAPHORE_FUNCTIONS)

/* Synchronisation: POSIX anonymous semaphores.
 * ..................................................... */

#include <semaphore.h>

/* Return a pointer to a fresh POSIX anonymous semaphore with the given initial
   value, or fail fatally. */
static sem_t *
jitter_gc_semaphore_make (unsigned initial_value) {
  sem_t *res = jitter_xmalloc (sizeof (sem_t));
  if (sem_init (res, false, initial_value) != 0)
    jitter_fatal ("sem_init failed");
  return res;
}

/* Destroy the pointed POSIX semaphore and free its resources. */
static void
jitter_gc_semaphore_destroy (sem_t *s)
{
  if (sem_destroy (s) != 0)
    jitter_fatal ("sem_destroy failed");
  free (s);
}

/* Execute Dijkstra's P operation on the pointed POSIX semaphore. */
static void
jitter_gc_semaphore_p (sem_t *s)
{
  if (__builtin_expect (sem_wait (s), 0) != 0)
    jitter_fatal ("sem_wait failed");
}

/* Execute Dijkstra's V operation on the pointed POSIX semaphore. */
static void
jitter_gc_semaphore_v (sem_t *s)
{
  if (__builtin_expect (sem_post (s), 0) != 0)
    jitter_fatal ("sem_post failed");
}

static void
jitter_gc_heap_synchronization_initialize (struct jitter_gc_heap *h)
{
  h->lock = jitter_gc_semaphore_make (1);
}

static void
jitter_gc_heaplet_synchronization_initialize (struct jitter_gc_heaplet *a)
{
  a->semaphore = jitter_gc_semaphore_make (0);
}

static void
jitter_gc_heap_synchronization_finalize (struct jitter_gc_heap *h)
{
  jitter_gc_semaphore_destroy (h->lock);
}

static void
jitter_gc_heaplet_synchronization_finalize (struct jitter_gc_heaplet *a)
{
  jitter_gc_semaphore_destroy (a->semaphore);
}

static void
jitter_gc_heap_lock (struct jitter_gc_heap *h)
{
  jitter_gc_semaphore_p (h->lock);
}

static void
jitter_gc_heap_unlock (struct jitter_gc_heap *h)
{
  jitter_gc_semaphore_v (h->lock);
}

static void
jitter_gc_heap_unlock_and_heaplet_block (struct jitter_gc_heap *h,
                                         struct jitter_gc_heaplet *a)
{
  jitter_gc_semaphore_v (h->lock);
  jitter_gc_semaphore_p (a->semaphore);
}

static void
jitter_gc_heaplet_wake_up (struct jitter_gc_heaplet *a)
{
  jitter_gc_semaphore_v (a->semaphore);
}

#else

/* Synchronisation: primitives not available.
 * ..................................................... */
# error "unknown synchronisation primitives: cannot use any of:"
# error "* ISO C mutexes and condition variables;"
# error "* POSIX mutexes and condition variables;"
# error "* POSIX anonymous semaphores."
# error "You cannot use sharing in this configuration (but the collector would"
# error "work correctly with sharing disabled)"
#endif /* conditional choosing synchornisation implementation */




/* Blocks.
 * ************************************************************************** */

/* Given a pointer to a block header, return a pointer to the beginning of its
   payload, skipping any required padding. */
static jitter_gc_heap_pointer
jitter_gc_block_payload (struct jitter_gc_block *b)
{
  return (char *) b + JITTER_GC_BLOCK_HEADER_ROUNDED_SIZE_IN_BYTES;
}

/* Given a pointer to a block header, return a pointer to the byte right past
   the last payload byte. */
static jitter_gc_heap_pointer
jitter_gc_block_limit (struct jitter_gc_block *b)
{
  return (char *) b + JITTER_GC_BLOCK_SIZE_IN_BYTES;
}

/* Fill the payload of the given block with copies of the given unboxed datum,
   starting from the given address which must be within the block payload or
   right past it. */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static void
jitter_gc_block_fill_from (struct jitter_gc_block *b,
                           jitter_gc_tagged_object *initial_address,
                           jitter_gc_tagged_object unboxed_datum,
                           int indentation_space_no)
{
  jitter_gc_tagged_object *limit
    = (jitter_gc_tagged_object *) jitter_gc_block_limit (b);
  int i;
  for (i = 0; i < indentation_space_no; i ++)
    jitter_gc_log (" ");
  jitter_gc_log ("Filling block %p [%p, %p) with %p starting from %p\n",
                 b, jitter_gc_block_payload (b), limit,
                 (void *) unboxed_datum, initial_address);
#if defined (JITTER_GC_DEBUG)
  struct jitter_gc_block *initial_address_block
    = JITTER_GC_BOXED_TO_BLOCK (initial_address);
  if (b != initial_address_block && initial_address != limit)
    jitter_fatal ("filling block %p [%p, %p) starting from %p, which is not "
                  "within the block payload or right past it",
                  b, jitter_gc_block_payload (b), jitter_gc_block_limit (b),
                  initial_address);
#endif // #if defined (JITTER_GC_DEBUG)
  jitter_gc_tagged_object *p;
  for (p = initial_address; p < limit; p ++)
    *p = unboxed_datum;
}

/* Fill the entire payload of the pointed block with copies of the given unboxed
   datum.  This is only used in debug mode. */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static void
jitter_gc_block_fill (struct jitter_gc_block *b,
                      jitter_gc_tagged_object unboxed_datum,
                      int indentation_space_no)
{
  jitter_gc_tagged_object *initial_address
    = (jitter_gc_tagged_object *) jitter_gc_block_payload (b);
  jitter_gc_block_fill_from (b, initial_address, unboxed_datum,
                             indentation_space_no);
}

/* Update the pointed space to have the pointed block (which must already belong
   to it) as its new allocation block.  Since we are stopping to use another
   block (unless the previous allocation block was NULL), update its used
   limit. */
static void
jitter_gc_space_set_allocation_block (struct jitter_gc_space *s,
                                      struct jitter_gc_block *b)
{
  /* If there was a previous allocation block make sure we remember where its
     used part ends, which will be important when scanning; and also count its
     used space as part of the total space size. */
  if (s->allocation_block != NULL)
    {
      s->allocation_block->used_limit = s->allocation_pointer;
      s->used_size_in_bytes
        += s->allocation_pointer - jitter_gc_block_payload (s->allocation_block);
    }

  /* Invalidate the used_limit in the new current allocation block: this is
     important for a scavenge-time optimisation (explained in the comment at the
     beginning of the body of jitter_gc_scan_next) not to break correctness, and
     with that optimisation in place not having the used_limit field set to NULL
     in the current allocation block when scavenging would cause subtle bugs. */
  b->used_limit = NULL;

  /* Use this block block as the new allocation block in the space. */
  s->allocation_block = b;

  /* Update pointers within the space which refer the block. */
  s->allocation_pointer = jitter_gc_block_payload (b);
  s->limit = jitter_gc_block_limit (b);

  jitter_gc_log ("    The allocation block for %s is now %p [%p, %p)\n",
                 s->name, b, jitter_gc_block_payload (b),
                 jitter_gc_block_limit (b));
}

/* Update the pointed space to have the pointed block (which must already belong
   to it) as its new scan block.  Differently from
   jitter_gc_space_set_allocation this does not change the used limit pointer on
   any block, since the allocation block, which is used for actually making new
   objects, does not change here.
   If set_scan_pointer_to_allocation_limit is non-false then set the scan
   pointer to the current allocation pointer, as needed when scanning within a
   non-empty space; otherwise set it to the beginning of the payload. */
static void
jitter_gc_space_set_scan_block (struct jitter_gc_space *s,
                                struct jitter_gc_block *b,
                                bool set_scan_pointer_to_allocation_pointer)
{
  /* Use the new block as the scan block in the space. */
  s->scan_block = b;

  /* Update the scan pointer within the space which refer the block. */
  if (set_scan_pointer_to_allocation_pointer)
    {
#if defined (JITTER_GC_DEBUG)
      if (s->allocation_pointer == NULL)
        jitter_fatal ("allocation pointer NULL when resetting scan pointer to "
                      "allocation pointer: something is horribly wrong");
      struct jitter_gc_block *allocation_pointer_block
        = JITTER_GC_BOXED_TO_BLOCK (s->allocation_pointer);
      if (allocation_pointer_block != b
          /* It *is* acceptable for the allocation pointer to be right at the
             limit: in that case the first allocation into tospace will change
             the allocation block; if no allocation is performed scanning will
             terminate immediately, finding the scan pointer equal to the
             allocation pointer which is the terminating condition for Cheney's
             algorithm. */
          && s->allocation_pointer != jitter_gc_block_limit (b))
        jitter_fatal ("allocation pointer %p not within new block %p [%p, %p) "
                      "or right past it when resetting scan pointer to "
                      "allocation pointer: something is horribly wrong",
                      s->allocation_pointer, b,
                      jitter_gc_block_payload (b), jitter_gc_block_limit (b));
#endif // #if defined (JITTER_GC_DEBUG)
      s->scan_pointer = s->allocation_pointer;
    }
  else
    s->scan_pointer = jitter_gc_block_payload (b);

  /* There is no explicit limit: the limit is the allocation pointer, possibly
     in a different block. */

  jitter_gc_log ("    The scan block for %s is now %p [%p, %p), sp %p\n",
                 s->name, b, jitter_gc_block_payload (b),
                 jitter_gc_block_limit (b), s->scan_pointer);
}

/* Modify the pointed space in the pointed heaplet by adding the pointed block,
   and make it the current allocation block.  Adapt the block to refer to the
   space and its generation.  If debugging, fill the block payload with copies
   of the invalid object for defensiveness.
   If unlink_first is non-false, then start by unlinking the block from the
   unused space in the heaplet (the block *must* be from the unused space,
   even if its space pointer may contradict this: keeping the space pointer up
   to date would require per-block processing when cleaning spaces),
   decrementing the unused space size by the correct amount. */
static void
jitter_gc_space_add_allocation_block (struct jitter_gc_heaplet *a,
                                      struct jitter_gc_space *s,
                                      struct jitter_gc_block *b,
                                      bool unlink_first)
{
#if defined (JITTER_GC_DEBUG)
  if (b == NULL)
    jitter_fatal ("trying to add a NULL block to a space: this should never "
                  "happen");
#endif // #if defined (JITTER_GC_DEBUG)

  /* Unlink from the unused space if requested. */
  if (unlink_first)
    {
      /* Notice that the space pointer within the block may be different from
         unused_space; it would be inefficient to keep that up to date. */
      struct jitter_gc_space *unused_space = & a->unused_space;
      JITTER_LIST_UNLINK (jitter_gc_block, links,
                          & unused_space->blocks, b);
      unused_space->allocated_size_in_bytes
        -= JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES;
    }

  /* Set space-dependent fields within the block. */
  b->space = s;
  b->generation = s->generation;

  /* Add the block to the pointed space, at the end of its list of blocks. */
  JITTER_LIST_LINK_LAST (jitter_gc_block, links,
                         & s->blocks, b);
  s->allocated_size_in_bytes += JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES;

  /* Make the new block be the allocation block for the space. */
  jitter_gc_space_set_allocation_block (s, b);

#if defined (JITTER_GC_DEBUG)
  /* Fill the entire block payload with “uninitialised” unboxed objects, for
     defensiveness's sake. */
  jitter_gc_block_fill (b, a->shape_table->uninitialized_object, 6);
#endif // #if defined (JITTER_GC_DEBUG)
}

/* Procure an empty block, either repurposing an unused block from the unused
   space of the pointed heaplet or allocating a new one; add it to the pointed
   space at the end of its list, and make it the current allocation block for
   the space.  If debugging fill the entire block payload with copies of the
   given word. */
static void
jitter_gc_space_procure_allocation_block (struct jitter_gc_heaplet *a,
                                          struct jitter_gc_space *s)
{
  /* Decide whether we need to make a new block or we can repurpose one from
     the unused space.  If we need to make a block make it. */
  struct jitter_gc_space *unused_space = & a->unused_space;
  struct jitter_gc_block *b = unused_space->blocks.first;
  bool make_new = b == NULL;
  if (make_new)
    {
      /* Allocate an aligned block, and keep its id inside it to be able to free
         it later. */
      jitter_aligned_block_id id;
      b = ((struct jitter_gc_block *)
           jitter_aligned_block_make (& id, JITTER_GC_BLOCK_SIZE_IN_BYTES,
                                      JITTER_GC_BLOCK_SIZE_IN_BYTES));
      b->block_id = id;
      jitter_gc_log ("    Make a new block %p [%p, %p)\n",
                     b, jitter_gc_block_payload (b), jitter_gc_block_limit (b));
    }

  /* Add the block to the space and set it as its new allocation block, first
     unlinking it from the unused space if the block was repurposed.  This
     also initialises the space and generation fields within the block; on
     debugging it also fills the payload with copies of the invalid object. */
  jitter_gc_space_add_allocation_block (a, s, b, ! make_new);
}

/* Destroy the pointed block, unlinking it first only if unlink_first is
   non-false. */
static void
jitter_gc_block_destroy (struct jitter_gc_block *block, bool unlink_first)
{
  /* Unlink the block from the space list before destroying it, if demanded. */
  if (unlink_first && block->space != NULL)
    JITTER_LIST_UNLINK (jitter_gc_block, links,
                        & block->space->blocks, block);

  /* Free the space. */
  jitter_gc_log ("  Destroy block %p [%p, %p)\n", block,
                 jitter_gc_block_payload (block), jitter_gc_block_limit (block));
  jitter_aligned_block_destroy (block->block_id);
}




/* Temporary root sets.
 * ************************************************************************** */

/* The functionality in this section is only used internally. */

/* Inititialize the pointed root set to be empty.  This assumes that the
   pointed root set has never been initialized or has been finalized. */
__attribute__ ((nonnull (1)))
static void
jitter_gc_temporary_root_set_initialize (jitter_gc_temporary_root_set *rs)
{
  jitter_dynamic_buffer_initialize (rs);
}

/* Finalise the pointed root making it empty, and freeing its resources.  This
   assumes that the pointed root set was initialized. */
__attribute__ ((nonnull (1)))
static void
jitter_gc_temporary_root_set_finalize (jitter_gc_temporary_root_set *rs)
{
  jitter_dynamic_buffer_finalize (rs);
}

/* Push a new root with the given elements to the pointed temporary root set. */
__attribute__ ((nonnull (1)))
static void
jitter_gc_temporary_root_set_push (jitter_gc_temporary_root_set *rs,
                                   jitter_gc_tagged_object *buffer,
                                   size_t buffer_size_in_bytes)
{
  struct jitter_gc_root root;
  root.buffer = buffer;
  root.buffer_size_in_bytes = buffer_size_in_bytes;

  jitter_dynamic_buffer_push (rs, & root, sizeof (struct jitter_gc_root));
}

/* Compact the pointed temporary root set, so as not to waste space.  This is
   useful to call periodically (a good time is at the end of a collection) to
   make sure that the root set does not become wastefully large after some
   unusual phase of intensive growth and reduction.  Such scenarios are common
   with deeply recursive C functions. */
__attribute__ ((nonnull (1)))
static void
jitter_gc_temporary_root_set_compact (jitter_gc_temporary_root_set *rs)
{
  jitter_dynamic_buffer_compact (rs, 0);
}

/* Remove every root from the root set. */
__attribute__ ((nonnull (1)))
static void
jitter_gc_temporary_root_set_empty (jitter_gc_temporary_root_set *rs)
{
  /* This is easy because individual struct jitter_gc_root objects do not
     require finalization. */
  size_t used_bytes = jitter_dynamic_buffer_size (rs);
  jitter_dynamic_buffer_pop (rs, used_bytes);
}




/* Garbage collection hooks.
 * ************************************************************************** */

/* Initialise the pointed hook list. */
static void
jitter_gc_hooks_initialize (struct jitter_list_header *hooks)
{
  JITTER_LIST_INITIALIZE_HEADER (hooks);
}

/* Finalise the pointed hook list, destroying every hook. */
static void
jitter_gc_hooks_finalize (struct jitter_list_header *hooks)
{
  struct jitter_gc_hook *h;
  for (h = hooks->first; h != NULL; /* Nothing. */)
    {
      struct jitter_gc_hook *next = h->links.next;
      free (h);
      h = next;
    }
}

/* Helper function for jitter_gc_hook_register_pre_collection ,
   jitter_gc_hook_register_post_collection and the like .  The first argument
   points to the list to which the hook is to be added. */
static jitter_gc_hook
jitter_gc_hook_register (struct jitter_list_header *list,
                         jitter_gc_hook_f hook_function,
                         void *hook_data)
{
  struct jitter_gc_hook *hook = jitter_xmalloc (sizeof (struct jitter_gc_hook));
  hook->function = hook_function;
  hook->data = hook_data;
  JITTER_LIST_LINK_LAST (jitter_gc_hook, links,
                         list, hook);
  return hook;
}

jitter_gc_hook
jitter_gc_hook_register_pre_collection (struct jitter_gc_heaplet *a,
                                        jitter_gc_hook_f hook_function,
                                        void *hook_data)
{
  return jitter_gc_hook_register (& a->pre_collection_hooks, hook_function,
                                  hook_data);
}
jitter_gc_hook
jitter_gc_hook_register_post_collection (struct jitter_gc_heaplet *a,
                                         jitter_gc_hook_f hook_function,
                                         void *hook_data)
{
  return jitter_gc_hook_register (& a->post_collection_hooks, hook_function,
                                  hook_data);
}
jitter_gc_hook
jitter_gc_hook_register_pre_ssb_flush (struct jitter_gc_heaplet *a,
                                       jitter_gc_hook_f hook_function,
                                       void *hook_data)
{
  return jitter_gc_hook_register (& a->pre_ssb_flush_hooks, hook_function,
                                  hook_data);
}
jitter_gc_hook
jitter_gc_hook_register_post_ssb_flush (struct jitter_gc_heaplet *a,
                                        jitter_gc_hook_f hook_function,
                                        void *hook_data)
{
  return jitter_gc_hook_register (& a->post_ssb_flush_hooks, hook_function,
                                  hook_data);
}

/* Helper function for jitter_gc_hook_deregister_pre_collection ,
   jitter_gc_hook_deregister_post_collection and the like .  The first argument
   points to the list containing the hook. */
static void
jitter_gc_hook_deregister (struct jitter_list_header *list,
                           jitter_gc_hook hook)
{
  JITTER_LIST_UNLINK (jitter_gc_hook, links,
                      list, hook);
  free (hook);
}

void
jitter_gc_hook_deregister_pre_collection (struct jitter_gc_heaplet *a,
                                          jitter_gc_hook hook)
{
  jitter_gc_hook_deregister (& a->pre_collection_hooks, hook);
}
void
jitter_gc_hook_deregister_post_collection (struct jitter_gc_heaplet *a,
                                           jitter_gc_hook hook)
{
  jitter_gc_hook_deregister (& a->post_collection_hooks, hook);
}
void
jitter_gc_hook_deregister_pre_ssb_flush (struct jitter_gc_heaplet *a,
                                         jitter_gc_hook hook)
{
  jitter_gc_hook_deregister (& a->pre_ssb_flush_hooks, hook);
}
void
jitter_gc_hook_deregister_post_ssb_flush (struct jitter_gc_heaplet *a,
                                          jitter_gc_hook hook)
{
  jitter_gc_hook_deregister (& a->post_ssb_flush_hooks, hook);
}

/* Run every hook in the pointed list on the pointed accumulator, in
   registration order. */
static void
jitter_gc_hooks_run (struct jitter_gc_heaplet *a,
                     struct jitter_list_header *hooks,
                     enum jitter_gc_collection_kind k)
{
  struct jitter_gc_hook *h;
  for (h = hooks->first; h != NULL; h = h->links.next)
    h->function (a, h->data, k);
}




/* Space initialization and finalization.
 * ************************************************************************** */

/* The functionality in this section is only used internally. */

/* Return non-false iff the pointed space belongs to the given space-pointer
   array, limited by the given limit.
   This is meant to be applied to heaplet fields such as tospaces and
   tospaces_limit .
   This is a helper for jitter_gc_add_space_pointer , which is a helper for
   jitter_gc_set_up_spaces_initialize . */
static int /*bool*/
jitter_gc_space_pointer_array_has (struct jitter_gc_space **array,
                                   struct jitter_gc_space **array_limit,
                                   struct jitter_gc_space *s)
{
  /* Performing a linear search is perfectly acceptable here, and probably
     faster than any alternative with such a small number of elements. */
  struct jitter_gc_space **sp;
  for (sp = array; sp < array_limit; sp ++)
    if (* sp == s)
      return true;
  return false;
}

/* Add the given new element to the given space-pointer array using the pointed
   limit, updating the limit as well; if the element already belongs to the
   array do nothing.  This is meant to be applied to heaplet fields such as
   tospaces and tospaces_limit . */
static void
jitter_gc_add_space_pointer (struct jitter_gc_space **array,
                             struct jitter_gc_space ***array_limit,
                             struct jitter_gc_space *new_element)
{
  if (jitter_gc_space_pointer_array_has (array, * array_limit, new_element))
    return;
  ** array_limit = new_element;
  (* array_limit) ++;
}

/* Return non-false iff the given size in bytes is correctly "aligned", in the
   sense of being a multiple of the minimum object size. */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static int //bool
jitter_gc_is_good_size_in_bytes (size_t size)
{
  return size == JITTER_GC_ROUND_SIZE_UP (size);
}

/* Return non-false iff the given pointer is aligned correctly to point to a
   heap object.  The range is not checked, so this is independent from any
   heaplet. */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static int //bool
jitter_gc_is_object_aligned_pointer (void *p)
{
  return (jitter_uint) p == JITTER_GC_ROUND_SIZE_UP ((jitter_uint) p);
}

/* Same as jitter_gc_is_aligned_pointer but instead of checking for heap-object
   alignment just check for word alignment; a word being, as always here, the
   size of a pointer. */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static int //bool
jitter_gc_is_word_aligned_pointer (void *p)
{
  return ((jitter_uint) p
          == JITTER_NEXT_MULTIPLE_OF_POWER_OF_TWO ((jitter_uint) p,
                                                   sizeof (jitter_uint)));
}

/* Initialise the pointed space descriptor; if completely_empty is non-false
   make an initial block for it and set is as the current allocation block.
   Store a copy of the given name.  This assumes that the space descriptor was
   not initialized already.
   If the heaplet pointer is NULL do not alter heap data; this is, of course,
   meant for the case of a space belonging to a heap and not to any heaplet. */
__attribute__ ((nonnull (2, 4)))
static void
jitter_gc_space_initialize (struct jitter_gc_heaplet *a,
                            struct jitter_gc_space *s,
                            enum jitter_gc_generation g,
                            const char *name,
                            bool completely_empty)
{
  /* Make one block for this space; but first initialise enough of the internal
     state so that the block itself can be initialised. */
  s->scavenged_from = false; /* Just to ease debugging; not really needed. */
  s->generation = g;
  JITTER_LIST_INITIALIZE_HEADER (& s->blocks);
  s->allocated_size_in_bytes = 0;
  s->used_size_in_bytes = 0;
  s->allocation_block = NULL;
  s->name = jitter_xmalloc (strlen (name) + 1);
  strcpy (s->name, name);
  if (! completely_empty)
    jitter_gc_space_procure_allocation_block (a, s);

  /* The space is empty and therefore contains no finalisable objects yet. */
  JITTER_LIST_INITIALIZE_HEADER (& s->mutation_time_finalizables.header);

  /* Invalidate the fields which are not in use now and should not be used by
     mistake. */
  s->scan_block = NULL;

  if (a != NULL)
    {
      /* Add a pointer to the new space to the space array in the heaplet. */
      jitter_gc_add_space_pointer (a->spaces, & a->spaces_limit, s);

      /* Also add it to the the array for its generation. */
      jitter_gc_add_space_pointer (a->generation_spaces
                                      [JITTER_GC_GENERATION_INDEX_OFFSET + g],
                                   & a->generation_spaces_limit
                                      [JITTER_GC_GENERATION_INDEX_OFFSET + g],
                                   s);
    }
}

/* Free the resources occupied by the pointed space, including its blocks. */
__attribute__ ((nonnull (1)))
static void
jitter_gc_space_finalize (struct jitter_gc_space *s)
{
  /* We do not really need to do anything on the list used for finalisation,
     which at this point must be empty unless we made some horrible mistake. */
#if defined (JITTER_GC_DEBUG)
  if (s->mutation_time_finalizables.header.first != NULL)
    jitter_fatal ("mutation_time_finalizables non-empty at space destruction "
                  "time for %s", s->name);
#endif /* #if defined (JITTER_GC_DEBUG) */

  /* Free memory not associated to blocks. */
  free (s->name);

  /* Destroy every block belonging to the space, without unlinking it first from
     the list; modifying the list would be useless, since we are destroying the
     entire list and the entire space containing it. */
  struct jitter_gc_block *b = s->blocks.first;
  while (b != NULL)
    {
      struct jitter_gc_block *next = b->links.next;
      jitter_gc_block_destroy (b, false);
      b = next;
    }

#if defined (JITTER_GC_DEBUG)
  /* Invalidate the other fields, particularly the allocation pointer, for
     defensiveness's sake. */;
  s->allocated_size_in_bytes = (size_t) -1;
  s->used_size_in_bytes = (size_t) -1;
  s->limit = NULL;
  s->allocation_pointer = NULL;
  s->blocks.first = NULL;
  s->blocks.last = NULL;
  s->allocation_block = NULL;
  s->scan_block = NULL;
#endif // #if defined (JITTER_GC_DEBUG)
}




/* Space accessors.
 * ************************************************************************** */

/* Return the total allocated size in bytes for the pointed space.  This only
   counts block payloads, ignoring the size of their headers. */
__attribute__ ((unused))
static size_t
jitter_gc_space_allocated_size_in_bytes (const struct jitter_gc_space *s)
{
  return s->allocated_size_in_bytes;
}

/* Return how many bytes are in use for the pointed space, also counting the
   used size in the current block, which has not been added to the stored total
   yet.  This only counts block payloads, ignoring the size of their headers. */
static size_t
jitter_gc_space_used_size_in_bytes (const struct jitter_gc_space *s)
{
  size_t res = s->used_size_in_bytes;
  if (s->allocation_block != NULL)
    res += (s->allocation_pointer
            - jitter_gc_block_payload (s->allocation_block));
  return res;
}




/* Space utility.
 * ************************************************************************** */

/* Return non-false iff the given pointer points within the pointed space.  The
   pointer type is generic so as not to require casts: any actual argument of
   pointer type will be implicitly converted to void * , which is what we want
   here. */
inline static
int// bool
jitter_gc_space_has (struct jitter_gc_space *s, void *p)
{
  struct jitter_gc_block *b = JITTER_GC_BOXED_TO_BLOCK (p);
  return (b->space == s);
}

/* Given a pointer to a heap object return the name of the space containing it,
   including a valid string if the object belongs to no known space. */
#if ! defined (JITTER_GC_LOG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_LOG)
static char *
jitter_gc_space_name_for (struct jitter_gc_heaplet *a, void *p)
{
#if JITTER_GC_NON_NURSERY_STEP_NO > 0 /* GCC 11 and 12 give spurious warnings
                                         about sprintf writing out-of-range when
                                         this is 0 .  The warnings are spurious
                                         because the loop iterates zero times
                                         when non-nursery steps are zero. */
  static bool initialized = false;
  static char ageing_names [JITTER_GC_NON_NURSERY_STEP_NO] [100];
  static char young_reserve_names [JITTER_GC_NON_NURSERY_STEP_NO] [100];
  int i;
  if (! initialized)
    {
      for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
        {
          sprintf (ageing_names [i], "ageing-%i", i);
          sprintf (young_reserve_names [i], "young-reserve-%i", i);
        }
      initialized = true;
    }
#endif
  /* FIXME: I could make this safe even for invalid out-of-heap pointers, at the
     cost of it being much more inefficient: check every block of every space. */
  //return JITTER_GC_BOXED_TO_BLOCK (p)->space->name;
  if (jitter_gc_space_has (& a->nursery, p))
    return "nursery";
#if JITTER_GC_NON_NURSERY_STEP_NO > 0
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    if (jitter_gc_space_has (a->young_ageing_spaces [i], p))
      return ageing_names [i];
    else if (jitter_gc_space_has (a->young_reserves [i], p))
      return young_reserve_names [i];
#endif

  if (jitter_gc_space_has (a->oldspace, p))
    return "old-space";
  else if (jitter_gc_space_has (a->old_reserve, p))
    return "old-reserve";
#if defined (JITTER_GC_ENABLE_SHARING)
  else if (jitter_gc_space_has (& a->shared_space_own, p))
    return "shared-own";
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */
  else if (jitter_gc_space_has (& a->unused_space, p))
    return "unused-space (SUSPECT!)";
  else
    {
      /*
      struct jitter_gc_block *b = JITTER_GC_BOXED_TO_BLOCK (p);
      printf ("!!!! pointer %p (block %p [%p, %p)) has no known space\n", p, b,
              jitter_gc_block_payload (b), jitter_gc_block_limit (b)); fflush (stdout);
      */
      return "[unknown space]";
    }
}

/* Update the pointed space to make the next block current for allocation,
   procuring a new block if needed.
   This function is used internally at garbage collection time, when moving
   objects: it never triggers another garbage collection. */
static void
jitter_gc_space_change_allocation_block (struct jitter_gc_heaplet *a,
                                         struct jitter_gc_space *s)
{
  struct jitter_gc_block *current = s->allocation_block;
#if defined (JITTER_GC_DEBUG)
  if (current == NULL)
    jitter_fatal ("there is no current allocation block in %s at collection "
                  "time: this should never happen", s->name);
#endif // #if defined (JITTER_GC_DEBUG)

  /* Use the next block in the list for allocation, if one exists; if there are
     no more blocks in the list add one: making it adds it to the end of the
     list and makes it current for allocation, which is what we need here. */
  struct jitter_gc_block *next = current->links.next;
  if (next == NULL)
    jitter_gc_space_procure_allocation_block (a, s);
  else
    {
      jitter_fatal ("FIXME: if this can still happen I am wrong: this must be "
                    "very early surviving code");
      jitter_gc_space_set_allocation_block (s, next);
    }
}

/* Update the pointed space to make the next block current for scanning.  Since
   scanning must always follow allocation within the same tospace there is never
   need to add a new block: the next block *must* be already present in the
   list. */
static void
jitter_gc_space_change_scan_block (struct jitter_gc_heaplet *a,
                                   struct jitter_gc_space *s)
{
  struct jitter_gc_block *current = s->scan_block;
#if defined (JITTER_GC_DEBUG)
  if (current == NULL)
    jitter_fatal ("there is no current scan block in %s at collection "
                  "time: this should never happen", s->name);
#endif // #if defined (JITTER_GC_DEBUG)

  /* Use the next block in the list for scanning.  One must exist, since
     scanning follows allocation. */
  struct jitter_gc_block *next = current->links.next;
#if defined (JITTER_GC_DEBUG)
  if (next == NULL)
    jitter_fatal ("cannot change scan block in %s, as there are no more blocks "
                  "after the current one: this should never happen", s->name);
#endif // #if defined (JITTER_GC_DEBUG)
  jitter_gc_space_set_scan_block (s, next, false);
}

/* Allocate a fresh object from the pointed space in the pointed heaplet.  Use
   the current block if the new object fits; it it does not then switch to the
   next block; if there is no further block add a new one.  Return an untagged
   pointer to the new object.
   This function is used internally at collection time, when moving objects: it
   never triggers another collection. */
static jitter_gc_heap_pointer
jitter_gc_allocate_from (struct jitter_gc_heaplet *a,
                         struct jitter_gc_space *s,
                         size_t aligned_size_in_bytes)
{
#if defined (JITTER_GC_DEBUG)
  if (aligned_size_in_bytes != JITTER_GC_ROUND_SIZE_UP (aligned_size_in_bytes))
    jitter_fatal ("trying to allocate %li B, which is not a multiple of the "
                  "minimum object size %li B, while copying at collection "
                  "time.  This should never happen",
                  (long) aligned_size_in_bytes,
                  (long) JITTER_GC_MINIMUM_OBJECT_SIZE_IN_BYTES);
#endif // #if defined (JITTER_GC_DEBUG)
  /* Allocate by pointer-bumping; this will succeed most of the times. */
  jitter_gc_heap_pointer res = s->allocation_pointer;
  s->allocation_pointer += aligned_size_in_bytes;
  if (__builtin_expect (s->allocation_pointer <= s->limit,
                        true))
    return res;
  else
    {
      /* The fast path did not succeed.  Undo the failed allocation pointer
         change, switch to the next block and bump the pointer again: the next
         attempt is guaranteed to succeed. */
      s->allocation_pointer -= aligned_size_in_bytes;
      jitter_gc_space_change_allocation_block (a, s);
      res = s->allocation_pointer;
      s->allocation_pointer += aligned_size_in_bytes;
#if defined (JITTER_GC_DEBUG)
      if (s->allocation_pointer > s->limit)
        jitter_fatal ("allocation of %li B failed after changing block in %s; "
                      "this should never happen", (long) aligned_size_in_bytes,
                      s->name);
#endif // #if defined (JITTER_GC_DEBUG)
      return res;
    }
}

// FIXME: move ////////////////////////////////////////////////////////////////////////
/* Return non-false iff the given pointer points within one of the current
   destination spaces or to the shared own space.  This is used when debugging
   to validate forwarding pointers in broken hearts.
   Rationale: when performing a share operation some broken hearts leading to
   the shared own space may remain: these are cleared by a collection
   immediately after share: see _jitter_gc_share_young_or_old . */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static int //bool
jitter_gc_is_in_tospace_or_shared_own (struct jitter_gc_heaplet *a, char *p)
{
  struct jitter_gc_space *s = JITTER_GC_BOXED_TO_BLOCK (p)->space;
  return (jitter_gc_space_pointer_array_has (a->tospaces, a->tospaces_limit, s)
#if defined (JITTER_GC_ENABLE_SHARING)
          || s == & a->shared_space_own
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */
          );
}

/* Perform the first part of the work of jitter_gc_space_move_to_from , the
   part that can be executed without synchronisation and without changing
   the destination space.
   This is part of the work can to be executed out of a critical section, with
   jitter_gc_space_move_to_from_synchronized_2 called later in a critical
   section; the last part, jitter_gc_space_move_to_from_synchronized_3 , again
   requires no synchronisation. */
static void
jitter_gc_space_move_to_from_unsynchronized_1 (struct jitter_gc_heap *h,
                                               struct jitter_gc_space *to_p,
                                               struct jitter_gc_space *from_p,
                                               int /*bool*/ set_space,
                                               int /*bool*/ set_generation,
                                               int /*bool*/ clear,
                                               int indentation_level)
{
#if defined (JITTER_GC_DEBUG)
  if (to_p == from_p)
    jitter_fatal ("jitter_gc_space_move_to_from: to_p == from_p");
#endif /* #if defined (JITTER_GC_DEBUG) */
  jitter_gc_log_i (indentation_level,
                   "Move all blocks from %s to %s: %s, %s, %s\n",
                   from_p->name, to_p->name,
                   (set_space ? "SET space" : "do NOT set space"),
                   (set_generation ? "SET generation" : "do NOT set generation"),
                   (clear ? "INVALIDATE" : "do NOT invalidate"));

  /* Before doing anything else (this datum is used in logging as well),
     temporarily update from_p's used size field to also consider the allocation
     block, if any.  We need this because later in the second part (see
     jitter_gc_space_move_to_from_synchronized_2 ) we will detach all the blocks
     and lose the information about any being an allocation block, whose used
     size was not counted yet. */
  from_p->used_size_in_bytes = jitter_gc_space_used_size_in_bytes (from_p);

#if defined (JITTER_GC_LOG)
  size_t allocated_size_in_bytes
    = jitter_gc_space_allocated_size_in_bytes (from_p);
  size_t used_size_in_bytes = jitter_gc_space_used_size_in_bytes (from_p);
  long block_no
    = JITTER_QUOTIENT_CEILING (allocated_size_in_bytes,
                               JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES);
  JITTER_HUMAN_READABLE_ (from_size, allocated_size_in_bytes, true);
  JITTER_HUMAN_READABLE_ (used_from_size, used_size_in_bytes, true);
#endif /* #if defined (JITTER_GC_LOG) */
  jitter_gc_log_i (indentation_level + 2,
                   "Prepare %li blocks (%.1f %sB used out of "
                   "%.1f %sB payload)\n",
                   block_no,
                   used_from_size, used_from_size_prefix,
                   from_size, from_size_prefix);

  /* Optimisation: if we are not changing blocks in any way then we have nothing
     to do. */
  if (! set_space && ! set_generation && ! clear)
    {
      jitter_gc_log_i (indentation_level + 4,
                       "Trivial case: nothing to do\n");
      return;
    }

  /* First part: update every block in the source space.  Notice that it is
     harmless to read to_p->generation without synchronisation, since a space's
     generation field is never updated after the space is initialised.  That is
     of course not the case for blocks, since we reuse them for different
     purposes when moving them from space to space. */
  enum jitter_gc_generation g = to_p->generation;
  jitter_gc_tagged_object invalid = h->shape_table->invalid_object;
  struct jitter_gc_block *b;
  for (b = from_p->blocks.first; b != NULL; b = b->links.next)
    {
      jitter_gc_log_i (indentation_level + 4,
                       "Prepare block %p [%p, %p)\n", b,
                       jitter_gc_block_payload (b), jitter_gc_block_limit (b));
      if (set_space)
        b->space = to_p;
      if (set_generation)
        b->generation = g;
      if (clear)
        jitter_gc_block_fill (b, invalid, indentation_level + 6);
    }
  /* Now the blocks are still linked in from_p->blocks , but they already
     refer to_p and its generation. */
}

/* Validate parameters in jitter_gc_space_move_to_from_synchronized_2 and
   jitter_gc_space_move_to_from_unsynchronized_3 , when debugging. */
static void
jitter_gc_space_move_to_from_validate (struct jitter_gc_space *to_p,
                                       struct jitter_gc_space *from_p,
                                       int /*bool*/ move_to_an_unused_space)
{
#if defined (JITTER_GC_DEBUG)
  if (to_p == from_p)
    jitter_fatal ("jitter_gc_space_move_to_from: to_p == from_p");
  /* It would be perfectly possible not to have move_to_an_unused_space as an
     agument and compute it here inside the function instead; however computing
     it should be done only once, and out of the critical section the second
     part runs in.
     If is better to let the caller do this, and here to just validate when
     debugging.  We use !! to normalise generalised Booleans.  A single ! would
     have also worked here. */
  if (!! move_to_an_unused_space
      != !! (to_p->generation == jitter_gc_generation_unused))
    jitter_fatal ("the argument move_to_an_unused_space is wrong");
#endif /* #if defined (JITTER_GC_DEBUG) */
}

/* Perform the second part of the work of jitter_gc_space_move_to_from after
   jitter_gc_space_move_to_from_unsynchronized_1 has been executed.  This
   updates the space pointed by to_p, and is meant to be executed inside a
   critical section.
   A third and last part follows, jitter_gc_space_move_to_from_unsynchronized_3
   , once more requiring no synchronisation. */
static void
jitter_gc_space_move_to_from_synchronized_2
   (struct jitter_gc_space *to_p,
    struct jitter_gc_space *from_p,
    int indentation_level,
    int /*bool*/ move_to_an_unused_space)
{
  /* Validate parameters on debugging. */
  jitter_gc_space_move_to_from_validate (to_p, from_p, move_to_an_unused_space);

  /* Append the blocks, which is an O(1) operation) */
  jitter_gc_log_i (indentation_level + 2,
                   "Append already prepared blocks from %s to %s, increasing %s "
                   "size\n",
                   from_p->name, to_p->name, to_p->name);
  JITTER_LIST_APPEND_LIST (jitter_gc_block, links,  /* struct name, field name */
                           & to_p->blocks,          /* to */
                           & from_p->blocks);       /* from */

  /* Update sizes in the destination space. */
  to_p->allocated_size_in_bytes += from_p->allocated_size_in_bytes;
  if (move_to_an_unused_space)
    {
      jitter_gc_log_i (indentation_level + 4,
                       "...But leave used size at 0 for unused-generation %s\n",
                       to_p->name);
#if defined (JITTER_GC_DEBUG)
      if (to_p->used_size_in_bytes != 0)
        jitter_fatal ("unused-generation space %s has used_size_in_bytes %li",
                      to_p->name, (long) to_p->used_size_in_bytes);
#endif /* #if defined (JITTER_GC_DEBUG) */
    }
  else
    to_p->used_size_in_bytes += from_p->used_size_in_bytes;

  /* Move the list of finalisable objects (alive as per the last collection)
     from the old space to the new, as long as we are not moving to the unused
     space -- in which case do not touch the target finalisable list. */
  if (! move_to_an_unused_space)
    JITTER_LIST_APPEND_LIST (jitter_gc_finalization_data, links,
                             & to_p->mutation_time_finalizables.header,
                             & from_p->mutation_time_finalizables.header);
}

/* Perform the third part of the work of jitter_gc_space_move_to_from , updating
   the source space.  This can be executed out of a critical section. */
static void
jitter_gc_space_move_to_from_unsynchronized_3
   (struct jitter_gc_space *to_p,
    struct jitter_gc_space *from_p,
    int indentation_level,
    int /*bool*/ move_to_an_unused_space)

{
  /* Validate parameters on debugging. */
  jitter_gc_space_move_to_from_validate (to_p, from_p, move_to_an_unused_space);

  /* Third, trivial part: now the origin space is empty so we can set its size
     to zero. */
  jitter_gc_log_i (indentation_level + 2,
                   "Set %s's size to zero after moving its blocks to %s\n",
                   from_p->name, to_p->name);
  from_p->allocated_size_in_bytes = 0;
  from_p->used_size_in_bytes = 0;

  /* The space has no block, and therefore no allocation block either.  This is
     also important to show the used size correctly, since a space with an
     allocation block is considered to have its used size field not up to date:
     one is supposed to deference its allocation_block field to add its used
     size. */
  from_p->allocation_block = NULL;

#if defined (JITTER_GC_DEBUG)
  /* For defensiveness, invalidate all the from_p fields which depend on having
     a block. */
  from_p->allocation_pointer = NULL;
  from_p->limit = NULL;

  /* It is also true that we have no scan block but we do not need to worry
     about setting that explicitly, when not debugging: at the beginning of
     collections tospaces have this field updated. */
  from_p->scan_block = NULL;
#endif /* #if defined (JITTER_GC_DEBUG) */

  /* Update the from_p finalisable list (containing the objects that were known
     to be alive at the last collection): it must now be empty in all cases,
     even if we have nothing particular to do in case we did not move to an
     unused space in part 2. */
  if (move_to_an_unused_space)
    {
#if defined (JITTER_GC_DEBUG)
      if (from_p->mutation_time_finalizables.header.first != NULL)
        jitter_fatal ("the mutation_time_finalizables list is not empty after moving "
                      "all blocks away from %s into %s",
                      from_p->name, to_p->name);
#endif /* #if defined (JITTER_GC_DEBUG) */
    }
  else
    {
      /* In part 2 we did not modify the lists.  We can just empty it here. */
      from_p->mutation_time_finalizables.header.first = NULL;
      from_p->mutation_time_finalizables.header.last = NULL;
    }
}

/* Move all the blocks of the space pointed by from_p into the space pointed by
   to_p , also updating allocated and used sizes in both spaces.
   If set_space is non-false this changes the space pointer in each block.  If
   set_generation is non-false, it changes the generation field.
   If clear is non-false it fills each block payload with uninitialised objects.
   The two spaces are assumed to be different.
   This function behaves like calling the following three functions in sequence:
   - jitter_gc_space_move_to_from_unsynchronized_1
   - jitter_gc_space_move_to_from_synchronized_2
   - jitter_gc_space_move_to_from_unsynchronized_3
   This function does not execute any synchronisation primitive, and is meant
   to be used in a case where no synchronisation is needed -- in a
   single-threaded context, or where a lock has been acquired already. */
static void
jitter_gc_space_move_to_from (struct jitter_gc_heap *h,
                              struct jitter_gc_space *to_p,
                              struct jitter_gc_space *from_p,
                              int /*bool*/ set_space,
                              int /*bool*/ set_generation,
                              int /*bool*/ clear,
                              int indentation_level)
{
  jitter_gc_space_move_to_from_unsynchronized_1 (h, to_p, from_p, set_space,
                                                 set_generation, clear,
                                                 indentation_level);
  bool to_unused = (to_p->generation == jitter_gc_generation_unused);
  jitter_gc_space_move_to_from_synchronized_2 (to_p, from_p, indentation_level,
                                               to_unused);
  jitter_gc_space_move_to_from_unsynchronized_3 (to_p, from_p,
                                                 indentation_level, to_unused);
}




/* Heaplet list handling.
 * ************************************************************************** */

/* The utility functions in this section serve to handle the heaplet lists
   contained in the heap struct. */

/* Unlink the pointed heaplet from the pointed list header.  The heaplet must
   belong to the list when the function is called. */
static void
jitter_gc_heaplet_unlink (struct jitter_list_header *lh,
                          struct jitter_gc_heaplet *a)
{
  JITTER_LIST_UNLINK(jitter_gc_heaplet, links, lh, a);
}

static void
jitter_gc_heaplet_link_first (struct jitter_list_header *lh,
                              struct jitter_gc_heaplet *a)
{
  JITTER_LIST_LINK_FIRST(jitter_gc_heaplet, links, lh, a);
}

#if defined (JITTER_GC_ENABLE_SHARING)
static void
jitter_gc_heaplet_link_last (struct jitter_list_header *lh,
                             struct jitter_gc_heaplet *a)
{
  JITTER_LIST_LINK_LAST(jitter_gc_heaplet, links, lh, a);
}
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */



/* Heap: initialisation and finalisation.
 * ************************************************************************** */

void
jitter_gc_heap_initialize (struct jitter_gc_heap *h,
                           struct jitter_gc_shape_table *shape_tablep)
{
  jitter_gc_log ("Make heap at %p\n", h);
  /* Sanity checks.  See the comment before the function definition. */
  jitter_gc_check_sanity ();

  h->shape_table = shape_tablep;
  jitter_gc_heap_synchronization_initialize (h);
  h->request = jitter_gc_request_no_request;
  jitter_gc_space_initialize (NULL, & h->unused_space,
                              jitter_gc_generation_unused, "unused-heap", true);
#if defined(JITTER_GC_ENABLE_SHARING)
  jitter_gc_space_initialize (NULL, & h->shared_space,
                              jitter_gc_generation_shared, "shared-heap", true);
#endif /* #if defined(JITTER_GC_ENABLE_SHARING) */
  JITTER_LIST_INITIALIZE_HEADER(& h->heaplets_in_use);
  JITTER_LIST_INITIALIZE_HEADER(& h->heaplets_not_in_use);
}

void
jitter_gc_heap_finalize (struct jitter_gc_heap *h)
{
  jitter_gc_log ("Destroy heap %p\n", h);

  jitter_gc_object_finalize_all_heap_finalizables (h);
  // FIXME: jitter_gc_object_finalize_all_heap_finalizables returns a result: use it for statistics

  jitter_gc_space_finalize (& h->unused_space);
#if defined(JITTER_GC_ENABLE_SHARING)
  jitter_gc_space_finalize (& h->shared_space);
#endif /* #if defined(JITTER_GC_ENABLE_SHARING) */

  if (h->heaplets_in_use.first != NULL)
    jitter_fatal ("cannot finalise heap with (in use) heaplets");
  if (h->heaplets_not_in_use.first != NULL)
    jitter_fatal ("cannot finalise heap with (not in use) heaplets");

  jitter_gc_heap_synchronization_finalize (h);
}

struct jitter_gc_heap *
jitter_gc_heap_make (struct jitter_gc_shape_table *shape_tablep)
{
  struct jitter_gc_heap *res = jitter_xmalloc (sizeof (struct jitter_gc_heap));
  jitter_gc_heap_initialize (res, shape_tablep);
  return res;
}

void
jitter_gc_heap_destroy (struct jitter_gc_heap *h)
{
  jitter_gc_heap_finalize (h);
  free (h);
}




/* Heaplet: initialisation and finalisation.
 * ************************************************************************** */

void
jitter_gc_heaplet_initialize (struct jitter_gc_heaplet *a,
                              struct jitter_gc_heap *h)
{
  jitter_gc_log ("Make heaplet for heap %p at %p\n", h, a);
  printf ("| making heaplet %p\n", a);
  a->heap = h;
  a->shape_table = (const struct jitter_gc_shape_table *) h->shape_table;
  a->collection_enabled = true;
  jitter_gc_heaplet_synchronization_initialize (a);

  jitter_gc_global_roots_initialize (& a->global_roots);
  jitter_gc_temporary_root_set_initialize (& a->temporary_root_set);
  jitter_gc_hooks_initialize (& a->pre_collection_hooks);
  jitter_gc_hooks_initialize (& a->post_collection_hooks);
  jitter_gc_hooks_initialize (& a->pre_ssb_flush_hooks);
  jitter_gc_hooks_initialize (& a->post_ssb_flush_hooks);

#if defined (JITTER_GC_ENABLE_SHARING)
  jitter_gc_temporary_root_set_initialize (& a->objects_being_shared);
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* Initialise space arrays (to be empty: they will be added to as spaces
     are made). */
  a->spaces_limit = a->spaces;
  enum jitter_gc_generation g;
  for (g = jitter_gc_generation_first; g < jitter_gc_generation_past_last; g ++)
    a->generation_spaces_limit [JITTER_GC_GENERATION_INDEX_OFFSET + g]
      = a->generation_spaces [JITTER_GC_GENERATION_INDEX_OFFSET + g];

  /* Initialise spaces, starting with the unused space since blocks for
     non-completely-empty spaces are procured from there.  Two spaces deserving
     comment are & a->oldspace_a (the initial oldspace) and shared-own: both
     must *not* be completely empty: young collections or share collections will
     use them as tospaces, but they can not be cleared before collections since
     they may hold data already.  For this reason they must be made non-empty
     at initialisation. */
  jitter_gc_space_initialize (a, & a->unused_space,
                              jitter_gc_generation_unused, "unused-own", true);
  jitter_gc_space_initialize (a, & a->nursery,
                              jitter_gc_generation_young, "nursery", false);
  int i;
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    {
      char name [100];
      sprintf (name, "young-a-%i", i);
      jitter_gc_space_initialize (a, a->steps_a + i,
                                  jitter_gc_generation_young, name,
                                  true); /* This changes after a minor GC. */
      sprintf (name, "young-b-%i", i);
      jitter_gc_space_initialize (a, a->steps_b + i,
                                  jitter_gc_generation_young, name, true);
    }
  jitter_gc_space_initialize (a, & a->oldspace_a,
                              jitter_gc_generation_old, "old-a",
                              false); /* See the comment above. */
  jitter_gc_space_initialize (a, & a->oldspace_b,
                              jitter_gc_generation_old, "old-b",
                              true);  /* This instead can be completely empty. */
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    {
      a->young_ageing_spaces [i] = a->steps_a + i;
      a->young_reserves [i] = a->steps_b + i;
    }
  a->oldspace = & a->oldspace_a;
  a->old_reserve = & a->oldspace_b;
#if defined (JITTER_GC_ENABLE_SHARING)
  jitter_gc_space_initialize (a, & a->shared_space_own,
                              jitter_gc_generation_shared, "shared-own", false);
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* Initialise write-barrier data structures. */
  jitter_word_set_initialize (& a->remembered_set);

  /* Initialise the lists used for finalisation, which of course are empty at
     the beginning. */
  JITTER_LIST_INITIALIZE_HEADER (& a->candidate_dead_finalizables.header);
  JITTER_LIST_INITIALIZE_HEADER (& a->to_be_finalized_finalizables.header);

  /* Inizialize tuning parameters. */
  a->minimum_nursery_size_in_bytes
    = JITTER_GC_DEFAULT_MINIMUM_NURSERY_SIZE_IN_BYTES;
  a->maximum_nursery_size_in_bytes
    = JITTER_GC_DEFAULT_MAXIMUM_NURSERY_SIZE_IN_BYTES;
  a->low_nursery_survival_rate
    = JITTER_GC_DEFAULT_LOW_NURSERY_SURVIVAL_RATE;
  a->high_nursery_survival_rate
    = JITTER_GC_DEFAULT_HIGH_NURSERY_SURVIVAL_RATE;
  a->nursery_growth_ratio
    = JITTER_GC_DEFAULT_NURSERY_GROWTH_RATIO;
  a->nursery_shrinkage_ratio
    = JITTER_GC_DEFAULT_NURSERY_SHRINKAGE_RATIO;
  a->minimum_oldspace_size_in_bytes
    = JITTER_GC_DEFAULT_MINIMUM_OLDSPACE_SIZE_IN_BYTES;
  a->maximum_oldspace_size_in_bytes
    = JITTER_GC_DEFAULT_MAXIMUM_OLDSPACE_SIZE_IN_BYTES;
  a->target_major_survival_rate
    = JITTER_GC_DEFAULT_TARGET_MAJOR_SURVIVAL_RATE;

  /* Initialise the automatically computed sizing parameters.  This can only be
     a pure guess before the first statistics. */
  a->nursery_used_size_threshold_in_bytes
    = jitter_gc_round_threshold ((size_t)
                                 JITTER_GC_INITIAL_NURSERY_SIZE_IN_BYTES);
  /* Initialise the oldspace threshold to a value between the minimum and the
     maximum, unbalanced towards the minimum. */
  double initial_oldspace_threshold_minimum_weight = .95;//.99;
  double initial_oldspace_threshold
    = (initial_oldspace_threshold_minimum_weight *
       a->minimum_oldspace_size_in_bytes
       + (1 - initial_oldspace_threshold_minimum_weight)
       * a->maximum_oldspace_size_in_bytes);
  a->oldspace_used_size_threshold_in_bytes
    = jitter_gc_round_threshold ((size_t) initial_oldspace_threshold);

  /* Inizialise statistics. */
  a->collection_start_time = jitter_point_in_time_make ();
  a->total_collection_time = 0;
  a->total_minor_collection_time = 0;
  a->total_major_collection_time = 0;
  a->total_share_time = 0;
  a->collection_no = 0;
  a->minor_collection_no = 0;
  a->major_collection_no = 0;
  a->share_no = 0;
  /* We set initial survival rates to 0 rather than to 1 or some arbitrary
     value; this means that the heap is by default considered "large", and
     will only grow if there is actual need. */
  for (i = 0; i < JITTER_GC_SURVIVAL_RATIO_NO; i ++)
    a->latest_nursery_survival_ratios [i] = 0.0;
  a->latest_nursery_survival_ratio_index = 0;
  for (i = 0; i < JITTER_GC_SURVIVAL_RATIO_NO; i ++)
    a->latest_major_survival_ratios [i] = 0.0;
  a->latest_major_survival_ratio_index = 0;
  a->total_minor_survival_ratio = 0;
  a->total_nursery_survival_ratio = 0;
  a->total_major_survival_ratio = 0;
  a->used_bytes_at_the_beginning_of_this_phase = 0;
  a->total_bytes_allocated = 0;
  a->total_bytes_copied_minor = 0;
  a->total_bytes_copied_major = 0;
  a->total_bytes_copied = 0;
  a->total_bytes_copied_to_oldspace = 0;
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->total_root_size_in_bytes = 0;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->total_initial_remembered_set_size = 0;
  a->total_final_remembered_set_size = 0;
  a->total_ssb_length = 0;
  a->ssb_flush_no = 0;
  a->total_nursery_used_size_in_bytes = 0;
  a->total_old_space_used_size_in_bytes = 0;
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->ssb_flush_begin_time = jitter_point_in_time_make ();
  a->total_ssb_flush_time = 0;
  a->finalization_begin_time = jitter_point_in_time_make ();
  a->total_finalization_time = 0;
  a->total_finalized_object_no = 0;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)

#if defined (JITTER_GC_DEBUG)
  jitter_word_set_initialize (& a->unique_root_table);
  a->runtime_fields_owned = true;
#endif // #if defined (JITTER_GC_DEBUG)

  /* The initialisation phase for this heaplet is over.  Start measuring time
     from here when computing times. */
  a->time_at_the_end_of_initialization = jitter_point_in_time_make ();
  jitter_time_set_now (a->time_at_the_end_of_initialization);

  /* Initialise convenience runtime fields to invalid values, for
     defensiveness. */
  a->convenience_runtime_allocation_pointer = NULL;
  a->convenience_runtime_limit = NULL;

  /* Link the heaplet in the heap. */
  a->used_state = jitter_gc_heaplet_used_state_in_use;
  jitter_gc_heap_lock (h);
  jitter_gc_heaplet_link_first(& h->heaplets_in_use, a);
  printf ("+ made heaplet   %p\n", a);
  jitter_gc_global_gc_if_needed_and_unlock (h, a);
}

void
jitter_gc_heaplet_finalize (struct jitter_gc_heaplet *a)
{
  jitter_gc_log ("Destroy heaplet %p\n", a);
  struct jitter_gc_heap *h = a->heap;
#if defined (JITTER_GC_ENABLE_SHARING)
  /* We are destroying the heaplet, but not its heap: it is important that all
     the used block from the shared space are transferred to the shared space in
     the heap.  We have to iterate on each block, changing the space pointer --
     the generation, shared, is already correct.  We can do this here, out of
     the critical section. */
  struct jitter_gc_space *own_shared_space = & a->shared_space_own;
  struct jitter_gc_space *h_shared_space = & h->shared_space;
  jitter_gc_space_move_to_from_unsynchronized_1 (h, h_shared_space,
                                                 own_shared_space,
                                                 true, false, false, 2);
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* Now we have to update the heap as well, and this requires a critical
     section; of course we want to keep it short. */
  jitter_gc_heap_lock (h);

#if defined (JITTER_GC_ENABLE_SHARING)
  /* Move all the blocks from the shared-own space to the heap's shared
     space, and update the heap shared-space size. */
  jitter_gc_space_move_to_from_synchronized_2 (h_shared_space,
                                               own_shared_space, 2, false);
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* Unlink from the heap structure. */
  switch (a->used_state)
    {
    case jitter_gc_heaplet_used_state_in_use:
      jitter_gc_heaplet_unlink (& h->heaplets_in_use, a);
      break;
    case jitter_gc_heaplet_used_state_to_be_woken_up:
    case jitter_gc_heaplet_used_state_not_to_be_woken_up:
      jitter_gc_heaplet_unlink (& h->heaplets_not_in_use, a);
      break;
    default:
      jitter_fatal ("unexpected heaplet used state %i", (int) a->used_state);
    }
  /* We have just removed a heaplet from the heap, which might have been the
     last one the others were waiting for to leave for a global collection. */
  jitter_gc_global_gc_if_needed_and_unlock (h, NULL);

  /* Here we could finish the shared-own -> shared transfer by calling the third
     function, but that is pertunctory here: updating the source space is
     useless since we are about to destroy it. */
  /* jitter_gc_space_move_to_from_unsynchronized_3 (h_shared_space,
                                                    & a->shared_space_own, 2,
                                                    false); */

  /* Destroy own synchronisation structures. */
  jitter_gc_heaplet_synchronization_finalize (a);

  /* Finalise any finalisable non-shared object; there is no need to trace
     roots. */
  jitter_gc_log ("  Finalise non-shared objects at heaplet finalisation\n");
  __attribute__ ((unused)) size_t finalised_object_no
    = jitter_gc_object_finalize_all_heaplet_finalizables (a);
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->total_finalized_object_no += finalised_object_no;
#endif /* #if defined (JITTER_GC_EXPENSIVE_STATISTICS) */

  jitter_gc_global_roots_finalize (& a->global_roots);
  jitter_gc_temporary_root_set_finalize (& a->temporary_root_set);
  jitter_gc_hooks_finalize (& a->pre_collection_hooks);
  jitter_gc_hooks_finalize (& a->post_collection_hooks);
  jitter_gc_hooks_finalize (& a->pre_ssb_flush_hooks);
  jitter_gc_hooks_finalize (& a->post_ssb_flush_hooks);

#if defined (JITTER_GC_ENABLE_SHARING)
  jitter_gc_temporary_root_set_finalize (& a->objects_being_shared);
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* Finalise spaces. */
  jitter_gc_space_finalize (& a->unused_space);
  jitter_gc_space_finalize (& a->nursery);
  int i;
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    {
      jitter_gc_space_finalize (a->steps_a + i);
      jitter_gc_space_finalize (a->steps_b + i);
    }
  jitter_gc_space_finalize (& a->oldspace_a);
  jitter_gc_space_finalize (& a->oldspace_b);
#if defined (JITTER_GC_ENABLE_SHARING)
  /* We can now safely finalise the shared-own space, at this point empty: we
     moved its data to the heap's shared space. */
  jitter_gc_space_finalize (& a->shared_space_own);
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* Finalise write-barrier data structures. */
  jitter_word_set_finalize (& a->remembered_set);

  /* We do not really need to do anything on the lists used for finalisation,
     which at this point must be empty unless we made some horrible mistake. */
#if defined (JITTER_GC_DEBUG)
  if (a->candidate_dead_finalizables.header.first != NULL)
    jitter_fatal ("candidate_dead_finalizables non-empty at heaplet "
                  "destruction time");
  if (a->to_be_finalized_finalizables.header.first != NULL)
    jitter_fatal ("to_be_finalized_finalizables non-empty at heaplet "
                  "destruction time");
#endif /* #if defined (JITTER_GC_DEBUG) */

  /* Destroy statistic data structures. */
  jitter_point_in_time_destroy (a->collection_start_time);
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  jitter_point_in_time_destroy (a->ssb_flush_begin_time);
  jitter_point_in_time_destroy (a->finalization_begin_time);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)

#if defined (JITTER_GC_DEBUG)
  jitter_word_set_finalize (& a->unique_root_table);
#endif // #if defined (JITTER_GC_DEBUG)

  jitter_point_in_time_destroy (a->time_at_the_end_of_initialization);
  printf ("- destroyed heaplet %p\n", a);
}

struct jitter_gc_heaplet *
jitter_gc_heaplet_make (struct jitter_gc_heap *h)
{
  struct jitter_gc_heaplet *res
    = jitter_xmalloc (sizeof (struct jitter_gc_heaplet));
  jitter_gc_heaplet_initialize (res, h);
  return res;
}

void
jitter_gc_heaplet_destroy (struct jitter_gc_heaplet *heapletp)
{
  jitter_gc_heaplet_finalize (heapletp);
  free (heapletp);
}




/* Temporary root registration.
 * ************************************************************************** */

void
jitter_gc_remove_all_temporary_roots (struct jitter_gc_heaplet *a)
{
  jitter_gc_temporary_root_set_empty (& a->temporary_root_set);
}

void
jitter_gc_push_temporary_root (struct jitter_gc_heaplet *a,
                               void *buffer,
                               size_t buffer_size_in_bytes)
{
  jitter_gc_temporary_root_set_push (& a->temporary_root_set, buffer,
                                     buffer_size_in_bytes);
}

void
jitter_gc_push_temporary_root_1 (struct jitter_gc_heaplet *a,
                                 void *buffer)
{
  jitter_gc_push_temporary_root (a, buffer, sizeof (jitter_int));
}

const struct jitter_gc_root
jitter_gc_pop_temporary_root (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_root res =
    * ((struct jitter_gc_root *)
       jitter_dynamic_buffer_pop (& a->temporary_root_set,
                                  sizeof (struct jitter_gc_root)));
  return res;
}




/* Temporary root registration for C blocks.
 * ************************************************************************** */

jitter_gc_temporary_root_set_height
jitter_gc_get_temporary_root_set_height (struct jitter_gc_heaplet *a)
{
  jitter_gc_temporary_root_set * s = & a->temporary_root_set;
  char *beginning = jitter_dynamic_buffer_to_pointer (s);
  char *after_end = jitter_dynamic_buffer_first_unused_char (s);
  return after_end - beginning;
}

void
jitter_gc_reset_temporary_root_set_height (struct jitter_gc_heaplet *a,
                                           jitter_gc_temporary_root_set_height h)
{
  jitter_gc_temporary_root_set * s = & a->temporary_root_set;
  size_t current_size = jitter_dynamic_buffer_size (s);
  jitter_dynamic_buffer_pop (s, current_size - h);
}




/* Global root registration and deregistration.
 * ************************************************************************** */

/* Initialise the pointed global-root list. */
static void
jitter_gc_global_roots_initialize (struct jitter_list_header *grp)
{
  JITTER_LIST_INITIALIZE_HEADER (grp);
}

/* Finalise the pointed global-root list, destroying every root. */
static void
jitter_gc_global_roots_finalize (struct jitter_list_header *grp)
{
  struct jitter_gc_global_root *gr;
  for (gr = grp->first; gr != NULL; /* Nothing. */)
    {
      struct jitter_gc_global_root *next = gr->links.next;
      free (gr);
      gr = next;
    }
}

jitter_gc_global_root
jitter_gc_register_global_root (struct jitter_gc_heaplet *a,
                                void *buffer,
                                size_t buffer_size_in_bytes)
{
  struct jitter_gc_global_root *gr
    = jitter_xmalloc (sizeof (struct jitter_gc_global_root));
  gr->root.buffer = buffer;
  gr->root.buffer_size_in_bytes = buffer_size_in_bytes;
  JITTER_LIST_LINK_LAST (jitter_gc_global_root, links,
                         & a->global_roots, gr);
  return gr;
}

jitter_gc_global_root
jitter_gc_register_global_root_1 (struct jitter_gc_heaplet *a,
                                  void *buffer)
{
  return jitter_gc_register_global_root (a, buffer,
                                         sizeof (jitter_gc_tagged_object));
}

void
jitter_gc_deregister_global_root (struct jitter_gc_heaplet *a,
                                  jitter_gc_global_root global_root)
{
  JITTER_LIST_UNLINK (jitter_gc_global_root, links,
                      & a->global_roots, global_root);
  free (global_root);
}




/* Heaplet space accessors.   FIXME: retitle section ///////////////////////////////////////////
 * ************************************************************************** */


/* Exchange the role of fromspaces and tospaces in the pointed heaplet. */
static void
jitter_gc_heaplet_flip (struct jitter_gc_heaplet *a,
                        enum jitter_gc_collection_kind kind)
{
  jitter_gc_log ("  Flip.\n");

  /* Exchange ageing space and copy reserve for each step.  This is necessary
     for all proper collection kinds (by which we means every kind except
     share), since share is the only operation not emptying young-generation
     fromspaces. */
  if (kind != jitter_gc_collection_kind_share)
    {
      int i;
      for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
        {
          struct jitter_gc_space *previous_young_ageing_space
            = a->young_ageing_spaces [i];
          a->young_ageing_spaces [i] = a->young_reserves [i];
          a->young_reserves [i] = previous_young_ageing_space;
          jitter_gc_log ("    ageing-%i is now        %s\n", i,
                         a->young_ageing_spaces [i]->name);
          jitter_gc_log ("    young-reserve-%i is now %s\n", i,
                         a->young_reserves [i]->name);
        }
    }

  switch (kind)
    {
    case jitter_gc_collection_kind_minor:
      /* We already did what we needed above, flipping ageing spaces with
         young reserves. */
      break;

    case jitter_gc_collection_kind_share:
      jitter_gc_log ("    No spaces to flip for collection kind %s\n",
                     jitter_gc_collection_kind_to_string (kind));
      break;

    case jitter_gc_collection_kind_major:
      {
        /* Exchange old reserve and old. */
        struct jitter_gc_space *previous_oldspace = a->oldspace;
        a->oldspace = a->old_reserve;
        a->old_reserve = previous_oldspace;
        jitter_gc_log ("    oldspace is now        %s\n", a->oldspace->name);
        jitter_gc_log ("    old reserve is now     %s\n", a->old_reserve->name);
        break;
      }

    default:
      jitter_fatal ("invalid collection kind %s %i",
                    jitter_gc_collection_kind_to_string (kind), (int) kind);
    }
}




/* Heaplet accessors.
 * ************************************************************************** */

jitter_gc_heap_pointer
jitter_gc_heaplet_update_runtime_fields
   (struct jitter_gc_heaplet *a,
    jitter_gc_heap_pointer runtime_allocation_pointer,
    jitter_gc_heap_pointer runtime_allocation_limit)
{
  a->nursery.allocation_pointer = runtime_allocation_pointer;
  a->nursery.limit = _jitter_gc_ssb_flush_0 (a, runtime_allocation_limit);
  return a->nursery.limit;
}




/* Shape description.
 * ************************************************************************** */

void
jitter_gc_shape_table_initialize (struct jitter_gc_shape_table *st,
                                  jitter_gc_tagged_object invalid_object,
                                  jitter_gc_tagged_object uninitialized_object,
                                  jitter_gc_tagged_object broken_heart_type_code,
                                  jitter_gc_object_has_shape_f is_unboxed)
{
  st->invalid_object = invalid_object;
  st->uninitialized_object = uninitialized_object;
  st->broken_heart_type_code = broken_heart_type_code;
  st->is_unboxed = is_unboxed;
  jitter_dynamic_buffer_initialize (& st->shapes);
  jitter_dynamic_buffer_initialize (& st->headerful_shapes);
  jitter_dynamic_buffer_initialize (& st->finalizable_shapes);
  jitter_dynamic_buffer_initialize (& st->quickly_finalizable_shapes);
  jitter_dynamic_buffer_initialize (& st->complete_object_finalizable_shapes);
}

void
jitter_gc_shape_table_finalize (struct jitter_gc_shape_table *st)
{
  /* Destroy the only malloc-allocated fields from the shape array; the other
     array just contain pointer into the shape array. */
  struct jitter_gc_shape *shapes
    = jitter_dynamic_buffer_to_pointer (& st->shapes);
  struct jitter_gc_shape *s_limit
    = jitter_dynamic_buffer_first_unused_char (& st->shapes);
  struct jitter_gc_shape *s;
  for (s = shapes; s < s_limit; s ++)
    free (s->name);
  jitter_dynamic_buffer_finalize (& st->shapes);
  jitter_dynamic_buffer_finalize (& st->headerful_shapes);
  jitter_dynamic_buffer_finalize (& st->finalizable_shapes);
  jitter_dynamic_buffer_finalize (& st->quickly_finalizable_shapes);
  jitter_dynamic_buffer_finalize (& st->complete_object_finalizable_shapes);
}

struct jitter_gc_shape_table *
jitter_gc_shape_table_make (jitter_gc_tagged_object invalid_object,
                            jitter_gc_tagged_object uninitialized_object,
                            jitter_gc_tagged_object broken_heart_type_code,
                            jitter_gc_object_has_shape_f is_unboxed)
{
  struct jitter_gc_shape_table *res
    = jitter_xmalloc (sizeof (struct jitter_gc_shape_table));
  jitter_gc_shape_table_initialize (res,
                                    invalid_object,
                                    uninitialized_object,
                                    broken_heart_type_code,
                                    is_unboxed);
  return res;
}

void
jitter_gc_shape_table_destroy (struct jitter_gc_shape_table *shape_table_p)
{
  jitter_gc_shape_table_finalize (shape_table_p);
  free (shape_table_p);
}

/* Add a new shape descriptor.  This factors all of the simpler functions
   intended for the user.
   Remark: the name string is not copied, and therefore in debugging mode it
   must remain allocated and valid as long as any heaplet using the shape
   table is used.  This should not be a problem, as the name will be a literal
   string in almost all practical circumstances.
   Remark: for headerless unboxed shapes the arguments is_type_code and
   object_update_fields must both be NULL; they must both be non-NULL for boxed
   shapes with a header.
   Remark: only one of quick_finalizer and complete_object_finalizer can be
   non-NULL; if either is then the shape must be headerful. */
static void
jitter_gc_shape_add (struct jitter_gc_shape_table *shape_table_p,
                     const char *original_name,
                     jitter_gc_object_has_shape_f object_has_shape,
                     jitter_gc_object_size_in_bytes_f object_size_in_bytes,
                     jitter_gc_is_type_code_f is_type_code,
                     jitter_gc_object_copy_f object_copy,
                     jitter_gc_object_update_fields_f object_update_fields,
                     jitter_gc_object_finalize_f quick_finalizer,
                     jitter_gc_object_finalize_f complete_object_finalizer)
{
  /* Arguement sanity checks. */
  if (is_type_code == NULL && object_update_fields != NULL)
    jitter_fatal ("shape %s: is_type_code NULL and object_update_fields "
                  "non-NULL", original_name);
  if (is_type_code != NULL && object_update_fields == NULL)
    jitter_fatal ("shape %s: is_type_code non-NULL and object_update_fields "
                  "NULL", original_name);
  if (is_type_code == NULL && quick_finalizer != NULL)
    jitter_fatal ("shape %s: is_type_code NULL and quick_finalizer non-NULL",
                  original_name);
  if (is_type_code == NULL && complete_object_finalizer != NULL)
    jitter_fatal ("shape %s: is_type_code NULL and complete_object_finalizer "
                  "non-NULL", original_name);
  if (quick_finalizer != NULL && complete_object_finalizer != NULL)
    jitter_fatal ("shape %s: quick_finalizer non-NULL and "
                  "complete_object_finalizer also non-NULL", original_name);

  size_t name_length = strlen (original_name);
  char *name_copy = jitter_xmalloc (name_length + 1);
  strcpy (name_copy, original_name);

  /* Make a struct with the given data, and add it to the known shapes. */
  struct jitter_gc_shape *s
    = ((struct jitter_gc_shape *)
       jitter_dynamic_buffer_reserve (& shape_table_p->shapes,
                                      sizeof (struct jitter_gc_shape)));
  s->name = name_copy;
  s->object_has_shape = object_has_shape;
  //s->object_tag = object_tag;
  //s->object_untag = object_untag;
  s->object_size_in_bytes = object_size_in_bytes;
  s->is_type_code = is_type_code;
  s->object_copy = object_copy;
  s->object_update_fields = object_update_fields;
  if (quick_finalizer != NULL)
    s->finalization_kind = jitter_gc_shape_finalization_kind_quick;
  else if (complete_object_finalizer != NULL)
    s->finalization_kind = jitter_gc_shape_finalization_kind_complete_object;
  else
    s->finalization_kind = jitter_gc_shape_finalization_kind_none;
  /* Use the one non-NULL finaliser as the shape finaliser, if any. */
  if (quick_finalizer == NULL)
    s->finalize = complete_object_finalizer;
  else
    s->finalize = quick_finalizer;

  /* If the shape has a header then add another copy of the struct to the
     dynamic buffer of headerful shapes. */
  if (is_type_code != NULL)
    jitter_dynamic_buffer_push (& shape_table_p->headerful_shapes,
                                s,
                                sizeof (struct jitter_gc_shape));

  /* If the shape has a finaliser then add two more copies: one to the dynamic
     buffer of finalisable shapes... */
  if (s->finalize != NULL)
    jitter_dynamic_buffer_push (& shape_table_p->finalizable_shapes,
                                s, sizeof (struct jitter_gc_shape));
  /* ...And one more to the dynamic buffer appropriate for the kind of
     finalisation. */
  if (quick_finalizer != NULL)
    jitter_dynamic_buffer_push (& shape_table_p->quickly_finalizable_shapes,
                                s, sizeof (struct jitter_gc_shape));
  else if (complete_object_finalizer != NULL)
    jitter_dynamic_buffer_push (& shape_table_p
                                   ->complete_object_finalizable_shapes,
                                s, sizeof (struct jitter_gc_shape));
}


void
jitter_gc_shape_add_headerless
   (struct jitter_gc_shape_table *shape_table_p,
    const char *name,
    jitter_gc_object_has_shape_f object_has_shape,
    jitter_gc_object_size_in_bytes_f object_size_in_bytes,
    jitter_gc_object_copy_f object_copy)
{
  jitter_gc_shape_add (shape_table_p, name, object_has_shape,
                       object_size_in_bytes,
                       NULL, /* is_type_code */
                       object_copy,
                       NULL /* object_update_fields */,
                       NULL /* quick_finalizer */,
                       NULL /* complete_object_finalizer */);
}

void
jitter_gc_shape_add_headered_non_finalizable
   (struct jitter_gc_shape_table *shape_table_p,
    const char *name,
    jitter_gc_object_has_shape_f object_has_shape,
    jitter_gc_object_size_in_bytes_f object_size_in_bytes,
    jitter_gc_is_type_code_f is_type_code,
    jitter_gc_object_copy_f object_copy,
    jitter_gc_object_update_fields_f object_update_fields)
{
  jitter_gc_shape_add (shape_table_p, name, object_has_shape,
                       object_size_in_bytes,
                       is_type_code,
                       object_copy,
                       object_update_fields,
                       NULL /* quick_finalizer */,
                       NULL /* complete_object_finalizer */);
}

void
jitter_gc_shape_add_headered_quickly_finalizable
   (struct jitter_gc_shape_table *shape_table_p,
    const char *name,
    jitter_gc_object_has_shape_f object_has_shape,
    jitter_gc_object_size_in_bytes_f object_size_in_bytes,
    jitter_gc_is_type_code_f is_type_code,
    jitter_gc_object_copy_f object_copy,
    jitter_gc_object_update_fields_f object_update_fields,
    jitter_gc_object_finalize_f quick_finalizer)
{
  jitter_gc_shape_add (shape_table_p, name, object_has_shape,
                       object_size_in_bytes,
                       is_type_code,
                       object_copy,
                       object_update_fields,
                       quick_finalizer,
                       NULL /* complete_object_finalizer */);
}

void
jitter_gc_shape_add_headered_complete_object_finalizable
   (struct jitter_gc_shape_table *shape_table_p,
    const char *name,
    jitter_gc_object_has_shape_f object_has_shape,
    jitter_gc_object_size_in_bytes_f object_size_in_bytes,
    jitter_gc_is_type_code_f is_type_code,
    jitter_gc_object_copy_f object_copy,
    jitter_gc_object_update_fields_f object_update_fields,
    jitter_gc_object_finalize_f complete_object_finalizer)
{
  jitter_gc_shape_add (shape_table_p, name, object_has_shape,
                       object_size_in_bytes,
                       is_type_code,
                       object_copy,
                       object_update_fields,
                       NULL /* quick_finalizer */,
                       complete_object_finalizer);
}

/* Return non-false iff the pointed shape table has at least one shape which
   is complete-object finalisable. */
static int /*bool*/
jitter_gc_shape_table_has_complete_object_finalizable
   (const struct jitter_gc_shape_table *shape_table_p)
{
  const struct jitter_gc_shape *complete_finalizable_shapes
    = JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER
    (& shape_table_p->complete_object_finalizable_shapes);
  const struct jitter_gc_shape *complete_finalizable_shapes_limit
    = JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST
    (& shape_table_p->complete_object_finalizable_shapes);

  /* If there is at least one complete-object finalisable shape then the first
     element of the array of such shapes does not begin as the same address as
     the limit pointer. */
  return complete_finalizable_shapes != complete_finalizable_shapes_limit;
}




/* Write barrier.
 * ************************************************************************** */

/* Measure time at the beginning of an SSB flush, when such statistics are
   enabled. */
inline static void
jitter_gc_ssb_flush_time_begin (struct jitter_gc_heaplet *a)
{
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  jitter_time_set_now (a->ssb_flush_begin_time);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
}

/* Measure time at the end of an SSB flush, when such statistics are enabled,
   adding the time for this flush to the total. */
inline static void
jitter_gc_ssb_flush_time_end (struct jitter_gc_heaplet *a)
{
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->total_ssb_flush_time
    += jitter_time_subtract_from_now (a->ssb_flush_begin_time);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
}

/* Add the given updated object to the remembered set of the pointed heaplet,
   unless it is in the heaplet youngspace.  See the comment for
   jitter_gc_ssb_flush_1 about requirements on the pointer.
   The object is not allowed to be shared: if sharing is enabled at all then
   the write-share barrier checks for that case before we arrive here. */
inline static void
jitter_gc_add_to_remembered_set (struct jitter_gc_heaplet *a,
                                 jitter_gc_tagged_object updated_object,
                                 int indentation_level)
{
#if defined (JITTER_GC_DEBUG)
  if (a->shape_table->is_unboxed (updated_object))
    jitter_fatal ("jitter_gc_add_to_remembered_set: trying to add unboxed "
                  "object %p to remembered set", (void *) updated_object);
  if (JITTER_GC_BOXED_TO_BLOCK (updated_object)->generation
      == jitter_gc_generation_shared)
    jitter_fatal ("jitter_gc_add_to_remembered_set: trying to add shared "
                  "object %p to remembered set", (void *) updated_object);
#endif // #if defined (JITTER_GC_DEBUG)

  /* If the object is not in youngspace enter it into the remembered set; having
     youngspace objects in the remembered set would be a waste: they would have
     to be filtered out, either now or later; it is faster not to let them into
     the table at all.  This range check here, inlined within
     jitter_gc_ssb_flush_n and executed in a tight loop, will be comparatively
     inexpensive to perform on every element on the queue.

     In this operation, meant to be fast, we do not look at the *pointed* object
     at all: the same pointer will in general have been updated many times
     before the SSB is flushed, and every update may well write a different
     value at the same address: the pointer-set structure will store only one
     entry for the assigned pointer, quickly filtering out duplicates, as long
     as the address is old; what the address will end up containing in the end
     will only be relevant later, at collection time.  Adding another test here
     for every SSB element would lead to fewer accesses to the pointer set, but
     at the cost of many tests on the content (all of them being useless except
     for the last one) instead of just one per pointer. */
  if (JITTER_GC_BOXED_TO_BLOCK (updated_object)->generation
      == jitter_gc_generation_old)
    {
#if defined (JITTER_GC_LOG)
      /* It is better in practice not to pollute the log with repeated SSB
         flushes of the same datum; only log the add operations which actually
         add something new. */
      int /* bool */ already_present;
      JITTER_WORD_SET_SET_HAS (& a->remembered_set, updated_object,
                               already_present);
      if (! already_present)
        jitter_gc_log_i (indentation_level,
                         "Insert %p %s into the remembered set\n",
                         (void *) updated_object,
                         jitter_gc_space_name_for (a, (void *) updated_object));
#endif // #if defined (JITTER_GC_LOG)
      JITTER_WORD_SET_ADD_UNIQUE (& a->remembered_set, updated_object);
    }
}

/* FIXME: comment. */
__attribute__ ((flatten))
//__attribute__ ((noinline, noclone)) // FIXME: for profiling only
static jitter_gc_heap_pointer
_jitter_gc_ssb_flush_n (struct jitter_gc_heaplet *a,
                        jitter_gc_heap_pointer runtime_allocation_limit,
                        size_t overflowed_item_no /* This must be zero or one. */)
{
  jitter_gc_heap_pointer original_allocation_limit = a->nursery.limit;
  jitter_gc_tagged_object *queue
    = (jitter_gc_tagged_object *) a->nursery.limit - 1; // FIXME: explain well the "- 1" part, which is not related to overflowed_item_no
  jitter_gc_tagged_object *queue_limit
    = ((jitter_gc_tagged_object *) runtime_allocation_limit
       + overflowed_item_no /* If there was overflow then the leftmost slot has
                               not been used. */
       - 1 /* The address where the limit points, discounting overflow, does
              contain a valid element, but I prefer a strict ">" check. */);
  jitter_gc_log ("SSB flush: %i elements (overflowed_item_no is %i)\n",
                 (int) (queue - queue_limit),
                 (int) overflowed_item_no);
  jitter_gc_log ("  Run pre-flush hooks:\n");
  jitter_gc_hooks_run (a, & a->pre_ssb_flush_hooks,
                       jitter_gc_collection_kind_ssb_flush);

  jitter_gc_tagged_object *p;
  for (p = queue; p > queue_limit; p --)
    {
      jitter_gc_tagged_object o = * p;
      /* jitter_gc_log ("  Add %p from SSB to remembered set (if old and not there already)\n", (void *) o); */
      jitter_gc_add_to_remembered_set (a, o, 2);
      /*
      jitter_gc_log ("  After considering %p the remembering set has size %li\n",
                     (void *) o, (long) a->remembered_set.used_element_no);
      */
#if defined (JITTER_GC_DEBUG)
      /* Invalidate the SSB element on the heap, to make it obvious that
         something is wrong if the entry is used again later by mistake,
         particularly within an attempt at allocation with an incorrect
         allocation pointer or limit pointer. */
      * p = a->shape_table->invalid_object;
#endif // #if defined (JITTER_GC_DEBUG)
    }

  /* Update statistics, except for the overflowed element which is handled
     separately (see _jitter_gc_ssb_flush_1 ). */
  a->ssb_flush_no ++;
  if (queue > queue_limit)
    {
      size_t queue_length = queue - queue_limit;
      a->total_ssb_length += queue_length;
    }

  jitter_gc_log ("  Run post-flush hooks:\n");
  jitter_gc_hooks_run (a, & a->post_ssb_flush_hooks,
                       jitter_gc_collection_kind_ssb_flush);

  /* Return what the user should use as the new allocation limit. */
  return original_allocation_limit;
}

// FIXME: make static if not used from the macros.
jitter_gc_heap_pointer
_jitter_gc_ssb_flush_0 (struct jitter_gc_heaplet *a,
                        jitter_gc_heap_pointer runtime_allocation_limit)
{
  /* This SSB flush begins now. */
  jitter_gc_ssb_flush_time_begin (a);

  /* Do the actual work. */
  jitter_gc_heap_pointer res
    = _jitter_gc_ssb_flush_n (a, runtime_allocation_limit, 0);

  /* This SSB flush is now over. */
  jitter_gc_ssb_flush_time_end (a);
  return res;
}

jitter_gc_heap_pointer
_jitter_gc_ssb_flush_1 (struct jitter_gc_heaplet *a,
                        jitter_gc_heap_pointer runtime_allocation_limit,
                        jitter_gc_tagged_object tagged_updated_object)
{
  /* This SSB flush begins now. */
  jitter_gc_ssb_flush_time_begin (a);

  /* Do the actual work for the SSB; the one element not fitting in the SSB will
     be handled later. */
  jitter_gc_heap_pointer res
    = _jitter_gc_ssb_flush_n (a, runtime_allocation_limit, 1);

  /* Add the one pointer which did not fit in the SSB.  By adding it last we
     follow the same order as the queue, which might be helpful for debugging
     and particularly for logging.
     Of course the remembered set is a hash, so the elements will not be checked
     in insertion order at collection time. */
  jitter_gc_add_to_remembered_set (a, tagged_updated_object, 2);
  a->total_ssb_length ++;

  /* This SSB flush is now over. */
  jitter_gc_ssb_flush_time_end (a);

  return res;
}




/* Collection space set up.
 * ************************************************************************** */

/* Initialise the space set up in the pointed healpet, before beginning to
   add entries.  This gives a NULL destination to every space, sets the set
   of tospaces to be empty and marks every space as not scavenged-from.
   This is a helper for jitter_gc_set_up_spaces . */
static void
jitter_gc_set_up_spaces_initialize (struct jitter_gc_heaplet *a)
{
  /* Make the space pointer arrays all empty, by setting each limit pointer
     (which points to the first unused element) point to the first array
     element. */
  a->fromspaces_limit = a->fromspaces;
  a->tospaces_limit = a->tospaces;
  a->spaces_to_clean_before_limit = a->spaces_to_clean_before;
  a->spaces_to_clean_after_limit = a->spaces_to_clean_after;

  /* For each space clear its scavenged-from flag and its destination space. */
  struct jitter_gc_space **sp;
  for (sp = a->spaces; sp < a->spaces_limit; sp ++)
    {
      struct jitter_gc_space *s = * sp;
      s->scavenged_from = false;
      s->destination_space = NULL;
    }
}

/* Add the pointed space as a fromspace in the pointed heaplet, if it is not
   a fromspace already.
   This is a helper for jitter_gc_set_up_spaces_initialize . */
static void
jitter_gc_add_fromspace (struct jitter_gc_heaplet *a,
                         struct jitter_gc_space *s)
{
  jitter_gc_add_space_pointer (a->fromspaces, & a->fromspaces_limit, s);
}

/* Like jitter_gc_add_fromspace , for a tospace instead of a fromspace. */
static void
jitter_gc_add_tospace (struct jitter_gc_heaplet *a,
                       struct jitter_gc_space *s)
{
  jitter_gc_add_space_pointer (a->tospaces, & a->tospaces_limit, s);
}

/* Like jitter_gc_add_tospace , but add a space to be emptied before
   collection. */
static void
jitter_gc_add_space_to_clean_before (struct jitter_gc_heaplet *a,
                                     struct jitter_gc_space *s,
                                     bool clean_completely)
{
  jitter_gc_add_space_pointer (a->spaces_to_clean_before,
                               & a->spaces_to_clean_before_limit, s);
  s->clean_completely_before = clean_completely;
}

/* Like jitter_gc_add_tospace , but add a space to be emptied after
   collection. */
static void
jitter_gc_add_space_to_clean_after (struct jitter_gc_heaplet *a,
                                    struct jitter_gc_space *s,
                                    bool clean_completely)
{
  jitter_gc_add_space_pointer (a->spaces_to_clean_after,
                               & a->spaces_to_clean_after_limit, s);
  s->clean_completely_after = clean_completely;
}

/* Configure the pointed heaplet so that the pointed origin space has the
   pointed destination space as its destination; make the origin space a
   fromspace, and the destination space a tospace.  Leave the destination of the
   destination as NULL: in this design no space can be a fromspace and a tospace
   at the same time.  Notice that in this design it is permitted for multiple
   origin spaces to have the same destination space.
   This is a helper for jitter_gc_set_up_spaces . */
static void
jitter_gc_set_up_add_from_to (struct jitter_gc_heaplet *a,
                              struct jitter_gc_space *from,
                              struct jitter_gc_space *to)
{
  /* FIXME: I think I should automatically add from to the array of spaces to
     be cleaned after the collection and remove explicit calls to
     jitter_gc_add_space_to_clean_after elsewhere.
     Cleaning to before the collection, instead, is not correct in every
     case. */
  jitter_gc_add_fromspace (a, from);
  jitter_gc_add_tospace (a, to);
  from->destination_space = to;
}

/* Set up the space data structures for the pointed heaplet in order to begin
   the given kind of collection.  The space data structures include the tospace
   array and its size, and the space map.
   This needs to be called at the beginning of a collection.*/
static void
jitter_gc_set_up_spaces (struct jitter_gc_heaplet *a,
                         enum jitter_gc_collection_kind kind)
{
  /* Make every space pointer array empty, and undo any from->to binding which
     may survive from previous collections. */
  jitter_gc_set_up_spaces_initialize (a);

  /* The share collection kind is unusual.  All the other kinds have a lot in
     common, and we call them “actual kinds”. */
  if (kind != jitter_gc_collection_kind_share)
    {
      /* At the beginning of a collection of any actual kind the young reserves
         are empty but are also always tospaces, so they must retain one
         block. */
      int i;
      for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
        jitter_gc_add_space_to_clean_before (a, a->young_reserves [i], false);

      /* At the end of a collection of any actual kind the nursery and the
         current young ageing spaces are empty.  The nursery will be used as a
         fromspace and therefore needs one block; the current young ageing
         spaces, however, will become young reserves, and can remain completely
         empty at mutation time. */
      jitter_gc_add_space_to_clean_after (a, & a->nursery, false);
      for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
        jitter_gc_add_space_to_clean_after (a, a->young_ageing_spaces [i], true);
    }

  /* The rest depends on the collection kind. */
  switch (kind)
    {
    case jitter_gc_collection_kind_minor:
    case jitter_gc_collection_kind_major: /* These are indeed similar. */
                                          // FIXME: global collections are indeed also similar, which is why in the comments below I speak of non-minor, rather than major, collections.
      {
        /* The "old target" is where objects promoted to the old generation
           go. */
        struct jitter_gc_space *old_target;
        if (kind == jitter_gc_collection_kind_minor)
          old_target = a->oldspace;
        else
          old_target = a->old_reserve;

        /* In any collection kind we are writing from the nursery into the first
           young reserve, from the first ageing space into the second young
           reserve, and so on; the last ageing space writes into the "old
           target". */
        if (JITTER_GC_NON_NURSERY_STEP_NO == 0)
          jitter_gc_set_up_add_from_to (a, & a->nursery, old_target);
        else
          {
            jitter_gc_set_up_add_from_to (a, & a->nursery,
                                          a->young_reserves [0]);
            int i;
            for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO - 1; i ++)
              jitter_gc_set_up_add_from_to (a,
                                            a->young_ageing_spaces [i],
                                            a->young_reserves [i + 1]);
            jitter_gc_set_up_add_from_to
              (a, a->young_ageing_spaces [JITTER_GC_NON_NURSERY_STEP_NO - 1],
               old_target);
          }

        /* We have already set up young-generation fromspaces and the
           old-generation tospace (the "old target").
           For non-minor collections we need a little more, since we are also
           scavenging the old space into the old reserve; the old reserve must
           be non-empty before scavenging, and the old space must be cleaned
           after scavenging. */
        if (kind != jitter_gc_collection_kind_minor)
          {
            jitter_gc_set_up_add_from_to (a, a->oldspace, a->old_reserve);
            jitter_gc_add_space_to_clean_before (a, a->old_reserve, false);
            jitter_gc_add_space_to_clean_after (a, a->oldspace, true);
          }
        break;
      }

#if defined (JITTER_GC_ENABLE_SHARING)
    case jitter_gc_collection_kind_share:
      /* In the share collection kind we scavenge:
         - from nursery and every ageing space to shared own;
         - from old space to shared own.
         No reserve is used. */
      jitter_gc_set_up_add_from_to (a, & a->nursery, & a->shared_space_own);
      int i;
      for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
        jitter_gc_set_up_add_from_to (a,
                                      a->young_ageing_spaces [i],
                                      & a->shared_space_own);
      jitter_gc_set_up_add_from_to (a, a->oldspace, & a->shared_space_own);
      for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
        jitter_gc_add_space_to_clean_before (a, a->young_reserves [i], true);
      jitter_gc_add_space_to_clean_before (a, a->old_reserve, true);
      break;
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

    default:
      jitter_fatal ("jitter_gc_set_up_spaces: unknown collection kind %s %i",
                    jitter_gc_collection_kind_to_string (kind), (int) kind);
    }
}




/* Space cleaning.
 * ************************************************************************** */

/* Remove every block from the pointed space (except one if clean_completely is
   non-false), returining them to the unused space of the pointed heaplet,
   which must be the same the pointed space belongs to.  If clean_completely
   is non-false then make the remaining block the allocation block, so that the
   space can be immediately used as a fromspace at mutation time or as a tospace
   at collection time.
   Unless we are debugging the complexity is O(1): blocks are transferred by
   linking their entire list to the unused space block list, without touching
   every block in the list. */
static void
jitter_gc_space_clean (struct jitter_gc_heaplet *a,
                       struct jitter_gc_space *s,
                       bool clean_completely)
{
  /* Move every block of s into the unused space. */
  jitter_gc_space_move_to_from (a->heap, & a->unused_space, s,
                                /*set_space*/      JITTER_GC_ARE_WE_DEBUGGING,
                                /*set_generation*/ JITTER_GC_ARE_WE_DEBUGGING,
                                /*set_clear*/      JITTER_GC_ARE_WE_DEBUGGING,
                                4);

  /* If not cleaning completely add one block back to the space.  This also sets
     the block as the current allocation block and resets the allocation and
     limit pointer.
     A space having an allocation block makes allocation possible, which is
     necessary if this space is to be used as a fromspace before mutation
     or as a tospace before collection. */
  if (! clean_completely)
    {
      jitter_gc_log ("    Move one block from unused back to %s to make %s "
                     "not completely empty\n", s->name, s->name);
      jitter_gc_space_procure_allocation_block (a, s);
    }
}

/* Clean the spaces set up by jitter_gc_set_up_spaces at the beginning of a
   collection, in order to make any unused block available for the new
   tospaces. */
static void
jitter_gc_clean_spaces_before (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_space **sp;
  for (sp = a->spaces_to_clean_before;
       sp < a->spaces_to_clean_before_limit;
       sp ++)
    {
      struct jitter_gc_space *s = * sp;
      jitter_gc_log ("  Clean %s, %s, at the beginning\n", s->name, s->clean_completely_before ? "COMPLETELY" : "NOT completely");
      jitter_gc_space_clean (a, s, s->clean_completely_before);
    }
}

/* Like jitter_gc_clean_spaces_before , for the spaces to be cleaned at the end
   of a collection; differently from jitter_gc_clean_spaces_before this adds one
   block to every space, so that it is usable as a fromspace at mutation time
   without a check for emptyness. */
static void
jitter_gc_clean_spaces_after (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_space **sp;
  for (sp = a->spaces_to_clean_after;
       sp < a->spaces_to_clean_after_limit;
       sp ++)
    {
      struct jitter_gc_space *s = * sp;
      jitter_gc_log ("  Clean %s, %s, at the end\n", s->name, s->clean_completely_after ? "COMPLETELY" : "NOT completely");
      jitter_gc_space_clean (a, s, s->clean_completely_after);
    }
}

/* Free unused blocks which should not exist according to the current size
   estimates.  This is meant to be called after flipping and cleaning spaces. */
static void
jitter_gc_free_unused_memory (struct jitter_gc_heaplet *a)
{
  jitter_gc_log ("  Freeing excess unused blocks.\n");
  struct jitter_gc_space *unused_space = & a->unused_space;
  size_t unused_space_bytes = unused_space->allocated_size_in_bytes;

  /* Compute an estimate of what should be kept in the working set.  This
     includes:
     - the oldspace size threshold;
     - the nursery size, multiplied by one plus the number of young steps times
       two (a conservative upper bound, which assumes 100% survival rate in
       youngspace: this choice ensures that even high survival rates will not
       entail new block allocations in the young generation). */
  size_t estimated_working_set_bytes
    = (a->oldspace_used_size_threshold_in_bytes
       + ((1 + 2 * JITTER_GC_NON_NURSERY_STEP_NO)
          * a->nursery_used_size_threshold_in_bytes));

  /* Subtract what is already allocated from the estimated working set size,
     obtaining the estimated best unused space size: the already used space is
     not part of what is being considered to be freed.
     Only count entire blocks. */
  size_t used_bytes = 0;
  used_bytes += jitter_gc_space_allocated_size_in_bytes (& a->nursery);
  int i;
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    {
      used_bytes
        += jitter_gc_space_allocated_size_in_bytes (a->young_ageing_spaces [i]);
      used_bytes
        += jitter_gc_space_allocated_size_in_bytes (a->young_reserves [i]);
    }
  used_bytes += jitter_gc_space_allocated_size_in_bytes (a->oldspace);
  used_bytes += jitter_gc_space_allocated_size_in_bytes (a->old_reserve);
  size_t new_unused_bytes;
  if (estimated_working_set_bytes > used_bytes)
    new_unused_bytes = estimated_working_set_bytes - used_bytes;
  else
    new_unused_bytes = 0;

  /* We want to free the unused blocks in excess of those which, according to
     the estimate, are needed in the working set.  If the unused blocks are less
     than that limit there is nothing to free, as we predict the working set to
     grow. */
  size_t bytes_to_free;
  if (unused_space_bytes > new_unused_bytes)
    bytes_to_free = unused_space_bytes - new_unused_bytes;
  else
    bytes_to_free = 0;

#if defined (JITTER_GC_LOG)
  JITTER_HUMAN_READABLE_ (initial_size,
                          unused_space->allocated_size_in_bytes, true);
#endif // #if defined (JITTER_GC_LOG)

  /* Free blocks starting from the end, so that the first we added will remain
     in use.  In case bytes_to_free is larger than the actual allocated size
     stop as soon as the unused space becomes empty.
     The arrays to be cleaned are filled in an order intended not to
     damage locality too much. */
  struct jitter_gc_block *b = unused_space->blocks.last;
  size_t freed_bytes = 0;
  while (b != NULL && freed_bytes < bytes_to_free)
    {
      struct jitter_gc_block *previous = b->links.previous;
      /* Notice that the space field within b may not point to the unused space
         now: it not kept up to date for reasons of efficiency, in order to move
         entire lists of blocks from one space to another.  Update the space
         pointer here, just in order to be able to unlink b from its correct
         list. */
      b->space = unused_space;
      jitter_gc_block_destroy (b, true);
      b = previous;
      freed_bytes += JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES;
    }
  unused_space->allocated_size_in_bytes -= freed_bytes;

#if defined (JITTER_GC_LOG)
  JITTER_HUMAN_READABLE_ (estimated_working_set_bytes_h, estimated_working_set_bytes, true);
  JITTER_HUMAN_READABLE_ (unused_bytes_h, unused_space_bytes, true);
  JITTER_HUMAN_READABLE_ (used_bytes_h, used_bytes, true);
  JITTER_HUMAN_READABLE_ (new_unused_bytes_h, new_unused_bytes, true);
  JITTER_HUMAN_READABLE_ (freed_bytes_h, freed_bytes, true);
  JITTER_HUMAN_READABLE_ (final_size,
                          unused_space->allocated_size_in_bytes, true);
#endif // #if defined (JITTER_GC_LOG)
  jitter_gc_log ("    Bytes already in use holding data: %.1f %sB\n",
                 used_bytes_h,
                 used_bytes_h_prefix);
  jitter_gc_log ("    Unused bytes:                      %.1f %sB\n",
                 unused_bytes_h,
                 unused_bytes_h_prefix);
  jitter_gc_log ("    Working set estimated size:        %.1f %sB\n",
                 estimated_working_set_bytes_h,
                 estimated_working_set_bytes_h_prefix);
  jitter_gc_log ("    New unused bytes:                  %.1f %sB\n",

                 new_unused_bytes_h,
                 new_unused_bytes_h_prefix);
  jitter_gc_log ("    After freeing %.1f %sB of %.1f %sB the unused space payload is %.1f %sB\n",
                 freed_bytes_h, freed_bytes_h_prefix,
                 initial_size, initial_size_prefix,
                 final_size, final_size_prefix);

  /* The unused space only contains unused blocks, and therefore does not need a
     correct allocation page, scan page, allocation pointer, limit pointer, scan
     pointer.  We are done. */
}




/* Object finalisation.
 * ************************************************************************** */

/* Return the length of the given list. */
static size_t
jitter_gc_list_length (struct jitter_list_header *list)
{
  /* This is implemented traversing first-to-last; of course it is assumed that
     the length would be the same last-to-first, and that next and previous
     pointers are consistent. */
  size_t res = 0;
  struct jitter_gc_finalization_data *field;
  for (field = list->first; field != NULL; field = field->links.next)
    res ++;
  return res;
}

/* Measure time at the beginning of an finalisation phase flush, when such
   statistics are enabled. */
inline static void
jitter_gc_finalization_time_begin (struct jitter_gc_heaplet *a)
{
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  jitter_time_set_now (a->finalization_begin_time);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
}

/* Measure time at the end of finalisation, when such statistics are enabled,
   adding the time for this phase to the total. */
inline static void
jitter_gc_finalization_time_end (struct jitter_gc_heaplet *a)
{
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  /* Count the latency for this finalisation time as part of finalisation.
     Notice that finalization time is contained as part as collection time,
     since at this point a collection is still in progress. */
  double latest_finalization_time
    = jitter_time_subtract_from_now (a->finalization_begin_time);
  a->total_finalization_time += latest_finalization_time;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
}

/* Join the mutation_time_finalizables lists in every fromspace into a single list
   candidate_dead_finalizables within the pointed heaplet.  Make every
   mutation_time_finalizables list empty. */
static void
jitter_gc_join_fromspace_finalizables_into_candidate_dead
   (struct jitter_gc_heaplet *a)
{
  /* This code is not timed, even with expensive statistics enabled.  It would
     be pointless and impossible to measure accurately: this code is O(1)
     since the number of spaces is bounded and in fact small; run time does not
     depend on list lengths.
     In practice I would expect at most a few tens of nanoseconds for this on a
     modern machine. */
// jitter_gc_finalization_time_begin (a);
  struct jitter_gc_space **sp;
  for (sp = a->fromspaces; sp < a->fromspaces_limit; sp ++)
    {
      struct jitter_gc_space *s = * sp;
      JITTER_LIST_APPEND_LIST (jitter_gc_finalization_data, links,
                               & a->candidate_dead_finalizables.header,
                               & s->mutation_time_finalizables.header);
    }
// jitter_gc_finalization_time_end (a);
}

/* Return a pointer to the shape of the pointed object, using the pointer shape
   table.
   The object is assumed to be finalisable, and not a broken heart. */
static const struct jitter_gc_shape *
jitter_gc_find_shape_for_finalizable (const struct jitter_gc_shape_table *st,
                                      void *untagged_initial_pointer)
{
  const struct jitter_gc_shape *shapes
    = JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER
         (& st->finalizable_shapes);
  const struct jitter_gc_shape *shape_limit
    = JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST
         (& st->finalizable_shapes);

  /* The first word must be a type code, since every finalisable object is also
     headered. */
  jitter_gc_tagged_object first_word
    = * (jitter_gc_tagged_object *) untagged_initial_pointer;

#if defined (JITTER_GC_DEBUG)
  /* Do a sanity check, making sure that this is not a broken heart. */
  jitter_gc_tagged_object broken_heart_type_code
    = st->broken_heart_type_code;
  if (first_word == broken_heart_type_code)
    jitter_fatal ("found a broken heart in object at %p being finalised",
                  untagged_initial_pointer);
#endif // #if defined (JITTER_GC_DEBUG)

  /* Now just check every shape in order. */
  const struct jitter_gc_shape *shape;
  for (shape = shapes; shape < shape_limit; shape ++)
    if (shape->is_type_code (first_word))
      return shape;

  /* If we arrived here we failed to find a match.  This should not happen. */
  jitter_fatal ("could not find finalisable shape for %p (first word %p)",
                untagged_initial_pointer, (void *) first_word);
}

/* Finalise the pointed object, which is assumed to be finalisable and, if
   non-quickly finalisable, is assumed to have all its fields already available
   without broken hearts.
   Do not unlink or link the object from or to any list.
   The object is assumed to belong to the pointed heap and, if the heaplet
   pointer is non-NULL, to the pointed heaplet.  The object is assumed to have
   one of the shapes in the pointed shape table, which is in its turn assumed to
   be the same used in the heap and (where given) the heaplet. */
static void
jitter_gc_finalize_untagged (const struct jitter_gc_shape_table *st,
                             struct jitter_gc_heap *h,
                             struct jitter_gc_heaplet *a,
                             void *untagged_initial_pointer,
                             int indentation_level)
{
  /* Find the correct shape for the object. */
  const struct jitter_gc_shape *shape
    = jitter_gc_find_shape_for_finalizable (st, untagged_initial_pointer);
  jitter_gc_log_i (indentation_level,
                   "Finalising %p as %s\n",
                   untagged_initial_pointer, shape->name);

  /* Use the finaliser from that shape. */
#if defined (JITTER_GC_DEBUG)
  if (shape->finalization_kind == jitter_gc_shape_finalization_kind_none)
    jitter_fatal ("cannot finalize object at %p of non-finalisable shape %s",
                  untagged_initial_pointer, shape->name);
#endif // #if defined (JITTER_GC_DEBUG)
  shape->finalize (h, a, untagged_initial_pointer);
}

/* Finalise every dead quickly-finalisable object, unlinking every such object
   from the list.  This is a helper for jitter_gc_handle_object_finalization .
   Return the number of finalised objects. */
static size_t
jitter_gc_finalize_dead_quickly_finalizables (struct jitter_gc_heaplet *a)
{
  const struct jitter_gc_shape *shapes
    = JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER
         (& a->shape_table->quickly_finalizable_shapes);
  const struct jitter_gc_shape *shape_limit
    = JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST
         (& a->shape_table->quickly_finalizable_shapes);
#if defined (JITTER_GC_DEBUG)
  jitter_gc_tagged_object broken_heart_type_code
    = a->shape_table->broken_heart_type_code;
#endif // #if defined (JITTER_GC_DEBUG)

  /* Finalise every object in the dead-finalisable list, in any order they
     happen to have been linked.  We can afford not to scavenge their fields, as
     quickly-finalisable object finalisers are supposed not to rely on them
     being correct: the values of non-heap fields suffice to do the work. */
  jitter_gc_log ("  Finalise dead quickly-finalisable objects:\n");
  struct jitter_list_header *list = & a->candidate_dead_finalizables.header;
// printf ("QLIST LENGTH %li\n", (long) jitter_gc_list_length (list));
  struct jitter_gc_finalization_data *field;
  int finalized_object_no = 0;
  for (field = list->first; field != NULL; /* Nothing. */)
    {
      struct jitter_gc_finalization_data *next = field->links.next;
      jitter_gc_tagged_object *untagged_initial_pointer
        = _JITTER_GC_FINALIZABLE_FINALIZATION_FIELD_TO_UNTAGGED (field);
      jitter_gc_tagged_object first_word = * untagged_initial_pointer;
#if defined (JITTER_GC_DEBUG)
      if (first_word == broken_heart_type_code)
        jitter_fatal ("dead quickly-finalisable at %p is a broken heart\n",
                      untagged_initial_pointer);
#endif // #if defined (JITTER_GC_DEBUG)
//printf ("QCONSIDERING %p %s\n", untagged_initial_pointer, jitter_gc_space_name_for (a, untagged_initial_pointer));
      const struct jitter_gc_shape *shape;
      for (shape = shapes; shape < shape_limit; shape ++)
        if (shape->is_type_code (first_word))
          {
            jitter_gc_log ("    %p %s: finalise %s\n", untagged_initial_pointer, jitter_gc_space_name_for (a, untagged_initial_pointer), shape->name);
            shape->finalize (a->heap, a, untagged_initial_pointer);
            JITTER_LIST_UNLINK (jitter_gc_finalization_data, links,
                                list, field);
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
            finalized_object_no ++;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
            break;
          }

      /* Look at the next element of the list, using the pointer we saved
         above before unlinking the field from the list. */
      field = next;
    }
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  jitter_gc_log ("  Finalised %i quickly-finalisable objects\n", finalized_object_no);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  return finalized_object_no;
}

/* Finalise dead finalisable objects of any shape, quickly finalisable or
   complete-object finalisable.  This assumes that some complete-object
   finalisable shape exists, and therefore first scavenges every object
   reachable from dead finalisable objects, then calls the finalisers.  When
   no complete-object finalisable shape exists this function is not used, as
   jitter_gc_finalize_dead_quickly_finalizables is a faster alternative.
   Return the number of finalised objects.
   This is a helper for jitter_gc_hanlde_finalization . */
static size_t
jitter_gc_finalize_dead_any_finalizables (struct jitter_gc_heaplet *a)
{
  size_t res = 0;
  /* Headerless or non-finalisable objects are irrelevant here, and in fact
     finding any of them in the list would be an error. */
  const struct jitter_gc_shape *shapes
    = JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER
         (& a->shape_table->finalizable_shapes);
  const struct jitter_gc_shape *shape_limit
    = JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST
         (& a->shape_table->finalizable_shapes);
  jitter_gc_tagged_object broken_heart_type_code
    = a->shape_table->broken_heart_type_code;
  const struct jitter_gc_shape *shape;

  /* Before finalising an unreachable object we need to trace its fields, so
     that the finaliser sees a consistent state.  This is not feasible using
     only the list, since tracing fields can modify the list itself in a complex
     way, potentially unlinking any number of elements in any order.  So build a
     temporary array of initially unreachable complete-object-finalisable
     objects, as untagged pointers. */
  struct jitter_list_header *list = & a->candidate_dead_finalizables.header;
  size_t initial_unreachable_finalizable_no = jitter_gc_list_length (list);
  jitter_gc_tagged_object **initial_unreachable_finalizables
    = jitter_xmalloc (initial_unreachable_finalizable_no
                      * sizeof (jitter_gc_tagged_object *));
  struct jitter_gc_finalization_data *field;
  int i = 0;
  for (field = list->first; field != NULL; field = field->links.next)
    {
      jitter_gc_tagged_object *untagged_initial_pointer
        = _JITTER_GC_FINALIZABLE_FINALIZATION_FIELD_TO_UNTAGGED (field);
      initial_unreachable_finalizables [i ++] = untagged_initial_pointer;
    }

  /* We now have the array.  Update the fields of every object in the array,
     without moving the object itself unless it is reached by some other object
     -- in which case the element will turn into a broken heart, and the new
     copy will remain alive. */
  jitter_gc_log ("  Update fields for %li unreachable finalisable objects:\n", (long) initial_unreachable_finalizable_no);
  for (i = 0; i < initial_unreachable_finalizable_no; i ++)
    {
      jitter_gc_tagged_object *untagged_initial_pointer
        = initial_unreachable_finalizables [i];
      jitter_gc_tagged_object first_word = * untagged_initial_pointer;
      if (first_word == broken_heart_type_code)
        {
          jitter_gc_log ("    %p %s: ignore broken heart\n", untagged_initial_pointer, jitter_gc_space_name_for (a, untagged_initial_pointer));
          continue;
        }
      bool found = false;
      for (shape = shapes; shape < shape_limit; shape ++)
        if (shape->is_type_code (first_word))
          {
            jitter_gc_log ("    %p %s: update %s fields without moving the object\n", untagged_initial_pointer, jitter_gc_space_name_for (a, untagged_initial_pointer), shape->name);
            shape->object_update_fields (a, untagged_initial_pointer);
            found = true;
            break;
          }
      if (! found)
        jitter_fatal ("invalid type code while updating fields");
    }
  free (initial_unreachable_finalizables);

  /* Scavenge, in order to make sure every object *reachable* from the fields
     is also copied.  Again this is needed in order for finalisers to see a
     consistent state, but will keep more objects alive. */
  jitter_gc_log ("  Scavenge for complete-object finalisers:\n");
  jitter_gc_scavenge (a);

  /* At this point whatever survives in the list is dead and not reachable from
     the dead set. */
  jitter_gc_log ("  Finalise:\n");
  size_t complete_object_finalized_no = 0;
  while ((field = list->first) != NULL)
    {
      jitter_gc_tagged_object *untagged_initial_pointer
        = _JITTER_GC_FINALIZABLE_FINALIZATION_FIELD_TO_UNTAGGED (field);
      jitter_gc_tagged_object first_word = * untagged_initial_pointer;
#if defined (JITTER_GC_DEBUG)
      if (first_word == broken_heart_type_code)
        jitter_fatal ("%p: broken heart in finalisation list: something is "
                      "horribly wrong", untagged_initial_pointer);
#endif // #if defined (JITTER_GC_DEBUG)
      bool found = false;
      for (shape = shapes; shape < shape_limit; shape ++)
        if (shape->is_type_code (first_word))
          {
            jitter_gc_log ("    %p %s: finalise %s\n", untagged_initial_pointer, jitter_gc_space_name_for (a, untagged_initial_pointer), shape->name);
/*printf ("C");*/
            shape->finalize (a->heap, a, untagged_initial_pointer); // FIXME: shall I finalize *after* unlinking instead?
            JITTER_LIST_UNLINK (jitter_gc_finalization_data, links,
                                list, field);
            // _JITTER_GC_SET_ALREADY_FINALIZED (field, true);
            // FIXME: think of what to do about resurrection.  It is now forbidden.
            // FIXME: this idea may actually allow resurrection if I handle it intelligently.
            // field->links.next = field->links.previous = NULL; ////////////////////////
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
            complete_object_finalized_no ++;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)

            found = true;
            break;
          }
      if (! found)
        jitter_fatal ("invalid type code while finalising");
    }
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  jitter_gc_log ("  Finalised %li of %li initially unreachable finalisable objects\n", (long) complete_object_finalized_no, (long) initial_unreachable_finalizable_no);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  res += complete_object_finalized_no;
  return res;
}

/* Perform finalisation, measuring time if needed.  This requires that
   jitter_gc_join_fromspace_finalizables_into_candidate_dead has been called
   already; it cannot be called here as it is required much earlier, before
   any object is moved.  This is supposed to be called near the end of a
   collection, after scavenging alive objects.
   Return non-false if there are still some objects in need to be finalised at
   the end. */
static int /* bool */
jitter_gc_handle_object_finalization (struct jitter_gc_heaplet *a)
{
  jitter_gc_finalization_time_begin (a);

  size_t finalized_object_no __attribute__ ((unused));
  struct jitter_list_header *list = & a->candidate_dead_finalizables.header;

  /* If there is nothing to finalise just return immediately.  While the value
     of this as an optimisation is questionable at least the log line may prove
     useful. */
  if (list->first == NULL)
    {
      jitter_gc_log ("  No finalisable objects reachable\n");
      finalized_object_no = 0;
    }
  /* If there are no complete-object-finalisable shapes then only finalise
     quickly-finalisable objects, which is cheaper. */
  else if (! jitter_gc_shape_table_has_complete_object_finalizable
                (a->shape_table))
    finalized_object_no = jitter_gc_finalize_dead_quickly_finalizables (a);
  /* Otherwise we are in the general case. */
  else
    finalized_object_no = jitter_gc_finalize_dead_any_finalizables (a);

#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->total_finalized_object_no += finalized_object_no;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  jitter_gc_finalization_time_end (a);

  /* Iff there are still objects to be finalised later then the list is not
     empty. */
  return list->first != NULL;
}

/* Finalize every finalizable object in the alive_finalisable list from the
   pointed space, without performing any collection, assuming there are no
   broken hearts.
   Return the number of objects which were finalised. */
static size_t
jitter_gc_object_finalize_all_finalizables_in_space
   (const struct jitter_gc_shape_table *st,
    struct jitter_gc_heap *h,
    struct jitter_gc_heaplet *a,
    struct jitter_gc_space *s)
{
  size_t finalised_object_no = 0;

  struct jitter_list_header *list_header
    = & s->mutation_time_finalizables.header;
  jitter_gc_log ("    Finalising all %li finalisable objects in %s\n",
                 (long) jitter_gc_list_length (list_header),
                 s->name);
  struct jitter_gc_finalization_data *field;
  while ((field = list_header->first) != NULL)
    {
      /* Detach the element we are working on from the list.  At the next
         iteration we will pick the new first element, whatever it is.  We have
         to do this instead of a simple for loop walking from list_header->first
         and then following field->links.next because each finaliser might alter
         the list*/
      JITTER_LIST_UNLINK (jitter_gc_finalization_data, links,
                          list_header, field);

      /* Finalise the element. */
      jitter_gc_tagged_object *untagged_initial_pointer
        = _JITTER_GC_FINALIZABLE_FINALIZATION_FIELD_TO_UNTAGGED (field);
      jitter_gc_finalize_untagged (st, h, a, untagged_initial_pointer, 6);

      /* We are done with this.  Count it and proceed with the next if any. */
      finalised_object_no ++;
    }

  return finalised_object_no;
}

/* Finalize every finalizable object in the pointed heaplet, assuming that they
   are in mutator spaces, all consistent and with no forwarding poitners.
   This is meant to be called at heap finalisation time, with no need for
   an actual collection.
   Return the number of objects which were finalised.
   Notice that for a heap to be finalised all of its heaplets must have been
   finalised already: the only existing object will be in the heap shared
   space. */
static size_t
jitter_gc_object_finalize_all_heap_finalizables (struct jitter_gc_heap *h)
{
  jitter_gc_log ("  Finalising finalisable objects in heap %p\n", h);

#if defined(JITTER_GC_ENABLE_SHARING)
  /* There is only one list to scan, which is the list of finalisable objects
     which were alive at the last collection, inside the heap shared space which
     is the only remaining space. */
  return jitter_gc_object_finalize_all_finalizables_in_space (h->shape_table, h, NULL,
                                                       & h->shared_space);
#else
  /* With sharing disabled there is nothing to do. */
  return 0;
#endif /* #if defined(JITTER_GC_ENABLE_SHARING) */
}

/* Finalize every finalizable object in the pointed heaplet (except the shared
   space, which belongs to the heap), assuming that the object to finalise are
   in mutator spaces, all consistent and with no broken hearts.
   Return the number of objects which were finalised.
   This is meant to be called at heaplet finalisation time with no need for a
   collection.*/
static size_t
jitter_gc_object_finalize_all_heaplet_finalizables (struct jitter_gc_heaplet *a)
{
  const struct jitter_gc_shape_table *st = a->shape_table;
  struct jitter_gc_heap *h = a->heap;

  size_t finalised_object_no = 0;

  /* Finalise all finalisable object in any space which may contain them. */
  finalised_object_no +=
    jitter_gc_object_finalize_all_finalizables_in_space (st, h, a, & a->nursery);
  int i;
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    finalised_object_no +=
      jitter_gc_object_finalize_all_finalizables_in_space (st, h, a,
                                                    a->young_ageing_spaces [i]);
  finalised_object_no +=
    jitter_gc_object_finalize_all_finalizables_in_space (st, h, a, a->oldspace);
  /* Do not finalise objects in shared_space_own : they belong to the heap which
     is not being finalised here. */
#if defined (JITTER_GC_DEBUG)
  /* There must be no objects to finalise in unused_space . */
  if (a->unused_space.mutation_time_finalizables.header.first != NULL)
    jitter_fatal ("there are finalisable objects in the unused space at heap "
                  "finalisation");
#endif /* #if defined (JITTER_GC_DEBUG) */

  return finalised_object_no;
}




/* Collection.
 * ************************************************************************** */

// FIXME: move to a debugging section.
/* Return a non-NULL value equal to the boxed shape name iff the given word is a
   boxed header.  This is intended to check, when debugging, that no object has
   the value of a boxed header for any known shape.  Having such values when
   headerless boxed objects also exist would horribly confuse the garbage
   collector, and would be a symptom of some serious bug. */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static const char *
jitter_gc_is_boxed_header (struct jitter_gc_heaplet *a,
                           jitter_gc_tagged_object w)
{
  /* Check whether the first word is a header, iterating over the existing
     shapes with a header.  If it is return the shape name. */
  const struct jitter_gc_shape *shapes
    = JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER
         (& a->shape_table->headerful_shapes);
  const struct jitter_gc_shape *shape_limit
    = JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST
         (& a->shape_table->headerful_shapes);
  const struct jitter_gc_shape *shape;
  for (shape = shapes; shape < shape_limit; shape ++)
    if (shape->is_type_code (w))
      return shape->name;

  /* If we arrived here then w does not match any boxed header. */
  return NULL;
}

// FIXME: move to a debugging section.
/* Fail fatally if the pointed word is a header for a broken heart.
   Rationale: see jitter_gc_is_boxed_header . */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static void
jitter_gc_check_non_broken_heart_type_code (struct jitter_gc_heaplet *a,
                                            jitter_gc_tagged_object *p)
{
  if (* p == a->shape_table->broken_heart_type_code)
    jitter_fatal ("found a broken heart header as an object at %p", p);
}

// FIXME: move to a debugging section.
/* Fail fatally if the pointed word is a header for a broken heart header or a
   header for some boxed shape.
   Rationale: see jitter_gc_is_boxed_header . */
#if ! defined (JITTER_GC_DEBUG)
__attribute__ ((unused))
#endif // #if ! defined (JITTER_GC_DEBUG)
static void
jitter_gc_check_non_header (struct jitter_gc_heaplet *a,
                            jitter_gc_tagged_object *p)
{
  jitter_gc_check_non_broken_heart_type_code (a, p);
  const char *boxed_header_name = jitter_gc_is_boxed_header (a, * p);
  if (boxed_header_name != NULL)
    jitter_fatal ("found a %s header %p as an object at %p",
                  boxed_header_name, (void *) (jitter_uint) * p, p);
}

/* Perform cleanup functions, to be called at the end of each collection. */
static void
jitter_gc_post_collection_cleanup (struct jitter_gc_heaplet *a,
                                   enum jitter_gc_collection_kind kind)
{
  jitter_gc_temporary_root_set_compact (& a->temporary_root_set);

  switch (kind)
    {
    case jitter_gc_collection_kind_major:
      /* Here after a major collection, we should typically have cleaned up a
         good number of blocks.
         This is a good time to release unused memory, according
         to the new oldspace size estimate. */
      jitter_gc_free_unused_memory (a);
      break;

    default:
      /* Nothing to do in the other cases. */
      break;
    }
}

/* FIXME: here flatten appears to be beneficial, even if not as dramatically
   as in jitter_gc_scavenge -- which of course flattens this function as well,
   since jitter_gc_scavenge calls jitter_gc_handle_word . */
//__attribute__ ((noinline, noclone, flatten))
__attribute__ ((flatten))
void
jitter_gc_handle_word (struct jitter_gc_heaplet *a, jitter_gc_tagged_object *p)
{
  jitter_gc_tagged_object o = * p;
  //jitter_gc_log ("  + Handling pointer at %p: %li (%lu %lx):\n", p, o, o, o);
  const struct jitter_gc_shape_table *shape_table = a->shape_table;

#if defined (JITTER_GC_DEBUG)
  /* There is something horribly wrong if we find a header as the value of an
     object. */
  jitter_gc_check_non_header (a, p);
#endif // #if defined (JITTER_GC_DEBUG)

  /* If the object is unboxed we do not need to do anything. */
  if (shape_table->is_unboxed (o))
    {
      jitter_gc_log ("      %p: unboxed %p\n", p, (void *) o);
      return;
    }

  /* If we arrived here then p is a tagged pointer to some boxed object. */

  /* If the object is a broken heart we do not even need to check for its shape;
     that would in fact be impossible in some cases, as the broken heart would
     have overwritten the first two words of the original copy. */
  jitter_gc_tagged_object *address_untagged
    = (jitter_gc_tagged_object *) (o & JITTER_GC_BOXED_NON_TAG_BIT_MASK);
  //jitter_gc_log ("      %p: %p must be a tagged boxed object (untagged %p)\n", p, (void *) o, address_untagged);
  if (* address_untagged == shape_table->broken_heart_type_code)
    {
      /* The object is a broken heart.  The new destination is kept right after
         the broken-heart header, already tagged according to the shape of the
         copy, which may be different from the original. */
      jitter_gc_tagged_object destination_tagged = address_untagged [1];
#if defined (JITTER_GC_DEBUG)
      /* If the tagged address following the broken heart does not point within
         one of the destination spaces then something is horribly wrong; maybe
         the user has made a boxed object with no header and a first element
         which looks like a broken-heart header in fromspace.
         It is acceptable for the pointer to refer the shared-own space: see the
         comment before jitter_gc_is_in_tospace_or_shared_own for an
         explanation. */
      char *destination_untagged = ((char *)
                                    (destination_tagged
                                     & JITTER_GC_BOXED_NON_TAG_BIT_MASK));
      if (! jitter_gc_is_in_tospace_or_shared_own (a, destination_untagged))
        jitter_fatal ("non-tospace non-shared-own pointer (untagged %p space "
                      "%s) following the broken-heart header at %p",
                      destination_untagged,
                      JITTER_GC_BOXED_TO_BLOCK(destination_untagged)->space->name,
                      address_untagged);
#endif // #if defined (JITTER_GC_DEBUG)
      /* The object is a broken heart.  Replace p with the new tagged pointer
         we have found after the broken-heart header. */
      jitter_gc_log ("      %p: follow broken heart from %p %s to %p %s\n", p, address_untagged, jitter_gc_space_name_for (a, address_untagged), (void *) (destination_tagged & JITTER_GC_BOXED_NON_TAG_BIT_MASK), jitter_gc_space_name_for (a, (void *) (destination_tagged & JITTER_GC_BOXED_NON_TAG_BIT_MASK)));
      * p = destination_tagged;
      return;
    }

  /* Find the destination space where the pointed object needs to be copied.
     This will be NULL if the object does not need to be copied (this happens
     for example with young-to-old pointers in minor collections, and for
     pointers which have already been copied to a tospace; it is not an
     error). */
  struct jitter_gc_block *source_block = JITTER_GC_BOXED_TO_BLOCK (o);
  struct jitter_gc_space *source_space = source_block->space;
  struct jitter_gc_space *destination_space = source_space->destination_space;
  if (destination_space == NULL)
    {
      jitter_gc_log ("      %p: ignore %p %s\n", p, address_untagged, jitter_gc_space_name_for (a, address_untagged));
      return;
    }

  /* If we arrived here the object is boxed, not a broken heart, and in a space
     from which it needs to be copied.  Mark the source space as scavenged from
     (about performance see the field comment in struct jitter_gc_heaplet) and
     copy the object. */
  source_space->scavenged_from = true;
  const struct jitter_gc_shape *shapes
    = JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER (& shape_table->shapes);
  const struct jitter_gc_shape *shape_limit
    = JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST (& shape_table->shapes);
  const struct jitter_gc_shape *shape;
  for (shape = shapes; shape < shape_limit; shape ++)
    if (shape->object_has_shape (o))
      {
        /* Allocate the new object.  Notice that this may change the current
           block in destination_space, so it cannot be a simple pointer
           bumping. */
        size_t old_size_in_bytes = shape->object_size_in_bytes (o);
        jitter_gc_tagged_object *new_address_untagged
          = ((jitter_gc_tagged_object *)
             jitter_gc_allocate_from (a, destination_space, old_size_in_bytes));
        jitter_gc_log ("      %p: move %s from %p %s to %p %s\n", p, shape->name, address_untagged, jitter_gc_space_name_for (a, address_untagged), new_address_untagged, jitter_gc_space_name_for (a, jitter_gc_block_payload (destination_space->allocation_block)));
        jitter_gc_tagged_object new_address_tagged;
        size_t new_size_in_bytes = shape->object_copy (a,
                                                       & new_address_tagged,
                                                       address_untagged,
                                                       new_address_untagged);
        if (new_size_in_bytes != old_size_in_bytes)
          jitter_gc_log ("        object size changed from %li B to %li B\n", (long) old_size_in_bytes, (long) new_size_in_bytes);
#if defined (JITTER_GC_DEBUG)
        if (new_size_in_bytes > old_size_in_bytes)
          jitter_fatal ("an %s grew at copy from %li B to %li B, instaed of "
                        "remaining the same size or shrinking", shape->name,
                        (long) old_size_in_bytes, (long) new_size_in_bytes);
#endif // #if defined (JITTER_GC_DEBUG)
        /* In case the object shrank during the copy, adjust the allocation
           pointer.  This is always safe and cannot cross a block boundary. */
        destination_space->allocation_pointer
          -= (old_size_in_bytes - new_size_in_bytes);

        /* Replace the beginning of the original copy with a broken heart. */
        address_untagged [0] = shape_table->broken_heart_type_code;
        address_untagged [1] = new_address_tagged;

        /* Make the pointer point to the new copy.  Notice that this is allowed
           to change the pointer tag as well, along with any header in the new
           copy. */
        * p = new_address_tagged;
        return;
      }

  /* If we arrived here the object looks boxed and is not a broken heart, but
     does not match any shape. */
  jitter_fatal ("invalid object at %p, %p", p, (* (void **)p));
}

/* Scan the next object in the pointed space (advancing the scan pointer and
   changing block as needed) within the pointed heaplet, handling each tagged
   object it contains in a way appropriate to the shape.
   If the first word of the object is not a header then assume that the pointed
   word belongs to a headerless boxed object (it may or may not be its first
   word) and handle it as such.
   Avance the scan pointer past the scanned words. */
//__attribute__ ((noinline, noclone, flatten))
__attribute__ ((flatten))
/* FIXME: flatten helps here. */
static void
jitter_gc_scan_next (struct jitter_gc_heaplet *a,
                     struct jitter_gc_space *s)
{
  /* Change the current scan block if needed.  We need to change blocks if the
     current block has been exhausted, and is not the current allocation block;
     in the current allocation block the used_limit field is kept NULL, so that
     this change condition never becomes true and here we can avoid the overhead
     of checking whether the scan block is the same as the allocation block. */
#if defined (JITTER_GC_DEBUG)
  if (s->scan_block == s->allocation_block
      && s->scan_block->used_limit != NULL)
    jitter_fatal ("the used limit in the current allocation block %p [%p, %p) "
                  "is %p instead of being NULL: this should never happen",
                  s->scan_block,
                  jitter_gc_block_payload (s->scan_block),
                  jitter_gc_block_limit (s->scan_block),
                  s->scan_block->used_limit);
#endif // #if defined (JITTER_GC_DEBUG)
  if (__builtin_expect (s->scan_pointer == s->scan_block->used_limit,
                        false))
    jitter_gc_space_change_scan_block (a, s);

  jitter_gc_tagged_object *untagged_pointer
    = (jitter_gc_tagged_object *) s->scan_pointer;
  jitter_gc_tagged_object first_word = * untagged_pointer;
  const struct jitter_gc_shape_table *shape_table = a->shape_table;

#if defined (JITTER_GC_LOG)
  char *space_name = jitter_gc_space_name_for (a, untagged_pointer);
#endif // #if defined (JITTER_GC_LOG)

#if defined (JITTER_GC_DEBUG)
  /* There is something horribly wrong if we find a broken heart type code as
     the value of an object: broken hearts are in fromspace, but scavenging
     happens in tospace. */
  jitter_gc_check_non_broken_heart_type_code (a, ((jitter_gc_tagged_object *)
                                                  untagged_pointer));
#endif // #if defined (JITTER_GC_DEBUG)

  /* Check whether the first word is a type code for a known boxed shape; in
     that case call the user-supplied function to update the object fields.  The
     user-supplied function returns the object size, which is what we need here
     to know by how much to advance the scan pointer, which is the result of
     this function. */
  const struct jitter_gc_shape *shapes
    = JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER
         (& shape_table->headerful_shapes);
  const struct jitter_gc_shape *shape_limit
    = JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST
         (& shape_table->headerful_shapes);
  const struct jitter_gc_shape *shape;
  for (shape = shapes; shape < shape_limit; shape ++)
    if (shape->is_type_code (first_word))
      {
        jitter_gc_log ("    %p %s: update %s fields:\n", untagged_pointer, space_name, shape->name);
        s->scan_pointer += shape->object_update_fields (a, untagged_pointer);
        return;
      }

  /* If we arrived here then the object has no header, and each word of it must
     be handled like an ordinary tagged object.  We cannot know the exact object
     length without a header, but its size in words must be some multiple of
     JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS .  Instead of handling only one word
     it is more efficient to handle JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS at
     once, and avoid the previous checks in this function on every word but the
     first.  If the object continues past JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS
     words from this point then more checks will be needed later, but by this
     optimisation we will have still avoided at least half of them. */
#if defined (JITTER_GC_DEBUG)
  /* If debugging just handle one word.  This allows for consistency checks in
     every word after the first one as well. */
  jitter_gc_log ("    %p %s: headerless boxed object word:\n", untagged_pointer, space_name);
  jitter_gc_handle_word (a, (jitter_gc_tagged_object *) untagged_pointer);
  s->scan_pointer += sizeof (jitter_gc_tagged_object);
#else
  /* When not debugging handle JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS words
     instead of just one, as per the long comment above. */
  int i;
  for (i = 0; i < JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS; i ++)
    {
      jitter_gc_log ("    %p %s: headerless boxed object word %i of %i:\n", untagged_pointer + i, space_name, i + 1, (int) JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS);
      jitter_gc_handle_word (a,
                             (jitter_gc_tagged_object *) untagged_pointer + i);
    }
  s->scan_pointer += (JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS
                      * sizeof (jitter_gc_tagged_object));
#endif
}

void
jitter_gc_handle_root_pointer (struct jitter_gc_heaplet *a,
                               jitter_gc_tagged_object *p)
{
  jitter_gc_log ("    %p root word:\n", p);
#if defined (JITTER_GC_DEBUG)
  /* Check if the root is already known.  If not store it in the table, so that
     we may recognize it if we meet it again in this same collection. */
  bool already_known;
  JITTER_WORD_SET_SET_HAS (& a->unique_root_table, (jitter_uint) p,
                           already_known);
  if (already_known)
    {
      // FIXME FIXME FIXME (@mnabipoor)
      return;
      jitter_fatal ("root %p registered more than once", p);
    }
  else
    JITTER_WORD_SET_ADD_NEW (& a->unique_root_table, (jitter_uint) p);
#endif // #if defined (JITTER_GC_DEBUG)
  jitter_gc_handle_word (a, p);
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->total_root_size_in_bytes_for_this_collection
    += sizeof (jitter_gc_tagged_object);
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
}

/* Handle the pointed root in the pointed heaplet, migrating boxed objects to
   tospace.  This is a helper for jitter_gc_handle_roots , which handles every
   global and temporary root. */
void
jitter_gc_handle_root (struct jitter_gc_heaplet *a, struct jitter_gc_root *r)
{
  jitter_gc_tagged_object *p = (jitter_gc_tagged_object *) r->buffer;
  jitter_gc_tagged_object *p_limit
    = ((jitter_gc_tagged_object *)
       ((char *) p + r->buffer_size_in_bytes));
  for (/* Nothing. */; p < p_limit; p ++)
    jitter_gc_handle_root_pointer (a, p);
}

/* Handle global roots in the pointed heaplet, migrating boxed objects to
   tospace.  This is executed at the beginning of jitter_gc_collect
   when handling roots. */
static void
jitter_gc_handle_global_roots (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_global_root *gr;
  for (gr = a->global_roots.first; gr != NULL; gr = gr->links.next)
    jitter_gc_handle_root (a, & gr->root);
}

/* Handle temporary roots in the pointed heaplet using the pointed set,
   migrating boxed objects to tospace.  This is executed at the beginning of
   jitter_gc_collect when handling roots. */
static void
jitter_gc_handle_temporary_roots (struct jitter_gc_heaplet *a,
                                  jitter_gc_temporary_root_set *rs)
{
  struct jitter_gc_root *temporary_roots
    = ((struct jitter_gc_root*)
       jitter_dynamic_buffer_to_pointer (rs));
  struct jitter_gc_root *r_limit
    = ((struct jitter_gc_root*)
       jitter_dynamic_buffer_first_unused_char (rs));
  struct jitter_gc_root *r;
  for (r = temporary_roots; r < r_limit; r ++)
    jitter_gc_handle_root (a, r);
}

/* Handle inter-generational roots from the remembered set, in the pointed
   heaplet.  Remove any uninteresting root from the remembered set in the
   process, to make this step faster at the next collection. */
//__attribute__ ((noinline, noclone, flatten)) // FIXME: for profiling only
static void
jitter_gc_handle_inter_generational_roots (struct jitter_gc_heaplet *a)
{
  /* If the remembered set is completely empty do not bother looking at each
     element.  There can be no deleted elements here, by construction. */
  if (a->remembered_set.used_element_no == 0)
    {
      jitter_gc_log ("    remembered set empty\n");
      return;
    }
  /* In the following I can assume that there is at least one entry. */

  const struct jitter_gc_shape_table *shape_table = a->shape_table;
  const struct jitter_gc_shape *shapes
    = JITTER_DYNAMIC_BUFFER_TO_CONST_VOID_POINTER (& shape_table->shapes);
  const struct jitter_gc_shape *shape_limit
    = JITTER_DYNAMIC_BUFFER_FIRST_UNUSED_CHAR_CONST (& shape_table->shapes);

  /* Check every entry in the remembered set, sequentially. */
  // FIXME: add a pointer-set function to cleanly access the buffer.
  jitter_gc_tagged_object *roots =
    (jitter_gc_tagged_object *) a->remembered_set.buffer;
  jitter_gc_tagged_object *roots_limit
    = roots + a->remembered_set.allocated_element_no;
  jitter_gc_tagged_object *rootp;
  for (rootp = roots; rootp < roots_limit; rootp ++)
    {
      /* Ignore the pointer-set element if it is an unused element.  It cannot
         be a deleted element: no such elements should ever be visible out of
         this function.*/
      jitter_gc_tagged_object root = * rootp;

#if defined (JITTER_GC_DEBUG)
      /* Notice the comment below about clearing the pointer-set for the
         rationale. */
      if ((jitter_uint) root == JITTER_WORD_SET_DELETED)
        jitter_fatal ("the remembered set contains deleted entries: something "
                      "is wrong");
#endif // #if defined (JITTER_GC_DEBUG)
      if (! JITTER_WORD_SET_IS_VALID ((jitter_uint) root))
        continue;

#if defined (JITTER_GC_DEBUG)
      if (shape_table->is_unboxed (root))
        jitter_fatal ("the remembered set contains the unboxed object %p",
                      (void *) root);
      if (JITTER_GC_BOXED_TO_BLOCK (root)->generation
          != jitter_gc_generation_old)
        jitter_fatal ("the remembered set contains the non-oldspace object %p",
                      (void *) root);
#endif // #if defined (JITTER_GC_DEBUG)
      jitter_gc_tagged_object *untagged_initial_pointer
        = ((jitter_gc_tagged_object *)
           (root & JITTER_GC_BOXED_NON_TAG_BIT_MASK));

      /* If we arrived at this point then root contains an address that was
         registered by the write barrier, not in youngspace. */
      const struct jitter_gc_shape *shape;
      bool found = false;
      for (shape = shapes; shape < shape_limit; shape ++)
        if (shape->object_has_shape (root))
          {
            /* Update the object fields, of course without copying the object.
               Old fields will be ignored, which is what we want. */
            jitter_gc_log ("    Update fields for remembered-set %s at %p %s\n",
                           shape->name, untagged_initial_pointer,
                           jitter_gc_space_name_for (a,
                                                     untagged_initial_pointer));
            if (shape->object_update_fields != NULL)
              shape->object_update_fields (a, untagged_initial_pointer);
            else
              {
                int i;
                for (i = 0; i < JITTER_GC_MINIMUM_OBJECT_SIZE_IN_WORDS; i ++)
                  jitter_gc_handle_word (a,
                                         ((jitter_gc_tagged_object *)
                                          untagged_initial_pointer) + i);
              }

            /* Do not check the following shapes. */
            found = true;
            break;
          }
      if (! found)
        jitter_fatal ("invalid shape for cross-generational object %p",
                      untagged_initial_pointer);
    }
}

/* Rebuild the set of inter-generational roots.  This is called at the end of a
   major or global collection after not using inter-generational roots as roots:
   some of the former inter-generational roots will be dead, others will be
   moved. */
static void
jitter_gc_update_inter_generational_roots (struct jitter_gc_heaplet *a)
{
  jitter_gc_log ("  Update inter-generational roots.\n");

  /* Easy special case: when the remembered set was empty to beging with we
     have nothing to go. */
  if (a->remembered_set.used_element_no == 0)
    {
      jitter_gc_log ("    Remembered set empty: nothing to do\n");
      return;
    }

  /* Keep a copy of the old remembered set; we will delete it at the end of this
     function.  Make a new remembered set, initially empty, and add it to the
     heaplet in the old's place. */
  struct jitter_word_set old_set = a->remembered_set;
  jitter_word_set_initialize (& a->remembered_set);

  /* Check every entry in the old remembered set, sequentially... */
  jitter_gc_log ("    Building a new remembered set from %li old "
                 "inter-generational roots:\n", (long) old_set.used_element_no);
  jitter_gc_tagged_object broken_heart_type_code
    = a->shape_table->broken_heart_type_code;
  jitter_gc_tagged_object *old_roots =
    (jitter_gc_tagged_object *) old_set.buffer;
  jitter_gc_tagged_object *old_roots_limit
    = old_roots + old_set.allocated_element_no;
  jitter_gc_tagged_object *old_rootp;
  for (old_rootp = old_roots; old_rootp < old_roots_limit; old_rootp ++)
    {
      jitter_gc_tagged_object old_root = * old_rootp;

      /* If the hash table has no element in this slot go on. */
#if defined (JITTER_GC_DEBUG)
      if ((jitter_uint) old_root == JITTER_WORD_SET_DELETED)
        jitter_fatal ("deleted entry in remembered set");
#endif // #if defined (JITTER_GC_DEBUG)
      if (! JITTER_WORD_SET_IS_VALID ((jitter_uint) old_root))
        continue;

      /* If we arrived here then old_root was an inter-generational root stored
         in the old remembered set.  The object is now either dead or moved. */
#if defined (JITTER_GC_DEBUG)
      if (a->shape_table->is_unboxed (old_root))
        jitter_fatal ("the old remembered set contains the unboxed object %p",
                      (void *) old_root);
      if (JITTER_GC_BOXED_TO_BLOCK (old_root)->generation
          != jitter_gc_generation_old)
        jitter_fatal ("the old remembered set contains the non-oldspace object "
                      "%p",
                      (void *) old_root);
#endif // #if defined (JITTER_GC_DEBUG)
      jitter_gc_tagged_object *old_untagged_initial_pointer
        = ((jitter_gc_tagged_object *)
           (old_root & JITTER_GC_BOXED_NON_TAG_BIT_MASK));
      jitter_gc_tagged_object old_first_word = * old_untagged_initial_pointer;
      if (old_first_word != broken_heart_type_code)
        jitter_gc_log ("      Former inter-generational root %p is now dead\n",
                       (void *) old_root);
      else
        {
          /* The old root has been overwritten with a broken heart: it means
             that the object is still alive.  The word following the broken
             heart type code contains the new tagged pointer, which we need to
             keep as an inter-generational root in the new remembered set. */
          jitter_gc_tagged_object new_root = old_untagged_initial_pointer [1];
          jitter_gc_log ("      Former inter-generational root %p %s is now "
                         "%p %s\n",
                         (void *) old_root,
                         jitter_gc_space_name_for (a, (void *) old_root),
                         (void *) new_root,
                         jitter_gc_space_name_for (a, (void *) new_root));
          jitter_gc_add_to_remembered_set (a, new_root, 8);
        }
    }
  jitter_gc_log ("    The new remembered set has %li "
                 "inter-generational roots\n",
                 (long) a->remembered_set.used_element_no);

  /* Delete the old remembered set. */
  jitter_word_set_finalize (& old_set);
}

/* Start root handling.  This only has effect when debugging; the purpose is
   initialising data structures to verify that no root is registered more than
   once. */
static void
jitter_gc_handle_roots_begin (struct jitter_gc_heaplet *a)
{
#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->total_root_size_in_bytes_for_this_collection = 0;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)
}

/* End root handling.  See the jitter_gc_handle_roots_begin comment. */
static void
jitter_gc_handle_roots_end (struct jitter_gc_heaplet *a)
{
#if defined (JITTER_GC_DEBUG)
  //printf ("\n\n");
  //jitter_word_set_print (& a->unique_root_table);
  //jitter_word_set_print_statistics (& a->unique_root_table);
  /* Clear the root table: it will no longer be needed for this collection. */
  jitter_word_set_clear_and_minimize (& a->unique_root_table);
#endif // #if defined (JITTER_GC_DEBUG)
}

/* Reset the scan block and the scan pointer to the beginning of the currently
   unused part in every tospace.  This needs to be called near the beginning of
   a collection, after setting up the spaces but before handling any root: roots
   will be copied into some of the tospaces, and they must be scanned; therefore
   we need to scan starting from the point before they were allocated. */
static void
jitter_gc_reset_for_scanning (struct jitter_gc_heaplet *a)
{
  /* For every tospace set the scan pointer to the beginning of the currently
     unused part of the current allocation block.  It will be the left finger of
     Cheney's algorithm.
     Also invalidate the used_limit field to NULL in the first block, which is
     necessary for scavenging: see the comment at the beginning of the body of
     jitter_gc_scan_next .  This is not necessary for the other blocks following
     the first one, as the field will be set to NULL at block change time. */
  jitter_gc_log ("  Resetting scan blocks and scan pointers for tospaces\n");
  struct jitter_gc_space **sp;
  for (sp = a->tospaces; sp < a->tospaces_limit; sp ++)
    {
      struct jitter_gc_space *s = * sp;
#if defined (JITTER_GC_DEBUG)
      if (s->allocation_block == NULL)
        jitter_fatal ("cannot reset scan block on %s which has no allocation "
                      "block; this should never happen", s->name);
#endif // #if defined (JITTER_GC_DEBUG)
      jitter_gc_space_set_scan_block (s, s->allocation_block, true);
      s->allocation_block->used_limit = NULL;
    }
}

/* Execute a variant of Cheney's algorithm, scavenging every alive object into
   the appopriate tospace.  This must be called after the roots have been
   handled. */
static
//__attribute__ ((noinline, noclone, flatten))
//__attribute__ ((flatten))
/* FIXME: the flatten attribute actually seems to help.  Inlining
   jitter_gc_scan_next might be a good idea, and maybe even more inlining the
   calls to jitter_gc_handle_word within jitter_gc_scan_next . */
void
jitter_gc_scavenge (struct jitter_gc_heaplet *a)
{
  /* In Cheney's algorithm the scan pointer plays the role of the left finger,
     and the allocation pointer plays the role of the right finger; the classic
     Cheney algorithm terminates When the two fingers meet.  This variant is
     slightly more complicated because it involves one left and one right finger
     for each tospace; we keep iterating as long as any finger moves. */
  struct jitter_gc_space **tospaces_limit = a->tospaces_limit;
  bool finger_moved;
  do
    {
      /* Scan all the tospaces.  If we find an alive object in any of them we
         have to check them all again, as scanning one object can copy more
         objects into the other tospaces. */
      finger_moved = false;
      struct jitter_gc_space **sp;
      for (sp = a->tospaces; sp < tospaces_limit; sp ++)
        {
          /* This inner while loop serves to avoid repeatedly scanning empty
             spaces and possibly make locality better by scanning the same
             tospace repeatedly until no more work remains to do on it.
             If we were to ignore performance considerations then an if
             conditional would have been just as correct as this while loop. */
          struct jitter_gc_space *s = * sp;
          while (s->scan_pointer != s->allocation_pointer)
            {
              jitter_gc_scan_next (a, s);
              finger_moved = true;
            }
        }
    }
  while (finger_moved);
}

/* This is a helper for jitter_gc_collect , handling the roots as appropriate
   for the collection kind. */
static void
jitter_gc_handle_roots (struct jitter_gc_heaplet *a,
                        enum jitter_gc_collection_kind kind)
{
#if defined (JITTER_GC_ENABLE_SHARING)
  /* If this is a share collection kind instead of an actual collection do not
     handle roots at all; do not even run the hooks. */
  if (kind == jitter_gc_collection_kind_share)
    {
      jitter_gc_log ("  Handle \"roots\" for sharing: only handle objects "
                     "being shared\n");
      jitter_gc_handle_roots_begin (a); /* Initialise statistics. */
      jitter_gc_handle_temporary_roots (a, & a->objects_being_shared);
      jitter_gc_handle_roots_end (a); /* Clear a->unique_root_table on debug. */
      return;
    }
#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */

  /* Run pre-collection hooks.  This is the first time we may handle roots in
     this collection. */
  jitter_gc_handle_roots_begin (a);
  jitter_gc_log ("  Run pre-collection hooks:\n");
  jitter_gc_hooks_run (a, & a->pre_collection_hooks, kind);

  /* Handle the roots (global and temporary) which were not discovered by
     pre-collection hooks. */
  jitter_gc_log ("  Handle global roots:\n");
  jitter_gc_handle_global_roots (a);
  jitter_gc_log ("  Handle temporary roots:\n");
  jitter_gc_handle_temporary_roots (a, & a->temporary_root_set);

  /* Scan inter-generational roots if appropriate for this collection kind. */
  switch (kind)
    {
    case jitter_gc_collection_kind_minor:
      jitter_gc_log ("  Handling inter-generational roots:\n");
      jitter_gc_handle_inter_generational_roots (a);
      break;
    case jitter_gc_collection_kind_major:
    case jitter_gc_collection_kind_global:
      jitter_gc_log ("  Not a minor collection: not scanning from "
                     "inter-generation roots (we will update them later "
                     "instead).\n");
      break;
    case jitter_gc_collection_kind_ssb_flush:
      jitter_fatal ("invalid collection kind SSB flush: not actually a "
                    "collection kind");
    default:
      jitter_fatal ("invalid collection kind %s %i",
                    jitter_gc_collection_kind_to_string (kind), (int) kind);
    }

  /* We have finished handling roots for this collection. */
  jitter_gc_handle_roots_end (a);
}


/* Perform exactly one garbage collection of the given kind. */
static void
jitter_gc_collect (struct jitter_gc_heaplet *a,
                     enum jitter_gc_collection_kind kind)
{
  JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_OWNED (a);

  /* Measure the time at the moment when this collections starts.  Even if we
     have not called jitter_gc_collect_0 yet we start measuring the time from
     this point, since in this function we may need to resize some space, a
     relatively expensive operation. */
  jitter_time_set_now (a->collection_start_time);
  jitter_gc_log ("Collection %lu (%s):\n", (unsigned long) a->collection_no, jitter_gc_collection_kind_to_string (kind));

  /* Measure how many bytes are used at the beginning, for statistics. */
  size_t initial_nursery_used_byte_no
    = jitter_gc_space_used_size_in_bytes (& a->nursery);
  size_t initial_steps_used_byte_no = 0;
  int i;
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    initial_steps_used_byte_no
      += jitter_gc_space_used_size_in_bytes (a->young_ageing_spaces [i]);
  size_t initial_oldspace_used_byte_no
    = jitter_gc_space_used_size_in_bytes (a->oldspace);

  /* Define which spaces are tospaces and the association between each origin
     space and its destination space for this collection.  Also define which
     spaces will need cleaning at the beginning or the end of a collection,
     all according to the collection kind. */
  jitter_gc_set_up_spaces (a, kind);

#if defined (JITTER_GC_LOG)
  if (! jitter_gc_log_muted)
    jitter_gc_heaplet_dump (a);
#endif // #if defined (JITTER_GC_LOG)
//printf ("\n"); jitter_gc_heaplet_dump (a); printf ("\n");

  /* Clean the spaces which are not in use in this collection, so that we may
     reuse their blocks for tospaces. */
  jitter_gc_clean_spaces_before (a);

  /* Reset the scan block and scan pointer for every tospace.  We need to do
     this before handling any roots, including those managed by pre-collection
     hooks. */
  jitter_gc_reset_for_scanning (a);

  /* Join finalisable objects in every fromspace into a single list within the
     heaplet.  We need to do this before handling any object, which if
     finalisable and found to be alive will be moved from this joined list
     into a list of (at collection time) alive objects. */
  jitter_gc_join_fromspace_finalizables_into_candidate_dead (a);

  /* Save the remembered set size; we will alter it later, in some collection
     kinds. */
  size_t initial_remembered_set_size = a->remembered_set.used_element_no;

  /* Handle the roots, as appropriate for this kind of collection.*/
  jitter_gc_handle_roots (a, kind);

  /* Now the roots are updated and the heap objects they were directly pointing
     have been moved to tospace.  Scavange the remaining reachable objects,
     from fromspaces to tospaces. */
  jitter_gc_log ("  Scavenge:\n");
  jitter_gc_scavenge (a);

  /* Finalise unreachable objects.  If some shape requires complete-object
     finalisation then this requires another scavenge phase, even if typically
     much shorter than the main scavenge above.
     As a special case, we do not finalise any object as part of a share
     operation: share is special in that:
     - it leaves alive objects, and also broken hearts, in fromspaces
     - it does not prove unreachability; in fact it even ignores roots.
     So no object dies during the share operation itself. */
  if (kind == jitter_gc_collection_kind_share)
      jitter_gc_log ("  Not finalising any object for collection kind share.  [FIXME: On sharing we do not finalise, but we might need to do something to finalisation lists to make them consistent the way they are at the end.  We have moved some objects to shared-own's mutation_time_finalizables , and that is good; however now we have to restore the space's original lists...  Which will be messy, because we currently destroy this information.  We should not destroy it, and instead add a conditional in JITTER_GC_FINALIZABLE_COPY or possibly even change the logic to have finalisation lists only in spaces.]");
  else
    while (jitter_gc_handle_object_finalization (a))
      jitter_gc_log ("  Not finished finalising: run another round\n");

  /* Update inter-generational roots if appropriate for this collection kind. */
  switch (kind)
    {
    case jitter_gc_collection_kind_minor:
    case jitter_gc_collection_kind_share:
      jitter_gc_log ("  Not updating inter-generational roots for collection "
                     "kind %s\n", jitter_gc_collection_kind_to_string (kind));
      break;
    case jitter_gc_collection_kind_major:
    case jitter_gc_collection_kind_global:
      jitter_gc_update_inter_generational_roots (a);
      break;
    case jitter_gc_collection_kind_ssb_flush:
      jitter_fatal ("invalid collection kind SSB flush: not actually a "
                    "collection kind");
    default:
      jitter_fatal ("invalid collection kind %s %i",
                    jitter_gc_collection_kind_to_string (kind), (int) kind);
    }

  /* Measure how many bytes are used at the end, for statistics.  The space
     names are the same as the beginning of the collection: this measurement
     is taken before flipping. */
  size_t final_steps_used_byte_no = 0;
  for (i = 0; i < JITTER_GC_NON_NURSERY_STEP_NO; i ++)
    final_steps_used_byte_no
      += jitter_gc_space_used_size_in_bytes (a->young_reserves [i]);
  size_t final_oldspace_used_byte_no
    = jitter_gc_space_used_size_in_bytes (a->oldspace);
  size_t final_old_reserve_used_byte_no
    = jitter_gc_space_used_size_in_bytes (a->old_reserve);
  size_t copied_to_oldspace_byte_no
    = final_oldspace_used_byte_no - initial_oldspace_used_byte_no;
  size_t survived_from_nursery_byte_no;
  if (JITTER_GC_NON_NURSERY_STEP_NO == 0)
    survived_from_nursery_byte_no = copied_to_oldspace_byte_no;
  else
    survived_from_nursery_byte_no
      = jitter_gc_space_used_size_in_bytes (a->young_reserves [0]);

  /* Clean the spaces which are not in use at the end of this collection, so
     that we may reuse their blocks for the new fromspaces at mutation time. */
  jitter_gc_clean_spaces_after (a);

  /* We are done.  Flip fromspace and tospace.  This also resets the allocation
     pointer in the new tospace.  The new fromspace has the allocation pointer
     pointing right after the last copied object, which is what we want. */
  jitter_gc_heaplet_flip (a, kind);

  /* Update heuristics regulating space size limits. */
  jitter_gc_update_heuristics (a, kind);

  /* Run post-collection hooks for actual (non-share) collections. */
  if (kind != jitter_gc_collection_kind_share)
    {
      jitter_gc_log ("  Run post-collection hooks:\n");
      jitter_gc_hooks_run (a, & a->post_collection_hooks, kind);
    }

  /* Perform the post-collection periodic cleanup. */
  jitter_gc_post_collection_cleanup (a, kind);

  /* Update statistics. */
  a->collection_no ++;
  switch (kind)
    {
    case jitter_gc_collection_kind_minor:
      a->minor_collection_no ++; break;
    case jitter_gc_collection_kind_major:
      a->major_collection_no ++; break;
    case jitter_gc_collection_kind_share:
      a->share_no ++; break;
    default:
      jitter_fatal ("invalid collection kind %s %i",
                    jitter_gc_collection_kind_to_string (kind), (int) kind);
    }

#if defined (JITTER_GC_EXPENSIVE_STATISTICS)
  a->total_root_size_in_bytes
    += a->total_root_size_in_bytes_for_this_collection;
#endif // #if defined (JITTER_GC_EXPENSIVE_STATISTICS)

  size_t copied_byte_no /* silence a spurious GCC warning */ = 0;
  size_t final_used_byte_no /* silence a spurious GCC warning */ = 0;

  /* Update nursery size statistics, for any kind of collection. */
  a->total_nursery_used_size_in_bytes += initial_nursery_used_byte_no;

  /* Update statistics for minor collections, and set copied_byte_no which is
     used below. */
  if (kind == jitter_gc_collection_kind_minor)
    {
      size_t initial_non_old_byte_no
        = initial_nursery_used_byte_no + initial_steps_used_byte_no;
      size_t final_oldspace_new_byte_no
        = final_oldspace_used_byte_no - initial_oldspace_used_byte_no;

      final_used_byte_no
        = final_steps_used_byte_no + final_oldspace_used_byte_no;

      size_t copied_byte_no_minor
        = final_steps_used_byte_no + final_oldspace_new_byte_no;
      a->total_bytes_copied_minor += copied_byte_no_minor;
      copied_byte_no = copied_byte_no_minor;

      a->total_bytes_copied_to_oldspace += copied_to_oldspace_byte_no;

      /* It is debatable how the survival rate of an empty space should be
         defined.  I find it in practice more useful to define it as 0 rather
         than as 1 or some arbitrary number, since survival ratios are used by
         heuristics to resize the heap, and low survival rates indicate that the
         space is under-used; an empty space is under-used. */
      double minor_survival_ratio = 0.0;
      if (initial_non_old_byte_no != 0)
        minor_survival_ratio = ((double) copied_byte_no_minor
                                / initial_non_old_byte_no);
      a->total_minor_survival_ratio += minor_survival_ratio;

      double nursery_survival_ratio = 0.0;
      if (initial_nursery_used_byte_no != 0)
        nursery_survival_ratio
          = ((double) survived_from_nursery_byte_no
             / initial_nursery_used_byte_no);
      a->total_nursery_survival_ratio += nursery_survival_ratio;
      a->latest_nursery_survival_ratio_index ++;
      a->latest_nursery_survival_ratio_index %= JITTER_GC_SURVIVAL_RATIO_NO;
      a->latest_nursery_survival_ratios [a->latest_nursery_survival_ratio_index]
        = nursery_survival_ratio;
      // printf ("latest: %.3f\n", nursery_survival_ratio);
    }

  /* Update statistics for major collections, and set copied_byte_no which is
     used below. */
  if (kind == jitter_gc_collection_kind_major)
    {
      a->total_old_space_used_size_in_bytes += initial_oldspace_used_byte_no;

      final_used_byte_no = jitter_gc_space_used_size_in_bytes (a->old_reserve);

      size_t bytes_copied_major = final_old_reserve_used_byte_no;
      copied_byte_no = bytes_copied_major;
      a->total_bytes_copied_major += bytes_copied_major;

      double major_survival_ratio = 0.0;
      size_t initial_alive_byte_no
        = (initial_nursery_used_byte_no
           + initial_steps_used_byte_no
           + initial_oldspace_used_byte_no);
      if (initial_alive_byte_no != 0)
        major_survival_ratio
          = ((double) final_old_reserve_used_byte_no / initial_alive_byte_no);
      a->latest_major_survival_ratio_index ++;
      a->latest_major_survival_ratio_index %= JITTER_GC_SURVIVAL_RATIO_NO;
      a->latest_major_survival_ratios [a->latest_major_survival_ratio_index]
        = major_survival_ratio;
      a->total_major_survival_ratio += major_survival_ratio;
    }

  /* Update statistics for share. */
  if (kind == jitter_gc_collection_kind_share)
    {
      // ????
    }

  /* Update copied bytes statistics.  It is important that we know how to do
     this indepdendently from the collection kind. */
  a->total_bytes_copied += copied_byte_no;

  /* The size of what was allocated during a mutation phase is equal to the used
     size of fromspace at the end of the phase (which is the beginning of a
     collection) minus what was already alive in it at the beginning (which,
     therefore, survived from previous phases). */
  size_t initial_used_byte_no
    = (initial_nursery_used_byte_no + initial_steps_used_byte_no
       + initial_oldspace_used_byte_no);
  a->total_bytes_allocated +=
    (initial_used_byte_no - a->used_bytes_at_the_beginning_of_this_phase);
  a->used_bytes_at_the_beginning_of_this_phase = final_used_byte_no;

  size_t final_remembered_set_size = a->remembered_set.used_element_no;
  a->total_initial_remembered_set_size += initial_remembered_set_size;
  a->total_final_remembered_set_size += final_remembered_set_size;

  /* Now the collection is over.  Measure the time at this point. */
  double elapsed_time = jitter_time_subtract_from_now (a->collection_start_time);
  a->total_collection_time += elapsed_time;
  switch (kind)
    {
    case jitter_gc_collection_kind_minor:
      a->total_minor_collection_time += elapsed_time; break;
    case jitter_gc_collection_kind_major:
      a->total_major_collection_time += elapsed_time; break;
    case jitter_gc_collection_kind_share:
      a->total_share_time += elapsed_time; break;
    default:
      jitter_fatal ("invalid collection kind %s %i",
                    jitter_gc_collection_kind_to_string (kind), (int) kind);
    }
  jitter_gc_log ("End of collection %lu (%s):\n",
                 (unsigned long) a->collection_no - 1,
                 jitter_gc_collection_kind_to_string (kind));
#if defined (JITTER_GC_LOG)
  if (! jitter_gc_log_muted)
    jitter_gc_heaplet_dump (a);
#endif // #if defined (JITTER_GC_LOG)
}




/* Collection heuristics.
 * ************************************************************************** */

/* Eastimate survival at the next collection based on recent history.  This
   is meant to be used on the fields latest_nursery_survival_ratios and
   latest_major_survival_ratios array fields in struct jitter_gc_heaplet,
   and the index is supposed to be latest_nursery_survival_ratio_index or
   latest_major_survival_ratio_index.  Notice that the index is incremented
   (in modulo) before each update, so that the array at the index holds the
   most recent value.
   This factors the common logic in jitter_gc_nursery_survival_estimate and
   jitter_gc_major_survival_estimate . */
static double
jitter_gc_survival_estimate (double *latest_survival_ratios,
                             unsigned latest_survival_ratio_index)
{
  /* Compute a weigthed average, unbalanced towards the most recent element. */
  double average = 0;
  double remaining_weight = 1.0;
  unsigned i;
  for (i = 0; i < JITTER_GC_SURVIVAL_RATIO_NO; i ++)
    {
      unsigned array_index
        = ((latest_survival_ratio_index + JITTER_GC_SURVIVAL_RATIO_NO - i)
           % (unsigned) JITTER_GC_SURVIVAL_RATIO_NO);
      double survival = latest_survival_ratios [array_index];
      /* Make the weight decrease as we get further away from the most recent
         value, at i == 0.  Of course the sum of all the weights is 1. */
      double weight;
      if (i != JITTER_GC_SURVIVAL_RATIO_NO - 1)
        {
          weight = remaining_weight * JITTER_GC_RECENT_BIAS;
          remaining_weight -= weight;
        }
      else
        weight = remaining_weight;
      average += weight * survival;
      //printf ("* survival: %.3f   weight: %.3f   i:%u array_index:%u\n", survival, weight, i, array_index);
    }
  //printf ("  average:  %.3f\n\n", average);
  return average;
}

/* Return an estimate of the nursery survival rate at the next minor
   collection for the pointed heaplet. */
static double
jitter_gc_nursery_survival_estimate (struct jitter_gc_heaplet *a)
{
  //printf ("Estimating nursery survival...\n");
  return jitter_gc_survival_estimate (a->latest_nursery_survival_ratios,
                                      a->latest_nursery_survival_ratio_index);
}

/* Return an estimate of the total rate at the next major collection for the
   pointed heaplet. */
static double
jitter_gc_major_survival_estimate (struct jitter_gc_heaplet *a)
{
  //printf ("Estimating major survival...\n");
  return jitter_gc_survival_estimate (a->latest_major_survival_ratios,
                                      a->latest_major_survival_ratio_index);
}

/* Return the larger of the two given numbers. */
static double
jitter_gc_max (double a, double b)
{
  if (a > b)
    return a;
  else
    return b;
}

/* Return the smaller of the two given numbers. */
static double
jitter_gc_min (double a, double b)
{
  if (a < b)
    return a;
  else
    return b;
}

/* Given an actual limit on the used size for a space, return a size threshold
   to be used in a strict check at block change time. */
  // Disabled: see the comment inside the body.
  /* /\* Since the check is run when a new block may be added or not, the threshold */
  /*    must count one block less than the desired size, to avoid passing the limit */
  /*    by one block.  Do not actually decrease the size if the limit is too small, */
  /*    to allow the growth ratio to take effect without being canceled by */
  /*    rounding. *\/ */
static size_t
jitter_gc_round_threshold (size_t threshold_in_bytes)
{
  size_t block_no
    = JITTER_QUOTIENT_CEILING (threshold_in_bytes,
                               JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES);
  /* printf ("%lu /^ %lu = %lu\n", */
  /*         (unsigned long) threshold_in_bytes, */
  /*         (unsigned long) JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES, */
  /*         (unsigned long) block_no); */

  // Summer 2022: I no longer understand why I did this, and it seems wrong.
  //              with this statement disabled now the nursery has the intended
  //              size.
  /* if (threshold_in_bytes > 2 * JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES) */
  /*   block_no --; */
  return block_no * JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES;
}

static void
jitter_gc_update_nursery_heuristics (struct jitter_gc_heaplet *a)
{
  double nursery_survival_ratio = jitter_gc_nursery_survival_estimate (a);
  double good_nursery_size
    = a->nursery_used_size_threshold_in_bytes;
  //printf ("nursery size: %3.3f:   survival ratio %.3f (low %.4f  --  high %.4f): \n", good_nursery_size, nursery_survival_ratio, a->low_nursery_survival_rate, a->high_nursery_survival_rate);
  jitter_gc_log ("Good nursery size is %3.3f:   survival ratio %.3f (low %.4f  --  high %.4f): ", good_nursery_size, nursery_survival_ratio, a->low_nursery_survival_rate, a->high_nursery_survival_rate);
  if (nursery_survival_ratio < a->low_nursery_survival_rate)
    {
      jitter_gc_log ("shrink nursery\n");
      good_nursery_size *= a->nursery_shrinkage_ratio;
    }
  else if (nursery_survival_ratio > a->high_nursery_survival_rate)
    {
      jitter_gc_log ("GROW nursery\n");
      good_nursery_size *= a->nursery_growth_ratio;
    }
  else
    jitter_gc_log ("goldilocks\n");

  /* Force the "good" computed size to fit within the current limits. */
  good_nursery_size = jitter_gc_max (good_nursery_size,
                                     a->minimum_nursery_size_in_bytes);
  good_nursery_size = jitter_gc_min (good_nursery_size,
                                     a->maximum_nursery_size_in_bytes);

  a->nursery_used_size_threshold_in_bytes
    = jitter_gc_round_threshold (good_nursery_size);
}

static void
jitter_gc_update_major_heuristics (struct jitter_gc_heaplet *a)
{
  // FIXME: I am not using major_survival_ratio at all. //////////////////////////////////////
  double major_survival_ratio __attribute__ ((unused)) = jitter_gc_major_survival_estimate (a);
  size_t alive_bytes
    = jitter_gc_space_used_size_in_bytes (a->oldspace);
  double good_oldspace_size
    = (1 / a->target_major_survival_rate * alive_bytes);
  /* Correct the "good" computed size to fit withint the current limits. */
  good_oldspace_size = jitter_gc_max (good_oldspace_size,
                                      a->minimum_oldspace_size_in_bytes);
  /* Set a maximum limit to the "good" size to prevent the heap to grow to
     an extreme size despite any survival-rate target; but... */
  good_oldspace_size = jitter_gc_min (good_oldspace_size,
                                      a->maximum_oldspace_size_in_bytes);
  /* ...But then immediately amend it that so that the space is at least large
     enough to contain every piece of currently alive data. */
  good_oldspace_size = jitter_gc_max (good_oldspace_size, alive_bytes);

  a->oldspace_used_size_threshold_in_bytes
    = jitter_gc_round_threshold (good_oldspace_size);
}

static void
jitter_gc_update_share_heuristics (struct jitter_gc_heaplet *a)
{
  /* Do nothing, at least for now. */
}

static void
jitter_gc_update_heuristics (struct jitter_gc_heaplet *a,
                             enum jitter_gc_collection_kind k)
{
  switch (k)
    {
    case jitter_gc_collection_kind_minor:
      jitter_gc_update_nursery_heuristics (a);
      break;
    case jitter_gc_collection_kind_major:
      jitter_gc_update_major_heuristics (a);
      break;
    case jitter_gc_collection_kind_share:
      jitter_gc_update_share_heuristics (a);
      break;
    default:
      jitter_fatal ("invalid collection kind %s %i",
                    jitter_gc_collection_kind_to_string (k), (int) k);
    }
}

/* Return non-false iff according to our heuristics it is time to perform a
   minor collect.
   This is always checked after jitter_gc_should_collect_major , as major
   collections are more urgent and they also collect young objects anyway. */
static int /* bool */
jitter_gc_should_collect_minor (struct jitter_gc_heaplet *a)
{
  /*
  printf ("(minor) used:%f   threshold:%f\n",
          (double) jitter_gc_space_used_size_in_bytes (& a->nursery),
          (double) a->nursery_used_size_threshold_in_bytes);
  */
  return (jitter_gc_space_used_size_in_bytes (& a->nursery)
          >= a->nursery_used_size_threshold_in_bytes);
}

/* Return non-false iff according to our heuristics it is time to perform a
   major collect.
   This is always checked before jitter_gc_should_collect_minor , as major
   collections are more urgent and they also collect young objects anyway. */
static int /* bool */
jitter_gc_should_collect_major (struct jitter_gc_heaplet *a)
{
  /*
  printf ("[MAJOR] used:%f   threshold:%f\n",
          (double) jitter_gc_space_used_size_in_bytes (a->oldspace),
          (double) a->oldspace_used_size_threshold_in_bytes);
  */
  /* Never perform a major collection if there is still usable young space; when
     the space is very tight and a major collection is needed we should not make
     the situation worse by doing major collections when we can still use some
     existing young space. */
  if (! jitter_gc_should_collect_minor (a))
    return false;

  return (jitter_gc_space_used_size_in_bytes (a->oldspace)
          >= a->oldspace_used_size_threshold_in_bytes);
}




/* Collection disabling.
 * ************************************************************************** */

void
jitter_gc_enable_collection (struct jitter_gc_heaplet *a)
{
  jitter_gc_log ("Enabling collection (collection was %s)\n",
                 (a->collection_enabled
                  ? "already enabled"
                  : "disabled up to this point"));
  a->collection_enabled = true;
}

void
jitter_gc_disable_collection (struct jitter_gc_heaplet *a)
{
  jitter_gc_log ("Disabling collection (collection was %s)\n",
                 (a->collection_enabled
                  ? "enabled up to this point"
                  : "alraedy disabled"));
  a->collection_enabled = false;
}

/* Return non-false iff the given action does necessarily require a collection,
   and is forbidden when collection is disabled.  Notice that "sharing" is
   forbidden, because sharing some data does necessarily require a collection;
   but "default" is not forbidden, because the action can be accomplished by
   changing block. */
static int /*bool*/
jitter_gc_is_mandatory_collection_action (enum jitter_gc_action action)
{
  switch (action)
    {
    case jitter_gc_action_default:
    case jitter_gc_action_block_change:
      return false;

    case jitter_gc_action_force_minor:
    case jitter_gc_action_force_major:
    case jitter_gc_action_force_either:
    case jitter_gc_action_force_global:
    case jitter_gc_action_share:
      return true;

    default:
      jitter_fatal ("invalid action %i %s", (int) action,
                    jitter_gc_action_to_string (action));
    }
}

/* If collection is currently disabled in the pointed heap fail fatally,
   displaying as part of the fatal error message the given text.  Do nothing
   otherwise. */
static void
jitter_gc_fail_if_collection_disabled (struct jitter_gc_heaplet *a,
                                       const char *context)
{
  if (! a->collection_enabled)
    jitter_fatal ("failure in %s: collection is disabled", context);
}

/* Like jitter_gc_fail_if_collection_disabled, but only fail if the given action
   is a mandatory collection action as definde in
   jitter_gc_is_mandatory_collection_action . */
static void
jitter_gc_fail_if_collection_disabled_on (struct jitter_gc_heaplet *a,
                                          enum jitter_gc_action action,
                                          const char *context)
{
  if (jitter_gc_is_mandatory_collection_action (action))
    jitter_gc_fail_if_collection_disabled (a, context);
}




/* Collection driver.
 * ************************************************************************** */

void
_jitter_gc_allocate_slow_path_with_runtime_fields_owned_and_ssb_flushed
   (struct jitter_gc_heaplet *a,
    size_t failed_allocation_size_in_bytes,
    enum jitter_gc_action action)
{
  JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_OWNED (a);
  jitter_gc_log ("Allocation slow path after failing to allocate %li B; "
                 "requested action is %s\n",
                 (long) failed_allocation_size_in_bytes,
                 jitter_gc_action_to_string (action));

  jitter_gc_fail_if_collection_disabled_on (a, action, "allocate slow path");

  /* Fail fatally if the request is impossible to satisfy even after a
     collection. */
  if (failed_allocation_size_in_bytes > JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES)
    jitter_fatal ("cannot allocate %li B with %li-B blocks",
                  (long) failed_allocation_size_in_bytes,
                  (long) JITTER_GC_BLOCK_PAYLOAD_SIZE_IN_BYTES);
#if defined (JITTER_GC_DEBUG)
  if (! jitter_gc_is_good_size_in_bytes (failed_allocation_size_in_bytes))
    jitter_fatal ("attempting to allocate %lu B which is not a multiple of "
                  "%li B (likely a mutator bug)",
                  (unsigned long) failed_allocation_size_in_bytes,
                  (long) JITTER_GC_MINIMUM_OBJECT_SIZE_IN_BYTES);
  jitter_gc_heap_pointer allocation_pointer
    = JITTER_GC_HEAPLET_ALLOCATION_POINTER (a);
  if (! jitter_gc_is_object_aligned_pointer (allocation_pointer))
    jitter_fatal ("allocation pointer %p not aligned", allocation_pointer);
  jitter_gc_heap_pointer allocation_limit
    = JITTER_GC_HEAPLET_ALLOCATION_LIMIT (a);
  if (! jitter_gc_is_word_aligned_pointer (allocation_limit))
    jitter_fatal ("allocation limit %p not aligned (did you forget to update "
                  "runtime fields?", allocation_limit);
#endif // #if defined (JITTER_GC_DEBUG)

  /* Decide what to do, making a choice now in case the requested action allows
     for multiple possibilities.  This statement either replaces action with a
     more specific action, or has no effect. */
  switch (action)
    {
    case jitter_gc_action_default:
      if (jitter_gc_should_collect_major (a))
        action = jitter_gc_action_force_major;
      else if (jitter_gc_should_collect_minor (a))
        action = jitter_gc_action_force_minor;
      else
        action = jitter_gc_action_block_change;
      break;

    case jitter_gc_action_force_either:
      if (jitter_gc_should_collect_major (a))
        action = jitter_gc_action_force_major;
      else
        action = jitter_gc_action_force_minor;
      break;

    default:
      /* Do nothing: the action is already determined. */;
    }
  jitter_gc_log ("  Action is %s\n",
                 jitter_gc_action_to_string (action));

  /* Change the action if we decided to collect but collection is disabled.
     Notice that, if we arrived here, an explicit collection was not requested
     so there is no need to fail: we can just procure a new block instead of
     collecting. */
  if (! a->collection_enabled
      && jitter_gc_is_mandatory_collection_action (action))
    {
      jitter_gc_log ("  Since collection is disabled we have to change:");
      action = jitter_gc_action_block_change;
      jitter_gc_log ("    Action is now %s\n",
                     jitter_gc_action_to_string (action));
    }

  /* Execute the action, which at this point is fixed. */
  switch (action)
    {
    case jitter_gc_action_block_change:
      jitter_gc_space_change_allocation_block (a, & a->nursery);
      break;

    case jitter_gc_action_force_minor:
      jitter_gc_collect (a, jitter_gc_collection_kind_minor);
      break;

    case jitter_gc_action_force_major:
      jitter_gc_collect (a, jitter_gc_collection_kind_major);
      break;

    case jitter_gc_action_share:
      jitter_fatal ("you should never use _jitter_gc_allocate_slow_path_with_runtime_fields_owned_and_ssb_flushed "
                    "for sharing");

    case jitter_gc_action_default:
    case jitter_gc_action_force_either:
      jitter_fatal ("undetermined action: this should never happen");

    default:
      jitter_fatal ("invalid action %s %i: this should never happen",
                    jitter_gc_action_to_string (action), (int) action);
    }

  /* At this point we can guarantee that the free space is sufficient, since the
     nursery now has at least one completely empty block and we have already
     checked that the object to be allocated is smaller than a block payload.
     The next allocation attempt will succeed. */

  jitter_gc_log ("Allocation slow path: done\n");
}




/* Sharing.
 * ************************************************************************** */

#if defined (JITTER_GC_ENABLE_SHARING)

/* Return non-false iff any space in the given generation has been scavenged
   from in the pointed heaplet, according to space scavenged_from flags. */
__attribute__ ((unused))
static int /*bool*/
jitter_gc_generation_scavenged_from (struct jitter_gc_heaplet *a,
                                     enum jitter_gc_generation g)
{
  struct jitter_gc_space **limit
    = a->generation_spaces_limit [JITTER_GC_GENERATION_INDEX_OFFSET + g];
  struct jitter_gc_space **sp;
  for (sp = a->generation_spaces [JITTER_GC_GENERATION_INDEX_OFFSET + g];
       sp < limit;
       sp ++)
    {
      struct jitter_gc_space *s = * sp;
      if (s->scavenged_from)
        return true;
    }
  return false;
}

/* Like _jitter_gc_share, but assume without checking that the object to be
   shared is a boxed object, either young or old but not already shared. */
static void
_jitter_gc_share_young_or_old (struct jitter_gc_heaplet *a,
                               jitter_gc_tagged_object *p)
{
bool old_jitter_gc_log_muted = jitter_gc_log_muted;  // jitter_gc_log_muted = false; //////////////////////////////////////////////////
//printf ("share young-or-old %p: beginning (old muted %s)\n", (void *) p, old_jitter_gc_log_muted ? "true" : "false");

  JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_OWNED (a);
#if defined (JITTER_GC_DEBUG)
  enum jitter_gc_generation generation
    = JITTER_GC_TAGGED_BOXED_TO_GENERATION (* p);
  if (generation != jitter_gc_generation_young
      && generation != jitter_gc_generation_old)
    jitter_fatal ("_jitter_gc_share_young_or_old: the object is not in fact "
                  "young or old, but is instead %s %i",
                  jitter_gc_generation_to_string (generation), (int) generation);
#endif // #if defined (JITTER_GC_DEBUG)

  /* Perform a share-kind collection, using as root the single object being
     shared. */
  jitter_gc_log ("share the object pointed by %p: %p\n", p, (void *) (* p));
  jitter_gc_temporary_root_set_push (& a->objects_being_shared,
                                     p,
                                     sizeof (jitter_gc_tagged_object));
  jitter_gc_collect (a, jitter_gc_collection_kind_share);
  jitter_gc_temporary_root_set_empty (& a->objects_being_shared);

  /* Now we have promoted p along with its reachable objects to shared-own; but
     we cannot let the mutator resume yet, since the young and / or old spaces
     now contain broken hearts; heap objects and even roots may point to
     formerly young or old objects which are now shared.
     We need another collection to solve this problem: according to which
     objects were moved, the collection will be minor or major. */
  if (jitter_gc_generation_scavenged_from (a, jitter_gc_generation_old))
    {
      jitter_gc_log ("When sharing we scavenged old objects: perform a major "
                     "collection to make sure no pointers to old addresses "
                     "for now-shared objects remain\n");
      jitter_gc_collect (a, jitter_gc_collection_kind_major);
    }
  else if (jitter_gc_generation_scavenged_from (a, jitter_gc_generation_young))
    {
      jitter_gc_log ("When sharing we scavenged young (but no old) objects: "
                     "perform a minor collection to make sure no pointers to "
                     "young addresses for now-shared objects remain\n");
      jitter_gc_collect (a, jitter_gc_collection_kind_minor);
    }
  else
    jitter_fatal ("impossible: no objects were scavenged after sharing a young "
                  "or old object");

jitter_gc_log_muted = old_jitter_gc_log_muted;  ////////////////////////////////////////////////// RESTORE VERBOSITY
// printf ("share young/old %p: end [restoring old muted %s]\n", (void *) p, old_jitter_gc_log_muted ? "true" : "false");
}

void
_jitter_gc_share (struct jitter_gc_heaplet *a, jitter_gc_tagged_object *p)
{
  JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_OWNED (a);
  jitter_gc_fail_if_collection_disabled_on (a, jitter_gc_action_share, "share");
#if defined (JITTER_GC_DEBUG)
  jitter_gc_tagged_object *p_ = p; /* Avoid a GCC warning about comparing with
                                      NULL: p is affected by attribute_nonnull
                                      but here, when debugging, we want to be
                                      safe even without GNU C attributes. */
  if (p_ == NULL)
    jitter_fatal ("_jitter_gc_share: pointer to object is NULL");
#endif
  /* Check for the fast path: if the object to be shared is not either young or
     old, then we do not need to do anything. */
  enum jitter_gc_generation generation
    = JITTER_GC_TAGGED_BOXED_TO_GENERATION (* p);
  if (generation != jitter_gc_generation_young
      && generation != jitter_gc_generation_old)
    return;

  /* Do the actual work. */
  _jitter_gc_share_young_or_old (a, p);
}

void
jitter_gc_share_barrier_slow_path (struct jitter_gc_heaplet *a,
                                   jitter_gc_tagged_object *updated_p,
                                   jitter_gc_tagged_object *new_pointed_p)
{
  /* If the heaplet does not own its runtime fields then something is very
     wrong. */
  JITTER_GC_DEBUG_ASSERT_RUNTIME_FIELDS_OWNED (a);

  /* It is (currently) forbidden to share when collection is disabled, since
     sharing some objects necessarily requires a collection. */
  jitter_gc_fail_if_collection_disabled_on (a, jitter_gc_action_share,
                                            "write barrier, writing to a "
                                            "shared object");

#if defined (JITTER_GC_DEBUG)
  if (updated_p == NULL || new_pointed_p == NULL)
    jitter_fatal ("share barrier: updated or pointed object is null");
  if (a->shape_table->is_unboxed (* updated_p))
    jitter_fatal ("share barrier: the updated object is unboxed");
  enum jitter_gc_generation updated_generation
    = JITTER_GC_TAGGED_BOXED_TO_GENERATION (* updated_p);
  if (updated_generation != jitter_gc_generation_shared)
    jitter_fatal ("share barrier: the updated object belongs to generation %i "
                  "instead of to the shared generation",
                  (int) updated_generation);
#endif // #if defined (JITTER_GC_DEBUG)

  /* Determine what the generation of the new pointed object is.  In some cases
     we might even not need to do anything. */
  enum jitter_gc_generation new_pointed_generation
    = JITTER_GC_TAGGED_BOXED_TO_GENERATION (* new_pointed_p);
  switch (new_pointed_generation)
    {
    case jitter_gc_generation_immortal:
    case jitter_gc_generation_shared:
      /* The new pointed object is in fact unboxed, or already shared: we
         do not need to do anything. */
      return;

#if defined (JITTER_GC_DEBUG)
    case jitter_gc_generation_young:
    case jitter_gc_generation_old:
#else /* ! defined (JITTER_GC_DEBUG) */
    default:
#endif /* #if defined (JITTER_GC_DEBUG) */
      /* We actually need the expensive operation. */
      break;

#if defined (JITTER_GC_DEBUG)
    default:
      jitter_fatal ("unexpected generation %s %i",
                    jitter_gc_generation_to_string (new_pointed_generation),
                    (int) new_pointed_generation);
#endif /* #if defined (JITTER_GC_DEBUG) */
    }

  /* If we arrived here then we actually need to make the new pointed object
     shared. */

  /* Protect as root the object to be modified. */
  JITTER_GC_BLOCK_BEGIN (a);
  JITTER_GC_BLOCK_ROOT (a, updated_p);

  /* Share the new pointed object. */
  _jitter_gc_share_young_or_old (a, new_pointed_p);

  /* We no longer need to keep the passed pointers as root.  However the caller
     will need to assume that the tagged object she pointed to when calling this
     function may now be changed. */
  JITTER_GC_BLOCK_END (a);
}

#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */




/* Global collection.
 * ************************************************************************** */

#if defined (JITTER_GC_ENABLE_SHARING)

/* Execute a global garbage collection if needed and if possible at this moment,
   also using the pointed heaplet (if non-NULL) controlled by the calling
   thread, which must be either jitter_gc_heaplet_used_state_in_use
   or jitter_gc_heaplet_used_state_not_to_be_woken_up .
   This must be called inside a critical section when the heap's lock has
   already been acquired.  By the time this function returns the lock will have
   been released. */
static void
jitter_gc_global_gc_if_needed_and_unlock (struct jitter_gc_heap *h,
                                          struct jitter_gc_heaplet *this_a)
{
  /* Sanity check. */
  if (this_a != NULL
      && this_a->used_state != jitter_gc_heaplet_used_state_in_use
      && this_a->used_state != jitter_gc_heaplet_used_state_not_to_be_woken_up)
    jitter_fatal ("heaplet in unexpected state %i instead of in-use or"
                  " not-to-be-woken-up", (int) this_a->used_state);

  switch (h->request)
    {
    case jitter_gc_request_no_request:
      /* No request is pending, so we have nothing to do or to check. */
      jitter_gc_heap_unlock (h);
      return;
    case jitter_gc_request_global_gc:
      /* Fall into the function body after the switch statement. */
      break;
    default:
      jitter_fatal ("unexpected request %i", (int) h->request);
    }
  /* If we arrived here h->request == jitter_gc_request_global_gc . */

  /* If this thread is controlling an in-use heaplet we should make it
     not-in-use now... */
  enum jitter_gc_heaplet_used_state original_state
    = jitter_gc_heaplet_used_state_invalid;
  if (this_a != NULL)
    {
      original_state = this_a->used_state;
      if (original_state == jitter_gc_heaplet_used_state_in_use)
        {
          jitter_gc_heaplet_unlink (& h->heaplets_in_use, this_a);
          jitter_gc_heaplet_link_last (& h->heaplets_not_in_use, this_a);
        }
      /* ...And check if it was the last heaplet in-use we were waiting for
         before collecting. */
      int /*bool*/ was_this_a_the_last = (h->heaplets_in_use.first == NULL);
      if (was_this_a_the_last)
        this_a->used_state = jitter_gc_heaplet_used_state_collecting;
      else
        {
          /* Make this heaplet available for collection and block until a global
             collection actually happens; but the actual collection will not
             happen in this thread, since we are still waiting for some other
             heaplet not to be in use any longer. */
          this_a->used_state = jitter_gc_heaplet_used_state_to_be_woken_up;
          printf ("[P] making %p available for collection and sleeping\n", this_a);
          jitter_gc_heap_unlock_and_heaplet_block (h, this_a);
          printf ("* %p has been awakened after global collection\n", this_a);
          return;
        }
    }

  /* If we arrived here we can collect: a global collection was requested and
     there are no in-use heaplets. */

  printf ("Start a GLOBAL COLLECTION from %p\n", this_a);

  struct jitter_gc_heaplet *some_a;
  int /*bool*/ at_least_one_heaplet __attribute__ ((unused)) = false;
  int heaplet_no = 0;
  // struct jitter_list_header heaplets_to_handle = h->heaplets_not_in_use;
  struct jitter_list_header heaplets_still_in_use_at_the_end;
  JITTER_LIST_INITIALIZE_HEADER (& heaplets_still_in_use_at_the_end);
  for (some_a = h->heaplets_not_in_use.first;
       some_a != NULL;
       some_a = some_a->links.next)
    {
      heaplet_no ++;
      at_least_one_heaplet = true;
      printf ("  * (pretend to) collect from heaplet %p\n", some_a);
    }

  /* After global-collecting perform another pass over not-in-use heaplets,
     resetting their states and waking them up as appropriate. */
  struct jitter_gc_heaplet *next_a;
  for (some_a = h->heaplets_not_in_use.first; some_a != NULL; some_a = next_a)
    {
      at_least_one_heaplet = true;
      next_a = some_a->links.next;
      switch (some_a->used_state)
        {
        case jitter_gc_heaplet_used_state_invalid:
          jitter_fatal ("impossible: uninitialised some_a->used_state");
        case jitter_gc_heaplet_used_state_in_use:
          jitter_fatal ("impossible: collecting from in-use heaplet");
        case jitter_gc_heaplet_used_state_to_be_woken_up:
          jitter_gc_heaplet_unlink (& h->heaplets_not_in_use, some_a);
          jitter_gc_heaplet_link_last (& h->heaplets_in_use, some_a);
          some_a->used_state = jitter_gc_heaplet_used_state_in_use;
          printf ("    [V] waking up %p\n", some_a);
          jitter_gc_heaplet_wake_up (some_a);
          break;
        case jitter_gc_heaplet_used_state_not_to_be_woken_up:
          /* Nothing to do. */
          break;
        case jitter_gc_heaplet_used_state_collecting:
          if (original_state == jitter_gc_heaplet_used_state_in_use)
            {
              jitter_gc_heaplet_unlink (& h->heaplets_not_in_use, some_a);
              jitter_gc_heaplet_link_last (& h->heaplets_in_use, some_a);
            }
          /* else not-to-be-woken-up: nothing to do. */
          some_a->used_state = original_state;
          break;
        default:
          jitter_fatal ("invalid used_state %i", (int) some_a->used_state);
        }
    }

  /* Now that the global collection has been performed there is no pending
     request any more. */
  h->request = jitter_gc_request_no_request;

  printf ("  global collection over %i heaplets ended (run from %p)\n", heaplet_no, this_a);

  /* We can finally unlock.  This is the end of the long critical section
     started out of this function, before the global collection began. */
  jitter_gc_heap_unlock (h);
}

void
jitter_gc_request_global_collection (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_heap *h = a->heap;

  printf ("Requesting a global collection from heaplet %p\n", a);
  jitter_gc_heap_lock (h);
  h->request = jitter_gc_request_global_gc;
  jitter_gc_global_gc_if_needed_and_unlock (h, a);
}

void
jitter_gc_before_blocking (struct jitter_gc_heaplet *a)
{
  if (a->used_state != jitter_gc_heaplet_used_state_in_use)
    jitter_fatal ("heaplet not in use before blocking");

  struct jitter_gc_heap *h = a->heap;
  jitter_gc_heap_lock (h);
  jitter_gc_heaplet_unlink (& h->heaplets_in_use, a);
  jitter_gc_heaplet_link_last (& h->heaplets_not_in_use, a);
  a->used_state = jitter_gc_heaplet_used_state_not_to_be_woken_up;
  jitter_gc_global_gc_if_needed_and_unlock (h, a);
}

void
jitter_gc_after_blocking (struct jitter_gc_heaplet *a)
{
  if (a->used_state == jitter_gc_heaplet_used_state_in_use)
    jitter_fatal ("heaplet already in use after blocking");
  else if (a->used_state != jitter_gc_heaplet_used_state_not_to_be_woken_up)
    jitter_fatal ("heaplet in unexpected state %i after blocking, instead of the"
                  " expected jitter_gc_heaplet_used_state_not_to_be_woken_up",
                  (int) a->used_state);

  struct jitter_gc_heap *h = a->heap;
  jitter_gc_heap_lock (h);
  jitter_gc_heaplet_unlink (& h->heaplets_not_in_use, a);
  jitter_gc_heaplet_link_last (& h->heaplets_in_use, a);
  a->used_state = jitter_gc_heaplet_used_state_in_use;
  jitter_gc_global_gc_if_needed_and_unlock (h, a);
}

void
jitter_gc_global_collection_safe_point_slow_path (struct jitter_gc_heaplet *a)
{
  struct jitter_gc_heap *h = a->heap;
  jitter_gc_heap_lock (h);
  jitter_gc_global_gc_if_needed_and_unlock (h, a);
}

#endif /* #if defined (JITTER_GC_ENABLE_SHARING) */




/* Scratch.
 * ************************************************************************** */

/* Something like this might in fact be useful, but not done this way. */
bool
jitter_gc_log_muted = false;
