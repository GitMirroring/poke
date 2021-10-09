/* Jitter: defective VM instruction header.

   Copyright (C) 2018, 2021 Luca Saiu
   Updated in 2020 by Luca Saiu
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


#ifndef JITTER_DEFECT_H_
#define JITTER_DEFECT_H_

/* Include core headers.
 * ************************************************************************** */

/* Include macro definitions about whether we have a machine file, and about the
   dispatching model. */
#include <jitter/jitter.h>

#include <jitter/jitter-print.h>

/* Include sectioning macros, if they are supported. */
#if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
# include <jitter/jitter-sections.h>
#endif // #if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)

/* The debugging facility relies on standard I/O. */
#include <stdio.h>




/* Introduction.
 * ************************************************************************** */

// FIXME: write.




/* Feature macro.
 * ************************************************************************** */

/* The defect-replacement subsystem is enabled iff the dispatch is
   minimal-threading or no-threading. */
#if defined (JITTER_DISPATCH_MINIMAL_THREADING)  \
    || defined (JITTER_DISPATCH_NO_THREADING)
# define JITTER_HAVE_DEFECT_REPLACEMENT  1
  /* Defect replacement relies on having a supported binary format and on
     the GNU C assembly names extension.  If either is not supported then
     something is wrong. */
# if ! defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
#   error "defect replacement is enabled but the binary format is not known;"
#   error "this should never happen."
# endif // # if ! defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
# if ! defined (JITTER_HAVE_ASSEMBLY_NAMES_FOR_NON_FUNCTIONS)
#   error "defect replacement is enabled but the GNU C extension allowing"
#   error "to define assembly names for non-functions; this should never happen."
# endif // # if ! defined (JITTER_HAVE_ASSEMBLY_NAMES_FOR_NON_FUNCTIONS)
#endif




/* Expansion conditional on replacement being enabled.
 * ************************************************************************** */

/* About JITTER_WHEN_DEFECT_ [respectively: JITTER_UNLESS_DEFECT_ ]: expand to
   the argument if defect replacement is enabled [respectively: disabled] , to
   nothing otherwise.
   About JITTER_IF2_DEFECT_ : expand to the first argument if defect replacement
   is enabled, to the second argument otherwise.
   About JITTER_WHEN_DEFECT_COMMA_ [respectively: JITTER_UNLESS_DEFECT_COMMA_ ]:
   expand to the argument followed by a comma if defect replacement is enabled
   [respectively: disabled] , to nothing otherwise.
   The expansion is not protected by do..while(0), parens or anything else. */
#if defined (JITTER_HAVE_DEFECT_REPLACEMENT)
# define JITTER_WHEN_DEFECT_(thing)            thing
# define JITTER_UNLESS_DEFECT_(thing)          /* nothing*/
# define JITTER_IF2_DEFECT_(thing_a, thing_b)  thing_a
# define JITTER_WHEN_DEFECT_COMMA_(thing)      thing ,
# define JITTER_UNLESS_DEFECT_COMMA_(thing)    /* nothing */
#else
# define JITTER_WHEN_DEFECT_(thing)            /* nothing */
# define JITTER_UNLESS_DEFECT_(thing)          thing
# define JITTER_IF2_DEFECT_(thing_a, thing_b)  thing_b
# define JITTER_WHEN_DEFECT_COMMA_(thing)      /* nothing */
# define JITTER_UNLESS_DEFECT_COMMA_(thing)    thing ,
#endif // #if defined (JITTER_HAVE_DEFECT_REPLACEMENT)





/* Defect descriptor data structures: C API.
 * ************************************************************************** */

/* A descriptor associated to each static program point possibly causing a
   specialized VM instruction to be defective.  A specialized VM instruction is
   defective if it has at least one such descritpor with a non-zero
   displacement. */
struct jitter_defect_descriptor
{
  /* The opcode of the specialized instruction in question. */
  jitter_uint specialized_opcode;

  /* The distance between a used label as seen from C and the same label as seen
     from assembly.  A displacement different from the distance between
     jitter_fake_target (macro-expanding to a C label) and
     jitter_fake_target_asm means that the instruction is defective. */
  jitter_int displacement;
};




/* Prefix-dependent names for globals.
 * ************************************************************************** */

/* The name of the global descriptor vector. */
#define JITTER_DEFECT_DESCRIPTORS_NAME(_jitter_vm_the_prefix)         \
  JITTER_CONCATENATE_TWO(_jitter_vm_the_prefix, _defect_descriptors)

/* The name of the global descriptor vector. */
#define JITTER_DEFECT_DESCRIPTORS_SIZE_IN_BYTES_NAME(_jitter_vm_the_prefix)     \
  JITTER_CONCATENATE_TWO(JITTER_DEFECT_DESCRIPTORS_NAME(_jitter_vm_the_prefix), \
                         _size_in_bytes)

/* The name of the global descriptor vector. */
#define JITTER_DEFECT_CORRECT_DISPLACEMENT_NAME(_jitter_vm_the_prefix)          \
  JITTER_CONCATENATE_TWO(JITTER_DEFECT_DESCRIPTORS_NAME(_jitter_vm_the_prefix), \
                         _correct_displacement)

/* Expand to extern declaration of the variables defined in assembly, to be used
   from C. */
#define JITTER_DEFECT_DESCRIPTOR_DECLARATIONS_(_jitter_vm_the_prefix)   \
  extern const struct jitter_defect_descriptor                          \
  JITTER_DEFECT_DESCRIPTORS_NAME(_jitter_vm_the_prefix) [];             \
  extern const jitter_uint                                              \
  JITTER_DEFECT_DESCRIPTORS_SIZE_IN_BYTES_NAME(_jitter_vm_the_prefix);  \
  extern jitter_int                                                     \
  JITTER_DEFECT_CORRECT_DISPLACEMENT_NAME(_jitter_vm_the_prefix);




/* Defect descriptor opening and closing macros.
 * ************************************************************************** */

/* Each defect descriptor contains information about one program point possibly
   causing a defect. */

/* Expand to a string literal containing the .rodata subsection number
   containing the defect descriptors. */
#define JITTER_ASM_DEFECT_SUBSECTION  \
  "11"

/* Expand to a C top-level inline asm statement containing the defect
   header. */
#define JITTER_DEFECT_HEADER(_jitter_vm_the_prefix)                   \
  asm (JITTER_ASM_OPEN_DEFINITION                                     \
          (JITTER_ASM_DEFECT_SUBSECTION,                              \
           JITTER_DEFECT_DESCRIPTORS_NAME (_jitter_vm_the_prefix)));

/* Expand to a C top-level inline asm statement containing the defect
   footer. */
#define JITTER_DEFECT_FOOTER(_jitter_vm_the_prefix)                  \
  asm (JITTER_ASM_CLOSE_DEFINITION                                   \
          (JITTER_ASM_DEFECT_SUBSECTION,                             \
           JITTER_DEFECT_DESCRIPTORS_NAME (_jitter_vm_the_prefix)))




/* Defect descriptor macros.
 * ************************************************************************** */

/* Expand to a literal template string adding a defect descriptor for the
   current specialized instruction.  This should be used within an inline asm
   goto statement having jitter_fake_target as gotolabel. */
#define JITTER_ASM_DEFECT_DESCRIPTOR                                \
  JITTER_ASM_COMMENT_UNIQUE(                                        \
     "Defect descriptor for "                                       \
     JITTER_STRINGIFY(JITTER_SPECIALIZED_INSTRUCTION_MANGLED_NAME)) \
  JITTER_ASM_ENTER_SUBSECTION(JITTER_ASM_DEFECT_SUBSECTION) "\n\t"  \
  JITTER_ASM_WORD " "                                               \
     JITTER_STRINGIFY(JITTER_SPECIALIZED_INSTRUCTION_OPCODE) "\n\t" \
  JITTER_ASM_WORD " "                                               \
     "(%l[jitter_fake_target] - jitter_fake_target_asm)\n"    \
  JITTER_ASM_EXIT_SUBSECTION "\n\t"




/* Defect efficient data structures.
 * ************************************************************************** */

/* Compiling code with defect descriptors yields an object file containing all
   the required information, but the subsection hack leaves the data ordered in
   a very inconvenient and inefficient way.  This functionality generates a
   "defect table", which is a C array which can be indexed by a specialized
   instruction opcode, whose elements are each the opcode of the replacement
   specialized instruction; this opcode will be the same as the index if the
   instruction is not defective.

   The "worst-case defect table" is a defect table mapping each specialized
   opcode for an instruction which can possibly be defective to its replacement.
   The worst-case defect table is a global constant for each VM.

   The idea is, of course, to make searching for a replacement instruction a
   fast O(1) operation which can be executed unconditionally on any specialized
   instruction, be it defective or not. */

/* I can just use a forward-declaration here instead of a header inclusion.
   Anyway this is the struct defined in jitter/jitter-vm.h . */
struct jitter_vm;

/* Given a pointer to the VM struct, an initial pointer to the defect descriptor
   array, a pointer to the defective-instruction array, the number of defects
   descriptors, a pointer to the worst-case defect table and the correct
   displacement initialize the pointed defect table and the
   defective-instruction array.
   In the defective-instruction array the first
   defective_specialized_instruction_no elements are initialised to contain the
   specialized_instruction_ids of defective instructions; the remaining elements
   are set to -1.
   The replacement table is a global, already existing for any given VM, which
   only needs to be initialized once even if a VM subsystem is finalized and
   re-initialized multiple times; the same holds for the
   defective_specialized_instructions table.
   Also set the defect fields in the pointed VM struct. */
void
jitter_fill_replacement_table
   (jitter_uint *replacement_table,
    jitter_int *defective_specialized_instructions,
    struct jitter_vm *vm,
    const jitter_uint *worst_case_replacement_table,
    const jitter_uint *call_related_specialized_instruction_ids,
    jitter_uint call_related_specialized_instruction_id_no,
    const bool *specialized_instruction_call_relateds,
    const struct jitter_defect_descriptor *descs,
    size_t desc_no,
    jitter_int correct_displacement)
  __attribute__ ((nonnull (1, 2, 3, 4, 5, 7, 8)));




/* Defect debugging.
 * ************************************************************************** */

/* Dump the pointed replacement table to the given stream. */
void
jitter_dump_replacement_table (FILE *f,
                               const jitter_uint *replacement_table,
                               const struct jitter_vm *vm)
  __attribute__ ((nonnull (1, 2, 3)));

/* Dump the pointed defective specialised instruction id array to the given
   stream.  The given call-relatedness data and VM struct pointer must belong
   to the same VM as the defective_specialized_instructions array. */
void
jitter_dump_defects (FILE *f,
                     const jitter_int *defective_specialized_instructions,
                     const struct jitter_vm *vm,
                     const bool *specialized_instruction_call_relateds)
  __attribute__ ((nonnull (1, 2, 3)));





/* Defect printing.
 * ************************************************************************** */

/* Print compact information about defects for the pointed VM to the given print
   context.  This uses the following classes, where "vmprefix" is replaced by
   the VM lower-case name:
   - "vmprefix-comment";
   - "vmprefix-warning".
   This function is defined unconditionally, and in order to be less cumbersome
   for the user to call is usable even when defect replacement is not supported
   or needed. */
void
jitter_defect_print_summary (jitter_print_context cx,
                             const struct jitter_vm *vm)
  __attribute__ ((nonnull (1, 2)));

/* Print human-readable information about defective instructions for the pointed
   VM, using the given print context, indented by the given number of
   characters.  This uses the same classes as the function above. */
void
jitter_defect_print (jitter_print_context cx,
                     const struct jitter_vm *vm,
                     unsigned indentation_column_no)
  __attribute__ ((nonnull (1, 2)));

/* Print the replacement table for the pointed VM in human-readable form, using
   the given print context, indented by the given number of characters.  This
   uses the same classes as the function above. */
void
jitter_defect_print_replacement_table (jitter_print_context cx,
                                       const struct jitter_vm *vm,
                                       unsigned indentation_column_no)
  __attribute__ ((nonnull (1, 2)));

#endif // JITTER_DEFECT_H_
