/* Jitter: Routine unified API.

   Copyright (C) 2019, 2020 Luca Saiu
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


#include <jitter/jitter-routine.h>
#include <jitter/jitter-disassemble.h>
#include <jitter/jitter-print.h>
#include <jitter/jitter-specialize.h>


/* Unified routine operations.
 * ************************************************************************** */

/* The only operations implemented here are the ones requiring a new C
   function in the unified API.

   See the comments in <jitter/jitter-mutable-routine.h> and
   <jitter/jitter-specialize.h> about the underlying ordinary API. */

void
jitter_destroy_routine (jitter_routine r)
{
  /* Destroy the executable routine for r, if it exists.  Differenrly from what
     happens in the ordinary API, which has to handle the case of each routine
     kind being destroyed separately, here I can require r to be non-NULL. */
  struct jitter_executable_routine *e = r->executable_routine;
  if (e != NULL)
    jitter_destroy_executable_routine (e);

  /* Now unconditionally destroy the mutable version as well. */
  jitter_destroy_mutable_routine (r);
}

void
jitter_pin_routine (jitter_routine r)
{
  jitter_pin_executable_routine (r->executable_routine);
}

void
jitter_unpin_routine (jitter_routine r)
{
  jitter_unpin_executable_routine (r->executable_routine);
}

void
jitter_routine_print (jitter_print_context out, const jitter_routine r)
{
  /* Labels must be resolved before we can print.  Make an executable routine,
     so that we have labels as well. */
  jitter_routine_make_executable_if_needed (r);

  /* Now printing is possible. */
  jitter_mutable_routine_print (out, r);
}

void
jitter_routine_disassemble (jitter_print_context out,
                            const jitter_routine r, bool raw,
                            const char *objdump_name,
                            const char *objdump_options_or_NULL)
{
  struct jitter_executable_routine *e
    = jitter_routine_make_executable_if_needed (r);
  jitter_executable_routine_disassemble (out, e, raw, objdump_name,
                                         objdump_options_or_NULL);
}

struct jitter_executable_routine *
jitter_routine_make_executable_if_needed (jitter_routine r)
{
  if (r->executable_routine != NULL)
    return r->executable_routine;
  else
    /* The returned expression also changes r->executable_routine as a side
       effect. */
    return jitter_make_executable_routine (r);
}
