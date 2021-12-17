/* VM library: specializer.

   Copyright (C) 2016, 2017, 2018, 2019, 2020 Luca Saiu
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


#include <assert.h>

#include <jitter/jitter-malloc.h>
#include <jitter/jitter-fatal.h>

#include <jitter/jitter.h>
#include <jitter/jitter-instruction.h>
#include <jitter/jitter-mutable-routine.h>
#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-specialize.h>
#include <jitter/jitter-replicate.h>
#include <jitter/jitter-vm.h>
#include <jitter/jitter-mmap.h>  /* For executable deallocation. */


/* Executable routines: initialization and finalization.
 * ************************************************************************** */

/* Initialize the pointed executable routine from the pointed non-executable
   routine, without filling in the actual routine data.
   Fail fatally if the routine already has another executable routine. */
static void
jitter_initialize_executable_routine (struct jitter_executable_routine *er,
                                      struct jitter_mutable_routine *r)
{
  //fprintf (stderr, "[Making an executable routine at %p]\n", er);
  /* Fail if this executable routine is not the first for the routine. */
  if (r->executable_routine != NULL)
    jitter_fatal ("cannot generate an executable routine from %p twice", r);

  /* Link the two routines together thru pointers, so that destroying one can
     update the other. */
  r->executable_routine = er;
  er->routine = r;
  er->vm = r->vm;

  /* Initialize the other fields, where we already have enough information. */
  er->reference_count = 1;
  er->slow_register_per_class_no = r->slow_register_per_class_no;

  /* The remaining fields are still uninitialized. */
}

void
jitter_destroy_executable_routine (struct jitter_executable_routine *er)
{
  //fprintf (stderr, "[Destroying executable routine at %p]\n", er);
  /* If the non-executable routine which was translated into *er still exists
     then unlink this. */
  struct jitter_mutable_routine *r = er->routine;
  if (r != NULL)
    r->executable_routine = NULL;

  /* Check that the reference count is exactly one.  It is sensible to require
     that when this function is called by the user the reference count be
     exactly one; when called by jitter_unpin_executable_routine this field is
     automatically set to 1 before arriving here, just in order to pass this
     check. */
  if (er->reference_count != 1)
    jitter_fatal ("destroying executable routine with reference count %lu",
                  er->reference_count);

  /* Destroy heap-allocated fields reachable from the compiled routine. */
#if   (defined(JITTER_DISPATCH_SWITCH)                \
       || defined(JITTER_DISPATCH_DIRECT_THREADING))
  free (er->specialized_program);
#elif (defined(JITTER_DISPATCH_MINIMAL_THREADING)  \
       || defined(JITTER_DISPATCH_NO_THREADING))
  jitter_executable_deallocate (er->native_code);
#else
# error "unknown dispatch: this should not happen"
#endif /* dispatch */

  /* Release memory for the struct itself. */
  free (er);
}




/* Reference counting.
 * ************************************************************************** */

void
jitter_pin_executable_routine (struct jitter_executable_routine *er)
{
  /* Just increment the field.  This operation is easy and fast, as it never
     involves construction or destruction. */
  er->reference_count ++;
}

void
jitter_unpin_executable_routine (struct jitter_executable_routine *er)
{
  /* Decrement the field.  If the new value does not reach zero then we are
     done. */
  er->reference_count --;
  if (er->reference_count > 0)
    return;

  /* If we arrived here the reference count has reached zero. */

  /* Destroy the mutable companion, if it still exists. */
  struct jitter_mutable_routine *r = er->routine;
  if (r != NULL)
    jitter_destroy_mutable_routine (r);

  /* Destroy the executable routine itself.  Temporarily set the reference count
     field to one, as the executable destruction function expects, then call the
     executable destruction function. */
  er->reference_count = 1;
  jitter_destroy_executable_routine (er);
}




/* Specialization.
 * ************************************************************************** */

void
jitter_add_specialized_instruction_opcode
   (struct jitter_mutable_routine *p,
    /* This is actually an enum vmprefix_specialized_instruction_opcode , but
       the type is VM-dependent. */
    jitter_uint specialized_opcode)
{
  // FIXME: this comment is probably obsolete.
  /* Without replication p->specialized_program holds direct-threaded code (each
     VM thread followed by its zero or more residual arguments); without
     replication p->specialized_program does *not* hold threads, but only
     residual arguments.

     In either case we push a new element onto p->replicated_blocks , which is
     useful in one case at specialization time, and in the other only for
     disassembling. */
  struct jitter_replicated_block replicated_block = {specialized_opcode, NULL, 0};
  jitter_dynamic_buffer_push (& p->replicated_blocks,
                                   & replicated_block,
                                   sizeof (struct jitter_replicated_block));
#ifndef JITTER_REPLICATE
# if   defined(JITTER_DISPATCH_SWITCH)
  union jitter_word w = {.fixnum = specialized_opcode};
# elif defined(JITTER_DISPATCH_DIRECT_THREADING)
  union jitter_word w = {.thread = p->vm->threads [specialized_opcode]};
# else
#   error "replication enabled, but not switch nor direct-threading"
# endif// #if defined(JITTER_DISPATCH_SWITCH)...
  jitter_dynamic_buffer_push (& p->specialized_program, & w, sizeof (w));
#endif // #ifndef JITTER_REPLICATE
}

void
jitter_add_specialized_instruction_literal (struct jitter_mutable_routine *p,
                                            jitter_uint literal)
{
  // fprintf (stderr, "Adding specialized instruction literal %i\n", (int)literal);
  // FIXME: this will need generalization once more literal kinds are supported.
  union jitter_word w = {.ufixnum = literal};
  jitter_dynamic_buffer_push (& p->specialized_program, & w, sizeof (w));
}

void
jitter_add_specialized_instruction_label_index (struct jitter_mutable_routine *p,
                                                jitter_label_as_index
                                                unspecialized_instruction_index)
{
  // fprintf (stderr, "Adding specialized instruction label_index %i\n", (int)unspecialized_instruction_index);
  jitter_int next_word_index
    = jitter_dynamic_buffer_size (& p->specialized_program)
      / sizeof (jitter_int);
  union jitter_word w
    = {.ufixnum  = unspecialized_instruction_index};
  jitter_dynamic_buffer_push (& p->specialized_program, & w, sizeof (w));
  jitter_dynamic_buffer_push (& p->specialized_label_indices,
                              & next_word_index, sizeof (jitter_int));
}

static void
jitter_backpatch_labels_in_specialized_routine (struct jitter_mutable_routine *p)
{
  union jitter_word *specialized_program
    = jitter_dynamic_buffer_to_pointer (& p->specialized_program);
  const jitter_int *specialized_label_indices
    = jitter_dynamic_buffer_to_pointer (& p->specialized_label_indices);
  const jitter_int * const instruction_index_to_specialized_instruction_offset
    = p->instruction_index_to_specialized_instruction_offset;
  const int specialized_label_indices_no
    = jitter_dynamic_buffer_size (& p->specialized_label_indices)
      / sizeof (jitter_int);

  int i;
  for (i = 0; i < specialized_label_indices_no; i ++)
    {
      union jitter_word *argument
        = specialized_program + specialized_label_indices[i];
      argument->pointer
        = (union jitter_word *)
          ((char*)specialized_program
           + instruction_index_to_specialized_instruction_offset
             [argument->ufixnum]);
    }
}

/* Add implicit instructions at the end of an unspecialized program. */
static void
jitter_mutable_routine_add_epilog (struct jitter_mutable_routine *p)
{
  /* Add the final instructions which are supposed to close every VM routine.
     Having each label, including the ones at the very end of the program when
     they exist, associated to an actual unspecialized instruction makes
     replication easier. */
  if (p->options.add_final_exitvm)
    jitter_mutable_routine_append_meta_instruction
       (p, p->vm->exitvm_meta_instruction);
  else
    jitter_mutable_routine_append_meta_instruction
       (p, p->vm->unreachable_meta_instruction);
}




/* Making an executable routine.
 * ************************************************************************** */

struct jitter_executable_routine*
jitter_make_executable_routine (struct jitter_mutable_routine *p)
{
  if (p->stage != jitter_routine_stage_unspecialized)
    jitter_fatal ("specializing non-unspecialized program");
  if (p->expected_parameter_no != 0)
    jitter_fatal ("specializing program with last instruction incomplete");
  if (p->native_code != NULL)
    jitter_fatal ("specializing program with native code already defined");

  /* Add epilog instructions.  This way we can be sure that the program
     ends with an "exitvm" or "unreachable" instruction. */
  jitter_mutable_routine_add_epilog (p);

  /* Resolve label arguments in unspecialized instruction parameters. */
  jitter_mutable_routine_resolve_labels (p);
  /* Now label arguments refer unspecialized instruction indices. */

  /* Compute jump targets. */
  assert (p->jump_targets == NULL);
  p->jump_targets = jitter_mutable_routine_jump_targets (p);

  /* Now that we know how many instructions there are we can allocate
     p->instruction_index_to_specialized_instruction_offset once and for all.
     Its content will still be uninitialized. */
  const int instruction_no = jitter_mutable_routine_instruction_no (p);
  assert (p->instruction_index_to_specialized_instruction_offset == NULL);
  p->instruction_index_to_specialized_instruction_offset
    = jitter_xmalloc (sizeof (jitter_int) * instruction_no);

#ifdef JITTER_REPLICATE
  /* Whenever some specialized instruction is the first one, or is reachable by
     a jump, we need to insert a BEGINBASICBLOCK specialized instruction
     before its specialization.  This is needed for VM branches in minimal-threaded
     code: there are no real threads in the thread array, only residual arguments;
     however for every jump target we have to introduce a thread as the residual
     argument of a BEGINBASICBLOCK special specialized instruction, which does
     nothing but advancing the thread pointer past the argument.

     We need this special case out of the loop to support empty programs, whose
     specialization must still begin by a BEGINBASICBLOCK.  Dealing with
     this case within a loop is messy, because if the first instruction is
     also a jump target then we need to set
       p->instruction_index_to_specialized_instruction_offset [0]
     before adding BEGINBASICBLOCK ; except that
       p->instruction_index_to_specialized_instruction_offset [0]
     it out of bounds if the program is empty.

     In the no-threading case BEGINBASICBLOCK specialized instructions are
     redundant but they cost nothing at run time, since they expand to zero
     assembly instructions.  FIXME: can I just not generate them in the
     no-threading case by changing CPP conditionals?  Test. */
  if (instruction_no == 0)
    jitter_insert_beginbasicblock (p);
#endif // #ifdef JITTER_REPLICATE

  /* Specialize instructions, filling
     p->instruction_index_to_specialized_instruction_offset at the same time. */
  const struct jitter_instruction **instructions
    = (const struct jitter_instruction **)
      jitter_dynamic_buffer_to_pointer (& p->instructions);
  int (* const specialize_instruction) (struct jitter_mutable_routine *p,
                                        const struct jitter_instruction *ins)
    = p->vm->specialize_instruction;
  int instruction_index = 0;
  while (instruction_index < instruction_no)
    {
      const struct jitter_instruction *next_instruction
        = instructions [instruction_index];

      /* The next specialized instruction will begin where the (current) free
         space in specialized program begins.  Notice that this may leave
         uninitialized holes in
         p->instruction_index_to_specialized_instruction_offset , at the indices
         corresponding to some instructions specialized into
         superinstructions.  Also notice that the next generated native instructions
         might belong to a translation of BEGINBASICBLOCK . */
      p->instruction_index_to_specialized_instruction_offset [instruction_index]
        = jitter_dynamic_buffer_size (& p->specialized_program);

#ifdef JITTER_REPLICATE
      /* See the comment above about BEGINBASICBLOCK . */
      if (instruction_index == 0 || p->jump_targets [instruction_index])
        jitter_insert_beginbasicblock (p);
#endif // #ifdef JITTER_REPLICATE

      /* Specialize the next instruction, obtaining as result the number of
         unspecialized instructions covered by the one specialized instruction
         they are translated into.  This adds as many words as needed to
         p->specialized_program . */
      instruction_index += specialize_instruction (p, next_instruction);
    }

  /* Notice that p->instruction_index_to_specialized_instruction_offset is
     accessed only when needed to patch labels, and it's harmless to leave its
     content undefined in places where no labels are involved. */

  /* Now that p->instruction_index_to_specialized_instruction_offset is filled
     we have enough information to resolve label literals. */
  jitter_backpatch_labels_in_specialized_routine (p);

  /* The program is now specialized.  FIXME: shall I free p->jump_targets
     and set it to NULL now? */
  p->stage = jitter_routine_stage_specialized;

#ifdef JITTER_REPLICATE
  /* If replication is enabled then build the native code; this will change the
     program stage again. */
  jitter_replicate_program (p);
#endif // #ifdef JITTER_REPLICATE

  /* Make an executable routine containing what we need to actually run the
     code. */
  struct jitter_executable_routine *res
    = jitter_xmalloc (sizeof (struct jitter_executable_routine));
  jitter_initialize_executable_routine (res, p);

  /* Transfer the relevant fields from the non-executable routine to the
     executable routine, invalidating the originals to avoid double freeing. */

  /* Set the specialized_program field, where it exists. */
#if (defined(JITTER_DISPATCH_SWITCH)                 \
     || defined(JITTER_DISPATCH_DIRECT_THREADING)    \
     || defined(JITTER_DISPATCH_MINIMAL_THREADING))
  res->specialized_program
    = jitter_dynamic_buffer_extract (& p->specialized_program);
#elif defined(JITTER_DISPATCH_NO_THREADING)
  /* Nothing. */
#else
# error "unknown dispatch: this should not happen"
#endif /* dispatch */

  /* Set the native_code and native_code_size fields, where they exist. */
#if (defined(JITTER_DISPATCH_SWITCH)             \
  || defined(JITTER_DISPATCH_DIRECT_THREADING))  \
  /* Nothing. */
#elif (defined(JITTER_DISPATCH_MINIMAL_THREADING)  \
     || defined(JITTER_DISPATCH_NO_THREADING))
  res->native_code = p->native_code;
  res->native_code_size = p->native_code_size;
  p->native_code = NULL;
#else
# error "unknown dispatch: this should not happen"
#endif /* dispatch */

  /* Return a pointer to the executable routine. */
  return res;
}
