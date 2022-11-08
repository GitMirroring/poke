/* Jitter: profiling subsystem: header.

   Copyright (C) 2020 Luca Saiu
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


#ifndef JITTER_PROFILE_H_
#define JITTER_PROFILE_H_


#include <jitter/jitter.h>
#include <jitter/jitter-print.h>
#include <jitter/jitter-vm.h>


/* Introduction.
 * ************************************************************************** */

/* At the current time this subsystem implements a counting profile of VM
   instructions, by instrumentation.  This style of profiling is exact and
   deterministic but disruptive for performance and therefore not enabled by
   default.
   In the case of a VM not instrumented for profiling this API remains available
   for compatibility, but the functions do nothing useful.

   The functions in this header are independent from the VM, and serve to factor
   the common logic.  The user should use the equivalent VM-specific
   functionality from generated code, which do not require a VM pointer. */


/* The period in milliseconds between two samplings.  This includes both user
   and system time.
   The GNU libc manual warns that the profiling timer is low-resolution, and
   that seems correct.  Setting a sampling period too much below 5ms results in
   signals not arriving fast enough to keep the pace.  GProf has traditionally
   used 10ms. */
#define JITTER_PROFILE_SAMPLE_PERIOD_IN_MILLISECONDS 10




/* Runtime data structures.
 * ************************************************************************** */

/* Each VM state contains a runtime data structure which is efficient to update,
   and that can be merged with other structures of the same kind -- see struct
   jitter_profile_runtime in templates/vm.h .
   The user should treat that data structure as abstract, and either print its
   content by calling the functions below, or convert it into struct
   jitter_profile (see below) to analyse herself. */

/* These state-local structures are kept within the special-purpose struct
   within The Array, quickly accessible at run time from VM code. */

/* Runtime data for count-profiling. */
struct jitter_count_profile_runtime
{
  /* Since the count is updated once per VM instruction being executed we have
     to use 64-bit counters even on 32-bit machines: a fast machine can overflow
     a 32-bit counter in a few seconds when executing short instructions in a
     tight loop. */

  /* A pointer to a profiling array holding an execution count for each
     specialised instruction.
     [FIXME: Having an entire array here as part of the struct, instead of a
      pointer to it, would be impractical on most architectures: it would make
      accessing the commonly used fields slower, because of too wide offsets
      from the base.  However I could actually afford it on a few architectures
      such as x86 and m68k, and possibly on others when the number of
      specialised instructions is low.  I could conditionalise this.] */
  uint64_t *counts;
};

/* Runtime data for sample-profiling. */
struct jitter_sample_profile_runtime
{
  /* Since realistically the sampling period will not be shorter than a
     hundredth of a second it is useless to keep potentially expensive 64-bit
     counters on 32-bit machines: at 10 samples per second it takes an unsigned
     32-bit counter 2 ** 32 / 10 seconds to overflow, which is almost 5000
     days.  A 16-bit counter would overflow in less than two hours. */

  /* The opcode for the specialised instruction currently being executed,
     periodically sampled during execution. */
  jitter_int current_specialized_instruction_opcode;

  /* A pointer to a profiling array holding an execution count for each
     specialised instruction, updated periodically by incrementing the element
     at index current_specialized_instruction_opcode.
     [FIXME: the same remark in the comment within struct jitter_count_profile
      applies here.] */
  uint32_t *counts;

  /* How many samples have been taken. */
  unsigned int sample_no;
};

/* A struct containing both profiling runtime structs. */
struct jitter_profile_runtime
{
  /* It would be sensible to define these conditionally, removing the fields
     altogether when profiling is disabled.  However this would be very fragile
     in a header, since it is difficult to prevent the user from using
     inconsistent CPPFLAGS when compiling the VM and all the code using the VM,
     and such mistakes could be common in practice.
     The generated code itself does not suffer from this problem since the code
     in vm2 checks that vm1 has been compiled with consistent feature macros. */

  /* Runtime data for count-profiling in this state. */
  struct jitter_count_profile_runtime count_profile_runtime;

  /* Runtime data for sample-profiling in this state. */
  struct jitter_sample_profile_runtime sample_profile_runtime;
};

/* Initialise the pointed profile runtime data structure which must be for the
   pointed VM, allocating memory for the fields. */
void
jitter_profile_runtime_initialize (const struct jitter_vm *vm,
                                   struct jitter_profile_runtime *prd)
  __attribute__ ((nonnull (1, 2)));

/* Finalise the pointed profile runtime data structure, releasing memory
   for the fields. */
void
jitter_profile_runtime_finalize (const struct jitter_vm *vm,
                                 struct jitter_profile_runtime *prd)
  __attribute__ ((nonnull (1, 2)));

/* Return a pointer to a fresh runtime profile structure for the pointed VM,
   containing zero counters and zero run times for every instruction. */
struct jitter_profile_runtime *
jitter_profile_runtime_make (const struct jitter_vm *vm)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Destroy the pointed runtime profile structure, which must have been allocated
   via jitter_profile_runtime_make (before being possibly altered by
   jitter_profile_reset_runtime or jitter_profile_runtime_merge_from). */
void
jitter_profile_runtime_destroy (const struct jitter_vm *vm,
                                struct jitter_profile_runtime *pd)
  __attribute__ ((nonnull (1, 2)));

/* Change the pointed runtime profile structure from, which must be for the
   pointed VM, to contain zero execution counts and zero run times for every
   instruction. */
void
jitter_profile_runtime_clear (const struct jitter_vm *vm,
                              struct jitter_profile_runtime *from)
  __attribute__ ((nonnull (1, 2)));

/* Alter the runtime profile structure pointed by "to" by merging data from
   the runtime profile structure pointed by "from".  Both must be profiles
   for the pointed vm. */
void
jitter_profile_runtime_merge_from (const struct jitter_vm *vm,
                                   struct jitter_profile_runtime *to,
                                   const struct jitter_profile_runtime
                                   *from)
  __attribute__ ((nonnull (1, 2, 3)));




/* Processed data structures.
 * ************************************************************************** */

/* An execution profile holding one item for every instruction either
   (unspecialised or specialised), according to what was requested which has
   been executed at least once. */
struct jitter_profile
{
  /* An array of items, as defined below.  Items are sorted first by total
     run time, then by execution count, then by name. */
  struct jitter_profile_item *items;

  /* How many items there are. */
  size_t item_no;
};

/* One entry in the execution profile. */
struct jitter_profile_item
{
  /* The name of the instruction, specialised or not.  The pointed memory
     belongs to the global VM structure and is not copied here. */
  const char *name;

  /* The counted number of executions, or zero if not count-profiling. */
  jitter_ulong_long execution_count;

  /* An estimate of the total run time for this instruction in seconds, or zero
     if not sample-profiling.  This includes every execution of this
     instruction: dividing by the execution count, when the execution count is
     available, can be used to estimate the average run time of a single
     execution. */
  double total_run_time_in_seconds;
};

/* Return a pointer to a fresh struct jitter_profile structure, extracting
   information from the pointed struct jitter_profile_runtime struct for
   the pointed VM. */
struct jitter_profile *
jitter_profile_specialized_from_runtime (const struct jitter_vm *vm,
                                         const struct jitter_profile_runtime *rp)
  __attribute__ ((returns_nonnull, nonnull (1, 2)));

/* Like jitter_profile_specialized_from_runtime , but returning a profile about
   unspecialised instructions. */
struct jitter_profile *
jitter_profile_unspecialized_from_runtime (const struct jitter_vm *vm,
                                           const struct jitter_profile_runtime
                                           *rp)
  __attribute__ ((returns_nonnull, nonnull (1, 2)));

/* Destroy the pointed struct jitter_profile structure, which must have been
   allocated by jitter_profile_unspecialized_from_runtime or by
   jitter_profile_specialized_from_runtime . */
void
jitter_profile_destroy (struct jitter_profile *p)
  __attribute__ ((nonnull (1)));




/* Run-time update.
 * ************************************************************************** */

/* This functionality is used by machine-generated code in vm2. */

/* Given an expression evaluating to a pointer to special-purpose data from a
   state Array and an expression evaluating to a specialised instruction opcode,
   expand to a statement updating profile information.
   In order to minimise latency The specialised opcode should be a compile-time
   constant, and the special-purpose data pointer should be accessible from a
   hardware register at a small compile-time-constant offset; this is the case
   when this macro is used in generated code. */
#define JITTER_PROFILE_COUNT_UPDATE(special_purpose_data,  \
                                    specialised_opcode)    \
  do                                                       \
    {                                                      \
      (special_purpose_data)                               \
         ->profile_runtime                            \
         .count_profile_runtime                       \
         .counts [specialised_opcode] ++;                  \
    }                                                      \
  while (false)
#define JITTER_PROFILE_SAMPLE_UPDATE(special_purpose_data,  \
                                     specialized_opcode)    \
  do                                                        \
    {                                                       \
      (special_purpose_data)                                \
         ->profile_runtime                             \
         .sample_profile_runtime                       \
         .current_specialized_instruction_opcode            \
        = (specialized_opcode);                             \
    }                                                       \
  while (false)




/* Printing.
 * ************************************************************************** */

/* Print a human-readable version of the given profile, which must belong to
   the pointed VM, using the given print context.
   The output uses the classes:
   - "vmprefix-warning";
   - "vmprefix-profile-instruction";
   - "vmprefix-profile-execution-count";
   - "vmprefix-profile-run-time";
   - "vmprefix-profile-time-ratio";
   with "vmprefix" replaced by the lower-case name for the VM. */
void
jitter_profile_print (jitter_print_context ct,
                      const struct jitter_vm *vm,
                      const struct jitter_profile *p)
  __attribute__ ((nonnull (1, 2, 3)));

/* Same as above, using a temporary jitter_profile for specialised instructions
   generated from the pointed runtime profile instead of a struct
   jitter_profile. */
void
jitter_profile_runtime_print_specialized (jitter_print_context ct,
                                          const struct jitter_vm *vm,
                                          const struct jitter_profile_runtime
                                          *prd)
  __attribute__ ((nonnull (1, 2, 3)));


/* Like jitter_profile_runtime_print_specialized , but printing a profile about
   unspecialised instructions. */
void
jitter_profile_runtime_print_unspecialized (jitter_print_context ct,
                                            const struct jitter_vm *vm,
                                            const struct jitter_profile_runtime
                                            *prd)
  __attribute__ ((nonnull (1, 2, 3)));


#endif // #ifndef JITTER_PROFILE_H_
