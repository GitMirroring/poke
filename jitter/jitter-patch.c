/* VM library: native code patching, machine-independent part.

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


/* Ignore the rest of this file if the dispatch is not no-threading.
 * ************************************************************************** */

#include <jitter/jitter.h>

/* If we are not using no-threading ignore the rest of this file. */
#if defined (JITTER_DISPATCH_NO_THREADING)




/* Include headers.
 * ************************************************************************** */

#include <jitter/jitter-patch.h>
#include <string.h>
#include <limits.h>

#include <jitter/machine/jitter-machine.h>




/* Machine-independent convenience functions.
 * ************************************************************************** */

enum jitter_snippet_to_patch
jitter_snippet_for_loading (const char *immediate_pointer,
                            unsigned int residual_index,
                            const char *loading_code_to_write)
{
  if (residual_index < JITTER_RESIDUAL_REGISTER_NO)
    return jitter_snippet_for_loading_register (immediate_pointer,
                                                residual_index,
                                                loading_code_to_write);
  else
    return jitter_snippet_for_loading_memory (immediate_pointer,
                                              residual_index
                                              - JITTER_RESIDUAL_REGISTER_NO,
                                              loading_code_to_write);
}

void
jitter_patch_load_immediate (char *native_code,
                             unsigned int residual_index,
                             const char *immediate_pointer,
                             enum jitter_snippet_to_patch snippet)
{
  size_t native_code_size = jitter_snippet_size (snippet);
  if (residual_index < JITTER_RESIDUAL_REGISTER_NO)
    jitter_patch_load_immediate_to_register (native_code, native_code_size,
                                             immediate_pointer, snippet);
  else
    jitter_patch_load_immediate_to_memory (native_code, native_code_size,
                                           residual_index
                                           - JITTER_RESIDUAL_REGISTER_NO,
                                           immediate_pointer,
                                           snippet);
}

void
jitter_copy_snippet (char *to_native_code, enum jitter_snippet_to_patch snippet)
{
  size_t snippet_length = jitter_snippet_size (snippet);
  const char *from_native_code = jitter_snippet_code (snippet);
  memcpy (to_native_code, from_native_code, snippet_length);
}




/* Machine-independent convenience functions.
 * ************************************************************************** */

const char*
jitter_snippet_code (enum jitter_snippet_to_patch snippet)
{
  return jitter_native_snippet_pointers [snippet];
}

size_t
jitter_snippet_size (enum jitter_snippet_to_patch snippet)
{
  return jitter_native_snippet_sizes [snippet];
}

const char*
jitter_snippet_name (enum jitter_snippet_to_patch snippet)
{
  return jitter_native_snippet_names [snippet];
}


#endif // #if defined (JITTER_DISPATCH_NO_THREADING)
