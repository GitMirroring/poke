/* Jitter: patch-in header.

   Copyright (C) 2017, 2018, 2019, 2020 Luca Saiu
   Updated in 2021 by Luca Saiu
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


#ifndef JITTER_PATCH_IN_H_
#define JITTER_PATCH_IN_H_


/* Include core headers.
 * ************************************************************************** */

/* Include macro definitions about whether we have a machine file, and about the
   dispatching model. */
#include <jitter/jitter.h>

/* Include sectioning macros, if we have section support for this platform. */
#if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
# include <jitter/jitter-sections.h>
#endif // #if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)




/* Do nothing if not using patch-ins.
 * ************************************************************************** */

/* First include the machine header which may contain a CPP definition of
   JITTER_MACHINE_SUPPORTS_PATCH_IN ; this only makes sense if the dispatch is
   no-threading, which is only possible if assembly is enabled.
   If the dispatch is different from no-threading patch-ins are always
   disabled. */
#if defined(JITTER_DISPATCH_NO_THREADING)
# include <jitter/jitter-machine-common.h>
# include <jitter/machine/jitter-machine.h>
# ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
#   define JITTER_HAVE_PATCH_IN 1
# endif // #ifdef JITTER_MACHINE_SUPPORTS_PATCH_IN
#endif //#if defined(JITTER_DISPATCH_NO_THREADING)

/* The rest of this header expands to nothing if patch-ins are not supported in
   this configuration. */
#ifdef JITTER_HAVE_PATCH_IN




/* Include headers.
 * ************************************************************************** */

#include <stdio.h>
#include <stdbool.h>

#include <jitter/jitter.h>
#include <jitter/jitter-cpp.h>
#include <jitter/jitter-defect.h>
#include <jitter/jitter-dynamic-buffer.h>




/* Introduction.
 * ************************************************************************** */

/* The generated code in .text , to be expanded from VM instructions, will
   either skip enough bytes for an instruction to be patched-in later, or will
   generate temporary instructions with some argument to be replaced later; at
   the same time, the code generates a patch-in descriptor in a separate
   subsection, referring the preliminary code to patch with enough information
   to fill-in the missing data after replication.

   GCC may optimize away an entire patch-in, when the code is unreachable or, at
   least in theory, duplicate it.  This is not a problem: if there is no code to
   patch, no descriptor will refer it; in other words the code in .text 0 and in
   the subsections are treated as a unit, either both present or both optimized
   away.  For each code sequence to patch there will be one descriptor.

   This relies on the functionality in jitter-sections.h . */




/* Fast-branch descriptor data structures: C API.
 * ************************************************************************** */

/* These structures are accessed from C, but their only definition is in assembly
   code; see the page right below this one. */

/* The descriptor for a specific patch-in.  Instances of this are defined in
   assembly, but are meant to be accessed from C as part of a global array.
   It is convenient that every field be word-sized.  Reordering fields requires
   changes in the assembly code below. */
struct jitter_patch_in_descriptor
{
  /* The specialized instruction opcode for this patch-in. */
  jitter_uint specialized_instruction_opcode;

  /* The patch-in offset, in bytes, from the beginning of the specialized
     instruction code. */
  jitter_uint offset;

  /* The patch-in length, in bytes.  This information might be redundant if
     patch_in_case assumes a known length, as it is the case now, but I keep
     this for future extensibility. */
  jitter_uint length;

  /* The patch-in case, indicating the kind of patching to perform.  In order to
     prevent mistakes, zero will not be used as a valid case. */
  jitter_uint patch_in_case;

  /* The following fields are case-dependent arguments, not all used for every
     case. */

  /* 0-th case-dependent word.  Each word is implemented as a union of
     word-sized elements, to make access more convenient from C code. */
  union
  {
    /* For a fast goto: fast label index, 0-based. */
    jitter_uint residual_index;

    /* Generic 0-th word, as an unsigned datum. */
    jitter_uint case_dependent_word_0_uint;

    /* Generic 0-th word, as an signed datum. */
    jitter_int  case_dependent_word_0_int;
  } __attribute__ ((packed));

  /* 1-st case-dependent word, not used yet. */
  union
  {
    jitter_uint case_dependent_word_1_uint;
    jitter_int  case_dependent_word_1_int;
  } __attribute__ ((packed));

  /* 2-nd case-dependent word, not used yet. */
  union
  {
    jitter_uint case_dependent_word_2_uint;
    jitter_int  case_dependent_word_2_int;
  } __attribute__ ((packed));

  /* 3-rd case-dependent word, not used yet. */
  union
  {
    jitter_uint case_dependent_word_3_uint;
    jitter_int  case_dependent_word_3_int;
  } __attribute__ ((packed));
} __attribute__ ((packed));




/* Prefix-dependent names for globals.
 * ************************************************************************** */

/* The name of the global descriptor vector. */
#define JITTER_PATCH_IN_DESCRIPTORS_NAME(_jitter_vm_the_prefix)  \
  JITTER_CONCATENATE_TWO(_jitter_vm_the_prefix,                  \
                         _patch_in_descriptors)

/* The name of the global descriptor vector. */
#define JITTER_PATCH_IN_DESCRIPTORS_SIZE_IN_BYTES_NAME(_jitter_vm_the_prefix)  \
  JITTER_CONCATENATE_TWO(JITTER_PATCH_IN_DESCRIPTORS_NAME(                     \
                           _jitter_vm_the_prefix),                             \
                         _size_in_bytes)

/* Expand to extern declaration of the variables defined in assembly, to be used
   from C. */
#define JITTER_PATCH_IN_DESCRIPTOR_DECLARATIONS_(_jitter_vm_the_prefix)   \
  extern const struct jitter_patch_in_descriptor                          \
  JITTER_PATCH_IN_DESCRIPTORS_NAME(_jitter_vm_the_prefix) [];             \
  extern const jitter_uint                                                \
  JITTER_PATCH_IN_DESCRIPTORS_SIZE_IN_BYTES_NAME(_jitter_vm_the_prefix);




/* Patch-in descriptor macros.
 * ************************************************************************** */

/* Each patch-in descriptor contains information about one patch-in
   placeholder. */

/* Expand to a string literal containing the .rodata subsection number
   containing the patch-in descriptors. */
#define JITTER_ASM_PATCH_IN_SUBSECTION  \
  "10"

/* Expand to an inline asm statement containing the patch-in header.  Here
   volatility serves to force this statement to be emitted before the other asm
   volatile statements within the executor and before the footer, therefore
   forcing a correct order in the generated assembly file. */
#define JITTER_PATCH_IN_HEADER(_jitter_vm_the_prefix)                 \
  asm (JITTER_ASM_OPEN_DEFINITION                                     \
          (JITTER_ASM_PATCH_IN_SUBSECTION,                            \
          JITTER_PATCH_IN_DESCRIPTORS_NAME (_jitter_vm_the_prefix)))

/* Expand to an inline asm statement containing the patch-in footer.  See the
   comment above.*/
#define JITTER_PATCH_IN_FOOTER(_jitter_vm_the_prefix)                  \
  asm (JITTER_ASM_CLOSE_DEFINITION                                     \
          (JITTER_ASM_PATCH_IN_SUBSECTION,                             \
           JITTER_PATCH_IN_DESCRIPTORS_NAME (_jitter_vm_the_prefix)))




/* Patch-in placeholder macros.
 * ************************************************************************** */

/* Expand to a string literal meant to be used as the the assembly template for
   a patch-in, referring the named input argument
   "jitter_vm_instruction_beginning".

   The "l" in the assembly template does *not* indicate a "gotolabel" argument
   for asm goto: jitter_vm_instruction_beginning is not passed as a gotolabel,
   but as an ordinary input argument.
   I used to pass the argument as a gotolabel, but the label was sometimes
   resolved to a slightly different address compared to what is computed by the
   prefix && operator under GCC 6 -- which broke patch-ins.  Elsewhere in Jitter
   I use prefix && to delimit the beginning and end of VM instruction code, so
   it is sensible to always compute these address the same way.

   The "l" modifier serves to omit the immediate-argument prefix "$" on i386 and
   x86_64, which would be syntactically incorrect in this context, out of an
   instruction.
   This "l" is documented in the GCC manual as one of the "x86 Operand
   Modifiers", but I see it working on any architecture -- doing nothing on
   architectures different from i386 and x86_64.  Should GCC's behavior change
   in the future it will be trivial to replace "l" with an architecture-specific
   macro expanding to "l" on i386 and x86_64 and nothing on every other. */
#define JITTER_ASM_PATCH_IN_PLACEHOLDER(size_in_bytes, case,          \
                                        arg0, arg1, arg2, arg3)       \
  JITTER_ASM_COMMENT_UNIQUE("Patch-in " JITTER_STRINGIFY(case))       \
  "\n1:\n\t"                                                          \
  JITTER_ASM_SKIP_BYTES(size_in_bytes) "\n"                           \
  JITTER_ASM_ENTER_SUBSECTION(JITTER_ASM_PATCH_IN_SUBSECTION)         \
    JITTER_ASM_WORD " "                                               \
      JITTER_STRINGIFY(JITTER_SPECIALIZED_INSTRUCTION_OPCODE) "\n\t"  \
    JITTER_ASM_WORD " (1b - %l[jitter_vm_instruction_beginning])\n\t" \
    JITTER_ASM_WORD " " JITTER_STRINGIFY(size_in_bytes) "\n\t"        \
    JITTER_ASM_WORD " " JITTER_STRINGIFY(case) "\n\t"                 \
    JITTER_ASM_WORD " " JITTER_STRINGIFY(arg0) "\n\t"                 \
    JITTER_ASM_WORD " " JITTER_STRINGIFY(arg1) "\n\t"                 \
    JITTER_ASM_WORD " " JITTER_STRINGIFY(arg2) "\n\t"                 \
    JITTER_ASM_WORD " " JITTER_STRINGIFY(arg3) "\n"                   \
  JITTER_ASM_EXIT_SUBSECTION

/* Expand to the (named) input constraint jitter_vm_instruction_beginning for an
   inline asm template generated by JITTER_ASM_PATCH_IN_PLACEHOLDER.

   The constraint is given as "X" rather than the more intuitive "i" because of
   the SH architecture, where that constraint would fail because of the small
   immediate size.
   Should this prove inadequate, it will be easy to make the constraint an
   architecture-specific macro in the future.  Anyway a failure will be very
   visible, since any problem caused by this will happen early, at compile
   or assemble time. */
#define JITTER_INPUT_VM_INSTRUCTION_BEGINNING             \
  [jitter_vm_instruction_beginning]                       \
     "X" (&& JITTER_SPECIALIZED_INSTRUCTION_BEGIN_LABEL)

/* Likewise for the instruction end. */
#define JITTER_INPUT_VM_INSTRUCTION_END                 \
  [jitter_vm_instruction_end]                           \
     "X" (&& JITTER_SPECIALIZED_INSTRUCTION_END_LABEL)

/* Expand to a C statement (currently not protected with do..while (false), as
   this is meant to be only used within other macros) containing the given
   patch-in for an unconditional VM branch.

   Notice how the (only) input argument is the label for the beginning of the
   current VM instruction, obtained by the prefix && operator: see the comment
   before JITTER_ASM_PATCH_IN_PLACEHOLDER . */
#define JITTER_PATCH_IN_PLACEHOLDER_GOTO_(size_in_bytes, case,       \
                                          arg0, arg1, arg2, arg3)    \
  asm goto (JITTER_ASM_DEFECT_DESCRIPTOR                             \
            JITTER_ASM_PATCH_IN_PLACEHOLDER(size_in_bytes, case,     \
                                            arg0, arg1, arg2, arg3)  \
            : /* outputs */                                          \
            : JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE,                 \
              JITTER_INPUT_VM_INSTRUCTION_BEGINNING /* inputs */     \
            : /* clobbers */                                         \
            : jitter_fake_target /* gotolabels */)

/* The inline asm inputs to be used as part of the input constraints for every
   patch-in. */
#define JITTER_PATCH_IN_INPUTS_FOR_EVERY_CASE  \
  [_jitter_dummy] "X" (jitter_ip)




/* Fallback fill-in byte.
 * ************************************************************************** */

/* Unless a definition is not already in machine-specific code. define the byte
   with which placeholder code is to be filled in.  This default is easy to
   recognize at a glance, but the best choice is machine-specific: ideally the
   byte should always be invalid at the beginning of an instruction, to make
   spotting mistakes easier. */
#ifndef JITTER_ASM_PATCH_IN_FILL_BYTE
# define JITTER_ASM_PATCH_IN_FILL_BYTE "0xff"
#endif // #ifndef JITTER_ASM_PATCH_IN_FILL_BYTE




/* Patch-in efficient data structures.
 * ************************************************************************** */

/* Compiling code with patch-ins yields an object file containing all the
   required information, but the subsection hack leaves the data ordered in a
   very inconvenient and inefficient way.  This functionality generates a C
   array which can be indexed by a specialized instruction opcode, whose
   elements each point to an array of patch-in descriptor pointers for the
   specialized instruction.

   The C array can be built just once at initialization time for each VM, at a
   cost linear in its instruction number.  Each patch-in filling, at run time,
   will then be O(1). */

/* The patch-ins for an instruction. */
struct patch_in_table_entry
{
  /* How many descriptors there are. */
  size_t descriptor_no;

  /* An array of pointers to descriptrs, of length descriptor_no. */
  const struct jitter_patch_in_descriptor **descriptors;
};

/* Return a freshly allocated C array of struct patch_in_table_entry objects,
   one element per specialized instruction. */
struct patch_in_table_entry *
jitter_make_patch_in_table (const struct jitter_patch_in_descriptor *descs,
                            size_t desc_no,
                            size_t specialized_instruction_no)
  __attribute__ ((nonnull (1)));

/* Free resources for a patch-in table as generated by
   jitter_make_patch_in_table . */
void
jitter_destroy_patch_in_table (struct patch_in_table_entry *,
                               size_t specialized_instruction_no)
  __attribute__ ((nonnull (1)));




/* Patch-in debugging.
 * ************************************************************************** */

/* Dump the pointed patch-in descriptor in human-readable form to the pointed
   stream. */
void
jitter_dump_patch_in_descriptor (FILE *f,
                                 const struct jitter_patch_in_descriptor *p)
  __attribute__ ((nonnull (1, 2)));

/* Like jitter_dump_patch_in_descriptor, but prepend the given string to each
   printed line. */
void
jitter_dump_patch_in_descriptor_with_prefix
   (FILE *f,
    const char *prefix,
    const struct jitter_patch_in_descriptor *p)
  __attribute__ ((nonnull (1, 2, 3)));

/* Dump the given patch-in descriptor array, having the given size in
   elements, in human-readable form to the pointed stream. */
void
jitter_dump_patch_in_descriptors
   (FILE *f,
    const struct jitter_patch_in_descriptor patch_ins [],
    size_t patch_in_no)
  __attribute__ ((nonnull (1)));

/* A convenience macro to call jitter_dump_patch_in_descriptors with the correct
   parameters. */
#define JITTER_DUMP_PATCH_IN_DESCRIPTORS(_jitter_vm_the_prefix)     \
  do                                                                \
    {                                                               \
      size_t patch_in_no                                            \
        = (JITTER_PATCH_IN_DESCRIPTORS_SIZE_IN_BYTES_NAME(          \
              _jitter_vm_the_prefix)                                \
           / sizeof (struct jitter_patch_in_descriptor));           \
      jitter_dump_patch_in_descriptors                              \
         (stderr,                                                   \
          JITTER_PATCH_IN_DESCRIPTORS_NAME(_jitter_vm_the_prefix),  \
          patch_in_no);                                             \
    }                                                               \
  while (false)

#endif // #ifdef JITTER_HAVE_PATCH_IN
#endif // #ifndef JITTER_PATCH_IN_H_
