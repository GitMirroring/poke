/* Jitter: VM-specific configuration and internal implementation header.

   Copyright (C) 2017, 2018, 2019, 2021 Luca Saiu
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


#ifndef JITTER_VM_H_
#define JITTER_VM_H_

#include <stdio.h>

#include <jitter/jitter.h>
#include <jitter/jitter-mutable-routine.h>
#include <jitter/jitter-patch-in.h>
#include <jitter/jitter-list.h>




/* VM-specific attributes.
 * ************************************************************************** */

/* The kind of profiling instrumentation enabled in a VM.  This is selected with
   CPP macros when compiling vm1 and vm2.

   Notice that the enum value can be used as a bit mask, with
   jitter_vm_instrumentation_count and jitter_vm_instrumentation_sample being
   two independent features that can be enabled or disabled, each contributing
   to the enum jitter_vm_instrumentation value as operands of a bitwise or
   operation. */
enum jitter_vm_instrumentation
  {
    /* No instrumentation, as suitable for production. */
    jitter_vm_instrumentation_none
      = 0,

    /* Counting instrumentation.  This permits to count how many time each
       specialised instruction is executed, exactly.  
       This mode is unsuitable for production as the instrumentation overhead
       is severe.  For every VM instruction:
       - load;
       - 64-bit load (often with an offset larger than 16 bits);
       - 64-bit increment;
       - 64-bit store (often with an offset larger than 16 bits).
       This is also heavy on l1d, since the 64-bit memory location accessed
       depends on the specialised opcode, and many different location can be
       touched. */
    jitter_vm_instrumentation_count
      = 1,

    /* Sampling instrumentation.  This permits to count how much time is spent
       executing each specialised instruction, subject to sampling errors.
       The instrumentation overhead is less extreme in this case.  For every
       VM instruction:
       - store of a 16-bit constant (zero-extended or sign-extended to word
         size) depending on the instruction;
       and then the overhead of handling a periodic signal.
       The address being written to is always the same. */
    jitter_vm_instrumentation_sample
      = 2,

    /* Enable both counting and sampling.  The overhead will be equal to the sum
       of the two overheads above, again making this mode is again unsuitable
       for production VMs. */
    jitter_vm_instrumentation_count_and_sample
      = jitter_vm_instrumentation_count | jitter_vm_instrumentation_sample
  };

/* Return a human-readable description of the given instrumentation.  The
   returned string points to global constant memory, and the user does not
   need to free it. */
const char *
jitter_vm_instrumentation_to_string (enum jitter_vm_instrumentation i);

/* A struct containing the configuration-specific parameters of a VM.  This
   struct exists in only one constant instance per VM, and does not depend on
   initialisation functions.  It is convenient to be used, for example, in
   handling the command-line option --version . */
struct jitter_vm_configuration
{
  /* Identifier prefixes for the generated C code. */
  char *lower_case_prefix, *upper_case_prefix, *hash_prefix;

  /* How many fast registers per class this VM can have, as a maximum.  -1 means
     that there is no limit. */
  int max_fast_register_no_per_class;

  /* How many nonresidual literals we support, as a maximum; -1 means that there
     is no limit. */
  int max_nonresidual_literal_no;

  /* A textual description of the dispatching technique. */
  char *dispatch_human_readable;

  /* The kind of profiling instrumentation for this VM. */
  enum jitter_vm_instrumentation instrumentation;
};

/* Print the current VM configuration, as set by jitterc and CPP macros, to the
   given stream in a human-readable format. */
void
jitter_print_vm_configuration (FILE *f,
                               const struct jitter_vm_configuration *c);




/* VM internal implementation.
 * ************************************************************************** */

/* Everything from this point on is subject to change and not meant for the
   user. */

/* A struct defining the VM-specific attributes of a VM.  Each VM has its own
   unique instance of this, shared by every routine for the same VM and
   initialized by vmprefix_initialize in template code.
   This structure is used internally, and the user does not need to see it. */
struct jitter_vm
{
  /* Configuration-specific data for this VM. */
  const struct jitter_vm_configuration *configuration;

/* Threads or pointers to native code blocks of course don't exist with
   switch-dispatching. */
#ifndef JITTER_DISPATCH_SWITCH
  /* True iff threads appear to be valid: of non-negative size, sequential,
     non-overlapping. */
  bool threads_validated;

  // FIXME: add a comment per field.
  jitter_thread *threads;
  long *thread_sizes;

  /* The address of the symbol defined in the data location subsection as
     a const char * global.  See jitter-data-locations.h . */
  const char *data_locations;
#endif // #ifndef JITTER_DISPATCH_SWITCH

  const size_t *specialized_instruction_residual_arities;
  const unsigned long *specialized_instruction_label_bitmasks;

  /* This is NULL when using a dispatching model not needing the bitmask. */
  const unsigned long *specialized_instruction_fast_label_bitmasks;

#if defined (JITTER_HAVE_PATCH_IN)
  const struct jitter_patch_in_descriptor *patch_in_descriptors;
  size_t patch_in_descriptor_no;
  /* A patch-in table as defined in jitter/jitter-patch-in.h . */
  struct patch_in_table_entry *patch_in_table;
#endif // #if defined (JITTER_HAVE_PATCH_IN)

  /* How many defects were found across all the instructions.
     This field is unconditionally present, in order to make user code
     consulting this struct simpler; if no defects are possible this field is
     set to zero. */
  int defect_no;

  /* How many specialised instructions are defective.  Notice that the fields
     keeps track of instructions, not of defects; the number of defects may be
     higher when one instruction has multiple defects.
     Defined unconditionally: see above. */
  int defective_specialized_instruction_no;

  /* How many call-related specialised instructions are defective.
     Defined unconditionally: see above. */
  int defective_call_related_specialized_instruction_no;

  /* How many specialised instructions were replaced because of defects.  This
     may be higher than defective_instruction_no when some but not all
     call-related instructions are defective: in such cases *every*
     call-releated instruction is replaced, in order to keep the calling
     convention compatible across instructions.
     Defined unconditionally: see above. */
  int replacement_specialized_instruction_no;

  const bool *specialized_instruction_relocatables;
  const bool *specialized_instruction_callers;
  const bool *specialized_instruction_callees;
  const char * const *specialized_instruction_names;
  size_t specialized_instruction_no;

  struct jitter_hash_table *meta_instruction_string_hash;

  struct jitter_meta_instruction *meta_instructions;
  size_t meta_instruction_no;

  /* An array whose indices are specialised instruction opcodes, and whose
     elements are the corresponding unspecialised instructions opcodes -- or -1
     when there is no mapping mapping having */
  const int *specialized_instruction_to_unspecialized_instruction;

  /* Specific meta-instruction pointers for implicit instructions.
     VM-independent routine specialization relies on those, so they have to be
     accessible to the Jitter library, out of generated code*/
  const struct jitter_meta_instruction *exitvm_meta_instruction;
  const struct jitter_meta_instruction *unreachable_meta_instruction;

  /* The longest unspecialized/meta instruction name length, not mangled,
     without counting the final '\0' character.  Special specialized
     instruction, having no unspecialized counterparts, are ignored here. */
  size_t max_meta_instruction_name_length;

  /* A function returning a pointer to a constant register class descriptor
     given the register class character, or NULL if the character is not
     associated to any register class. */
  const struct jitter_register_class *
  (* register_class_character_to_register_class) (char c);

  /* Translate one or more unspecialized instructions starting from *ins into
     p->specialized_program by calling the appropriate
     vmprefix_add_specialized_instruction_* functions for the opcode and every
     argument, returning the number of unspecialized instructions covered by the
     one new specialized instruction which is being added.  The result is always
     1 or more -- more than 1 when a superinstruction is being recognized.  The
     actual function is machine-generated. */
  /* FIXME: the comment above has been obsolete for a long time (in the end I
     decided not to have superinstructions), and the API can be simplified.
     The result should be void, not int. */
  int (*specialize_instruction) (struct jitter_mutable_routine *p,
                                 const struct jitter_instruction *ins);

  /* Rewrite an instruction.  This points to the vmprefix_rewrite function declared
     in templates/vm.h and implemented in Jitter-generated code. */
  void (*rewrite) (struct jitter_mutable_routine *p);

  /* A linked list of all the existing states for this VM. */
  struct jitter_list_header states;
};


#endif // #ifndef JITTER_VM_H_
