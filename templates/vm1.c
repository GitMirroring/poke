/* VM library: main VM C file template.

   Copyright (C) 2016, 2017, 2018, 2019, 2020 Luca Saiu
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


/* Generated file warning.
 * ************************************************************************** */

/* Unless this file is named exactly "vm1.c" , without any prefix, you are
   looking at a machine-generated derived file.  The original source is the vm.c
   template from Jitter, with added code implementing the vmprefix VM. */




#include <assert.h>
#include <string.h>

#include <jitter/jitter.h>

#if defined (JITTER_PROFILE_SAMPLE)
#include <sys/time.h>
#endif // #if defined (JITTER_PROFILE_SAMPLE)

#include <jitter/jitter-hash.h>
#include <jitter/jitter-instruction.h>
#include <jitter/jitter-mmap.h>
#include <jitter/jitter-mutable-routine.h>
#include <jitter/jitter-print.h>
#include <jitter/jitter-rewrite.h>
#include <jitter/jitter-routine.h>
#include <jitter/jitter-routine-parser.h>
#include <jitter/jitter-specialize.h>
#include <jitter/jitter-defect.h>
#include <jitter/jitter-patch-in.h>

/* I don't need to include <jitter/jitter-executor.h> here, nor to define
   JITTER_THIS_CAN_INCLUDE_JITTER_EXECUTOR_H ; doing so carelessly might
   lead to subtle bugs, that it is better to prevent.
   Of course I can reconsider this decision in the future. */

#include <jitter/jitter-data-locations.h>

#include "vmprefix-vm.h"
//#include "vmprefix-specialized-instructions.h"
//#include "vmprefix-meta-instructions.h"
#include <jitter/jitter-fatal.h>




/* Check requirements for particular features.
 * ************************************************************************** */

/* VM sample-profiling is only supported with GCC.  Do not bother activating it
   with other compilers, if the numbers are unreliable in the end. */
#if  defined (JITTER_PROFILE_SAMPLE)        \
     && ! defined (JITTER_HAVE_ACTUAL_GCC)
# error "Sample-profiling is only reliable with GCC: it requires (machine-independent)"
# error "GNU C extended asm, and it is not worth supporting other compilers if"
# error "the numbers turn out to be unreliable in the end."
#endif




/* Machine-generated data structures.
 * ************************************************************************** */

/* Machine-generated data structures defining this VM.  Initializing a static
   struct is problematic, as it requires constant expressions for each field --
   and const pointers don't qualify.  This is why we initialize the struct
   fields below in vmprefix_initialize. */
static struct jitter_vm
the_vmprefix_vm;

struct jitter_vm * const
vmprefix_vm = & the_vmprefix_vm;

struct jitter_list_header * const
vmprefix_states = & the_vmprefix_vm.states;

/* It is convenient to have this initialised at start up, even before calling
   any initialisation function.  This makes it reliable to read this when, for
   example, handling --version . */
static const struct jitter_vm_configuration
vmprefix_vm_the_configuration
  = {
      VMPREFIX_LOWER_CASE_PREFIX /* lower_case_prefix */,
      VMPREFIX_UPPER_CASE_PREFIX /* upper_case_prefix */,
      VMPREFIX_MAX_FAST_REGISTER_NO_PER_CLASS
        /* max_fast_register_no_per_class */,
      VMPREFIX_MAX_NONRESIDUAL_LITERAL_NO /* max_nonresidual_literal_no */,
      VMPREFIX_DISPATCH_HUMAN_READABLE /* dispatch_human_readable */,
      /* The instrumentation field can be seen as a bit map.  See the comment
         in jitter/jitter-vm.h . */
      (jitter_vm_instrumentation_none
#if defined (JITTER_PROFILE_COUNT)
       | jitter_vm_instrumentation_count
#endif
#if defined (JITTER_PROFILE_SAMPLE)
       | jitter_vm_instrumentation_sample
#endif
       ) /* instrumentation */
    };

const struct jitter_vm_configuration * const
vmprefix_vm_configuration
  = & vmprefix_vm_the_configuration;




/* Initialization and finalization: internal functions, not for the user.
 * ************************************************************************** */

/* Initialize threads.  This only needs to be called once at initialization, and
   the user doesn't need to bother with it.  Defined along with the executor. */
void
vmprefix_initialize_threads (void);

/* Check that the encodings in enum jitter_specialized_instruction_opcode (as
   used in the specializer) are coherent with machine-generated code.  Making a
   mistake here would introduce subtle bugs, so it's better to be defensive. */
static void
vmprefix_check_specialized_instruction_opcode_once (void)
{
  static bool already_checked = false;
  if (already_checked)
    return;

  assert (((enum jitter_specialized_instruction_opcode)
           vmprefix_specialized_instruction_opcode__eINVALID)
          == jitter_specialized_instruction_opcode_INVALID);
  assert (((enum jitter_specialized_instruction_opcode)
           vmprefix_specialized_instruction_opcode__eBEGINBASICBLOCK)
          == jitter_specialized_instruction_opcode_BEGINBASICBLOCK);
  assert (((enum jitter_specialized_instruction_opcode)
           vmprefix_specialized_instruction_opcode__eEXITVM)
          == jitter_specialized_instruction_opcode_EXITVM);
  assert (((enum jitter_specialized_instruction_opcode)
           vmprefix_specialized_instruction_opcode__eDATALOCATIONS)
          == jitter_specialized_instruction_opcode_DATALOCATIONS);
  assert (((enum jitter_specialized_instruction_opcode)
           vmprefix_specialized_instruction_opcode__eNOP)
          == jitter_specialized_instruction_opcode_NOP);
  assert (((enum jitter_specialized_instruction_opcode)
           vmprefix_specialized_instruction_opcode__eUNREACHABLE0)
          == jitter_specialized_instruction_opcode_UNREACHABLE0);
  assert (((enum jitter_specialized_instruction_opcode)
           vmprefix_specialized_instruction_opcode__eUNREACHABLE1)
          == jitter_specialized_instruction_opcode_UNREACHABLE1);
  assert (((enum jitter_specialized_instruction_opcode)
           vmprefix_specialized_instruction_opcode__eUNREACHABLE2)
          == jitter_specialized_instruction_opcode_UNREACHABLE2);

  already_checked = true;
}

/* A prototype for a machine-generated function not needing a public
   declaration, only called thru a pointer within struct jitter_vm . */
int
vmprefix_specialize_instruction (struct jitter_mutable_routine *p,
                                 const struct jitter_instruction *ins);

/* Initialize the pointed special-purpose data structure. */
static void
vmprefix_initialize_special_purpose_data
   (volatile struct jitter_special_purpose_state_data *d)
{
  d->pending_notifications = 0;
  jitter_initialize_pending_signal_notifications
     (& d->pending_signal_notifications);

  /* Initialise profiling fields. */
  jitter_profile_runtime_initialize (vmprefix_vm,
                                     (struct jitter_profile_runtime *)
                                     & d->profile_runtime);
}

/* Finalize the pointed special-purpose data structure. */
static void
vmprefix_finalize_special_purpose_data
   (volatile struct jitter_special_purpose_state_data *d)
{
  jitter_finalize_pending_signal_notifications
     (d->pending_signal_notifications);

  jitter_profile_runtime_finalize (vmprefix_vm,
                                   (struct jitter_profile_runtime *)
                                   & d->profile_runtime);
}




/* Check that we link with the correct Jitter library.
 * ************************************************************************** */

/* It is possible to make a mistake at link time, and link a VM compiled with
   some threading model with the Jitter runtime for a different model.  That
   would cause crashes, that is better to prevent.  This is a way to detect such
   mistakes very early, by causing a link-time failure in case of mismatch. */
extern volatile const bool
JITTER_DISPATCH_DEPENDENT_GLOBAL_NAME;




/* Low-level debugging features relying on assembly: data locations.
 * ************************************************************************** */

#if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT) && ! defined (JITTER_DISPATCH_SWITCH)
/* A declaration for data locations, as visible from C.  The global is defined in
   assembly in its own separate section thru the machinery in
   jitter/jitter-sections.h . */
extern const char
JITTER_DATA_LOCATION_NAME(vmprefix) [];
#endif // #if ...

void
vmprefix_dump_data_locations (jitter_print_context output)
{
#ifndef JITTER_DISPATCH_SWITCH
  jitter_dump_data_locations (output, & the_vmprefix_vm);
#else
  jitter_print_char_star (output,
                          "VM data location information unavailable\n");
#endif // #ifndef JITTER_DISPATCH_SWITCH
}




/* Initialization and finalization.
 * ************************************************************************** */

#if defined (JITTER_HAVE_DEFECT_REPLACEMENT)
JITTER_DEFECT_DESCRIPTOR_DECLARATIONS_(vmprefix)

/* This global is defined from C: there is no particular need of doing it in
   assembly.  It is initialised in vmprefix_execute_or_initialize , where C
   labels are visible. */ // FIXME: unless it turns out to be simpler in assembly ...
jitter_int
vmprefix_defect_descriptors_correct_displacement;
#endif // #if defined (JITTER_HAVE_DEFECT_REPLACEMENT)
#if defined (JITTER_HAVE_PATCH_IN)
JITTER_PATCH_IN_DESCRIPTOR_DECLARATIONS_(vmprefix)
#endif // #if defined (JITTER_HAVE_PATCH_IN)

#ifndef JITTER_DISPATCH_SWITCH
/* True iff thread sizes are all non-negative and non-huge.  We refuse to
   disassemble otherwise, and when replication is enabled we refuse to run
   altogether.  See the comment right below. */
static bool
vmprefix_threads_validated = false;
#endif // #ifndef JITTER_DISPATCH_SWITCH

/* Omit vmprefix_validate_thread_sizes_once for switch-dispatching, as threads
   don't exist at all in that case.*/
#ifndef JITTER_DISPATCH_SWITCH
/* Check that VM instruction sizes are all non-negative, and that no thread
   starts before the end of the previous one.  Even one violation of such
   conditions is a symptom that the code has not been compiled with
   -fno-reorder-blocks , which would have disastrous effects with replication.
   It's better to validate threads at startup and fail immediately than to crash
   at run time.

   If even one thread appears to be wrong then refuse to disassemble when
   replication is disabled, and refuse to run altogether if replication is
   enabled. */
static void
vmprefix_validate_threads_once (void)
{
  /* Return if this is not the first time we got here. */
  static bool already_validated = false;
  if (already_validated)
    return;

#ifdef JITTER_REPLICATE
# define JITTER_FAIL(error_text)                                             \
    do                                                                       \
      {                                                                      \
        fprintf (stderr,                                                     \
                 "About specialized instruction %i (%s) at %p, size %liB\n", \
                 i, vmprefix_specialized_instruction_names [i],              \
                 vmprefix_threads [i],                                       \
                 vmprefix_thread_sizes [i]);                                 \
        jitter_fatal ("%s: you are not compiling with -fno-reorder-blocks",  \
                      error_text);                                           \
      }                                                                      \
    while (false)
#else
# define JITTER_FAIL(ignored_error_text)  \
    do                                    \
      {                                   \
        everything_valid = false;         \
        goto out;                         \
      }                                   \
    while (false)
#endif // #ifdef JITTER_REPLICATE

  /* The minimum address the next instruction code has to start at.

     This relies on NULL being zero, or in general lower in magnitude than any
     valid pointer.  It is not worth the trouble to be pedantic, as this will be
     true on every architecture where I can afford low-level tricks. */
  jitter_thread lower_bound = NULL;

  /* Check every thread.  We rely on the order here, following specialized
     instruction opcodes. */
  int i;
  bool everything_valid = true;
  for (i = 0; i < VMPREFIX_SPECIALIZED_INSTRUCTION_NO; i ++)
    {
      jitter_thread thread = vmprefix_threads [i];
      long size = vmprefix_thread_sizes [i];

      /* Check that the current thread has non-negative non-huge size and
         doesn't start before the end of the previous one.  If this is true for
         all threads we can conclude that they are non-overlapping as well. */
      if (__builtin_expect (size < 0, false))
        JITTER_FAIL("a specialized instruction has negative code size");
      if (__builtin_expect (size > (1 << 24), false))
        JITTER_FAIL("a specialized instruction has huge code size");
      if (__builtin_expect (lower_bound > thread, false))
        JITTER_FAIL("non-sequential thread");

      /* The next thread cannot start before the end of the current one. */
      lower_bound = ((char*) thread) + size;
    }

#undef JITTER_FAIL

#ifndef JITTER_REPLICATE
 out:
#endif // #ifndef JITTER_REPLICATE

  /* If we have validated every thread size then disassembling appears safe. */
  if (everything_valid)
    vmprefix_threads_validated = true;

  /* We have checked the thread sizes, once and for all.  If this function gets
     called again, thru a second vmprefix initialization, it will immediately
     return. */
  already_validated = true;
}
#endif // #ifndef JITTER_DISPATCH_SWITCH

#if defined (JITTER_HAVE_DEFECT_REPLACEMENT)
/* The actual replacement table.  We only need it when defect replacement is in
   use. */
jitter_uint
vmprefix_replacement_table [VMPREFIX_SPECIALIZED_INSTRUCTION_NO];
#endif // #if defined (JITTER_HAVE_DEFECT_REPLACEMENT)

void
vmprefix_initialize (void)
{
  /* Check that the Jitter library we linked is the right one.  This check
     actually only useful to force the global to be used.  I prefer not to use
     an assert, because assertions can be disabled. */
  if (! JITTER_DISPATCH_DEPENDENT_GLOBAL_NAME)
    jitter_fatal ("impossible to reach: the thing should fail at link time");

#ifdef JITTER_REPLICATE
  /* Initialize the executable-memory subsystem. */
  jitter_initialize_executable ();
#endif // #ifdef JITTER_REPLICATE

  /* Initialise the print-context machinery. */
  jitter_print_initialize ();

  /* Perform some sanity checks which only need to be run once. */
  vmprefix_check_specialized_instruction_opcode_once ();

  /* We have to initialize threads before vmprefix_threads , since the struct
     needs threads. */
  vmprefix_initialize_threads ();

#ifndef JITTER_DISPATCH_SWITCH
  /* Validate threads, to make sure the generated code was not compiled with
     incorrect options.  This only needs to be done once. */
  vmprefix_validate_threads_once ();
#endif // ifndef JITTER_DISPATCH_SWITCH

  /* Initialize the object pointed by vmprefix_vm (see the comment above as to
     why we do it here).  Before actually setting the fields to valid data, fill
     the whole struct with a -- hopefully -- invalid pattern, just to catch
     bugs. */
  static bool vm_struct_initialized = false;
  if (! vm_struct_initialized)
    {
      memset (& the_vmprefix_vm, 0xff, sizeof (struct jitter_vm));

      /* Make the configuration struct reachable from the VM struct. */
      the_vmprefix_vm.configuration = vmprefix_vm_configuration;
      //vmprefix_print_vm_configuration (stdout, & the_vmprefix_vm.configuration);

      /* Initialize meta-instruction pointers for implicit instructions.
         VM-independent program specialization relies on those, so they have to
         be accessible to the Jitter library, out of generated code.  Since
         meta-instructions are sorted alphabetically in the array, the index
         is not fixed. */
      the_vmprefix_vm.exitvm_meta_instruction
        = (vmprefix_meta_instructions + vmprefix_meta_instruction_id_exitvm);
      the_vmprefix_vm.unreachable_meta_instruction
        = (vmprefix_meta_instructions
           + vmprefix_meta_instruction_id_unreachable);

      /* Threads or pointers to native code blocks of course don't exist with
         switch-dispatching. */
#ifndef JITTER_DISPATCH_SWITCH
      the_vmprefix_vm.threads = (jitter_thread *)vmprefix_threads;
      the_vmprefix_vm.thread_sizes = (long *) vmprefix_thread_sizes;
      the_vmprefix_vm.threads_validated = vmprefix_threads_validated;
#if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
      the_vmprefix_vm.data_locations = JITTER_DATA_LOCATION_NAME(vmprefix);
#else
      the_vmprefix_vm.data_locations = NULL;
#endif // #if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
#endif // #ifndef JITTER_DISPATCH_SWITCH

      the_vmprefix_vm.specialized_instruction_residual_arities
        = vmprefix_specialized_instruction_residual_arities;
      the_vmprefix_vm.specialized_instruction_label_bitmasks
        = vmprefix_specialized_instruction_label_bitmasks;
#ifdef JITTER_HAVE_PATCH_IN
      the_vmprefix_vm.specialized_instruction_fast_label_bitmasks
        = vmprefix_specialized_instruction_fast_label_bitmasks;
      the_vmprefix_vm.patch_in_descriptors =
        JITTER_PATCH_IN_DESCRIPTORS_NAME(vmprefix);
      const size_t patch_in_descriptor_size
        = sizeof (struct jitter_patch_in_descriptor);
      the_vmprefix_vm.patch_in_descriptor_no
        = (JITTER_PATCH_IN_DESCRIPTORS_SIZE_IN_BYTES_NAME(vmprefix)
           / patch_in_descriptor_size);
      /* Cheap sanity check: if the size in bytes is not a multiple of
         the element size, we are doing something very wrong. */
      if (JITTER_PATCH_IN_DESCRIPTORS_SIZE_IN_BYTES_NAME(vmprefix)
          % patch_in_descriptor_size != 0)
        jitter_fatal ("patch-in descriptors total size %li not a multiple "
                      "of the element size %li",
                      (long) (JITTER_PATCH_IN_DESCRIPTORS_SIZE_IN_BYTES_NAME
                                 (vmprefix)),
                      (long) patch_in_descriptor_size);
      /* Initialize the patch-in table for this VM. */
      the_vmprefix_vm.patch_in_table
        = jitter_make_patch_in_table (the_vmprefix_vm.patch_in_descriptors,
                                      the_vmprefix_vm.patch_in_descriptor_no,
                                      VMPREFIX_SPECIALIZED_INSTRUCTION_NO);
#else
      the_vmprefix_vm.specialized_instruction_fast_label_bitmasks = NULL;
#endif // #ifdef JITTER_HAVE_PATCH_IN

      /* FIXME: I might want to conditionalize this. */
      the_vmprefix_vm.specialized_instruction_relocatables
        = vmprefix_specialized_instruction_relocatables;

      the_vmprefix_vm.specialized_instruction_callers
        = vmprefix_specialized_instruction_callers;
      the_vmprefix_vm.specialized_instruction_callees
        = vmprefix_specialized_instruction_callees;

      the_vmprefix_vm.specialized_instruction_names
        = vmprefix_specialized_instruction_names;
      the_vmprefix_vm.specialized_instruction_no
        = VMPREFIX_SPECIALIZED_INSTRUCTION_NO;

      the_vmprefix_vm.meta_instruction_string_hash
        = & vmprefix_meta_instruction_hash;
      the_vmprefix_vm.meta_instructions
        = (struct jitter_meta_instruction *) vmprefix_meta_instructions;
      the_vmprefix_vm.meta_instruction_no = VMPREFIX_META_INSTRUCTION_NO;
      the_vmprefix_vm.max_meta_instruction_name_length
        = VMPREFIX_MAX_META_INSTRUCTION_NAME_LENGTH;
      the_vmprefix_vm.specialized_instruction_to_unspecialized_instruction
        = vmprefix_specialized_instruction_to_unspecialized_instruction;
      the_vmprefix_vm.register_class_character_to_register_class
        = vmprefix_register_class_character_to_register_class;
      the_vmprefix_vm.specialize_instruction = vmprefix_specialize_instruction;
      the_vmprefix_vm.rewrite = vmprefix_rewrite;

#if defined (JITTER_HAVE_DEFECT_REPLACEMENT)
      /* Fill the replacement table.  Since the array in question is a global
         with a fixed size, this needs to be done only once. */
      jitter_fill_replacement_table
         (vmprefix_replacement_table,
          & the_vmprefix_vm,
          vmprefix_worst_case_replacement_table,
          vmprefix_call_related_specialized_instruction_ids,
          vmprefix_call_related_specialized_instruction_id_no,
          vmprefix_specialized_instruction_call_relateds,
          JITTER_DEFECT_DESCRIPTORS_NAME (vmprefix),
          (JITTER_DEFECT_DESCRIPTORS_SIZE_IN_BYTES_NAME (vmprefix)
           / sizeof (struct jitter_defect_descriptor)),
          JITTER_DEFECT_CORRECT_DISPLACEMENT_NAME (vmprefix));
#else /* no defect replacement */
      /* In this configuration it is impossible to have defects: set every
         defect count to zero. */
      the_vmprefix_vm.defect_no = 0;
      the_vmprefix_vm.defective_specialized_instruction_no = 0;
      the_vmprefix_vm.defective_call_related_specialized_instruction_no = 0;
      the_vmprefix_vm.replacement_specialized_instruction_no = 0;
#endif // #if defined (JITTER_HAVE_DEFECT_REPLACEMENT)

      /* Initialize the empty list of states. */
      JITTER_LIST_INITIALIZE_HEADER (& the_vmprefix_vm.states);

      vm_struct_initialized = true;
    }

  jitter_initialize_meta_instructions (& vmprefix_meta_instruction_hash,
                                         vmprefix_meta_instructions,
                                         VMPREFIX_META_INSTRUCTION_NO);

#if defined (JITTER_HAVE_DEFECT_REPLACEMENT)
  jitter_dump_replacement_table (stderr, vmprefix_replacement_table,
                                 & the_vmprefix_vm);
#endif // #if defined (JITTER_HAVE_DEFECT_REPLACEMENT)
#if defined (JITTER_HAVE_PATCH_IN)
  //printf ("======================= Patch-in descriptors: BEGIN\n");
  //JITTER_DUMP_PATCH_IN_DESCRIPTORS(vmprefix);
  //printf ("======================= Patch-in descriptors: END\n");
#endif // #if defined (JITTER_HAVE_PATCH_IN)
}

void
vmprefix_finalize (void)
{
  /* There's no need to touch the_vmprefix_vm ; we can keep it as it is, as it
     contains no dynamically-allocated fields. */
  /* Threads need no finalization. */
  jitter_finalize_meta_instructions (& vmprefix_meta_instruction_hash);

#ifdef JITTER_HAVE_PATCH_IN
  /* Destroy the patch-in table for this VM. */
  jitter_destroy_patch_in_table (the_vmprefix_vm.patch_in_table,
                                 VMPREFIX_SPECIALIZED_INSTRUCTION_NO);
#endif // #ifdef JITTER_HAVE_PATCH_IN

#ifdef JITTER_REPLICATE
  /* Finalize the executable-memory subsystem. */
  jitter_finalize_executable ();
#endif // #ifdef JITTER_REPLICATE

  /* Finalize the state list.  If it is not empty then something has gone
     wrong earlier. */
  if (the_vmprefix_vm.states.first != NULL
      || the_vmprefix_vm.states.last != NULL)
    jitter_fatal ("not every state structure was destroyed before VMPREFIX "
                  "finalisation.");
}




/* VM-dependant mutable routine initialization.
 * ************************************************************************** */

struct jitter_mutable_routine*
vmprefix_make_mutable_routine (void)
{
  return jitter_make_mutable_routine (vmprefix_vm);
}




/* Sample profiling: internal API.
 * ************************************************************************** */

#if defined (JITTER_PROFILE_SAMPLE)

/* Sample profiling depends on some system features: fail immediately if they
   are not available */
#if ! defined (JITTER_HAVE_SIGACTION) || ! defined (JITTER_HAVE_SETITIMER)
# jitter_fatal "sample-profiling depends on sigaction and setitimer"
#endif

static struct itimerval
vmprefix_timer_interval;

static struct itimerval
vmprefix_timer_disabled_interval;

/* The sampling data, currently global.  The current implementation does not
   play well with threads, but it can be changed later keeping the same user
   API. */
struct vmprefix_sample_profile_state
{
  /* The state currently sample-profiling.  Since such a state can be only one
     right now this field is useful for printing error messages in case the user
     sets up sample-profiling from two states at the same time by mistake.
     This field is also useful for temporarily suspending and then reenabling
     sampling, when The Array is being resized: if the signal handler sees that
     this field is NULL it will not touch the fields. */
  struct vmprefix_state *state_p;

  /* A pointer to the counts field within the sample_profile_runtime struct. */
  uint32_t *counts;

  /* A pointer to the current specialised instruction opcode within the
     sample_profile_runtime struct. */
  volatile jitter_int * specialized_opcode_p;

  /* A pointer to the field counting the number of samples, again within the
     sample_profile_runtime struct. */
  unsigned int *sample_no_p;
};

/* The (currently) one and only global state for sample-profiling. */
static struct vmprefix_sample_profile_state
vmprefix_sample_profile_state;

static void
vmprefix_sigprof_handler (int signal)
{
#if 0
  assert (vmprefix_sample_profile_state.state_p != NULL);
#endif

  jitter_int specialized_opcode
    = * vmprefix_sample_profile_state.specialized_opcode_p;
  if (__builtin_expect ((specialized_opcode >= 0
                         && (specialized_opcode
                             < VMPREFIX_SPECIALIZED_INSTRUCTION_NO)),
                        true))
    vmprefix_sample_profile_state.counts [specialized_opcode] ++;

  (* vmprefix_sample_profile_state.sample_no_p) ++;
}

void
vmprefix_profile_sample_initialize (void)
{
  /* Perform a sanity check over the sampling period. */
  if (JITTER_PROFILE_SAMPLE_PERIOD_IN_MILLISECONDS <= 0 ||
      JITTER_PROFILE_SAMPLE_PERIOD_IN_MILLISECONDS >= 1000)
    jitter_fatal ("invalid JITTER_PROFILE_SAMPLE_PERIOD_IN_MILLISECONDS: %f",
                  (double) JITTER_PROFILE_SAMPLE_PERIOD_IN_MILLISECONDS);
  struct sigaction action;
  sigaction (SIGPROF, NULL, & action);
  action.sa_handler = vmprefix_sigprof_handler;
  sigaction (SIGPROF, & action, NULL);

  long microseconds
    = (long) (JITTER_PROFILE_SAMPLE_PERIOD_IN_MILLISECONDS * 1000);
  vmprefix_timer_interval.it_interval.tv_sec = 0;
  vmprefix_timer_interval.it_interval.tv_usec = microseconds;
  vmprefix_timer_interval.it_value = vmprefix_timer_interval.it_interval;

  vmprefix_sample_profile_state.state_p = NULL;
  vmprefix_timer_disabled_interval.it_interval.tv_sec = 0;
  vmprefix_timer_disabled_interval.it_interval.tv_usec = 0;
  vmprefix_timer_disabled_interval.it_value
    = vmprefix_timer_disabled_interval.it_interval;
}

void
vmprefix_profile_sample_start (struct vmprefix_state *state_p)
{
  struct jitter_sample_profile_runtime *spr
    = ((struct jitter_sample_profile_runtime *)
       & VMPREFIX_STATE_TO_SPECIAL_PURPOSE_STATE_DATA (state_p)
           ->profile_runtime.sample_profile_runtime);

  if (vmprefix_sample_profile_state.state_p != NULL)
    {
      if (state_p != vmprefix_sample_profile_state.state_p)
        jitter_fatal ("currently it is only possible to sample-profile from "
                      "one state at the time: trying to sample-profile from "
                      "the state %p when already sample-profiling from the "
                      "state %p",
                      state_p, vmprefix_sample_profile_state.state_p);
      else
        {
          /* This situation is a symptom of a bug, but does not need to lead
             to a fatal error. */
          printf ("WARNING: starting profile on the state %p when profiling "
                  "was already active in the same state.\n"
                  "Did you call longjmp from VM code?", state_p);
          fflush (stdout);
        }
    }
  vmprefix_sample_profile_state.state_p = state_p;
  vmprefix_sample_profile_state.sample_no_p = & spr->sample_no;
  vmprefix_sample_profile_state.counts = spr->counts;
  vmprefix_sample_profile_state.specialized_opcode_p
    = & spr->current_specialized_instruction_opcode;
  //fprintf (stderr, "SAMPLE START\n"); fflush (NULL);
  if (setitimer (ITIMER_PROF, & vmprefix_timer_interval, NULL) != 0)
    jitter_fatal ("setitimer failed when establishing a timer");
}

void
vmprefix_profile_sample_stop (void)
{
  if (setitimer (ITIMER_PROF, & vmprefix_timer_disabled_interval, NULL) != 0)
    jitter_fatal ("setitimer failed when disabling a timer");

  vmprefix_sample_profile_state.state_p = NULL;

  /* The rest is just for defenisveness' sake. */
  * vmprefix_sample_profile_state.specialized_opcode_p = -1;
  vmprefix_sample_profile_state.sample_no_p = NULL;
  vmprefix_sample_profile_state.counts = NULL;
  vmprefix_sample_profile_state.specialized_opcode_p = NULL;
}
#endif // #if defined (JITTER_PROFILE_SAMPLE)




/* Slow register initialisation.
 * ************************************************************************** */

/* Initialise slow registers (for register classes defining an initial value) in
   a given Array, starting from a given rank up to another given rank.  The
   argument old_slow_register_no_per_class indicates the number of already
   initialised ranks, which this functions does not touch;
   new_slow_register_no_per_class indicates the new number of ranks.  Every rank
   from old_slow_register_no_per_class + 1 to new_slow_register_no_per_class,
   both included, will be initialised. */
static void
vmprefix_initialize_slow_registers (char *initial_array_pointer,
                                    jitter_int old_slow_register_no_per_class,
                                    jitter_int new_slow_register_no_per_class)
{
  /* Compute the address of the first slow registers, which is the beginning
     of the first rank. */
  union vmprefix_any_register *first_slow_register
    = ((union vmprefix_any_register *)
       ((char *) initial_array_pointer
        + VMPREFIX_FIRST_SLOW_REGISTER_UNBIASED_OFFSET));

  /* Initialise every *new* rank, without touching the old ones. */
  jitter_int i;
  for (i = old_slow_register_no_per_class;
       i < new_slow_register_no_per_class;
       i ++)
    {
      /* A pointer to the i-th rank of slow registers.  Every register
         in the rank is new and in general (according to its class) may
         need initialisation. */
      union vmprefix_any_register *rank
        = first_slow_register + (i * VMPREFIX_REGISTER_CLASS_NO);
      VMPREFIX_INITIALIZE_SLOW_REGISTER_RANK (rank);
    }
}




/* Array re-allocation.
 * ************************************************************************** */

char *
vmprefix_make_place_for_slow_registers (struct vmprefix_state *s,
                                        jitter_int new_slow_register_no_per_class)
{
  if (new_slow_register_no_per_class < 0)
    jitter_fatal ("vmprefix_make_place_for_slow_registers: negative slow "
                  "register number");
  jitter_int old_slow_register_no_per_class
    = s->vmprefix_state_backing.jitter_slow_register_no_per_class;
  /* Change nothing if we already have enough space for the required number of
     slow registers.  The no-change case will be the most common one, and
     this function might be worth optimizing. */
  if (__builtin_expect (new_slow_register_no_per_class
                        > old_slow_register_no_per_class,
                        false))
    {
#if defined (JITTER_PROFILE_SAMPLE)
      /* If sample-profiling is currently in progress on this state suspend it
         temporarily. */
      bool suspending_sample_profiling
        = (vmprefix_sample_profile_state.state_p == s);
      if (suspending_sample_profiling)
        vmprefix_profile_sample_stop ();
#endif // #if defined (JITTER_PROFILE_SAMPLE)

#if 0
      printf ("Increasing slow register-no (per class) from %li to %li\n", (long) old_slow_register_no_per_class, (long)new_slow_register_no_per_class);
      printf ("Array size %li -> %li\n", (long) VMPREFIX_ARRAY_SIZE(old_slow_register_no_per_class), (long) VMPREFIX_ARRAY_SIZE(new_slow_register_no_per_class));
#endif
      /* Save the new value for new_slow_register_no_per_class in the state
         structure; reallocate The Array. */
      s->vmprefix_state_backing.jitter_slow_register_no_per_class
        = new_slow_register_no_per_class;
      s->vmprefix_state_backing.jitter_array
        = jitter_xrealloc ((void *) s->vmprefix_state_backing.jitter_array,
                           VMPREFIX_ARRAY_SIZE(new_slow_register_no_per_class));

     /* Initialise the slow registers we have just added, for every class. */
     vmprefix_initialize_slow_registers (s->vmprefix_state_backing.jitter_array,
                                         old_slow_register_no_per_class,
                                         new_slow_register_no_per_class);

#if defined (JITTER_PROFILE_SAMPLE)
      /* Now we can resume sample-profiling on this state if we suspended it. */
      if (suspending_sample_profiling)
        vmprefix_profile_sample_start (s);
#endif // #if defined (JITTER_PROFILE_SAMPLE)
#if 0
      printf ("Done resizing The Array\n");
#endif
    }

  /* Return the new (or unchanged) base, by simply adding the bias to the
     Array as it is now. */
  return s->vmprefix_state_backing.jitter_array + JITTER_ARRAY_BIAS;
}

void
vmprefix_ensure_enough_slow_registers_for_executable_routine
   (const struct jitter_executable_routine *er, struct vmprefix_state *s)
{
  vmprefix_make_place_for_slow_registers (s, er->slow_register_per_class_no);
}




/* Program text frontend.
 * ************************************************************************** */

void
vmprefix_parse_mutable_routine_from_file_star (FILE *input_file,
                                               struct jitter_mutable_routine *p)
{
  jitter_parse_mutable_routine_from_file_star (input_file, p, vmprefix_vm);
}

void
vmprefix_parse_mutable_routine_from_file (const char *input_file_name,
                                          struct jitter_mutable_routine *p)
{
  jitter_parse_mutable_routine_from_file (input_file_name, p, vmprefix_vm);
}

void
vmprefix_parse_mutable_routine_from_string (const char *string,
                                            struct jitter_mutable_routine *p)
{
  jitter_parse_mutable_routine_from_string (string, p, vmprefix_vm);
}




/* State making and destroying.
 * ************************************************************************** */

/* State initialisation (with a given number of slow registers), reset and
   finalisation are machine-generated. */

void
vmprefix_state_initialize (struct vmprefix_state *sp)
{
  vmprefix_state_initialize_with_slow_registers (sp, 0);
}

struct vmprefix_state *
vmprefix_state_make_with_slow_registers (jitter_uint slow_register_no_per_class)
{
  struct vmprefix_state *res = jitter_xmalloc (sizeof (struct vmprefix_state));
  vmprefix_state_initialize_with_slow_registers (res,
                                                 slow_register_no_per_class);
  return res;
}

struct vmprefix_state *
vmprefix_state_make (void)
{
  return vmprefix_state_make_with_slow_registers (0);
}

void
vmprefix_state_destroy (struct vmprefix_state *state)
{
  vmprefix_state_finalize (state);
  free (state);
}




/* Executing code: unified routine API.
 * ************************************************************************** */

void
vmprefix_ensure_enough_slow_registers_for_routine
   (jitter_routine r, struct vmprefix_state *s)
{
  struct jitter_executable_routine *e
    = jitter_routine_make_executable_if_needed (r);
  vmprefix_ensure_enough_slow_registers_for_executable_routine (e, s);
}

enum vmprefix_exit_status
vmprefix_execute_routine (jitter_routine r,
                          struct vmprefix_state *s)
{
  struct jitter_executable_routine *e
    = jitter_routine_make_executable_if_needed (r);
  return vmprefix_execute_executable_routine (e, s);
}





/* Defects and replacements: user API.
 * ************************************************************************** */

/* These functions are all trivial wrappers around the functionality declared
   in jitter/jitter-defect.h, hiding the VM pointer. */

void
vmprefix_defect_print_summary (jitter_print_context cx)
{
  jitter_defect_print_summary (cx, vmprefix_vm);
}




/* Profiling: user API.
 * ************************************************************************** */

/* These functions are all trivial wrappers around the functionality declared
   in jitter/jitter-profile.h, hiding the VM pointer. */

struct vmprefix_profile_runtime *
vmprefix_state_profile_runtime (struct vmprefix_state *s)
{
  volatile struct jitter_special_purpose_state_data *spd
    = VMPREFIX_ARRAY_TO_SPECIAL_PURPOSE_STATE_DATA
        (s->vmprefix_state_backing.jitter_array);
  return (struct vmprefix_profile_runtime *) & spd->profile_runtime;
}

struct vmprefix_profile_runtime *
vmprefix_profile_runtime_make (void)
{
  return jitter_profile_runtime_make (vmprefix_vm);
}

void
vmprefix_profile_runtime_clear (struct vmprefix_profile_runtime * p)
{
  jitter_profile_runtime_clear (vmprefix_vm, p);
}

void
vmprefix_profile_runtime_merge_from (struct vmprefix_profile_runtime *to,
                                     const struct vmprefix_profile_runtime *from)
{
  jitter_profile_runtime_merge_from (vmprefix_vm, to, from);
}

void
vmprefix_profile_runtime_merge_from_state (struct vmprefix_profile_runtime *to,
                                           const struct vmprefix_state *from_state)
{
  const struct vmprefix_profile_runtime* from
    = vmprefix_state_profile_runtime ((struct vmprefix_state *) from_state);
  jitter_profile_runtime_merge_from (vmprefix_vm, to, from);
}

void
vmprefix_profile_runtime_print_unspecialized
   (jitter_print_context ct,
    const struct vmprefix_profile_runtime *p)
{
  jitter_profile_runtime_print_unspecialized (ct, vmprefix_vm, p);
}

void
vmprefix_profile_runtime_print_specialized (jitter_print_context ct,
                                            const struct vmprefix_profile_runtime
                                            *p)
{
  jitter_profile_runtime_print_specialized (ct, vmprefix_vm, p);
}

struct vmprefix_profile *
vmprefix_profile_unspecialized_from_runtime
   (const struct vmprefix_profile_runtime *p)
{
  return jitter_profile_unspecialized_from_runtime (vmprefix_vm, p);
}

struct vmprefix_profile *
vmprefix_profile_specialized_from_runtime (const struct vmprefix_profile_runtime
                                           *p)
{
  return jitter_profile_specialized_from_runtime (vmprefix_vm, p);
}




/* Evrything following this point is machine-generated.
 * ************************************************************************** */

/* What follows could be conceptually split into several generated C files, but
   having too many of them would be inconvenient for the user to compile and
   link.  For this reason we currently generate just three files: one is this,
   which also contains the specializer, another is for the executor, and then a
   header -- a main module is optional.  The executor will be potentially very
   large, so it is best compiled separately.  The specializer might be large as
   well at this stage, even if its compilation is usually much less
   expensive. */
