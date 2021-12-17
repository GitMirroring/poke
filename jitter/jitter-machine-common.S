/* VM library: macros to be optionally included in architecture-specific asm.

   Copyright (C) 2017, 2019 Luca Saiu
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


/* This is an assembly file independent from the architecture, containing Gas
   macros to be optionally used (by CPP inclusion) in the architecture-specific
   file machine/ARCHITECTURE/jitter/machine/jitter-machine-assembly.S .

   The macros defined here are meant to be called from the .text section.

   This file contains macro definitions only, and by itself generates nothing
   except possibly possibly a .note.GNU-stack section.
   Because of this this file is also safe to include as a source in Makefile.am
   , so that make can automatically discover dependencies. */

#ifndef MACHINE_COMMON_S_
#define MACHINE_COMMON_S_


/* Include feature macros.
 * ************************************************************************** */

/* The code from this header defines macros but expands to nothing except
   possibly a .note.GNU-stack section, so it is safe to include from assembly
   as well. */
#include <jitter/jitter-config.h>

/* We use CPP token concatenation here.  The header contains CPP macro
   definitions only, and is safe to use from assembly. */
#include <jitter/jitter-cpp.h>


/* Expand to nothing if we have no assembly support.
 * ************************************************************************** */

/* If in this configuration the assembler is not Gas or we do not know its
   syntax or either configure or the user has decided to disable assembly
   support, for any reason, expand the rest of this file to whitespace only. */

#if defined (JITTER_HAVE_ASSEMBLY) && defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)

/* If we arrived here we can acually use assembly. */




/* Global GNU Assembler settings and definitions.
 * ************************************************************************** */

/* I like the & separator and <> quoting. */
.altmacro




/* Machine dependencies.
 * ************************************************************************** */

/* FIXME: factor with jitter.h . */
#if   JITTER_SIZEOF_VOID_P == 8
# define jitter_asm_word .quad
#elif JITTER_SIZEOF_VOID_P == 4
# define jitter_asm_word .long
#elif JITTER_SIZEOF_VOID_P == 2
# define jitter_asm_word .word
# warning "Weird: running Jitter on a 16-bit machine; this is untested."
#else
# error "Weird: this machine's word size is not 8, 4 or 2 bytes"
#endif // #if JITTER_SIZEOF_VOID_P == ...




/* Stack executability not required.
 * ************************************************************************** */

/* Do not require an executable stack.  Jitter, or a Jittery VM runtime, never
   directly relies on that.

   Rationale: none of my assembly code relies on executable stacks, even if GCC
   assumes, by default, that an assembly source file does.  Since this file is
   included in every assembly source this is a good place to put this.  If some
   other source linked to the Jitter runtime requires an executable stack then
   the linked executable will have it, thanks to another
     .section .note.GNU-stack, "x", @progbits
   written somewhere else.
   The Jitter runtime itself, along with every example in the distribution,
   could easily be compiled and linked with
     -Wa,--noexecstack -Wl,-z,noexecstack
   , if there were nothing else. */
#if defined (JITTER_HOST_OS_IS_ELF) \
    && defined (JITTER_HAVE_SECTION_NOTE_GNU_STACK)
.section .note.GNU-stack, "", @progbits
.previous
#endif // defined (JITTER_HOST_OS_IS_ELF) && ...




/* Switching sections.
 * ************************************************************************** */

/* The macros defined in the rest of this file are called from a .text section.

   This assumption is important on weak binary formats for which the GNU
   Assembler does not support a section stack; when done with emitting output
   for a non-default section we can return to the previous stream with .text
   instead of a non-portable directive such as .popsection .  The full power of
   a subsection stack is not used here.

   These definitions should be simple and general enough to work on any platform
   where Gas works, with no conditionalization. */

/* Enter the given subsection in the named section (initial dot included). */
.macro jitter_asm_enter_section section, subsection
  \section \subsection
.endm

/* Exit the current subsection and change to .text. */
.macro jitter_asm_exit_section
  .text
.endm




/* Global definitions.
 * ************************************************************************** */

/* The definitions here are format-specific.  I know of no general way of
   emitting global definitions working on every format. */

/* Here the defined global may require an "_" prefix,
   .def .. .endef , and no .type .
   About the word size kludge, it may not be very portable.  Still, the only
   widely used COFF systems are supported this way, and even this effort is
   more than those systems deserve.
   FIXME: possibly factor with jitter/jitter-sections.h . */
#if JITTER_SIZEOF_VOID_P == 8
# define JITTER_ASM_WITH_COFF_GLOBAL_PREFIX(identifier) \
  identifier
#else
# define JITTER_ASM_WITH_COFF_GLOBAL_PREFIX(identifier) \
  JITTER_CONCATENATE_TWO(_, identifier)
#endif // word size


/* Open the definition of a global with the given name, without changing
   section.  Generate a label with the given name. */
.macro jitter_global_begin name
#if defined (JITTER_HOST_OS_IS_ELF)
        .balign 16
        .globl \name
        .type \name, STT_OBJECT
\name:
#elif defined (JITTER_HOST_OS_IS_COFF)
        .balign 16
        .globl \name
  .def JITTER_ASM_WITH_COFF_GLOBAL_PREFIX(\name); .scl 2; .type 32;
  .endef
JITTER_ASM_WITH_COFF_GLOBAL_PREFIX(\name):
#else
# error "unsupported binary format: this configuration should not use assembly"
#endif // binary format conditional
.endm

/* Close the definition of a global with the given name, without changing
   section. */
.macro jitter_global_end name
#if defined (JITTER_HOST_OS_IS_ELF)
  /* Nothing needed. */
#elif defined (JITTER_HOST_OS_IS_COFF)
  /* Nothing needed. */
#else
# error "unsupported binary format: this configuration should not use assembly"
#endif // binary format conditional
.endm




/* Arrays.
 * ************************************************************************** */

/* Declare the jitter_native_snippet_sizes symbol in the .data subsection 1,
   the jitter_native_snippet_pointers symbol in the .data subsection 2 and the
   jitter_native_snippet_names symbol in the .data subsection 3 (also using the
   .data subsection 4 to store internal data); by using the Gas macro
   jitter_snippet the subsections will be filled, respectively, with word-sized
   (in the Jitter sense) elements holding the size of each snippet and with
   word-sized elements (again in the Jitter sense) holding pointers to the
   beginning of each snippet, and with pointers (again Jitter-word-sized) to
   snippet names as strings.
   Every array element is added in the order of jitter_snippet calls.  The three
   symbols will be visible from C as objects of type const jitter_uint [],
   const char * const [] and const char* const [], which will be convenient
   for implementing jitter_snippet_size, jitter_snippet_code and jitter_snippet_name .
   The three arrays are meant to be indexed with enum jitter_snippet_to_patch
   objects, whose cases must follow the same order as the jitter_native calls
   here. */
.macro jitter_arrays
/* Define the array jitter_native_snippet_sizes in .data subsection 1 , making it
   visible from C. */
jitter_asm_enter_section .data, 1
  jitter_global_begin jitter_native_snippet_sizes
jitter_asm_exit_section
/* Define the array jitter_native_snippet_pointers in .data subsection 2 , making
   it visible from C. */
jitter_asm_enter_section .data, 2
  jitter_global_begin jitter_native_snippet_pointers
jitter_asm_exit_section
/* Define the array jitter_native_snippet_names in .data subsection 3 , making it
   visible from C. */
jitter_asm_enter_section .data, 3
  jitter_global_begin jitter_native_snippet_names
jitter_asm_exit_section
.endm

/* A simple Gas macro emitting the given label with the prefix "jitter_native_",
   followed by up to twenty instructions (or generically "lines"), plus
   a Jitter-word-sized datum with the same name as the function followed by
   "_size" holding the snippet size in bytes. */
.macro jitter_snippet snippet_name, \
                  insn1, insn2=<>, insn3=<>, insn4=<>, insn5=<>, insn6=<>, \
                  insn7=<>, insn8=<>, insn9=<>, insn10=<>, insn11=<>, insn12=<>, \
                  insn13=<>, insn14=<>, insn15=<>, insn16=<>, insn17=<>, \
                  insn18=<>, insn19=<>, insn20=<>
/* Emit the snippet in the current section.  Use an alignment sufficient for
   all architectures; Gas refuses to emit misaligned instructions on some
   platforms. */
        .balign 16
jitter_native_&\snippet_name&:
        \insn1
        \insn2
        \insn3
        \insn4
        \insn5
        \insn6
        \insn7
        \insn8
        \insn9
        \insn10
        \insn11
        \insn12
        \insn13
        \insn14
        \insn15
        \insn16
        \insn17
        \insn18
        \insn19
        \insn20
jitter_native_&\snippet_name&_end:
/* Add a word-sized datum containing the procedure size in .data subsection 1 ;
   it will be one element of jitter_native_snippet_sizes . */
jitter_asm_enter_section .data, 1
        jitter_asm_word (jitter_native_&\snippet_name&_end - jitter_native_&\snippet_name&)
jitter_asm_exit_section
/* Add a datum containing a pointer to the snippet beginning in .data
   subsection 2 . */
jitter_asm_enter_section .data, 2
        jitter_asm_word jitter_native_&\snippet_name
jitter_asm_exit_section
/* Add a datum containing the procedure name in .data subsection 4 . */
jitter_asm_enter_section .data, 4
        jitter_native_&\snippet_name&_name:
        .ascii "\snippet_name"
        .byte 0x0
jitter_asm_exit_section
/* Add a datum containing a pointer to the the procedure name in .data
   subsection 3 . */
jitter_asm_enter_section .data, 3
        jitter_asm_word jitter_native_&\snippet_name&_name
jitter_asm_exit_section
.endm

#endif // #if defined (JITTER_HAVE_ASSEMBLY) && defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)

#endif // #ifndef MACHINE_COMMON_S_
