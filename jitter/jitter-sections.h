/* Jitter: section header.

   Copyright (C) 2017, 2018, 2019, 2020 Luca Saiu
   Updated in 2021 by Luca Saiu
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



#ifndef JITTER_SECTIONS_H_
#define JITTER_SECTIONS_H_


/* Include core headers.
 * ************************************************************************** */

/* Include macro definitions about the dispatching model and about the machine. */
#include <jitter/jitter.h>




/* Introduction.
 * ************************************************************************** */

/* The functionality in this header allows to define a read-only global in
   assembly, accessible from C.  The assembly code is compiled along with
   ordinary C code, so that GCC doesn't know about it; the assembly part may
   refer labels and literals occurring elsewhere in C.

   This is delicate, since each patch-in placeholder is contained in compiled C
   code; some patch-in placeholders may even be duplicated or optimized away by
   GCC.

   The solution is using assembler subsections: the same inline asm code
   potentially expanding to instructions in the .text section temporarily enters
   a subsection, emits data, and then exits the subsection and goes back to
   .text .

   The descriptor array is defined, of course within its subsection, from a
   "header" in top-level inline assembly, which *must* come before the
   executor code containing patch-in placeholders.  Instead of relying on
   the no_reorder attribute as I used to do, I now generate header and footer
   within the same function, relying on the ordering semantics of inline asm
   statements with dependencies and asm volatile.
   Similarly, a top-level inline asm "footer" closes the global array
   definition, and defined a further global storing the array size.

   Interestingly no assembly *instructions* are required for this: the generated
   inline assembly contains only data, which is machine-independent.  The
   subsection mechanism, however, relies on ELF.  Support for other binary
   formats is almost certainly possible, but not prioritary.  Modern GNU systems
   use ELF.

   Syntactic convention: as always every macro whose name starts with
   JITTER_ASM_ expands to a literal string, meant to be emitted as part of the
   generated assembly code. */




/* Assembly utility.
 * ************************************************************************** */

/* Expand to a string literal for inclusion in inline assembly, skipping the
   given number of bytes (a constant assembly expression not referring
   variables) in the current section.  The fill byte is
   JITTER_ASM_PATCH_IN_FILL_BYTE . */
#define JITTER_ASM_SKIP_BYTES(size)                                          \
  JITTER_ASM_COMMENT_UNIQUE("Skip " JITTER_STRINGIFY(size) "B, but use a "   \
                            "conditional to avoid a warning if "             \
                            JITTER_STRINGIFY(size) " is zero")               \
  ".ifgt (" JITTER_STRINGIFY(size) ")\n"                                     \
  "  .skip " JITTER_STRINGIFY(size) ", " JITTER_ASM_PATCH_IN_FILL_BYTE "\n"  \
  ".endif\n"




/* Section-changing macros.
 * ************************************************************************** */

/* The macros provide a way of entering a given subsection, and exiting back to
   .text .  This is a simple wrapper over assembly functionality.

   ELF systems provide a very simple and elegant functionality based on LIFO
   logic, where a subsection can be entered and just exited; at exit, the current
   subsection reverts to the one which was active before entering.
   This functionality is simple and elegant, but unfortunately not portable
   to non-ELF systems.  However, since we don't really need our temporary
   subsection switches to nest, we can use a crude but more portable support
   from Gas on non-ELF systems and still get some of the functionality.

   The problem of switching subsections without the section stack on ELF is that
   debug informations interfere with this mechanism, and Gas complains with
   messages such as:
     Error: CFI instruction used without previous .cfi_startproc
     Error: .cfi_endproc without corresponding .cfi_startproc
     Error: previous CFI entry not closed (missing .cfi_endproc)
   and, as a consequence,
     Error: .size expression for XXXX does not evaluate to a constant

   For this reason we many need specific ELF support for some dispatches.
   Anyway some other functionality (such as locations: see
   jitter-data-locations.h) will be available even based on a simpler
   mechanism. */




/* Section-changing macros: the ELF solution.
 * ************************************************************************** */

/* Macros to enter and exit a subsection, expanding to literal assembly
   templates.  This is the solution relying on ELF.  The given section name must
   expand to a literal string. */
#define JITTER_ASM_ENTER_SUBSECTION_ELF(_jitter_section_name)  \
  "\n.pushsection .rodata, " _jitter_section_name "\n\t"
#define JITTER_ASM_EXIT_SUBSECTION_ELF  \
  "\n.popsection\n\t"




/* Section-changing macros: the GNU assembler solution.
 * ************************************************************************** */

/* Macros to enter and exit a subsection, expanding to literal assembly
   templates.  This is the solution just relying on the GNU assembler.  The
   given section name must expand to a literal string.

   In order for this to work I have to prevent GCC from generating CFI
   directives, which don't play well with subsections, and avoid
   -freorder-functions , which would .text into .text.hot and .text.unlikely
   and therefore break JITTER_ASM_EXIT_SUBSECTION_GAS preventing a simple
     .text 0
   from returning to the previous subsection. */
#define JITTER_ASM_ENTER_SUBSECTION_GAS(_jitter_section_name)       \
  /* Temporarily switch to a .data subsection.  The generated data  \
     will be read-only. */                                          \
  "\n.data " _jitter_section_name "\n\t"
#define JITTER_ASM_EXIT_SUBSECTION_GAS  \
  /* Go back to the subsection 0 . */   \
  "\n.text 0\n\t"




/* Section-changing macros: generic wrapper.
 * ************************************************************************** */

/* Macros to enter and exit a subsection, expanding to literal assembly
   templates.  The section name must expand to a literal string. */

#if defined(JITTER_HOST_OS_IS_ELF)
# define JITTER_ASM_ENTER_SUBSECTION(_jitter_section_name)  \
    JITTER_ASM_ENTER_SUBSECTION_ELF(_jitter_section_name)
# define JITTER_ASM_EXIT_SUBSECTION  \
    JITTER_ASM_EXIT_SUBSECTION_ELF
#elif defined(JITTER_HOST_ASSEMBLER_IS_GNU)
# define JITTER_ASM_ENTER_SUBSECTION(_jitter_section_name)  \
    JITTER_ASM_ENTER_SUBSECTION_GAS(_jitter_section_name)
# define JITTER_ASM_EXIT_SUBSECTION  \
    JITTER_ASM_EXIT_SUBSECTION_GAS
#else
# error "Not using ELF, nor the GNU assembler.  This will not work and you"
# error "should not have included this header."
#endif




/* Assembly global definition macros: ELF.
 * ************************************************************************** */

/* The macros here provide a way of generating assembly code to open and close a
   global definition. */

/* Expand to a literal assembly template string opening and closing a definition
   in the current section.
   Such a definition will generate two globally visible symbols: one main symbol
   with the given name, and another with the given name suffixed by
   "_size_in_bytes", a memory global containing the size in bytes of the main
   symbol.  The size has as many bytes as a Jitter machine word.
   The given name must expand to an identifier. */
#define JITTER_ASM_OPEN_DEFINITION_IN_CURRENT_SECTION_ELF(_jitter_name)  \
  "\n"                                                                   \
  ".balign 16\n"                                                         \
  ".globl " JITTER_STRINGIFY(_jitter_name) "\n"                          \
  ".type  " JITTER_STRINGIFY(_jitter_name) ", STT_OBJECT\n"              \
  JITTER_STRINGIFY(_jitter_name) ":\n\t"
#define JITTER_ASM_CLOSE_DEFINITION_IN_CURRENT_SECTION_ELF(_jitter_name)   \
  "\n"                                                                     \
  JITTER_STRINGIFY(_jitter_name) "_end:\n"                                 \
  ".balign 16\n"                                                           \
  ".globl " JITTER_STRINGIFY(_jitter_name) "_size_in_bytes\n"              \
  ".type  " JITTER_STRINGIFY(_jitter_name) "_size_in_bytes, STT_OBJECT\n"  \
  JITTER_STRINGIFY(_jitter_name) "_size_in_bytes:\n\t"                     \
  JITTER_ASM_WORD " (" JITTER_STRINGIFY(_jitter_name) "_end"               \
                       " - " JITTER_STRINGIFY(_jitter_name) ")\n\t"




/* Assembly global definition macros: COFF.
 * ************************************************************************** */

/* This is conceputally identical to the ELF version above. 
   Implementation note: here the defined global may require an "_" prefix,
   .def .. .endef , and no .type .
   About the word size kludge, it may not be very portable.  Still, the only
   widely used COFF systems are supported this way, and even this effort is
   more than those systems deserve. */
#if JITTER_SIZEOF_VOID_P == 8
# define JITTER_ASM_COFF_GLOBAL_PREFIX  \
    ""
#else
# define JITTER_ASM_COFF_GLOBAL_PREFIX  \
    "_"
#endif // word size
#define JITTER_ASM_OPEN_DEFINITION_IN_CURRENT_SECTION_COFF(_jitter_name)       \
  "\n"                                                                         \
  ".balign 16\n"                                                               \
  ".globl " JITTER_ASM_COFF_GLOBAL_PREFIX JITTER_STRINGIFY(_jitter_name) "\n"  \
  ".def " JITTER_ASM_COFF_GLOBAL_PREFIX JITTER_STRINGIFY(_jitter_name)         \
     "; .scl 2; .type 32; .endef\n"                                            \
   JITTER_ASM_COFF_GLOBAL_PREFIX JITTER_STRINGIFY(_jitter_name) ":\n\t"
#define JITTER_ASM_CLOSE_DEFINITION_IN_CURRENT_SECTION_COFF(_jitter_name)      \
  "\n"                                                                         \
  JITTER_ASM_COFF_GLOBAL_PREFIX JITTER_STRINGIFY(_jitter_name) "_end:\n"       \
  ".balign 16\n"                                                               \
  ".def " JITTER_ASM_COFF_GLOBAL_PREFIX JITTER_STRINGIFY(_jitter_name)         \
     "_size_in_bytes; .scl 2; .type 32; .endef\n"                              \
  ".globl _" JITTER_STRINGIFY(_jitter_name) "_size_in_bytes\n"                 \
  JITTER_ASM_COFF_GLOBAL_PREFIX JITTER_STRINGIFY(_jitter_name)                 \
     "_size_in_bytes:\n\t"                                                     \
  JITTER_ASM_WORD                                                              \
     " (" JITTER_ASM_COFF_GLOBAL_PREFIX JITTER_STRINGIFY(_jitter_name) "_end"  \
        " - " JITTER_ASM_COFF_GLOBAL_PREFIX JITTER_STRINGIFY(_jitter_name)     \
        ")\n"




/* Assembly global definition macros: generic wrapper.
 * ************************************************************************** */

#if defined(JITTER_HOST_OS_IS_ELF)
# define JITTER_ASM_OPEN_DEFINITION_IN_CURRENT_SECTION(_jitter_name)  \
    JITTER_ASM_OPEN_DEFINITION_IN_CURRENT_SECTION_ELF (_jitter_name)
# define JITTER_ASM_CLOSE_DEFINITION_IN_CURRENT_SECTION(_jitter_name)  \
    JITTER_ASM_CLOSE_DEFINITION_IN_CURRENT_SECTION_ELF (_jitter_name)
#elif defined(JITTER_HOST_OS_IS_COFF)
# define JITTER_ASM_OPEN_DEFINITION_IN_CURRENT_SECTION(_jitter_name)  \
    JITTER_ASM_OPEN_DEFINITION_IN_CURRENT_SECTION_COFF (_jitter_name)
# define JITTER_ASM_CLOSE_DEFINITION_IN_CURRENT_SECTION(_jitter_name)  \
    JITTER_ASM_CLOSE_DEFINITION_IN_CURRENT_SECTION_COFF (_jitter_name)
#else
# error "Not using ELF or COFF.  This will not work."
#endif // #ifdef JITTER_HOST_OS_IS_ELF




/* Definitions automatically switching sections: generic wrapper.
 * ************************************************************************** */

/* Expand to the literal string to be used in a top-level inline asm template as
   a descriptor header or footer.  The generated "code", which contains no
   machine instructions, switches to the appropriate subsection, opens or closes
   the definition, and goes back to .text . */
#define JITTER_ASM_OPEN_DEFINITION(_jitter_section_name,         \
                                   _jitter_name)                 \
  JITTER_ASM_ENTER_SUBSECTION(_jitter_section_name)              \
    JITTER_ASM_OPEN_DEFINITION_IN_CURRENT_SECTION(_jitter_name)  \
  JITTER_ASM_EXIT_SUBSECTION
#define JITTER_ASM_CLOSE_DEFINITION(_jitter_section_name,         \
                                    _jitter_name)                 \
  JITTER_ASM_ENTER_SUBSECTION(_jitter_section_name)               \
    JITTER_ASM_CLOSE_DEFINITION_IN_CURRENT_SECTION(_jitter_name)  \
  JITTER_ASM_EXIT_SUBSECTION


#endif // #ifndef JITTER_SECTIONS_H_
