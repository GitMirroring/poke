/* Jitter: replication functionality.

   Copyright (C) 2016, 2017, 2018, 2019 Luca Saiu
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


/* Do nothing if replication is not used.
 * ************************************************************************** */

#include <jitter/jitter-replicate.h>

/* This source file expands to nothing more if replication is disabled. */
#ifdef JITTER_REPLICATE




/* Include headers.
 * ************************************************************************** */

#include <stdio.h>
#include <assert.h>
#include <string.h> /* for memcpy */

#include <jitter/jitter.h>
/* Including jitter/jitter.h indirectly causes the inclusion of
   jitter/jitter-config.h , which is useful to know whether
   JITTER_MACHINE_SUPPORTS_PROCEDURE is defined: in this way we can avoid
   generating code for loading the implicit return address residual argument. */

#include <jitter/jitter-malloc.h>
#include <jitter/jitter-mmap.h>
#include <jitter/jitter-fatal.h>

#include <jitter/jitter-specialize.h>
#include <jitter/jitter-instruction.h>
#include <jitter/jitter-mutable-routine.h>
#include <jitter/jitter-vm.h>
#include <jitter/jitter-patch-in.h>
#include <jitter/jitter-fast-branch.h>


#ifdef JITTER_HAVE_ASSEMBLY
  /* Include architecture-specific icache flushing code. */
# include <jitter/jitter-machine-common.h>
#endif // #ifdef JITTER_HAVE_ASSEMBLY




/* Replication.
 * ************************************************************************** */

#ifdef JITTER_DISPATCH_NO_THREADING
# include <jitter/jitter-patch.h>
# include <jitter/machine/jitter-machine.h>

/* A structure containing enough information to resolve label references after
   the full native code is generated, and backpatch a native code address into
   a residual load snippet.  */
struct jitter_backpatch
{
#ifdef JITTER_HAVE_PATCH_IN
  /* A pointer to the patch-in descritor for this backpatch, if any; if NULL
     then the backpatch is for loading a residual, so there is no corresponding
     patch-in.  FIXME: shall I *always* use patch-ins instead?  It would be easy
     to machine-generate them at the beginning of VM instructions. */
  const struct jitter_patch_in_descriptor *patch_in_descriptor;
#endif //#ifdef JITTER_HAVE_PATCH_IN

  /* The native code of the load residual snippet we have to backpatch. */
  char *native_code;

  /* An residual parameter index relative to the specialized instruction,
     0-based. */
  unsigned residual_index;

  /* The thread argument to be translated into a native code address. */
  union jitter_word thread;

  /* Machine-specific snippet identifier. */
  enum jitter_snippet_to_patch snippet;
};
#endif // #ifdef JITTER_DISPATCH_NO_THREADING

/* Return the given address aligned to the given alignment, skipping up to
   alignment bytes minus one *forward*.  If the required alignment is one or
   zero then the result is the given address, unchanged. */
__attribute__ ((unused))
 static char *
jitter_align_branch_target (char *address, size_t alignment)
{
  char *res = address;
  if (alignment != 0)
    {
      size_t original_alignment
        = ((jitter_uint) address) % alignment;
      if (original_alignment != 0)
        res += alignment - original_alignment;
    }
  return res;
}

/* FIXME: the internal implementation of this needs to be cleaned up. */
void
jitter_replicate_program (struct jitter_mutable_routine *p)
{
  if (p->stage != jitter_routine_stage_specialized)
    jitter_fatal ("replicating non-specialized program");

#define fprintf(...) /* nothing */
  /* Compute a safe upper bound on the code size. */
  const int specialized_instruction_no
    = jitter_dynamic_buffer_size (& p->replicated_blocks)
      / sizeof (struct jitter_replicated_block);
  // FIXME: this is probably safe in most practical cases, but not correct nor efficient.
  // FIXME: implement an integer division rounding up in the utility library.
  size_t code_length = specialized_instruction_no * 400;

  char *code = jitter_executable_allocate (code_length);
  fprintf (stderr, "native code is in [%p, %p)\n",
           code, code + code_length);
//#define fprintf(...) /* nothing */

  // FIXME: just for testing/stressing the system, set the whole executable code
  // space to some known pattern.
  memset (code, 0xff, code_length);

  assert (p->native_code == NULL);
  p->native_code = code;
  /* The native code size will be set later, when we know it exactly. */

  struct jitter_replicated_block * const replicated_blocks
    = jitter_dynamic_buffer_to_pointer (& p->replicated_blocks);
  char *specialized_instructions
    = jitter_dynamic_buffer_to_pointer (& p->specialized_program);
  //printf ("specialized_instructions is %p\n", specialized_instructions);
  char *free_code = code;
  //printf ("specialized_instructions are %i\n", specialized_instruction_no);
  /* fprintf (stderr, "There are %i instructions.\n", instruction_no); */
  /*
  char *from = vmprefix_threads[0];
  char *to = ((char*)vmprefix_threads[VMPREFIX_SPECIALIZED_INSTRUCTION_NO - 1])
             + vmprefix_thread_sizes[VMPREFIX_SPECIALIZED_INSTRUCTION_NO - 1];
  fprintf (stderr,
           "The original instruction code ranges from %p to %p (%li bytes).\n",
           from,
           to,
           (long)(to - from));
  */
#ifdef JITTER_DISPATCH_NO_THREADING
  /* A dynamic array of struct jitter_backpatch elements, to be filled when
     generating code and scanned at the end to resolve references. */
  struct jitter_dynamic_buffer backpatches;
  jitter_dynamic_buffer_initialize (& backpatches);
#endif // #ifdef JITTER_DISPATCH_NO_THREADING
  int i;
  union jitter_word *next_thread
    = (union jitter_word *) specialized_instructions;
  for (i = 0; i < specialized_instruction_no; i ++)
    {
      /* Find which specialized instruction we are dealing with. */
      struct jitter_replicated_block *replicated_block = replicated_blocks + i;
      enum jitter_specialized_instruction_opcode opcode
        = replicated_block->specialized_opcode;
      bool relocatable __attribute__ ((unused))
        = p->vm->specialized_instruction_relocatables [opcode];
      bool caller __attribute__ ((unused))
        = p->vm->specialized_instruction_callers [opcode];
      bool is_beginbasicblock_before_callee __attribute__ ((unused))
        = (   (opcode == jitter_specialized_instruction_opcode_BEGINBASICBLOCK)
           && p->vm->specialized_instruction_callees
                 [(replicated_block + 1)->specialized_opcode]);
      size_t instruction_size = p->vm->thread_sizes [opcode];
      size_t residual_arity
        = p->vm->specialized_instruction_residual_arities [opcode];

      /* FIXME: this is currently disabled.  The problem is that when I
         allocated code memory simply with mmap, I got a very wide alignment for
         free; so if the very first VM instruction was procedure-prolog, this
         conditional didn't fire.  That turned out to be important, as the first
         native instructions (associated to the beginning) are not initialized:
         In practice I obtained, automatically, that the native code block
         started with the first instruction.
         Switching from mmap to jitter-mmap, whose alignment guarantees are
         weaker, broke this assumption. */
      /* /\* If this is the beginning of a callee basic block, align it: since */
      /*    callees can only be reached thru branch-and-link we can be sure that */
      /*    the skipped bytes will never be executed. */

      /*    FIXME: it would be nice to do the same for other branch targets as long */
      /*    as we can prove that they are not also reachable by fallthru */
      /*    control. *\/ */
      /* if (is_beginbasicblock_before_callee) */
      /*   free_code = jitter_align_branch_target (free_code, JITTER_CALLEE_ALIGNMENT); */

      /* Keep a pointer to the native code we are about to generate, coming next
         in the space we allocated for code, in the appropriate replicated
         block.  This will be useful later, for backpatching labels and for
         disassembly.  We do not know the native code size yet. */
      replicated_block->native_code = free_code;

      /* If the opcode is !BEGINBASICBLOCK then patch the residual argument,
         which is actually a thread: make it point to the code we are going to
         generate next. */
      if (opcode == jitter_specialized_instruction_opcode_BEGINBASICBLOCK)
        next_thread->thread = free_code;

#ifdef JITTER_DISPATCH_MINIMAL_THREADING
      /* If the specialized instruction is non-relocatable then fix the return
         label, which is always the last residual argument.  There is no need to
         backpatch this, since we know where the next relocated instruction
         begins: it's always right after this one.  Notice that we can't just do
         this in the case of no-threading dispatch: that is more complicated,
         since we can't know exactly where the next relocated instruction begins
         until we also generate code for loading residuals. */
      if (! relocatable)
        next_thread [residual_arity - 1].pointer
          = (void*) (free_code + instruction_size);

      /* The caller case would be conceptually similar to the non-relocatable
         case; the argument in that case would a thread, pointing to the very
         next cell after the argument where the next specialized instruction
         begins.  However we do not actually use that argument: it's faster to
         just add a known small constant to the current thread pointer, and this
         is what JITTER_BRANCH_AND_LINK does for minimal-threading dispatch.
         Just to be clean and make the thing visible, set the unused parameter
         to minus one.
         Notice that non-relocatable callers behave like ordinary
         non-relocatables, and not like callers. */
      else if (caller)
        next_thread [residual_arity - 1].fixnum = -1;
#endif // #ifdef JITTER_DISPATCH_MINIMAL_THREADING

#ifdef JITTER_DISPATCH_NO_THREADING
      /*
      printf ("opcode: %li (%s); residual arity: %li\n",
              (long) opcode,
              vmprefix_specialized_instruction_names [opcode],
              (long) residual_arity);
      */
      int j, jout_non_fast_label;
      int jout_fast_label __attribute__ ((unused));
      fprintf (stderr, "* opcode %u\n", opcode);
      if (opcode != jitter_specialized_instruction_opcode_BEGINBASICBLOCK)
        for (j = jout_non_fast_label = 0; j < residual_arity; j ++)
        {
          union jitter_word immediate // FIXME: ugly.  Get rid of union uninspired_specialized_word .
            = {.pointer = (void*)(next_thread [j]).pointer};
          bool is_label
            = p->vm->specialized_instruction_label_bitmasks [opcode]
              & (1UL << j);
          bool is_fast_label
#ifdef JITTER_HAVE_PATCH_IN
            = (p->vm->specialized_instruction_fast_label_bitmasks [opcode]
               & (1UL << j));
#else
            = false;
#endif // #ifdef JITTER_HAVE_PATCH_IN
          assert (! is_fast_label || is_label); // is_fast_label implies is_label .

          fprintf (stderr,
                   "  - residual argument %li (out_non_fast_label %li) of %li-1 (is it a label? %s) 0x%"JITTER_PRIu"x ("JITTER_INT_FORMAT") ("JITTER_UINT_FORMAT")\n",
                   (long)j, (long)jout_non_fast_label, (long)residual_arity,
                   (is_label ? (is_fast_label ? "yes, fast" : "yes, slow") : "no"),
                   (jitter_int)immediate.fixnum,
                   (jitter_int)immediate.fixnum,
                   (jitter_uint)immediate.fixnum);

          /* If this is the last residual of a non-relocatable or caller
             specialized instruction then we have to set the argument to the 
             return pointer.
             The return address is the beginning of the next replicated VM
             instruction, which we don't actually know, as it will come after
             the snippet to patch in the value we are computing.  Just like in
             the following case about labels we have to be pessimistic and
             possibly reserve space for a snippet slightly longer than needed,
             but working for any possible label in the useful range.
             FIXME: actually be more pessimistic: there should ge a
             jitter_snippet_for_loading_pessimistic , yielding a snippet working
             for a sufficiently wide range of arguments. */
          if ((! relocatable || caller) && j == residual_arity - 1)
            {
              /* This doesn't keep into account the snippet to load the return
                 target itself. */
              const char *sample_target
                = free_code + instruction_size;
              enum jitter_snippet_to_patch sample_snippet
                = jitter_snippet_for_loading ((const char*) & sample_target,
                                              jout_non_fast_label,
                                              free_code);
              const size_t sample_snippet_size
                = jitter_snippet_size (sample_snippet);

              /* Now we have an estimate of the snippet size.  Assuming that's
                 correct use it to compute the actual return target, and check
                 again what snippet will be needed to load the actual target.
                 If its size is the same as the eastimate we're golden.
                 Otherwise, currently, we bail out. */
              const char *actual_target = sample_target + sample_snippet_size;
              enum jitter_snippet_to_patch actual_snippet
                = jitter_snippet_for_loading ((const char*) & actual_target,
                                              jout_non_fast_label,
                                              free_code);
              const size_t actual_snippet_size
                = jitter_snippet_size (actual_snippet);
              if (sample_snippet_size != actual_snippet_size)
                jitter_fatal ("replication: failed estimating non-relocatable return address size");

              /* If we arrived here we know the actual target.  Set it as the
                 thread argument.  This will be loaded with a snippet below, as
                 any other literal argument.  Also setting the correct argument
                 in the thread field is useful for disassembly. */
              immediate.pointer
                = next_thread [residual_arity - 1].pointer
                = (void*) actual_target;
            }

          enum jitter_snippet_to_patch snippet;
          if (is_label)
            {
              /* We do not necessarily know the referred native code address at
                 this point; rather we want to be able to backpatch in any
                 native code address in the range we am using; free_code is just
                 a plausible example of what an actual code address might look
                 like.
                 FIXME: be more pessimistic: there should ge a
                 jitter_snippet_for_loading_pessimistic , yielding a snippet
                 working for a sufficiently wide range of arguments. */
              const char *sample_target = free_code;
              snippet = jitter_snippet_for_loading ((const char*)
                                                    & sample_target,
                                                    jout_non_fast_label,
                                                    free_code);
            }
          else
            snippet = jitter_snippet_for_loading ((const char*) & immediate,
                                                  jout_non_fast_label,
                                                  free_code);

          /* What follows up until the end of the loop body is not for fast
             labels, which are handled separately below. */
          if (is_fast_label)
            continue;

          /* The implicit residual return address argument for callers is
             useless when the implementation supports machine-specific
             procedures: in this case just skip the snippet... */
          bool can_use_native_procedures
#ifdef JITTER_MACHINE_SUPPORTS_PROCEDURE
            = true;
#else
            = false;
#endif // #ifdef JITTER_MACHINE_SUPPORTS_PROCEDURE

          /* ...Unless this specialised instructions is non-relocatable: in that
             case the return address must be materialised. */
          if (! relocatable)
            can_use_native_procedures = false;

          if (caller && j == residual_arity - 1 && can_use_native_procedures)
          {
            /* Replace the unused specialized argument with minus one, to make
               it more evident when disassembling. */
            next_thread [j].fixnum = -1;

            /* The argument we ignored was not a fast label. */
            jout_non_fast_label ++;

            /* Do not insert the snippet.  A break would work as well here. */
            continue;
          }

          size_t load_residual_code_size = jitter_snippet_size (snippet);
          jitter_copy_snippet (free_code, snippet);
          if (is_label)
            {
              /* We cannot resolve a label references if it refers a thread we
                 have not compiled yet.  Keep the information in a structure to
                 scan later. */
              struct jitter_backpatch bp;
#ifdef JITTER_HAVE_PATCH_IN
              bp.patch_in_descriptor = NULL;
#endif //#ifdef JITTER_HAVE_PATCH_IN
              bp.native_code = free_code;
              bp.residual_index = jout_non_fast_label;
              bp.thread = immediate;
              bp.snippet = snippet;
              jitter_dynamic_buffer_push (& backpatches, & bp,
                                          sizeof (struct jitter_backpatch));
            }
          else
            jitter_patch_load_immediate (free_code, jout_non_fast_label,
                                         (const char*) & immediate, snippet);

          /* size_t load_residual_code_size = vmprefix_native_nop_snippet_size; */
          /* printf ("%li bytes\n", (long) load_residual_code_size); */
          /* memcpy (free_code, vmprefix_native_nop_snippet, load_residual_code_size); */
          free_code += load_residual_code_size;

          /* If we arrived here then the j-th parameter is not a fast label.
             Increment the output counter. */
          jout_non_fast_label ++;

          // Testing a manually-encoded instruction..
          //unsigned char test[100] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
          //memcpy (free_code, test, sizeof (test));
          //free_code += sizeof (test);
        }
#endif // #ifdef JITTER_DISPATCH_NO_THREADING

      /* Copy the instruction identified by the opcode to the mmapped executable
         space, and replace the opcode with a pointer to the new code. */
      memcpy (free_code, p->vm->threads [opcode], instruction_size);

#if defined(JITTER_DISPATCH_NO_THREADING) && defined(JITTER_HAVE_PATCH_IN)
      /* Patch fast labels. */
      if (p->vm->specialized_instruction_fast_label_bitmasks [opcode]) // If there are fast labels...
        for (j = jout_fast_label = 0; j < residual_arity; j ++)
        {
          /* Skip the argument if it's not a fast label. */
          if (! (p->vm->specialized_instruction_fast_label_bitmasks [opcode]
                 & (1UL << j)))
            continue;

          union jitter_word immediate // FIXME: ugly.  Get rid of union uninspired_specialized_word .
            = {.pointer = (void*)(next_thread [j]).pointer};
          fprintf (stderr,
                   "# The %li-th argument (of %li-1) is the %li-th (0-based) fast label\n",
                   (long) j, (long) residual_arity, (long) jout_fast_label);
          fprintf (stderr, "  Its value is %p\n", immediate.pointer);

          /* Look for any appropriate patch-in descriptors referring this label.
             I cannot really rely on the order or the number of patch-ins here,
             so I will scan every patch-in for this specialized instruction.  In
             any sensible case they will not be more than a few. */
          const struct patch_in_table_entry *pite
            = p->vm->patch_in_table + opcode;
          int pi;
          for (pi = 0; pi < pite->descriptor_no; pi ++)
            {
              const struct jitter_patch_in_descriptor *pid
                = pite->descriptors [pi];
              if (pid->residual_index == jout_fast_label)
                {
                  fprintf (stderr, "  Found a patch-in descriptor for it at %p\n", pid);
                  //jitter_dump_patch_in_descriptor_with_prefix (stdout, "    ", pid);

                  struct jitter_backpatch bp;
                  bp.patch_in_descriptor = pid;
                  bp.native_code = free_code + pid->offset;
                  bp.residual_index = j;
                  bp.thread = immediate;
                  bp.snippet = jitter_snippet_for_patch_in (pid);
                  jitter_dynamic_buffer_push (& backpatches, & bp,
                                              sizeof (struct jitter_backpatch));
              }
            }

          /* We have processed one fast label.  Advance the output counter. */
          jout_fast_label ++;
        }
#endif // #if defined(JITTER_DISPATCH_NO_THREADING) && defined(JITTER_HAVE_PATCH_IN)

      /* The next instruction will come right after this one, without a threaded
         dispatch unless there is a VM branch: just advance the pointer into the
         executable space. */
      free_code += instruction_size;

      /* Now we know the native code size for this block including additional
         instructions to load residuals, if any. */
      replicated_block->native_code_size
        = (char*)free_code - (char*)replicated_block->native_code;

      /* Keep track of where this specialized instruction ends in the thread
         array, so that we may patch the residual arguments of other
         !BEGINBASICBLOCK specialized instructions we find later. */
      next_thread += p->vm->specialized_instruction_residual_arities [opcode];
    }
  size_t written_bytes = free_code - code;

#ifdef JITTER_DISPATCH_NO_THREADING
  /* Backpatch labels: while there are still backpatches to perform
     on the array pop one and perform it. */
  while (jitter_dynamic_buffer_size (& backpatches) > 0)
    {
      struct jitter_backpatch *bp
        = (((struct jitter_backpatch*)
            jitter_dynamic_buffer_first_unused_char (& backpatches))
           - 1);
      const char *target_native_code = (const char*) bp->thread.pointer->pointer;

#ifdef JITTER_HAVE_PATCH_IN
      const struct jitter_patch_in_descriptor *pid = bp->patch_in_descriptor;
      if (pid != NULL)
        {
          fprintf (stderr, "Backpatching with patch-in descriptor at %p (native code at %p, %u bytes), target %p\n",
                   pid, bp->native_code, (unsigned) pid->length, target_native_code);
          fprintf (stderr, "Pathing-in at [%p, %p]\n",
                  bp->native_code,
                  ((char *) bp->native_code) + jitter_snippet_size (bp->snippet) - 1);
          jitter_copy_snippet (bp->native_code, bp->snippet);
          jitter_patch_patch_in (bp->native_code,
                                 (const char*) & target_native_code,
                                 pid,
                                 bp->snippet);
        }
      else
#endif // ifdef JITTER_HAVE_PATCH_IN
        {
          /*
            printf ("Backpatching immediate label arg to %p (was %p)\n",
            target_native_code.pointer, bp->thread.pointer);
          */
          jitter_patch_load_immediate (bp->native_code, bp->residual_index,
                                       (const char*)& target_native_code,
                                       bp->snippet);
        }

      jitter_dynamic_buffer_pop (& backpatches,
                                 sizeof (struct jitter_backpatch));
    }
  jitter_dynamic_buffer_finalize (& backpatches);
#endif // #ifdef JITTER_DISPATCH_NO_THREADING

  /* Release unneeded memory at the end of the object. */
  jitter_executable_shrink_in_place (code, written_bytes);

  fprintf (stderr, "The written code is %li bytes (of %li estimated bytes: %.2f%%)\n",
           (long)written_bytes, (long)code_length,
           written_bytes / (double)code_length * 100.0);
  if (written_bytes > code_length)
    jitter_fatal ("generated more than the safe size upper bound: this should never happen");

  /* Set the exact code size length. */
  p->native_code_size = written_bytes;
  
  /* Invalidate the icache, at the logical addresses where we wrote.  This GCC
     builtin for doing so is not even needed on a few architectures (i386 and
     x86_64).  On others it is necessary and sufficient (I see that on my Ben
     NanoNote (MIPS)); on others (for example PowerPC) the builtin is not
     sufficient, and some machine-specific code must be called.  */
  __builtin___clear_cache (code, code + written_bytes);
#ifdef VMPREFIX_HAS_ASSEMBLY
  jitter_invalidate_icache (code, written_bytes);
#endif // #ifdef VMPREFIX_HAS_ASSEMBLY

  /* The program is now replicated. */
  p->stage = jitter_routine_stage_replicated;
}

void
jitter_insert_beginbasicblock (struct jitter_mutable_routine *p)
{
  /* Add a !BEGINBASICBLOCK specialized instruction.  Its residual argument
     will be the thread, to be filled in later when known. */
  jitter_add_specialized_instruction_opcode
    (p, jitter_specialized_instruction_opcode_BEGINBASICBLOCK);
  jitter_add_specialized_instruction_literal (p, 0);
}

#endif // #ifdef JITTER_REPLICATE
