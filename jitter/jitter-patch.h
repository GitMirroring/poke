/* VM library: native code patching, machine-independent header file.

   Copyright (C) 2017, 2019, 2020 Luca Saiu
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


#ifndef JITTER_MACHINE_INDEPENDENT_PATCH_
#define JITTER_MACHINE_INDEPENDENT_PATCH_


/* Ignore the rest of this file if the dispatch is not no-threading.
 * ************************************************************************** */

/* Import the preprocessor definition of JITTER_ENABLE_ASSEMBLY , if any. */
#include <jitter/jitter.h>

/* Here we need to know whether a <jitter/machine/jitter-machine.h> exists for
   this architecture.  Moreover architecture-specific files may rely on C type
   sizes and endianness, and it is convenient to include the configuration file
   here rather than in every machine-specific file. */

/* If the dispatch is different from no-threading then ignore the rest of this
   file. */
#if defined (JITTER_DISPATCH_NO_THREADING)




/* Include headers.
 * ************************************************************************** */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Include the architecture-dependent file.  This is found at Jitter compile
   time with an explicit -I option when working with uninstalled headers, or in
   standard include paths when installed.  This way we avoid the trouble of
   referring to directories whose names depend on the configuration. */
#include <jitter/machine/jitter-machine.h>

/* Include patch-in and fast-branch information, if any. */
#include <jitter/jitter-patch-in.h>
#include <jitter/jitter-fast-branch.h>




/* Machine-independent declarations for machine-specific snippets.
 * ************************************************************************** */

/* The architecture-independent declaration of an enumerate representing the
   different assembly snippets we support.  Of course the actual definition is
   architecture-dependent, and comes in vm/assembly/ARCHITECTURE/machine.h .  The
   last case must be called jitter_snippet_no , and must not describe an actual
   snippet; it is used as a snippet count. */
enum jitter_snippet_to_patch;

/* A constant array of snippet sizes (in bytes), to be defined for every
   architecture.  The array is meant to be indexed by enum jitter_snippet_to_patch
   objects. */
extern const jitter_uint
jitter_native_snippet_sizes [jitter_snippet_no];

/* A constant array of snippet native code pointers, to be defined for every
   architecture.  The array is meant to be indexed by enum jitter_snippet_to_patch
   objects. */
extern const char* const
jitter_native_snippet_pointers [jitter_snippet_no];

/* A constant array of snippet textual names mostly intended for debugging, to
   be defined for every architecture.  The array is meant to be indexed by enum
   jitter_snippet_to_patch objects. */
extern const char* const
jitter_native_snippet_names [jitter_snippet_no];




/* Machine-specific snippet-choosing functions.
 * ************************************************************************** */

/* This function needs to be implemented for every architecture.  Return what
   snippet to use for loading the pointed word-sized immediate into the given
   residual register.

   This function also takes the address of the loading code to be filled with
   the snippet is given, only to be able to take its alignment in consideration;
   the code is not copied here. */
enum jitter_snippet_to_patch
jitter_snippet_for_loading_register (const char* immediate_pointer,
                                     unsigned int residual_register_index,
                                     const char *loading_code_to_write);

/* This function needs to be implemented for every architecture.  Return what
   snippet to use for loading the pointed word-sized immediate into residual
   memory at the given index.  The index is in elements, relative to the
   residual memory base: if there are residual registers as well (which always
   take priority over memory), those are not counted here.

   This function also takes the address of the loading code to be filled with
   the snippet is given, only to be able to take its alignment in consideration;
   the code is not copied here. */
enum jitter_snippet_to_patch
jitter_snippet_for_loading_memory (const char *immediate_pointer,
                                   unsigned int index,
                                   const char *loading_code_to_write);

#ifdef JITTER_HAVE_PATCH_IN
/* This function needs to be implemented for each architecture supporting
   patch-ins.  Return the snippet for a patch-in in the given case.
   FIXME: this part of the API is still tentative. */
enum jitter_snippet_to_patch
jitter_snippet_for_patch_in (const struct jitter_patch_in_descriptor *dp);
#endif // #ifdef JITTER_HAVE_PATCH_IN





/* Machine-specific code-patching functions.
 * ************************************************************************** */

/* This function needs to be implemented for every architecture.  Patch the
   native code at the given address, which must already contain a copy of the
   specified snippet which is assumed to be of the given size, to the pointed
   word-sized immediate.  The function cannot assume that the pointed object
   remain valid or allocated after it returns. */
void
jitter_patch_load_immediate_to_register (char *native_code,
                                         size_t native_code_size,
                                         const char *immediate_pointer,
                                         enum jitter_snippet_to_patch snippet);

/* This function needs to be implemented for every architecture.  Patch the
   native code at the given address, which must already contain a copy of the
   specified snippet, assumed to be of the given size, to load the pointed
   word-sized immediate into residual memory, at the given index.  The index is
   specified in memory elements, 0-based; in other words index holds the element
   index within memory: if there are residual registers as well (which always
   take priority over memory), those are not counted here. */
void
jitter_patch_load_immediate_to_memory (char *native_code,
                                       size_t native_code_size,
                                       unsigned int memory_index,
                                       const char *immediate_pointer,
                                       enum jitter_snippet_to_patch snippet);


#ifdef JITTER_HAVE_PATCH_IN
/* This function needs to be implemented for each architecture supporting
   patch-ins.  Given a pointer to the native code to modify (within replicated
   code, with the offset already added), a pointer to the immediate value
   and a pointer to the description, patch the code using the given snippet.
   FIXME: this part of the API is still tentative. */
void
jitter_patch_patch_in (char *native_code,
                       const char *immediate_pointer,
                       const struct jitter_patch_in_descriptor *descriptor,
                       enum jitter_snippet_to_patch snippet);
#endif // #ifdef JITTER_HAVE_PATCH_IN




/* Machine-independent convenience functions.
 * ************************************************************************** */

/* The functions below are implemented in jitter-patch.c once and for all, and
   are used for every architecture.  They are convenient to be called from the
   replication subsystem, while the functions above serve to implement these. */

/* Call either jitter_snippet_for_loading_register or jitter_snippet_for_loading_memory
   as appropriate according to the given residual index. */
enum jitter_snippet_to_patch
jitter_snippet_for_loading (const char *immediate_pointer,
                            unsigned int residual_index,
                            const char *loading_code_to_write);

/* Copy the given snippet into memory starting from the given address. */
void
jitter_copy_snippet (char *native_code, enum jitter_snippet_to_patch snippet);

/* Call either jitter_patch_load_immediate_to_register or
   jitter_patch_load_immediate_to_memory , as appropriate according to the residual
   index.  Differently from jitter_patch_load_immediate_to_memory , here the index
   parameter (still 0-based) is an index of the residual argument, independent
   from how many residual registers are available. */
void
jitter_patch_load_immediate (char *native_code,
                             unsigned int residual_index,
                             const char *immediate_pointer,
                             enum jitter_snippet_to_patch snippet);

/* Return a pointer to the beginning of the native code of the given snippet.
   This relies on jitter_native_snippet_pointers, which must be defined for every
   architecture. */
const char*
jitter_snippet_code (enum jitter_snippet_to_patch snippet);

/* Return the size of the given snippet code, in bytes.  This relies on
   jitter_native_snippet_sizes, which must be defined for every architecture. */
size_t
jitter_snippet_size (enum jitter_snippet_to_patch snippet);

/* Return the name of the given snippet.  This relies on
   jitter_native_snippet_names, which must be defined for every architecture. */
const char*
jitter_snippet_name (enum jitter_snippet_to_patch snippet);

#endif // #if defined (JITTER_DISPATCH_NO_THREADING)
#endif // #ifndef JITTER_MACHINE_INDEPENDENT_PATCH_
