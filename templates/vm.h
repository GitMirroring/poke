/* VM library: main header file.

   Copyright (C) 2016, 2017, 2018, 2019, 2020, 2021 Luca Saiu
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


/* Generated file warning.
 * ************************************************************************** */

/* Unless this file is named exactly "vm.h" , without any prefix, you are
   looking at a machine-generated derived file.  The original source is the vm.h
   template from Jitter, with added code implementing the vmprefix VM. */




/* This multiple-inclusion guard is opened here in the template, and will be
   closed at the end of the generated code.  It is normal to find no matching
   #endif in the template file.  */
#ifndef VMPREFIX_VM_H_
#define VMPREFIX_VM_H_


/* This is the main VM header to use from hand-written code.
 * ************************************************************************** */

#include <jitter/jitter-early-header.h>

#include <stdio.h>
#include <stdbool.h>

#include <jitter/jitter.h>
#include <jitter/jitter-hash.h>
#include <jitter/jitter-stack.h>
#include <jitter/jitter-instruction.h>
#include <jitter/jitter-mutable-routine.h>
#include <jitter/jitter-print.h>
#include <jitter/jitter-routine.h>
//#include <jitter/jitter-specialize.h> // FIXME: what about only declaring jitter_specialize in another header, and not including this?
#include <jitter/jitter-disassemble.h>
#include <jitter/jitter-vm.h>
#include <jitter/jitter-profile.h>
#include <jitter/jitter-data-locations.h>
#include <jitter/jitter-arithmetic.h>
#include <jitter/jitter-bitwise.h>
#include <jitter/jitter-signals.h>
#include <jitter/jitter-list.h>




/* Initialization and finalization.
 * ************************************************************************** */

/* Initialize the runtime state for the vmprefix VM.  This needs to be called
   before using VM routines or VM states in any way. */
void
vmprefix_initialize (void);

/* Finalize the runtime state, freeing some resources.  After calling this no
   use of VM routines or states is allowed.  It is possible to re-initialize
   after finalizing; these later re-initializations might be more efficient than
   the first initialization. */
void
vmprefix_finalize (void);




/* State data structure initialization and finalization.
 * ************************************************************************** */

/* The machine state is separated into the backing and the more compact runtime
   data structures, to be allocated in registers as far as possible.  These are
   just a forward-declarations: the actual definitions are machine-generated. */
struct vmprefix_state_backing;
struct vmprefix_state_runtime;

/* A data structure containing both the backing and the runtime state.  This is
   a forward-declaration: the actual definition will come after both are
   defined. */
struct vmprefix_state;

/* Initialize the pointed VM state data structure, or fail fatally.  The
   function definition is machine-generated, even if it may include user code.
   The state backing and runtime are initialized at the same time, and in fact
   the distinction between them is invisible to the VM user.
   The version not specifying a given number of slow registers per class
   sets slow registers to be initially zero. */
void
vmprefix_state_initialize (struct vmprefix_state *state)
  __attribute__ ((nonnull (1)));
void
vmprefix_state_initialize_with_slow_registers (struct vmprefix_state *state,
                                               jitter_uint
                                               slow_register_no_per_class)
  __attribute__ ((nonnull (1)));

/* Finalize the pointed VM state data structure, or fail fatally.  The function
   definition is machine-generated, even if it may include user code.  The state
   backing and runtime are finalized at the same time. */
void
vmprefix_state_finalize (struct vmprefix_state *state)
  __attribute__ ((nonnull (1)));

/* The make/destroy counterparts of the initialize/finalize functions above. */
struct vmprefix_state *
vmprefix_state_make (void)
  __attribute__ ((returns_nonnull));
struct vmprefix_state *
vmprefix_state_make_with_slow_registers (jitter_uint slow_register_no_per_class)
  __attribute__ ((returns_nonnull));
void
vmprefix_state_destroy (struct vmprefix_state *state)
  __attribute__ ((nonnull (1)));

/* Reset the pointed VM state, restoring its initial content.  This is cheaper
   than finalising and re-initialising a state. */
void
vmprefix_state_reset (struct vmprefix_state *state)
  __attribute__ ((nonnull (1)));




/* State data structure: iteration.
 * ************************************************************************** */

/* The header of a doubly-linked list linking every state for the vmprefix VM
   together.  This global is automatically wrapped, and therefore also
   accessible from VM instruction code. */
extern struct jitter_list_header * const
vmprefix_states;

/* A pointer to the current state, only accessible from VM code.  This is usable
   for pointer comparison when iterating over states. */
#define VMPREFIX_OWN_STATE                           \
  ((struct vmprefix_state *) jitter_original_state)

/* Given an l-value of type struct vmprefix_state * (usually a variable name)
   expand to a for loop statement iterating over every existing vmprefix state
   using the l-value as iteration variable.  The expansion will execute the
   statement immediately following the macro call with the l-value in scope;
   in order words the loop body is not a macro argument, but follows the macro
   use.
   The l-value may be evaluated an unspecified number of times.
   This macro is safe to use within VM instruction code.
   For example:
     struct vmprefix_state *s;
     VMPREFIX_FOR_EACH_STATE (s)
       printf ("This is a state: %p\n", s); // (but printf unsafe in VM code) */
#define VMPREFIX_FOR_EACH_STATE(jitter_state_iteration_lvalue)     \
  for ((jitter_state_iteration_lvalue)                             \
          = vmprefix_states->first;                                \
       (jitter_state_iteration_lvalue)                             \
          != NULL;                                                 \
       (jitter_state_iteration_lvalue)                             \
         = (jitter_state_iteration_lvalue)->links.next)            \
    /* Here comes the body supplied by the user: no semicolon. */




/* Mutable routine initialization.
 * ************************************************************************** */

/* Return a freshly-allocated empty mutable routine for the vmprefix VM. */
struct jitter_mutable_routine*
vmprefix_make_mutable_routine (void)
  __attribute__ ((returns_nonnull));

/* Mutable routine finalization is actually VM-independent, but a definition of
   vmprefix_destroy_mutable_routine is provided below as a macro, for cosmetic
   reasons. */


/* Mutable routines: code generation C API.
 * ************************************************************************** */

/* This is the preferred way of adding a new VM instruction to a pointed
   routine, more efficient than vmprefix_mutable_routine_append_instruction_name
   even if only usable when the VM instruction opcode is known at compile time.
   The unspecialized instruction name must be explicitly mangled by the user as
   per the rules in jitterc_mangle.c .  For example an instruction named foo_bar
   can be added to the routine pointed by p with any one of
     vmprefix_mutable_routine_append_instruction_name (p, "foo_bar");
   ,
     VMPREFIX_MUTABLE_ROUTINE_APPEND_INSTRUCTION (p, foo_ubar);
   , and
     VMPREFIX_MUTABLE_ROUTINE_APPEND_INSTRUCTION_ID 
        (p, vmprefix_meta_instruction_id_foo_ubar);
   .
   The string "foo_bar" is not mangled, but the token foo_ubar is. */
#define VMPREFIX_MUTABLE_ROUTINE_APPEND_INSTRUCTION(                 \
          routine_p, instruction_mangled_name_root)                  \
  do                                                                 \
    {                                                                \
      jitter_mutable_routine_append_meta_instruction                 \
         ((routine_p),                                               \
          vmprefix_meta_instructions                                 \
          + JITTER_CONCATENATE_TWO(vmprefix_meta_instruction_id_,    \
                                   instruction_mangled_name_root));  \
    }                                                                \
  while (false)

/* Append the unspecialized instruction whose id is given to the pointed routine.
   The id must be a case of enum vmprefix_meta_instruction_id ; such cases have
   a name starting with vmprefix_meta_instruction_id_ .
   This is slightly less convenient to use than VMPREFIX_MUTABLE_ROUTINE_APPEND_INSTRUCTION
   but more general, as the instruction id is allowed to be a non-constant C
   expression. */
#define VMPREFIX_MUTABLE_ROUTINE_APPEND_INSTRUCTION_ID(_jitter_routine_p,       \
                                                       _jitter_instruction_id)  \
  do                                                                            \
    {                                                                           \
      jitter_mutable_routine_append_instruction_id                              \
         ((_jitter_routine_p),                                                  \
          vmprefix_meta_instructions,                                           \
          VMPREFIX_META_INSTRUCTION_NO,                                         \
          (_jitter_instruction_id));                                            \
    }                                                                           \
  while (false)

/* This is the preferred way of appending a register argument to the instruction
   being added to the pointed routine, more convenient than directly using
   vmprefix_mutable_routine_append_register_id_parameter , even if only usable
   when the register class is known at compile time.  Here the register class is
   only provided as a letter, but both the routine pointer and the register
   index are arbitrary C expressions.
   For example, in
     VMPREFIX_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER (p, r,
                                                         variable_to_index (x));
   the second macro argument "r" represents the register class named "r", and
   not the value of a variable named r. */
#define VMPREFIX_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER(routine_p,     \
                                                           class_letter,  \
                                                           index)         \
  do                                                                      \
    {                                                                     \
      vmprefix_mutable_routine_append_register_parameter                  \
         ((routine_p),                                                    \
          & JITTER_CONCATENATE_TWO(vmprefix_register_class_,              \
                                   class_letter),                         \
          (index));                                                       \
    }                                                                     \
  while (false)




/* Routine unified API: initialization.
 * ************************************************************************** */

/* See the comments above in "Mutable routines: initialization", and the
   implementation of the unified routine API in <jitter/jitter-routine.h> . */

#define vmprefix_make_routine vmprefix_make_mutable_routine




/* Routine unified API: code generation C API.
 * ************************************************************************** */

/* See the comments above in "Mutable routines: code generation C API". */

#define VMPREFIX_ROUTINE_APPEND_INSTRUCTION  \
  VMPREFIX_MUTABLE_ROUTINE_APPEND_INSTRUCTION
#define VMPREFIX_ROUTINE_APPEND_INSTRUCTION_ID  \
  VMPREFIX_MUTABLE_ROUTINE_APPEND_INSTRUCTION_ID
#define VMPREFIX_ROUTINE_APPEND_REGISTER_PARAMETER  \
  VMPREFIX_MUTABLE_ROUTINE_APPEND_REGISTER_PARAMETER




/* Array: special-purpose data.
 * ************************************************************************** */

/* The Array is a convenient place to store special-purpose data, accessible in
   an efficient way from a VM routine.
   Every item in special-purpose data is thread-local. */

/* The special-purpose data struct.  Every Array contains one of these at unbiased
   offset VMPREFIX_SPECIAL_PURPOSE_STATE_DATA_UNBIASED_OFFSET from the unbiased
   beginning of the array.
   This entire struct is aligned to at least sizeof (jitter_int) bytes.  The
   entire struct is meant to be always accessed through a pointer-to-volatile,
   as its content may be altered from signal handlers and from different
   threads.  In particualar the user should use the macro
     VMPREFIX_ARRAY_TO_SPECIAL_PURPOSE_STATE_DATA
   defined below and the macros defined from it as accessors.
   VM code accessing special-purpose data for its own state should use
     VMPREFIX_SPECIAL_PURPOSE_STATE_DATA
   and the macros defined from it. */
struct jitter_special_purpose_state_data
{
  /* Notification fields.
   * ***************************************************************** */

  /* This is a Boolean flag, held as a word-sized datum so as to ensure
     atomicity in access.  It is also aligned to at least sizeof (jitter_int)
     bytes.
     Non-zero means that there is at least one notification pending, zero means
     that there are no notifications.  The flag specifies no other details: it
     is meant to be fast to check, with detailed information about each pending
     notification available elsewhere.
     It is the receiver's responsibility to periodically poll for notifications
     in application-specific "safe-points":
     A check can be inserted, for example, in all of these program points:
     a) at every backward branch;
     b) at every procedure entry;
     c) right after a call to each blocking primitive (as long as primitives
       can be interrupted).
     Safe-point checks are designed to be short and fast in the common case.  In
     the common case no action is required, and the VM routine should simply
     fall through.  If an action is required then control should branch off to a
     handler, where the user may implement the required behavior.
     It is mandatory that, as long as notifications can arrive, this field
     is reset to zero (when handling pending notifications) only by a thread
     running VM code in the state containing this struct.
     Other threads are allowed to set this to non-zero, in order to send a
     notification.  */
  jitter_int pending_notifications;

  /* Information about pending signal notifications.  If any signal is pending
     then pending_notifications must also be set, so that a notification check
     can always just quickly check pending_notifications, and then look at more
     details (including in pending_signal_notifications) only in the rare case
     of pending_notifications being true. */
  struct jitter_signal_notification *pending_signal_notifications;


  /* Profiling instrumentation fields.
   * ***************************************************************** */
  struct jitter_profile_runtime profile_runtime;
};




/* The Array and volatility.
 * ************************************************************************** */

/* Some fields of The Array, seen from VM code, are meant to be volatile, since
   they can be set by signal handlers or by other threads.  However it is
   acceptable to not see such changes immediately after they occur (notifications
   will get delayed, but not lost) and always accessing such data through a
   volatile struct is suboptimal.

   Non-VM code does need a volatile qualifier.

   Advanced dispatches already need a trick using inline assembly to make the
   base pointer (a biased pointer to The Array beginning) appear to
   spontaneously change beween instruction.  That is sufficient to express the
   degree of volatility required for this purpose.
   Simple dispatches, on targets where inline assembly may not be available at
   all, will use an actual volatile qualifier. */
#if defined (JITTER_DISPATCH_SWITCH)               \
    || defined (JITTER_DISPATCH_DIRECT_THREADING)
# define VMPREFIX_ARRAY_VOLATILE_QUALIFIER volatile
#elif defined (JITTER_DISPATCH_MINIMAL_THREADING)  \
      || defined (JITTER_DISPATCH_NO_THREADING)
# define VMPREFIX_ARRAY_VOLATILE_QUALIFIER /* nothing */
#else
# error "unknown dispatch: this should not happen"
#endif /* dispatch conditional */




/* Array element access: residuals, transfers, slow registers, and more.
 * ************************************************************************** */

/* In order to cover a wider range of addresses with simple base + register
   addressing the base does not necessarily point to the beginning of the Array;
   instead the base points to the beginning of the Array plus JITTER_ARRAY_BIAS
   bytes.
   FIXME: define the bias as a value appropriate to each architecture.  I think
   I should just move the definition to jitter-machine.h and provide a default
   here, in case the definition is missing on some architecture. */

/* FIXME: Horrible, horrible, horrible temporary workaround!

   This is a temporary workaround, very ugly and fragile, to compensate
   a limitation in jitter-specialize.c , which I will need to rewrite anyway.
   The problem is that jitter-specialize.c patches snippets to load non-label
   residuals in a VM-independent way based only on slow-register/memory residual
   indices, which is incorrect.  By using this particular bias I am cancelling
   that error.
   Test case, on a machine having only one register residual and a VM having just
     one fast register:
     [luca@moore ~/repos/jitter/_build/native-gcc-9]$ Q=bin/uninspired--no-threading; make $Q && echo 'mov 2, %r1' | libtool --mode=execute valgrind $Q --disassemble - --print-locations
   If this bias is wrong the slow-register accesses in mov/nR/%rR will use two
   different offsets, one for reading and another for writing.  With this
   workaround they will be the same.
   Good, with workadound (biased offset 0x0 from the base in %rbx):
    # 0x4a43d38: mov/nR/%rR 0x2, 0x20 (21 bytes):
        0x0000000004effb30 41 bc 02 00 00 00    	movl   $0x2,%r12d
        0x0000000004effb36 48 c7 43 00 20 00 00 00 	movq   $0x20,0x0(%rbx)
        0x0000000004effb3e 48 8b 13             	movq   (%rbx),%rdx
        0x0000000004effb41 4c 89 24 13          	movq   %r12,(%rbx,%rdx,1)
   Bad, with JITTER_ARRAY_BIAS defined as zero: first write at 0x0(%rbx)
                                                then read at 0x10(%rbx):
    # 0x4a43d38: mov/nR/%rR 0x2, 0x30 (22 bytes):
        0x0000000004effb30 41 bc 02 00 00 00    	movl   $0x2,%r12d
        0x0000000004effb36 48 c7 43 00 30 00 00 00 	movq   $0x30,0x0(%rbx)
        0x0000000004effb3e 48 8b 53 10          	movq   0x10(%rbx),%rdx
        0x0000000004effb42 4c 89 24 13          	movq   %r12,(%rbx,%rdx,1) */
#define JITTER_ARRAY_BIAS \
  (sizeof (struct jitter_special_purpose_state_data))
//#define JITTER_ARRAY_BIAS //0//(((jitter_int) 1 << 15))//(((jitter_int) 1 << 31))//0//0//16//0

/* Array-based globals are not implemented yet.  For the purpose of computing
   Array offsets I will say they are zero. */
#define VMPREFIX_GLOBAL_NO 0

/* Transfer registers are not implemented yet.  For the purpose of computing
   Array offsets I will say they are zero. */
#define VMPREFIX_TRANSFER_REGISTER_NO 0

/* Define macros holding offsets in bytes for the first global, memory residual
   and transfer register, from an initial Array pointer.
   In general we have to keep into account:
   - globals (word-sized);
   - special-purpose state data;
   - memory residuals (word-sized);
   - transfer registers (word-sized);
   - slow registers (vmprefix_any_register-sized and aligned).
   Notice that memory
   residuals (meaning residuals stored in The Array) are zero on dispatching
   modes different from no-threading.  This relies on
   VMPREFIX_MAX_MEMORY_RESIDUAL_ARITY , defined below, which in its turn depends
   on VMPREFIX_MAX_RESIDUAL_ARITY, which is machine-generated. */
#define VMPREFIX_FIRST_GLOBAL_UNBIASED_OFFSET  \
  0
#define VMPREFIX_SPECIAL_PURPOSE_STATE_DATA_UNBIASED_OFFSET  \
  (VMPREFIX_FIRST_GLOBAL_UNBIASED_OFFSET                     \
   + sizeof (jitter_int) * VMPREFIX_GLOBAL_NO)
#define VMPREFIX_FIRST_MEMORY_RESIDUAL_UNBIASED_OFFSET   \
  (VMPREFIX_SPECIAL_PURPOSE_STATE_DATA_UNBIASED_OFFSET   \
   + sizeof (struct jitter_special_purpose_state_data))
#define VMPREFIX_FIRST_TRANSFER_REGISTER_UNBIASED_OFFSET        \
  (VMPREFIX_FIRST_MEMORY_RESIDUAL_UNBIASED_OFFSET               \
   + sizeof (jitter_int) * VMPREFIX_MAX_MEMORY_RESIDUAL_ARITY)
#define VMPREFIX_FIRST_SLOW_REGISTER_UNBIASED_OFFSET          \
  JITTER_NEXT_MULTIPLE_OF_POSITIVE                            \
     (VMPREFIX_FIRST_TRANSFER_REGISTER_UNBIASED_OFFSET        \
      + sizeof (jitter_int) * VMPREFIX_TRANSFER_REGISTER_NO,  \
      sizeof (union vmprefix_any_register))

/* Expand to the offset of the special-purpose data struct from the Array
   biased beginning. */
#define VMPREFIX_SPECIAL_PURPOSE_STATE_DATA_OFFSET       \
  (VMPREFIX_SPECIAL_PURPOSE_STATE_DATA_UNBIASED_OFFSET   \
   - JITTER_ARRAY_BIAS)

/* Given an expression evaluating to the Array unbiased beginning, expand to
   an expression evaluating to a pointer to its special-purpose data.
   This is convenient for accessing special-purpose data from outside the
   state -- for example, to set the pending notification flag for another
   thread.
   There are two versions of this feature:
     VMPREFIX_ARRAY_TO_SPECIAL_PURPOSE_STATE_DATA
   is meant to be used to access state data for some other thread, or in
   general out of VM code.
     VMPREFIX_OWN_SPECIAL_PURPOSE_STATE_DATA
   is for VM code accessing its own special-purpose data. */
#define VMPREFIX_ARRAY_TO_SPECIAL_PURPOSE_STATE_DATA_PRIVATE(qualifier,      \
                                                             array_address)  \
  ((qualifier struct jitter_special_purpose_state_data *)                    \
   (((char *) (array_address))                                               \
    + VMPREFIX_SPECIAL_PURPOSE_STATE_DATA_UNBIASED_OFFSET))
#define VMPREFIX_ARRAY_TO_SPECIAL_PURPOSE_STATE_DATA(array_address)       \
  VMPREFIX_ARRAY_TO_SPECIAL_PURPOSE_STATE_DATA_PRIVATE (volatile,         \
                                                        (array_address))
#define VMPREFIX_OWN_SPECIAL_PURPOSE_STATE_DATA          \
  VMPREFIX_ARRAY_TO_SPECIAL_PURPOSE_STATE_DATA_PRIVATE   \
     (VMPREFIX_ARRAY_VOLATILE_QUALIFIER,                 \
      ((char *) jitter_array_base) - JITTER_ARRAY_BIAS)

/* Given a state pointer, expand to an expression evaluating to a pointer to
   the state's special-purpose data.  This is meant for threads accessing
   other threads' special-purpose data, typically to set notifications. */
#define VMPREFIX_STATE_TO_SPECIAL_PURPOSE_STATE_DATA(state_p)  \
  (VMPREFIX_ARRAY_TO_SPECIAL_PURPOSE_STATE_DATA                \
     ((state_p)->vmprefix_state_backing.jitter_array))

/* Given a state pointer, expand to an expression evaluating to the
   pending_notification field for the state as an l-value.  This is meant for
   threads sending notifications to other threads. */
#define VMPREFIX_STATE_TO_PENDING_NOTIFICATIONS(state_p)   \
  (VMPREFIX_STATE_TO_SPECIAL_PURPOSE_STATE_DATA (state_p)  \
     ->pending_notifications)

/* Given a state pointer and a signal, expand to an l-value evaluating to a the
   pending field of the struct jitter_signal_notification element for the given
   signal in the pointed state.  This is meant for threads sending signal
   notifications to other threads and for C handler function. */
#define VMPREFIX_STATE_AND_SIGNAL_TO_PENDING_SIGNAL_NOTIFICATION(state_p,    \
                                                                 signal_id)  \
  (((VMPREFIX_STATE_TO_SPECIAL_PURPOSE_STATE_DATA (state_p)                   \
       ->pending_signal_notifications)                                        \
    + (signal_id))->pending)


/* Expand to the offset of the i-th register of class c in bytes from the Array
   beginning.
   The c argument must be a literal C (one-character) identifier.
   The i argument should always be a compile-time constant for performance, and
   it is in generated code.
   The i-th c-class register must be slow, otherwise the offset will be
   incorrect -- in fact fast registers are, hopefully, not in memory at all.

   Slow registers come in the Array ordered first by index, then by class.  For
   example if there are three classes "r" with 4 fast registers, "f" with 7 fast
   registers and "q" with 2 fast registers, slow registers can be accessed in
   this order:
     r4, f7, q2, r5, r8, q3, r6, r9, q4, and so on.
   Each contiguous group of slow registers spanning every class and starting
   from the first class (here for example <r5, r6, q3>) is called a "rank".
   This organization is convenient since changing the number of slow registers
   doesn't invalidate any offset computed in the past: the Array can simply be
   resized and its base pointer updated, without changing the code accessing it.

   This relies on macro such as VMPREFIX_REGISTER_CLASS_NO and
   VMPREFIX_REGISTER_?_FAST_REGISTER_NO and , defined below in machine-generated
   code. */
#define VMPREFIX_SLOW_REGISTER_UNBIASED_OFFSET(c, i)                     \
  (VMPREFIX_FIRST_SLOW_REGISTER_UNBIASED_OFFSET                          \
   + (sizeof (union vmprefix_any_register)                               \
      * (VMPREFIX_REGISTER_CLASS_NO                                      \
         * ((i) - JITTER_CONCATENATE_THREE(VMPREFIX_REGISTER_, c,        \
                                           _FAST_REGISTER_NO))           \
         + JITTER_CONCATENATE_THREE(VMPREFIX_REGISTER_, c, _CLASS_ID))))

/* Expand to the offset of the i-th register of class c in bytes from the base,
   keeping the bias into account. */
#define VMPREFIX_SLOW_REGISTER_OFFSET(c, i)                              \
  (VMPREFIX_SLOW_REGISTER_UNBIASED_OFFSET(c, i) - JITTER_ARRAY_BIAS)

/* Expand to the Array size in bytes, assuming the given number of slow
   registers per class.  This is an allocation size, ignoring the bias. */
#define VMPREFIX_ARRAY_SIZE(slow_register_per_class_no)                  \
  (VMPREFIX_FIRST_SLOW_REGISTER_UNBIASED_OFFSET                          \
   + (sizeof (union vmprefix_any_register)                               \
      * VMPREFIX_REGISTER_CLASS_NO                                       \
      * (slow_register_per_class_no)))




/* Residual access.
 * ************************************************************************** */

/* How many residuals we can have at most in memory, which is to say,
   without counting residuals kept in reserved registers.

   Implementation note: it would be wrong here to use a CPP conditional based on
   the value of VMPREFIX_MAX_RESIDUAL_ARITY , as I was doing in a preliminary
   version.  That lead to a tricky bug, since VMPREFIX_MAX_RESIDUAL_ARITY ,
   which is defined below but is not yet available here, simply counted as 0
   for the purposes of evaluating the CPP condititional. */
#ifdef JITTER_DISPATCH_NO_THREADING
  /* We are using no-threading dispatch.  If there are no more residuals
     than reserved residual registers then we never need to keep any in
     memory.  Otherwise we need to keep as many residuals in memory as the
     total number of residuals minus how many registers are reserved for
     them. */
# define VMPREFIX_MAX_MEMORY_RESIDUAL_ARITY                          \
    ((VMPREFIX_MAX_RESIDUAL_ARITY <= JITTER_RESIDUAL_REGISTER_NO)    \
     ? 0                                                             \
     : (VMPREFIX_MAX_RESIDUAL_ARITY - JITTER_RESIDUAL_REGISTER_NO))
#else // Not no-threading.
  /* No registers are reserved for residuals in this dispatch; even if
     in fact all residuals are memory residuals they don't count here, since
     residuals are not held in The Array in this dispatch. */
# define VMPREFIX_MAX_MEMORY_RESIDUAL_ARITY  \
  0
#endif // #ifdef JITTER_DISPATCH_NO_THREADING

#ifdef JITTER_DISPATCH_NO_THREADING
/* Expand to the offset from the base, in bytes, of the i-th residual.  The
   given index must be greater than or equal to JITTER_RESIDUAL_REGISTER_NO;
   residuals with indices lower than that number are not stored in The Array
   at all.
   This is not useful with any of the other dispatches, where residuals
   directly follow each VM instruction opcode or thread.  For good performance i
   should always be a compile-time constant, as it is in machine-generated
   code.
   Residuals always have the size of a jitter word, even if some register class
   may be wider. */
/* FIXME: if later I use a different policy than simply checking
   JITTER_RESIDUAL_REGISTER_NO to decide how many residuals to keep in
   registers, then I have to change this or meet very nasty bugs. */
# define VMPREFIX_RESIDUAL_UNBIASED_OFFSET(i)                      \
    (VMPREFIX_FIRST_MEMORY_RESIDUAL_UNBIASED_OFFSET                \
     + (sizeof (jitter_int) * (i - JITTER_RESIDUAL_REGISTER_NO)))
# define VMPREFIX_RESIDUAL_OFFSET(i)  \
    (VMPREFIX_RESIDUAL_UNBIASED_OFFSET(i) - JITTER_ARRAY_BIAS)
#endif // #ifdef JITTER_DISPATCH_NO_THREADING



/* Mutable routine text frontend.
 * ************************************************************************** */

/* An enum type. */
#define vmprefix_routine_edit_status  \
  jitter_routine_edit_status
/* Cases for an enum type. */
#define vmprefix_routine_edit_status_success  \
  jitter_routine_edit_status_success
#define vmprefix_routine_edit_status_label_defined_twice  \
  jitter_routine_edit_status_label_defined_twice
#define vmprefix_routine_edit_status_invalid_instruction  \
  jitter_routine_edit_status_invalid_instruction
#define vmprefix_routine_edit_status_invalid_register  \
  jitter_routine_edit_status_invalid_register
#define vmprefix_routine_edit_status_register_class_mismatch  \
  jitter_routine_edit_status_register_class_mismatch
#define vmprefix_routine_edit_status_nonexisting_register_class  \
  jitter_routine_edit_status_nonexisting_register_class
#define vmprefix_routine_edit_status_invalid_parameter_kind  \
  jitter_routine_edit_status_invalid_parameter_kind
#define vmprefix_routine_edit_status_too_many_parameters  \
  jitter_routine_edit_status_too_many_parameters
#define vmprefix_routine_edit_status_last_instruction_incomplete  \
  jitter_routine_edit_status_last_instruction_incomplete
#define vmprefix_routine_edit_status_other_parse_error  \
  jitter_routine_edit_status_other_parse_error

/* Given a parse status of type enum jitter_routine_edit_status return its
   written C representation. */
#define vmprefix_routine_edit_status_to_string  \
  jitter_routine_edit_status_to_string

/* The name of the struct returned by parsers. */
#define vmprefix_routine_parse_error  \
  jitter_routine_parse_error

/* The name of the function destroying a pointer to vmprefix_routine_parse_error
   , returned by routine parsers in case of error. */
#define vmprefix_routine_parse_error_destroy  \
  jitter_routine_parse_error_destroy

/* Parse VM code from the given file or string into the pointed VM routine,
   which is allowed but not required to be empty.
   These are simple wrappers around functions implemented in the Bison file. */
struct vmprefix_routine_parse_error *
vmprefix_parse_mutable_routine_from_file_star (FILE *input_file,
                                               struct jitter_mutable_routine *p)
  __attribute__ ((nonnull (1, 2)));
struct vmprefix_routine_parse_error *
vmprefix_parse_mutable_routine_from_file (const char *input_file_name,
                                          struct jitter_mutable_routine *p)
  __attribute__ ((nonnull (1, 2)));
struct vmprefix_routine_parse_error *
vmprefix_parse_mutable_routine_from_string (const char *string,
                                            struct jitter_mutable_routine *p)
  __attribute__ ((nonnull (1, 2)));




/* Unified routine text frontend.
 * ************************************************************************** */

/* The C wrappers for the ordinary API can be reused for the unified API, since
   it internally works with mutable routines. */
#define vmprefix_parse_routine_from_file_star  \
  vmprefix_parse_mutable_routine_from_file_star
#define vmprefix_parse_routine_from_file  \
  vmprefix_parse_mutable_routine_from_file
#define vmprefix_parse_routine_from_string  \
  vmprefix_parse_mutable_routine_from_string




/* Machine-generated data structures.
 * ************************************************************************** */

/* Declare a few machine-generated data structures, which together define a VM. */

/* Threads or pointers to native code blocks of course don't exist with
   switch-dispatching. */
#ifndef JITTER_DISPATCH_SWITCH
/* Every possible thread, indexed by enum jitter_specialized_instruction_opcode .
   This is used at specialization time, and the user shouldn't need to touch
   it. */
extern const jitter_thread *
vmprefix_threads;

/* VM instruction end label.  These are not all reachable at run time, but
   having them in a global array might prevent older GCCs from being too clever
   in reordering blocks. */
extern const jitter_thread *
vmprefix_thread_ends;

/* The size, in chars, of each thread's native code.  The elements are in the
   same order of vmprefix_threads.  Sizes could conceptually be of type size_t ,
   but in order to be defensive I'm storing pointer differences as signed
   values, so that we may catch compilation problems: if any VM instruction end
   *precedes* its VM instruction beginning, then the compiler has reordered
   labels, which would have disastrous effects with replicated code. */
extern const long *
vmprefix_thread_sizes;
#endif // #ifndef JITTER_DISPATCH_SWITCH

/* This is defined in the machine-generated vm/meta-instructions.c . */
extern struct jitter_hash_table
vmprefix_meta_instruction_hash;

/* An array specifying every existing meta-instruction, defined in the order of
   enum vmprefix_meta_instruction_id .  This is defined in vm/meta-instructions.c ,
   which is machine-generated. */
extern const struct jitter_meta_instruction
vmprefix_meta_instructions [];

/* An array whose indices are specialised instruction opcodes, and
   whose elements are the corresponding unspecialised instructions
   opcodes -- or -1 when there is no mapping mapping having */
extern const int
vmprefix_specialized_instruction_to_unspecialized_instruction [];

/* How many residual parameters each specialized instruction has.  The
   actual array definition is machine-generated. */
extern const size_t
vmprefix_specialized_instruction_residual_arities [];

/* An array of bitmasks, one per specialized instruction.  Each bitmask holds
   one bit per residual argument, counting from the least significant (the first
   residual arg maps to element & (1 << 0), the second to element & (1 << 1),
   and so on).
   Each bit is 1 if and only if the corresponding residual argument is a label
   or a fast label.
   Only residual arguments are counted: for example a specialized instruction
   foo_n1_lR_r2 would have a mask with the *first* bit set. */
extern const unsigned long // FIXME: possibly use a shorter type when possible
vmprefix_specialized_instruction_label_bitmasks [];

/* Like vmprefix_specialized_instruction_label_bitmasks , but for fast labels
   only.
   The actual definition is conditionalized so as to appear only when
   needed according to the dispatch. */
extern const unsigned long // FIXME: possibly use a shorter type when possible
vmprefix_specialized_instruction_fast_label_bitmasks [];

/* An array of booleans in which each element is true iff the specialized
   instruction whose opcode is the index is relocatable. */
extern const bool
vmprefix_specialized_instruction_relocatables [];

/* An array of booleans in which each element is true iff the specialized
   instruction whose opcode is the index is a caller. */
extern const bool
vmprefix_specialized_instruction_callers [];

/* An array of booleans in which each element is true iff the specialized
   instruction whose opcode is the index is a callee. */
extern const bool
vmprefix_specialized_instruction_callees [];

/* This big array of strings contains the name of each specialized instruction,
   in the order of enum vmprefix_specialized_instruction_opcode . */
extern const char* const
vmprefix_specialized_instruction_names [];


/* A pointer to a struct containing const pointers to the structures above, plus
   sizes; there will be only one instance of this per VM, machine-generated.
   Each program data structure contains a pointer to that instance, so that
   VM-independent functions, given a program, will have everything needed to
   work.  The one instance of struct jitter_vm for the vmprefix VM. */
extern struct jitter_vm * const
vmprefix_vm;

/* A pointer to a struct containing VM-specific parameters set in part when
   calling jitterc and in part when compiling the generated C code, such as the
   dispatch and the number of fast registers.  The data is fully
   initialized only after a call to vmprefix_initialize . */
extern const
struct jitter_vm_configuration * const
vmprefix_vm_configuration;




/* Compatibility macros.
 * ************************************************************************** */

/* It is convenient, for future extensibility, to expose an interface in which
   some VM-independent functions and data structures actually look as if they
   were specific to the user VM. */

/* What the user refers to as struct vmprefix_mutable_routine is actually a
   struct jitter_mutable_routine , whose definition is VM-independent. */
#define vmprefix_mutable_routine jitter_mutable_routine

/* Same for executable routines. */
#define vmprefix_executable_routine jitter_executable_routine

/* Same for unified routines. */
#define vmprefix_routine jitter_routine

/* Destroy a non-executable routine (routine initialization is actually
   VM-specific). */
#define vmprefix_destroy_mutable_routine jitter_destroy_mutable_routine

/* Destroy a unified routine. */
#define vmprefix_destroy_routine jitter_destroy_routine

/* Pin a unified routine. */
#define vmprefix_pin_routine jitter_pin_routine

/* Unpin a unified routine. */
#define vmprefix_unpin_routine jitter_unpin_routine

/* Print VM configuration. */
#define vmprefix_print_vm_configuration jitter_print_vm_configuration

/* Generic routine construction API. */
#define vmprefix_label \
  jitter_label
#define vmprefix_fresh_label \
  jitter_fresh_label

/* Mutable routine option API. */
#define vmprefix_set_mutable_routine_option_slow_literals_only \
  jitter_set_mutable_routine_option_slow_literals_only
#define vmprefix_set_mutable_routine_option_slow_registers_only \
  jitter_set_mutable_routine_option_slow_registers_only
#define vmprefix_set_mutable_routine_option_slow_literals_and_registers_only \
  jitter_set_mutable_routine_option_slow_literals_and_registers_only
#define vmprefix_set_mutable_routine_option_add_final_exitvm \
  jitter_set_mutable_routine_option_add_final_exitvm
#define vmprefix_set_mutable_routine_option_optimization_rewriting \
  jitter_set_mutable_routine_option_optimization_rewriting

/* Printing and disassembling: ordinary API. */
#define vmprefix_mutable_routine_print \
  jitter_mutable_routine_print
#define vmprefix_executable_routine_disassemble \
  jitter_executable_routine_disassemble

/* Mutable routine construction API: unsafe API */
#define vmprefix_mutable_routine_append_instruction_name \
  jitter_mutable_routine_append_instruction_name
#define vmprefix_mutable_routine_append_meta_instruction \
  jitter_mutable_routine_append_meta_instruction
#define vmprefix_mutable_routine_append_label \
  jitter_mutable_routine_append_label
#define vmprefix_mutable_routine_append_symbolic_label \
  jitter_mutable_routine_append_symbolic_label
#define vmprefix_mutable_routine_append_register_parameter \
  jitter_mutable_routine_append_register_parameter
#define vmprefix_mutable_routine_append_symbolic_register_parameter \
  jitter_mutable_routine_append_symbolic_register_parameter
#define vmprefix_mutable_routine_append_literal_parameter \
  jitter_mutable_routine_append_literal_parameter
#define vmprefix_mutable_routine_append_signed_literal_parameter \
  jitter_mutable_routine_append_signed_literal_parameter
#define vmprefix_mutable_routine_append_unsigned_literal_parameter \
  jitter_mutable_routine_append_unsigned_literal_parameter
#define vmprefix_mutable_routine_append_pointer_literal_parameter \
  jitter_mutable_routine_append_pointer_literal_parameter
#define vmprefix_mutable_routine_append_label_parameter \
  jitter_mutable_routine_append_label_parameter
#define vmprefix_mutable_routine_append_symbolic_label_parameter \
  jitter_mutable_routine_append_symbolic_label_parameter

/* Mutable routine construction API: safe API */
#define vmprefix_mutable_routine_append_label_safe  \
  jitter_mutable_routine_append_label_safe
#define vmprefix_mutable_routine_append_symbolic_label_safe  \
  jitter_mutable_routine_append_symbolic_label_safe
#define vmprefix_mutable_routine_append_instruction_name_safe  \
  jitter_mutable_routine_append_instruction_name_safe
#define vmprefix_mutable_routine_append_instruction_id_safe  \
  jitter_mutable_routine_append_instruction_id_safe
#define vmprefix_mutable_routine_append_meta_instruction_safe  \
  jitter_mutable_routine_append_meta_instruction_safe
#define vmprefix_mutable_routine_append_literal_parameter_safe  \
  jitter_mutable_routine_append_literal_parameter_safe
#define vmprefix_mutable_routine_append_signed_literal_parameter_safe  \
  jitter_mutable_routine_append_signed_literal_parameter_safe
#define vmprefix_mutable_routine_append_unsigned_literal_parameter_safe  \
  jitter_mutable_routine_append_unsigned_literal_parameter_safe
#define vmprefix_mutable_routine_append_pointer_literal_parameter_safe  \
  jitter_mutable_routine_append_pointer_literal_parameter_safe
#define vmprefix_mutable_routine_append_register_parameter_safe  \
  jitter_mutable_routine_append_register_parameter_safe
#define vmprefix_mutable_routine_append_symbolic_register_parameter_safe  \
  jitter_mutable_routine_append_symbolic_register_parameter_safe
#define vmprefix_mutable_routine_append_symbolic_label_parameter_safe  \
  jitter_mutable_routine_append_symbolic_label_parameter_safe
#define vmprefix_mutable_routine_append_label_parameter_safe  \
  jitter_mutable_routine_append_label_parameter_safe

/* Mutable routine destruction. */
#define vmprefix_destroy_executable_routine \
  jitter_destroy_executable_routine

/* Making executable routines from mutable routines. */
#define vmprefix_make_executable_routine \
  jitter_make_executable_routine

/* Unified routine option API. */
#define vmprefix_set_routine_option_slow_literals_only \
  jitter_set_mutable_routine_option_slow_literals_only
#define vmprefix_set_routine_option_slow_registers_only \
  jitter_set_mutable_routine_option_slow_registers_only
#define vmprefix_set_routine_option_slow_literals_and_registers_only \
  jitter_set_mutable_routine_option_slow_literals_and_registers_only
#define vmprefix_set_routine_option_add_final_exitvm \
  jitter_set_mutable_routine_option_add_final_exitvm
#define vmprefix_set_routine_option_optimization_rewriting \
  jitter_set_mutable_routine_option_optimization_rewriting

/* Printing and disassembling: unified API.  These do not follow the pattern of
   the rest: wrapped identifiers here are the names of C functions specific to
   the unified API */
#define vmprefix_routine_print \
  jitter_routine_print
#define vmprefix_routine_disassemble \
  jitter_routine_disassemble

/* Unified routine construction API. */
#define vmprefix_routine_append_instruction_name \
  jitter_mutable_routine_append_instruction_name
#define vmprefix_routine_append_meta_instruction \
  jitter_mutable_routine_append_meta_instruction
#define vmprefix_routine_append_label \
  jitter_mutable_routine_append_label
#define vmprefix_routine_append_symbolic_label \
  jitter_mutable_routine_append_symbolic_label
#define vmprefix_routine_append_register_parameter \
  jitter_mutable_routine_append_register_parameter
#define vmprefix_routine_append_literal_parameter \
  jitter_mutable_routine_append_literal_parameter
#define vmprefix_routine_append_signed_literal_parameter \
  jitter_mutable_routine_append_signed_literal_parameter
#define vmprefix_routine_append_unsigned_literal_parameter \
  jitter_mutable_routine_append_unsigned_literal_parameter
#define vmprefix_routine_append_pointer_literal_parameter \
  jitter_mutable_routine_append_pointer_literal_parameter
#define vmprefix_routine_append_label_parameter \
  jitter_mutable_routine_append_label_parameter
#define vmprefix_routine_append_symbolic_label_parameter \
  jitter_mutable_routine_append_symbolic_label_parameter

/* Mutable routine destruction. */
#define vmprefix_destroy_routine                                           \
  /* This does not follow the pattern of the rest: the wrapped identifier  \
     here is the name of a C function specific to the unified API. */      \
  jitter_destroy_routine

/* The unified API has no facility to explicitly make executable routines: their
   very existence is hidden.  For this reason some of the macros above, such
   vmprefix_make_executable_routine, have no unified counterpart here. */

/* Defects and replacements. */
void
vmprefix_defect_print_summary (jitter_print_context cx)
  __attribute__ ((nonnull (1)));
void
vmprefix_defect_print (jitter_print_context cx,
                       unsigned indentation_column_no)
  __attribute__ ((nonnull (1)));
void
vmprefix_defect_print_replacement_table (jitter_print_context cx,
                                         unsigned indentation_column_no)
  __attribute__ ((nonnull (1)));

/* Profiling.  Apart from vmprefix_state_profile, which returns a pointer to
   the profile within a pointed state structure, everything else here has the
   same API as the functionality in jitter/jitter-profile.h , without the VM
   pointer.
   Notice that this API does nothing useful onless at least one of the CPP
   macros VMPREFIX_PROFILE_COUNT or VMPREFIX_PROFILE_SAMPLE is defined. */
#define vmprefix_profile_runtime  \
  jitter_profile_runtime /* the struct name */
#define vmprefix_profile  \
  jitter_profile /* the struct name */
// FIXME: no: distinguish between struct jitter_profile_runtime and its user-friendly variant
struct jitter_profile_runtime *
vmprefix_state_profile_runtime (struct vmprefix_state *s)
  __attribute__ ((returns_nonnull, nonnull (1)));
struct vmprefix_profile_runtime*
vmprefix_profile_runtime_make (void)
  __attribute__ ((returns_nonnull));
void
vmprefix_profile_runtime_destroy (struct vmprefix_profile_runtime *p)
  __attribute__ ((nonnull (1)));
#define vmprefix_profile_destroy jitter_profile_destroy
void
vmprefix_profile_runtime_clear (struct vmprefix_profile_runtime *p)
  __attribute__ ((nonnull (1)));
void
vmprefix_profile_runtime_merge_from (struct vmprefix_profile_runtime *to,
                                     const struct vmprefix_profile_runtime *from)
  __attribute__ ((nonnull (1, 2)));
void
vmprefix_profile_runtime_merge_from_state (struct vmprefix_profile_runtime *to,
                                   const struct vmprefix_state *from_state)
  __attribute__ ((nonnull (1, 2)));
struct vmprefix_profile *
vmprefix_profile_unspecialized_from_runtime
   (const struct vmprefix_profile_runtime *p)
  __attribute__ ((returns_nonnull, nonnull (1)));
struct vmprefix_profile *
vmprefix_profile_specialized_from_runtime (const struct vmprefix_profile_runtime
                                           *p)
  __attribute__ ((returns_nonnull, nonnull (1)));
void
vmprefix_profile_runtime_print_unspecialized
   (jitter_print_context ct,
    const struct vmprefix_profile_runtime *p)
  __attribute__ ((nonnull (1, 2)));
void
vmprefix_profile_runtime_print_specialized (jitter_print_context ct,
                                            const struct vmprefix_profile_runtime
                                            *p)
  __attribute__ ((nonnull (1, 2)));




/* Register class types.
 * ************************************************************************** */

/* Return a pointer to a statically allocated register class descriptor, given
   the register class character, or NULL if the character does not represent a
   valid register class.

   A constant array indexed by a character would have been more efficient, but
   relying on character ordering is not portable, at least in theory.  A
   non-constant array could be initialized in a portable way, but that would
   probably not be worth the trouble. */
const struct jitter_register_class *
vmprefix_register_class_character_to_register_class (char c)
  __attribute__ ((pure));


/* A constant array of constant pointers to every existing register class
   descriptor, ordered by class id; each pointer within the array refers the
   only existing class descriptor for its class.  The number of elements is
   VMPREFIX_REGISTER_CLASS_NO , but that is not declared because the definition
   of VMPREFIX_REGISTER_CLASS_NO comes later in generated code.

   This is useful when the user code enumerates every existing register class,
   particularly for debugging. */
extern const struct jitter_register_class * const
vmprefix_regiter_classes [];




/* Array re-allocation.
 * ************************************************************************** */

/* Make the Array in the pointed state large enough to accommodate the given
   number of slow reigsters per class, adjusting the Array pointer as needed
   and recording information about the new size in the state; change nothing
   if the array is already large enough.  Return the new base.
   For example passing 3 as the value of slow_register_no would make
   place for three slow registers per register class: if the current VM had two
   classes 'r' and 'f' than the function would ensure that the Array can hold
   three 'r' and three 'f' slow registers, independently from the number
   of fast 'r' or 'f' registers.
   Any new elements allocated in the Array are left uninitialized, but its old
   content remains valid. */
char *
vmprefix_make_place_for_slow_registers (struct vmprefix_state *s,
                                        jitter_int slow_register_no_per_class)
  __attribute__ ((noinline));




/* **************************************************************************
 * Evrything following this point is for internal use only.
 * ************************************************************************** */




/* Replacement tables.
 * ************************************************************************** */

/* It is harmless to declare these unconditionally, even if they only used when
   defect replacement is available.  See jitter/jitter-defect.h . */

/* The worst-case replacement table.  This is a global constant array, having
   one element per specialised instruction. */
extern const jitter_uint
vmprefix_worst_case_replacement_table [];

/* An array whose first defective_specialized_instruction_no elements contain
   the specialized_instruction_ids of defective instructions; the remaining
   elements are set to -1.  This array is initialised by
   jitter_fill_replacement_table . */
extern jitter_int
vmprefix_defective_specialized_instructions [];

/* The actual replacement table, to be filled at initialization time. */
extern jitter_uint
vmprefix_replacement_table [];

/* An array of specialized instruction ids, holding the specialized instruction
   ids of any non-replacement specialized instruction which is "call-related",
   which is to say any of caller, callee or returning.  These are useful for
   defect replacements, since any defect in even just one of them requires
   replacing them all. */
extern const jitter_uint
vmprefix_call_related_specialized_instruction_ids [];

/* The size of vmprefix_call_related_specialized_instruction_ids in elements. */
extern const jitter_uint
vmprefix_call_related_specialized_instruction_id_no;

/* An array of Booleans in which each element is true iff the specialized
   instruction whose opcode is the index is call-related. */
extern const bool
vmprefix_specialized_instruction_call_relateds [];




/* Instruction rewriter.
 * ************************************************************************** */

/* Try to apply each rewrite rule in order and run the first one that matches,
   if any, on the pointed program.  When a rule fires the following ones are not
   checked but if a rule, after removing the last few instructions, adds another
   one, the addition will trigger another rewrite in its turn, and so on until
   no more rewriting is possible.  The rewriting process is inherently
   recursive.

   The implementation of this function is machine-generated, but the user can
   add her own code in the rewriter-c block, which ends up near the beginning of
   this function body, right after JITTTER_REWRITE_FUNCTION_PROLOG_ .  The
   formal argument seen from the body is named jitter_mutable_routine_p .

   Rationale: the argument is named differently in the body in order to keep
   the namespace conventions and, more importantly, to encourage the user to
   read this comment.

   The user must *not* append labels to the VM routines during rewriting: that
   would break it.  The user is responsible for destroying any instruction she
   removes, including their arguments.  The user can assume that
   jitter_rewritable_instruction_no is strictly greater than zero. */
void
vmprefix_rewrite (struct jitter_mutable_routine *jitter_mutable_routine_p);




/* Program points at run time in executable routines.
 * ************************************************************************** */

/* Provide a nice name for a program point type which looks VM-dependent. */
typedef jitter_program_point
vmprefix_program_point;

/* Again, provide a VM-dependent alias for an actually VM-independent macro. */
#define VMPREFIX_EXECUTABLE_ROUTINE_BEGINNING(_jitter_executable_routine_ptr)  \
  JITTER_EXECUTABLE_ROUTINE_BEGINNING(_jitter_executable_routine_ptr)




/* Program points at run time in routines: unified routine API.
 * ************************************************************************** */

/* Like VMPREFIX_EXECUTABLE_ROUTINE_BEGINNING for the unified routine API. */
#define VMPREFIX_ROUTINE_BEGINNING(_jitter_routine)                \
  JITTER_EXECUTABLE_ROUTINE_BEGINNING                              \
     (jitter_routine_make_executable_if_needed (_jitter_routine))



/* Executing code from an executable routine.
 * ************************************************************************** */

/* Make sure that the pointed state has enough slow registers to run the pointed
   executable routine; if that is not the case, allocate more slow registers. */
void
vmprefix_ensure_enough_slow_registers_for_executable_routine
   (const struct jitter_executable_routine *er, struct vmprefix_state *s)
  __attribute__ ((nonnull (1, 2)));

/* Run VM code starting from the given program point (which must belong to some
   executable routine), in the pointed VM state.
   Return the VM execution state at the end.

   Since no executable routine is given this cannot automatically guarantee that
   the slow registers in the pointed state are in sufficient number; it is the
   user's responsibility to check, if needed.

   This function is also usable with the unified routine API. */
enum vmprefix_exit_status
vmprefix_branch_to_program_point (vmprefix_program_point p,
                                  struct vmprefix_state *s)
  __attribute__ ((nonnull (1, 2)));

/* Run VM code starting from the beginning of the pointed executable routine,
   in the pointed VM state.
   Return the VM execution state at the end.
   This does ensure that the slow registers in the pointed state are in
   sufficient number, by calling vmprefix_ensure_enough_slow_registers_for .
   This function is slightly less efficient than
   vmprefix_branch_to_program_point , and vmprefix_branch_to_program_point
   should be preferred in contexts where C code repeatedly calls VM code. */
enum vmprefix_exit_status
vmprefix_execute_executable_routine (const struct jitter_executable_routine *er,
                                     struct vmprefix_state *s)
  __attribute__ ((nonnull (1, 2)));




/* Executing code: unified routine API.
 * ************************************************************************** */

/* Like vmprefix_ensure_enough_slow_registers_for_executable_routine , with the
   unified API. */
void
vmprefix_ensure_enough_slow_registers_for_routine
   (jitter_routine r, struct vmprefix_state *s)
  __attribute__ ((nonnull (1, 2)));

/* vmprefix_branch_to_program_point , declared above, is also usable with the
   unified routine API. */

/* Like vmprefix_execute_executable_routine, for a unified routine. */
enum vmprefix_exit_status
vmprefix_execute_routine (jitter_routine r,
                          struct vmprefix_state *s)
  __attribute__ ((nonnull (1, 2)));




/* VM exit status.
 * ************************************************************************** */

/* A value of this type is returned by a VM after execution, and the last
   returned value is also held in the VM state in order to make consistency
   checks. */
enum vmprefix_exit_status
  {
    /* This state has never been used for execution.  This is the initial value
       within the state, and is never returned after execution. */
    vmprefix_exit_status_never_executed = 0,

    /* The state is being used in execution right now; this is never returned by
       the executor.  It is an error (checked for) to execute code with a VM
       state containing this exit status, which shows that there has been a
       problem -- likely VM code was exited via longjmp, skipping the proper
       cleanup. */
    vmprefix_exit_status_being_executed = 1,

    /* Some VM code has been executed.  It is now possible to execute more code
       (including the same code again) in the same state. */
    vmprefix_exit_status_exited = 2,

    /* Code execution has been interrupted for debugging, but can be resumed. */
    vmprefix_exit_status_debug = 3,
  };




/* Low-level debugging features relying on assembly: data locations.
 * ************************************************************************** */

/* Dump human-readable information about data locations to the given print
   context.
   This is a trivial VM-dependent wrapper around jitter_dump_data_locations,
   which does not require a struct jitter_vm pointer as input. */
void
vmprefix_dump_data_locations (jitter_print_context output)
  __attribute__ ((nonnull (1)));




/* Sample profiling: internal API.
 * ************************************************************************** */

/* The functions in this sections are used internally by vm2.c, only when
   sample-profiling is enabled.  In fact these functions are not defined at all
   otherwise. */

/* Initialise global sampling-related structures. */
// FIXME: no: distinguish struct jitter_profile_runtime and struct jitter_profile
void
vmprefix_profile_sample_initialize (void);

/* Begin sampling. */
void
vmprefix_profile_sample_start (struct vmprefix_state *state)
  __attribute__ ((nonnull (1)));

/* Stop sampling. */
void
vmprefix_profile_sample_stop (void);




/* User macros to access VM state data structures (out of VM instructions).
 * ************************************************************************** */

/* Notice that these macros are to be used out of VM instruction code blocks:
   for use within instructions see the alternative definitions of
   _JITTER_STATE_RUNTIME_FIELD and _JITTER_STATE_BACKING_FIELD in
   jitter-executor.h .  The alternative definitions are *not* compatible: the
   macros defined here have one more argument, the VM state structure. */

/* Given a VM state pointer and a state runtime field name expand to an l-value
   referring the named field in the given VM state runtime.
   This macro is not usable within VM instruction code blocks: see the commmnt
   above. */
#define VMPREFIX_STATE_RUNTIME_FIELD(state_p /* see the comment above */,  \
                                     field_name)                           \
  ((state_p)->vmprefixhash_state_runtime.field_name)

/* Given a VM state pointer and a state backing field name expand to an l-value
   referring the named field in the given VM state backing.
   This macro is not usable within VM instruction code blocks: see the commmnt
   above. */
#define VMPREFIX_STATE_BACKING_FIELD(state_p /* see the comment above */,  \
                                     field_name)                           \
  ((state_p)->vmprefix_state_backing.field_name)




/* Machine-generated code.
 * ************************************************************************** */

/* What follows could be conceptually split into several generated header files,
   but having too many files would be inconvenient for the user to compile and
   link.  For this reason we generate a single header. */

