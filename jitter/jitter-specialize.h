/* VM library: specializer header file.

   Copyright (C) 2016, 2017, 2019, 2020 Luca Saiu
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


#ifndef JITTER_SPECIALIZE_H_
#define JITTER_SPECIALIZE_H_

#include <jitter/jitter.h>
#include <jitter/jitter-mutable-routine.h>
#include <jitter/jitter-instruction.h>


/* Executable routines.
 * ************************************************************************** */

/* An executable routine is a data structure obtained from a routine, with a
   different internal representation meant for direct execution rather than
   modificaion -- the internal representation is in fact *very* different,
   particularly in sophisticated dispatching modes involving native code.

   Only an executable routine can be run on a VM, and only a non-executable
   routine can be modified by adding instructions and labels.

   Each non-executable routine can generate at most one executable routine.

   Once the executable routine has been generated (thru
   jitter_make_executable_routine ) the user is free to destroy the original
   non-executable routine in ordre to free resources, while the executable
   routine can be safely executed.  Destroying a non-executable routine is
   always allowed but may make disassembly and debugging information less
   precise. */

/* The internal representation of an executable routine.  This should be treated
   as an abstract data type. */
struct jitter_executable_routine
{
  /* The non-executable version of this routine, if one still exists.  This
     field, non-NULL at initialization, becomes NULL if the original
     non-executable routine is destroyed. */
  struct jitter_mutable_routine *routine;

  /* A pointer to the VM this belongs to, which is never invalidated even
     if the mutable routine is destroyed. */
  const struct jitter_vm *vm;

  /* How many live pointers there are to this executable routine.  This is set
     to 1 at initialization, then updated manually with
     jitter_pin_executable_routine and jitter_unpin_executable_routine ,
     declared below.

     The executable routine is automatically destroyed, along with its companion
     mutable routine, when the reference count reaches zero.  Notice that, by
     contrast, when the user explicitly destroys an executable routine its
     mutable companion is *not* automatically destroyed. */
  unsigned long reference_count;

  /* The following fields, including the ones conditionalized over the
     dispatching mode, have the same meaning as the fields with the same name
     from struct jitter_mutable_routine, and are copied from there.  See the comments in
     jitter-routine.h .
     The fields represented as struct jitter_dynamic_buffer in struct
     jitter_routine, for ease of incremental construction, here are simply
     heap-allocated buffers, whose size will no longer change.  In fact their
     heap storage is the same allocated within struct jitter_mutable_routine, obtained
     by extraction.  The finalization API for struct jitter_mutable_routine keeps this
     into account, and of course avoids freeing heap memory twice. */
  jitter_int slow_register_per_class_no;
#if   (defined(JITTER_DISPATCH_SWITCH)                 \
       || defined(JITTER_DISPATCH_DIRECT_THREADING)    \
       || defined(JITTER_DISPATCH_MINIMAL_THREADING))
  union jitter_specialized_word *specialized_program;
#elif defined(JITTER_DISPATCH_NO_THREADING)
  /* Nothing. */
#elif ! defined (JITTER_INTERNAL)
# error "unknown dispatch: this should not happen"
#else
  /* This is a dispatch-independent compilation, either part of the Jitter
     utility library or of the C code generator.  The specific fields of this
     struct do not matter. */
#endif /* dispatch */
#if (defined(JITTER_DISPATCH_SWITCH)                \
     || defined(JITTER_DISPATCH_DIRECT_THREADING))
  /* Nothing. */
#elif (defined(JITTER_DISPATCH_MINIMAL_THREADING)  \
       || defined(JITTER_DISPATCH_NO_THREADING))
  char *native_code;
  size_t native_code_size;
#elif ! defined (JITTER_INTERNAL)
# error "unknown dispatch: this should not happen"
#else
  /* This is a dispatch-independent compilation, either part of the Jitter
     utility library or of the C code generator.  The specific fields of this
     struct do not matter. */
#endif /* dispatch */
};

/* Translate the pointed routine into a fresh executable routine, and return a
   pointer to it.
   The warn_unused_result attribute serves to aid for early users who may be
   surprised by an API change.  Early versions of this function used to return
   void. */
struct jitter_executable_routine*
jitter_make_executable_routine (struct jitter_mutable_routine *p)
  __attribute__ ((nonnull (1), returns_nonnull));

/* Destroy the pointed executable routine, freeing its resources.  If its
   original non-executable routine still exists -- in case it has been destroyed
   already its destruction function has updated a field in the executable
   routine, and this function will not free the non-executable routine twice.
   After this function is called it is no longer possible to run any code
   belonging to the executable routine, even if execution was already in
   progress or if the function is called from a VM instruction.

   When the user calls this function the reference count of the routine must be
   exactly one, otherwise this function fails fatally.  About destruction by
   jitter_unpin_executable_routine and the difference in behaviour compared to a
   direct use of this function, see the comment about the reference_count struct
   field above. */
void
jitter_destroy_executable_routine (struct jitter_executable_routine *p)
  __attribute__ ((nonnull (1)));

/* Increment the reference_count field of the pointed executable routine,
   notifying the system that one more reference handled by the user has been
   added. */
void
jitter_pin_executable_routine (struct jitter_executable_routine *er);

/* Decrement the reference_count field of the pointed executable routine,
   notifying the system that one reference to the executable routine has been
   destroyed.
   If the count reaches zero immediately destroy the executable routine and, if
   still existing, its mutable companion.  Notice that the behavior of
   jitter_destroy_executable_routine is different, in that
   jitter_destroy_executable_routine does not destroy the mutable companion. */
void
jitter_unpin_executable_routine (struct jitter_executable_routine *er);




/* Program points.
 * ************************************************************************** */

/* The type of a program point at run time in an executable routine.  This is
   the type of object than can be passed to JITTER_BRANCH , and its actual
   definition depends on the dispatching model.  Notice that however, in every
   case, a program point is a pointer-to-constant type and therefore fits in a
   word. */
#ifdef JITTER_DISPATCH_NO_THREADING
  /* With no-threading dispatch a program point is the address of a machine
     instruction -- from C, it's what a goto * statement accepts.  I don't
     need to worry about non-GCC compilers, since no-threading relies on GCC
     extensions. */
  typedef const void *
  jitter_program_point;
#else
  /* On every other dispatching model a program point is a pointer to a word
     in the executable routine -- in the case of switch dispatching that word
     will contain a specialized opcode, with threading it will contain the
     address of a machine instruction (see the case above) followed by the
     VM instruction arguments. */
  typedef const union jitter_word *
  jitter_program_point;
#endif // #ifdef JITTER_DISPATCH_NO_THREADING

/* Expand to an expression evaluating to the program point of the first
   instruction in the pointed executable routine, as some object which is
   correct to pass to JITTER_BRANCH.  The expression type will be
   jitter_program_point.

   This macro is usable from within VM instructions, which might for example
   perform a jump to the beginning of a different VM routine.  The expansion of
   this macro is guaranteed not to contain function calls, and is safe to use
   without extra C function wrappers. */
#ifdef JITTER_DISPATCH_NO_THREADING
  /* In this case the executable routine contains a separate pointer to the
     beginning of the executable region for the native routine.  Of course
     the first instruction is at the beginning of the region. */
# define JITTER_EXECUTABLE_ROUTINE_BEGINNING(_jitter_executable_routine_ptr)  \
    ((_jitter_executable_routine_ptr)->native_code)
#else
  /* With switch dispatching or threading the first program point is a pointer
     to the beginning of the specialized routine array. */
# define JITTER_EXECUTABLE_ROUTINE_BEGINNING(_jitter_executable_routine_ptr)  \
    ((jitter_program_point)                                                   \
     ((_jitter_executable_routine_ptr)->specialized_program))
#endif // ifdef JITTER_DISPATCH_NO_THREADING




/* Internal declarations.
 * ************************************************************************** */

/* What follows is not for the user. */

/* Some specialized instruction opcodes whose values must always be valid,
   independently from the VM.  This is checked with assertions at the first
   run of the specializer -- see templates/vm1.c . */
enum jitter_specialized_instruction_opcode
  {
    jitter_specialized_instruction_opcode_INVALID = 0,
    jitter_specialized_instruction_opcode_BEGINBASICBLOCK = 1,
    jitter_specialized_instruction_opcode_EXITVM = 2,
    jitter_specialized_instruction_opcode_DATALOCATIONS = 3,
    jitter_specialized_instruction_opcode_NOP = 4,
    jitter_specialized_instruction_opcode_UNREACHABLE0 = 5,
    jitter_specialized_instruction_opcode_UNREACHABLE1 = 6,
    jitter_specialized_instruction_opcode_PRETENDTOJUMPANYWHERE = 7,
  };

/* Add an opcode to the specialized program which is being built.  This is an
   auxiliary function used by vmprefix_specialize_instruction .  The opcode type
   should actually be the VM-dependent enum
   vmprefix_specialized_instruction_opcode , but it's safe to use a sufficiently
   wide unsigned type so that this code is VM-independent. */
void
jitter_add_specialized_instruction_opcode
   (struct jitter_mutable_routine *p,
    /* This is actually an enum vmprefix_specialized_instruction_opcode , but
       the type is VM-dependent. */
    jitter_uint opcode);

/* Add a fixnum literal to the specialized program which is being built.  This
   is an auxiliary function used by vmprefix_specialize_instruction . */
void
jitter_add_specialized_instruction_literal (struct jitter_mutable_routine *p,
                                            jitter_uint literal);

/* Add a label literal (as an instruction index) to the specialized program
   which is being built.  This is an auxiliary function used by
   vmprefix_specialize_instruction . */
void
jitter_add_specialized_instruction_label_index (struct jitter_mutable_routine *p,
                                                jitter_label_as_index
                                                unspecialized_instruction_index);

#endif // #ifndef JITTER_SPECIALIZE_H_
