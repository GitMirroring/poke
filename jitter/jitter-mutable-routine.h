/* Jitter: VM-independent mutable routine data structures: header.

   Copyright (C) 2016, 2017, 2018, 2019, 2022 Luca Saiu
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


#ifndef JITTER_MUTABLE_ROUTINE_H_
#define JITTER_MUTABLE_ROUTINE_H_

#include <jitter/jitter.h>
#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-hash.h>
#include <jitter/jitter-instruction.h>
#include <jitter/jitter-print.h>


/* Routine data structures.
 * ************************************************************************** */

// FIXME: comment.
enum jitter_mutable_routine_stage
  {
    jitter_routine_stage_unspecialized,
    jitter_routine_stage_specialized,
    jitter_routine_stage_replicated
  };

/* A descriptor for a replicated block of code. */
struct jitter_replicated_block
{
  /* The opcode of the specialized instruction the block is a translation of.
     This is actually an enum vmprefix_specialized_instruction_opcode , but
     since the fixnum type is wide enough to represent it on any VM we can
     define this once and for all. */
  //enum vmprefix_specialized_instruction_opcode specialized_opcode;
  jitter_uint specialized_opcode;

  /* A pointer to the beginning ot the native code which is the translation of
     one specific instance of a specialized instruction; the native code may
     also include (at the beginning) some instructions to load residual
     arguments.
     The pointer is NULL as long as the code has not been replicated.
     When non-NULL this field points into the same native_code block pointed to
     by a program, so the referred memory must not be freed separately. */
  char *native_code;

  /* The native code size, in bytes.  This is not necessarily the same as the
     specialized_opcode-th element of vmprefix_thread_sizes, again because of added
     code to load residuals.
     The size is 0 as long as the block has not been replicated. */
  size_t native_code_size;
};

/* Options selectively applied to a Jittery mutable routine, to be selected by
   the user.
   This data structure, like a program data structure, should be considered an
   opaque abstract type, and the user should only use the functions declared
   below to set options. */
struct jitter_mutable_routine_options
{
  /* Non-false if the other options can still be changed.  This is true at
     initialization, and becomes false as soon as the user appends the first
     label or instruction. */
  bool can_change;

  /* False if specialization is allowed to generate specialized instructions
     using fast registers, as would be normal in production runs.  If non-false,
     whenever a register parameter is appended to an instruction, the actual
     register index to be used is changed to be equal to the given index summed
     to the number of fast registers in the class.
     This option is designed for benchmarking, in order to compare with
     alternative VMs which should behave like a Jittery VM with this option on,
     up to slow register index shifting (never needed in a Jittery VM). */
  bool slow_registers_only;

  /* False is specialization is allowed to generate specialized instructions
     with non-residual immediate arguments, as would be normal in production
     runs.  If non-false always residualize literal arguments (except,
     currently, for fast labels).
     This option is designed for benchmarking, in order to compare with
     alternative VMs which should behave like a Jittery VM with this option
     on, up to fast branches (only available, and faster, in a Jittery VM). */
  bool slow_literals_only;

  /* If non-false then automatically add a final "exitvm" instruction at the end
     of each VM routine; otherwise add a final "unreachable" instruction
     instead, which saves memory and makes replicated code smaller, but assumes
     that the unreachable instruction is actually unreachable.  This is true by
     default. */
  bool add_final_exitvm;

  /* If non-false then enable optimization rewriting.  The option is designed
     for debugging of rewrite rules and for benchmarking, particularly when
     comparing a Jittery VM with an alternative not supporting rewriting.
     This is true by default.*/
  bool optimization_rewriting;
};

/* This is enough for declaring pointer to structs of this type here.  See
   jitter-specialize.h for an actual definition, and a description of the
   idea. */
struct jitter_executable_routine;

/* The internal representation of a routine being edited by the user.  This
   should be considered an abstract type, as the internal structure is subject
   to change. */
struct jitter_mutable_routine
{
  /* The mutable routine stage at the present time. */
  enum jitter_mutable_routine_stage stage;

  /* The options applying to this routine.  This field is initialized to default
     values at routine initialization, and should only be updated indirectly
     thru the functions declared in the "Mutable routine options" section below. */
  struct jitter_mutable_routine_options options;

  /* A dynamic array containing struct jitter_instruction * elements.  This is
     filled by parsing or by initialization with unspecialized instructions.
     The buffer contains pointers rather than directly instructions to make
     rewriting easier. */
  struct jitter_dynamic_buffer instructions;

  /* The opaque label to be generated as fresh at the next request. */
  jitter_label next_unused_opaque_label;

  /* A map associating symbolic label names to opaque labels.  There can be
     opaque labels without any symbolic names, but every symbolic name in use
     must always be associated to an opaque label.  The datum is handled as a
     pointer by the hash API, but here it should just be cast to and from an
     jitter_label .  Symbolic label names are copied into the map
     whenever a binding is added, and do not share memory with user data
     structures. */
  struct jitter_hash_table label_name_to_opaque_label;

  /* A dynamic array containing jitter_int elements.  Each array index
     represents an opaque label, and its associated array element the
     unspecialized instruction index where that label leads, or -1 if the
     label is unresolved.  It is not an error to have unresolved labels,
     even late after label resolution, as long as such labels are never
     used in instruction parameters.
     Rewriting poses no particular problem with respect to this mapping:
     once a label is assigned an instruction index, the *following*
     instructions may be rewritten, but not the previous one, maintaining
     the existing mapping valid. */
  struct jitter_dynamic_buffer opaque_label_to_instruction_index;

  /* A pointer to the instruction currently being initialized within
     instructions. */
  struct jitter_instruction *current_instruction;

  /* The next uninitialized parameter of *current_instruction . */
  struct jitter_parameter *next_uninitialized_parameter;

  /* The next expected parameter type, which *next_uninitialized_parameter will
     need to match. */
  const struct jitter_meta_instruction_parameter_type *
  next_expected_parameter_type;

  /* How many parameters are we still expecting before completing the
     instruction which is currently being appended.  If no instruction is
     incomplete, including right after initialization, the field is zero. */
  int expected_parameter_no;

  /* How many complete instructions at the end of the routine are candidate for
     rewriting.  This starts at zero, and gets reset every time a label is
     added. */
  size_t rewritable_instruction_no;

  /* A pointer to a malloced array of booleans, having the same size as the
     number of instructions in the routine.  Each element of the array is true
     if and only if the corresponding routine instruction is a jump target.
     This is allocated at specialization time, and NULL before. */
  bool *jump_targets;

  /* We need to map unspecialized instruction indices into specialized
     instruction offsets within specialized_routine, in chars from the
     beginning.  This is needed because specialized instructions have variable
     sizes, and label arguments need to be backpatched in to point to
     instruction beginning addresses rather than indices.
     Notice that this array is indexed by unspecialized instruction indices, and
     that not every possible index is valid: if an unspecialized instruction
     interval is specialized into one superinstruction then only its first
     unspecialized instruction in the interval has a corresponding
     superinstruction. */
  jitter_int *instruction_index_to_specialized_instruction_offset;

  /* The sequence of replicated blocks in the specialized routine, in order.
     Each element is a struct jitter_replicated_block .  This is only used with
     replication, and for disassembly. */
  struct jitter_dynamic_buffer replicated_blocks;

  /* Generated native code and its size, only used with replication [FIXME:
     generalize later, after I introduce alternatives even more sophisticated
     than replication, which will probably still need this]. */
  char *native_code;
  size_t native_code_size;

  /* The specialized routine.  Each element is a union jitter_specialized_word . */
  struct jitter_dynamic_buffer specialized_routine;

  /* The indices of label parameters within specialized_instructions as
     jitter_int's, to be patched at the end of specialization.  While
     specialized_instructions is being added to, label parameters are
     instruction indices; after patching they become pointers to the beginning
     of instructions within specialized_instructions. */
  struct jitter_dynamic_buffer specialized_label_indices;

  /* The number of slow registers needed *per class* in order to run this
     routine; or, alternatively, the maximum number of slow registers needed to
     run this routine in any class.  Slow registers are always added in the same
     number for all classes, even if each class may have a different number of
     fast registers. */
  jitter_int slow_register_per_class_no;

  /* The executable version of this routine, if one exists.  NULL before the
     executable routine is made. */
  struct jitter_executable_routine *executable_routine;

  /* A pointer to the VM-specific definitions of the VM for this routine. */
  const struct jitter_vm *vm;
};

/* Return the number of unspecialized instruction in the pointed mutable routine
   also counting the one which is currently incomplete, if any. */
size_t
jitter_mutable_routine_instruction_no (const struct jitter_mutable_routine *p)
  __attribute__ ((pure));

/* Return a freshly-allocated mutable routine, empty program with an empty vm
   field. */
struct jitter_mutable_routine*
jitter_make_mutable_routine (const struct jitter_vm *vm)
  __attribute__ ((nonnull (1), returns_nonnull));

/* Destroy the pointed mutable routine, if any.  Do nothing if p is NULL. */
void
jitter_destroy_mutable_routine (struct jitter_mutable_routine *p);




/* Mutable routine options.
 * ************************************************************************** */

/* The functions in this section set some user options for an existing program.
   Such settings are only possible on an "empty" mutable routine, before the
   first instruction or label is appended to it. */

/* Set the slow_registers_only option to the given value in the pointed mutable
   routine.  Fail fatally if the option is no longer settable. */
void
jitter_set_mutable_routine_option_slow_registers_only
   (struct jitter_mutable_routine *p, bool option)
  __attribute__ ((nonnull (1)));

/* Set the slow_registers_only option to the given value in the pointed mutable
   routine.  Fail fatally if the option is no longer settable. */
void
jitter_set_mutable_routine_option_slow_literals_only
   (struct jitter_mutable_routine *p, bool option)
  __attribute__ ((nonnull (1)));

/* A convenience function behaving in an equivalent way to a call to
   jitter_set_mutable_routine_option_slow_registers_only followed by a call to
   jitter_set_mutable_routine_option_slow_literals_only on the same mutable
   routine with the same option value. */
void
jitter_set_mutable_routine_option_slow_literals_and_registers_only
   (struct jitter_mutable_routine *p, bool option)
  __attribute__ ((nonnull (1)));

/* Set the add_final_exitvm option to the given value in the pointed mutable
   routine.  Fail fatally if the option is no longer settable. */
void
jitter_set_mutable_routine_option_add_final_exitvm
   (struct jitter_mutable_routine *p, bool option)
  __attribute__ ((nonnull (1)));

/* Set the optimization_rewriting option to the given value in the pointed
   mutable routine.  Fail fatally if the option is no longer settable. */
void
jitter_set_mutable_routine_option_optimization_rewriting
   (struct jitter_mutable_routine *p, bool option)
  __attribute__ ((nonnull (1)));




/* Label handing.
 * ************************************************************************** */

/* Return a fresh label for the pointed mutable routine, without an associated
   symbolic name.  The caller may later use the label in an instruction argument
   or associate it to a program point. */
jitter_label
jitter_fresh_label (struct jitter_mutable_routine *p)
  __attribute__ ((nonnull (1)));

/* Return a label for the pointed mutable routine, associated to the given
   symbolic name.  If the symbolic name is new for the program, associate the
   label to an internally-allocated copy of it; if the symbolic name is already
   known, return the label already associated to it. */
jitter_label
jitter_symbolic_label (struct jitter_mutable_routine *p,
                       const char *symbolic_name)
  __attribute__ ((nonnull (1)));




/* Mutable routine construction API: unsafe functions.
 * ************************************************************************** */

/* The functions declared in this section are unsafe, in the sense that they
   fail fatally in case of syntax errors: for example, appending a label when an
   instruction being emitted is not complete yet. */

/* Update the pointed mutable routine, adding the given label before the
   instruction which is coming next.
   When this function is called the previous instruction, if any, must have been
   completed. */
void
jitter_mutable_routine_append_label (struct jitter_mutable_routine *p,
                                     jitter_label label)
  __attribute__ ((nonnull (1)));

/* Update the pointed mutable routine, adding a label with the given name before
   the instruction which is coming next, and return the label.
   When this function is called the previous instruction, if any, must have been
   completed. */
jitter_label
jitter_mutable_routine_append_symbolic_label (struct jitter_mutable_routine *p,
                                              const char *label_name)
  __attribute__ ((nonnull (1, 2)));

/* Update the pointed mutable routine, beginning a new instruction with the
   given name, to be looked up in the meta-instruction hash table; the
   instruction parameters, if any, have to be supplied with calls to
   vmprefix_mutable_routine_append_*_parameter .  When this function is called
   the previous instruction, if any, must have been completed.

   This function is convenient but requires a hash lookup on the name.
   jitter_mutable_routine_append_meta_instruction is faster; see its comment for
   the recommended way to use it. */
void
jitter_mutable_routine_append_instruction_name (struct jitter_mutable_routine *p,
                                                const char *instruction_name)
  __attribute__ ((nonnull (1, 2)));

/* Update the pointed mutable routine, beginning a new instruction with the
   given unspecialized opcode, from the given array of meta-instructions.  When
   this function is called the previous instruction, if any, must have been
   completed.  The unspecialized_opcode argument is meant to be one case of enum
   vmprefix_meta_instruction_id ; the enum name depends on the VM, which is why
   this VM-independent prototype uses a generic integer type.

   The recommended way of using this function is via the VM-specific macro
   wrapper VMPREFIX_MUTABLE_ROUTINE_APPEND_INSTRUCTION_ID , which lets the user
   ignore the meta-instruction array and the number of meta-instructions.

   This function is flexible enough to use from user code where the instruction
   to be added is the result of some previous computation; the macro API
   recommended in the comment before jitter_append_meta_instruction doesn't fit
   the problem as the instruction name would need to be supplied as a literal,
   and the string-based API above would require a useless run-time hash
   lookup. */
void
jitter_mutable_routine_append_instruction_id
   (struct jitter_mutable_routine *p,
    const struct jitter_meta_instruction * const mis,
    size_t meta_instruction_no,
    unsigned unspecialized_opcode)
  __attribute__ ((nonnull (1, 2)));

/* Update the pointed mutable routine, beginning a new instruction which is an
   instance of the pointed meta-instruction; the instruction parameters, if any,
   have to be supplied with calls to vmprefix_append_*_parameter .  When this
   function is called the previous instruction, if any, must have been
   completed.

   Supplying a meta-instruction pointer from a user program is very
   inconvenient.  The recommended way of using this function, which is more
   efficient than jitter_append_instruction_name, is thru the machine-generated
   macro [VMPREFIX]_APPEND_INSTRUCTION. */
void
jitter_mutable_routine_append_meta_instruction
   (struct jitter_mutable_routine *p,
    const struct jitter_meta_instruction * const mi)
  __attribute__ ((nonnull (1, 2)));

/* Update the given program, adding one more parameter (left-to-right) to the
   unspecialized instruction currently being described.  Fail fatally if there
   are no instructions yet, or the last added instruction is already
   complete.  If appending a symbolic-label argument, return its opaque
   label identifier.
   Notice that the macro [VMPREFIX]_APPEND_REGISTER_PARAMETER provides a more
   convenient way of adding a register parameter. */
void
jitter_mutable_routine_append_literal_parameter
   (struct jitter_mutable_routine *p,
    union jitter_word immediate)
  __attribute__((nonnull (1)));
void
jitter_mutable_routine_append_signed_literal_parameter
   (struct jitter_mutable_routine *p,
    jitter_int immediate)
  __attribute__((nonnull (1)));
void
jitter_mutable_routine_append_unsigned_literal_parameter
   (struct jitter_mutable_routine *p,
    jitter_uint immediate)
  __attribute__((nonnull (1)));
void
jitter_mutable_routine_append_pointer_literal_parameter
   (struct jitter_mutable_routine *p,
    void *immediate)
  __attribute__((nonnull (1)));
void
jitter_mutable_routine_append_register_parameter
   (struct jitter_mutable_routine *p,
    const struct jitter_register_class *c,
    jitter_register_index register_index)
  __attribute__((nonnull (1, 2)));
void
jitter_mutable_routine_append_symbolic_register_parameter
   (struct jitter_mutable_routine *p,
    char register_class_as_char,
    jitter_register_index register_index)
  __attribute__((nonnull (1)));
jitter_label
jitter_mutable_routine_append_symbolic_label_parameter
   (struct jitter_mutable_routine *p,
    const char *label_name)
  __attribute__((nonnull (1, 2)));
void
jitter_mutable_routine_append_label_parameter
   (struct jitter_mutable_routine *p,
    jitter_label label)
  __attribute__((nonnull (1)));




/* Mutable routine parsing and construction result.
 * ************************************************************************** */

/* The status returned by safe routine parsing and editing functions.  When
   different from success (zero) the routine is not modified. */
enum jitter_routine_edit_status
  {
    /* Editing succeeded. */
    jitter_routine_edit_status_success = 0,

    /* A label is being defined more than once. */
    jitter_routine_edit_status_label_defined_twice,

    /* An instruction name or id is invalid. */
    jitter_routine_edit_status_invalid_instruction,

    /* A register cited as a parameter is invalid, because of either its class
       or its index. */
    jitter_routine_edit_status_invalid_register,

    /* A register cited as a parameter does not have the expected class. */
    jitter_routine_edit_status_register_class_mismatch,

    /* A register cited as a has a non-existing class. */
    jitter_routine_edit_status_nonexisting_register_class,

    /* A parameter of one kind was given when another was expected: for example
       a label was given when the parameter needed to be a register. */
    jitter_routine_edit_status_invalid_parameter_kind,

    /* An instruction was given more parameters than it takes. */
    jitter_routine_edit_status_too_many_parameters,

    /* An instruction was left incomplete at the end. */
    jitter_routine_edit_status_last_instruction_incomplete,

    /* Some other parse error happened.  This status is not returned by the
       functions in this section but it can happen when parsing s routine in
       textual form, for example because of mismatched parentheses. */
    jitter_routine_edit_status_other_parse_error
  };

/* Routine parsing functions return a pointer to this function, or NULL in
   case of success.  It is the user's responsiblity to destroy such returned
   structs with a call to jitter_routine_parse_error_destroy. */
struct jitter_routine_parse_error
{
  /* The parser status to return.  Notice that in case of error this status
     represents the first problem encountered: parsing stops at the first
     error. */
  enum jitter_routine_edit_status status;

  /* The input file name, or some textual representation of it if the input is
     not an actual file or if its name is unknown.  This is malloc-allocated
     but the user should not rely on this, and instead just use
     jitter_routine_parse_error_destroy to destroy the entire struct and its
     contents. */
  char *file_name;

  /* Line number of the error expressed in status, or -1 in case of
     success. */
  int error_line_no;

  /* The text of the problematic token, malloc-allocated, in case of error
     (but the user should not rely on this, and rely on
     jitter_routine_parse_error_destroy to destroy the entire struct and its
     contents). */
  char *error_token_text;
};

/* See the comment before struct jitter_routine_parse_error . */
void jitter_routine_parse_error_destroy (struct jitter_routine_parse_error *e)
  __attribute__ ((nonnull (1)));

/* Given a mutable-routine edit status return its text representation. */
const char*
jitter_routine_edit_status_to_string (enum jitter_routine_edit_status s);




/* Mutable routine construction API: safe functions.
 * ************************************************************************** */

/* The functions declared in this section are safe, in the sense that in case of
   parse error they return a non-zero result and do not alter the routine.
   Otherwise each function declared here behaves identically to its counterpart
   declared in the "unsafe" section above.
   Of course the functions in this section are much less convenient to use;
   notice that the VM routine parser needs to use these internally, in order
   to handle some errors without failing fatally.

   These function still fail fatally when an attempt is made to edit an
   already-specialised routine: that is a programmer error, always
   preventable. */

/* Each function declared here behaves identically to its counterpart declared
   in the "unsafe" section above. */
enum jitter_routine_edit_status
jitter_mutable_routine_append_label_safe (struct jitter_mutable_routine *p,
                                          jitter_label label)
  __attribute__ ((nonnull (1)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_symbolic_label_safe
   (jitter_label *result, /* This is allowed to be NULL. */
    struct jitter_mutable_routine *p,
    const char *label_name)
  __attribute__ ((nonnull (2, 3)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_instruction_name_safe
   (struct jitter_mutable_routine *p,
    const char *instruction_name)
  __attribute__ ((nonnull (1, 2)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_instruction_id_safe
   (struct jitter_mutable_routine *p,
    const struct jitter_meta_instruction * const mis,
    size_t meta_instruction_no,
    unsigned unspecialized_opcode)
  __attribute__ ((nonnull (1, 2)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_meta_instruction_safe
   (struct jitter_mutable_routine *p,
    const struct jitter_meta_instruction * const mi)
  __attribute__ ((nonnull (1, 2)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_literal_parameter_safe
   (struct jitter_mutable_routine *p,
    union jitter_word immediate)
  __attribute__((nonnull (1)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_signed_literal_parameter_safe
   (struct jitter_mutable_routine *p,
    jitter_int immediate)
  __attribute__((nonnull (1)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_unsigned_literal_parameter_safe
   (struct jitter_mutable_routine *p,
    jitter_uint immediate)
  __attribute__((nonnull (1)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_pointer_literal_parameter_safe
   (struct jitter_mutable_routine *p,
    void *immediate)
  __attribute__((nonnull (1)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_register_parameter_safe
   (struct jitter_mutable_routine *p,
    const struct jitter_register_class *c,
    jitter_register_index register_index)
  __attribute__((nonnull (1, 2)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_symbolic_register_parameter_safe
   (struct jitter_mutable_routine *p,
    char register_class_as_char,
    jitter_register_index register_index)
  __attribute__((nonnull (1)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_symbolic_label_parameter_safe
   (jitter_label *result_or_NULL,
    struct jitter_mutable_routine *p,
    const char *label_name)
  __attribute__((nonnull (2, 3)));
enum jitter_routine_edit_status
jitter_mutable_routine_append_label_parameter_safe
   (struct jitter_mutable_routine *p,
    jitter_label label)
  __attribute__((nonnull (1)));




/* Lower-level program-construction API.
 * ************************************************************************** */

/* This is used internally for rewriting. */

/* Append the pointed instruction (without cloning it) to the pointed mutable
   routine.  When this function is called the previous instruction, if any, must
   have been completed. */
void
jitter_mutable_routine_append_instruction (struct jitter_mutable_routine *p,
                                           const struct jitter_instruction *ip)
  __attribute__ ((nonnull (1, 2)));

/* Append a copy of the given parameter, without destroying it, to the current
   instruction of the pointed mutable_routine.

   Rationale: making a copy is more convenient in the current implementation
   than replacing an existing data structure, as the structure is pre-allocated.
   The fact that this function is called in machine-generated code makes
   convenience only a secondary consideration.  Moreover rewrite rules may copy
   the same parameter more than once from an instruction to be rewritten into a
   template. */
void
jitter_mutable_routine_append_parameter_copy (struct jitter_mutable_routine *p,
                                              const struct jitter_parameter *pp)
  __attribute__ ((nonnull (1, 2)));




/* Unspecialized program printer.
 * ************************************************************************** */

/* Print a readable representation of the pointed mutable routine to the
   given print context.
   
   The output uses the following class names (see jitter/jitter-print.h), with
   "vmprefix" replaced by the lower-case name of the VM for the mutable routine:
   - "vmprefix-punctuation";
   - "vmprefix-label";
   - "vmprefix-instruction";
   - "vmprefix-register";
   - "vmprefix-number";
   - "vmprefix-invalid".
   Some VMs will print instruction literals though custom printers, which may
   use other classes. */
void
jitter_mutable_routine_print (jitter_print_context ctx,
                              const struct jitter_mutable_routine *p);




/* Jump target computation on unspecialized routines.
 * ************************************************************************** */

/* Given a routine return a pointer to a new array of booleans, allocated with
   malloc, having the same size as the number of instructions in the routine.
   Each element of the array is true if and only if the corresponding routine
   instruction is a jump target.

   This is used at specialization time to compute the jump_targets field of a
   struct jitter_mutable_routine , but also elsewhere, for printing unspecialized routines
   -- therefore it cannot be a static function.

   This function is used internally, and the user does not need to see it. */
bool*
jitter_mutable_routine_jump_targets (const struct jitter_mutable_routine *p)
  __attribute__ ((returns_nonnull, nonnull (1)));




/* Label resolution in unspecialized routines.
 * ************************************************************************** */

/* Resolve label arguments in unspecialized instruction parameters, replacing
   opaque labels with unspecialized instruction indices.  After this is done
   instruction parameters refer labels as unspecialized instruction indices.
   Fail fatally if any referred label is still undefined, or if the routine is
   not unspecialized.

   This function is used internally, and the user does not need to see it. */
void
jitter_mutable_routine_resolve_labels (struct jitter_mutable_routine *p)
  __attribute__ ((nonnull (1)));

#endif // #ifndef JITTER_MUTABLE_ROUTINE_H_
