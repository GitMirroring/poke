/* Jitter: Routine unified API: header.

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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#ifndef JITTER_ROUTINE_H_
#define JITTER_ROUTINE_H_

#include <jitter/jitter.h>

#include <jitter/jitter-mutable-routine.h>
#include <jitter/jitter-print.h>
#include <jitter/jitter-specialize.h>




/* Introduction.
 * ************************************************************************** */

/* This user API is a simplified alternative to using both struct
   jitter_mutable_routine and struct jitter_executable_routine.  The ordinary
   API using both structs potentially allows for slight time savings due to
   fewer indirections, and more substantial space savings in case executable
   routines are generated without need for their mutable counterparts.

   This unified API has the advantage of simplicity: the user does not need to
   pay attention about which operation requires each kind of routine, including
   destruction which when handled carelessly from the ordinary API may open
   opportunities for memory leaks.

   The unified API follows the ordinary API, only changing the routine type in
   function and macro arguments.  Comments about individual operations are not
   duplicated here: see <jitter/jitter-mutable-routine.h> and
   <jitter/jitter-specialize.h> for more information. */




/* Data structure.
 * ************************************************************************** */

/* The internal implementation of a struct routine is to be considered
   abstract.  It is independent from the VM. */

/* A unified routine is actually implemented as a pointer to a mutable routine,
   with the convention that the unified API never destroys the executable
   routine pointed by it, if any, until the unified routine is destroyed as
   well.
   There is no need for a wrapper pointing to two routine kinds, when in the
   ordinary API they each point to each other already. */
typedef struct jitter_mutable_routine *
jitter_routine;




/* Operations.
 * ************************************************************************** */

/* See the comments in <jitter/jitter-mutable-routine.h> ,
   <jitter/jitter-specialize.h> and. <jitter/jitter-disassemble.h>

   The unified API only wraps the operations actually indended for the user.
   Since this is all only intended for the user, it makes little sense to define
   VM-indepdendent wrappers here to be wrapped again in templates/vm.h .  This
   section only provides declarations for what actually needs to be implemented
   as a VM-independent function; everything else is wrapped, once, directly in
   templates/vm.h . */

/* Destroy the given routine, which must have been previously allocated with
   vmprefix_make_routine. */
void
jitter_destroy_routine (jitter_routine r);

/* Like jitter_pin_excutable_routine , for the unified routine API. */
void
jitter_pin_routine (jitter_routine r)
  __attribute__ ((nonnull (1)));

/* Like jitter_pin_excutable_routine , for the unified routine API. */
void
jitter_unpin_routine (jitter_routine r)
  __attribute__ ((nonnull (1)));

/* Like jitter_mutable_routine_print , for the unified routine API. */
void
jitter_routine_print (jitter_print_context out, const jitter_routine r)
  __attribute__ ((nonnull (1, 2)));

/* Like jitter_excecutable_routine_disassemble , for the unified routine API. */
void
jitter_routine_disassemble (jitter_print_context out,
                            const jitter_routine r, bool raw,
                            const char *objdump_name,
                            const char *objdump_options_or_NULL)
  __attribute__ ((nonnull(1, 2, 4)));




/* In case it does not exist yet, make the executable counterpart of the given
   unified routine.  After this function returns it is safe to access the
   executable routine from a pointer in the mutable routine, without
   checking.
   It is not always necessary to use this function is user code, as a unified
   routine automatically generates an executable routine at its first execution
   or extraction of external labels [FIXME: external labels are not implemented
   yet; the other use case is already supported at this point].
   However relying on this automatic behavior is dangerous in a multi-threaded
   setting where an executable routine may be generated at the same time for the
   same unified routine: there is no mutex to automatically handle a critical
   section, and it is the user's responsibility to provide for it if needed.
   This function is also used internally. */
struct jitter_executable_routine *
jitter_routine_make_executable_if_needed (jitter_routine r)
  __attribute__ ((nonnull (1), returns_nonnull));

#endif // #ifndef JITTER_ROUTINE_H_
