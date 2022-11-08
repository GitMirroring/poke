/* Jitter: signal support: header.

   Copyright (C) 2019 Luca Saiu
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


#ifndef JITTER_SIGNALS_H_
#define JITTER_SIGNALS_H_

#include <stdbool.h>
#include <signal.h>

#include <jitter/jitter.h>


/* Introduction.
 * ************************************************************************** */

/* This facility is meant to be used along with the pending_notifications field
   in struct jitter_special_purpose_state_data, accessible in The Array: see
   templates/vm.h .

   The data structures here allow notifying threads, or signal handlers, to
   express information about the specific signal to be handled. */

/* Information about one specific signal.  The Array in every VM state points
   to an array of structs, indexed by a signal number.
   Notice that even the superior sigaction API does not guarantee that multiple
   signals of the same type can reliably be distinguished by the receiver: see
   see The GNU libc manual, §"Advanced Signal Handling". */
struct jitter_signal_notification
{
  /* A Boolean value, meant to be as large as a pointer for reasons of
     atomicity.
     This field:
     * is set to false at initialization;
     * should be set to true when a signal  is delievered
     * must be reset to false again after handling a signal. */
  jitter_int pending;

  /* Optional additional user data supplied by the notifier to the notified.
     How this information is used, or freed if necessary, depends entirely on
     user policies.  This field is set to NULL at initialization. */
  void *user_data;
};

/* Expand to the number of signals supported by this system.  Every signal
   index from 0 to JITTER_SIGNAL_NO - 1 is valid.
   This macro has the same purpose of NSIG, but allows the user not to
   explicitly include <signal.h> -- in fact the user could rely on this header
   file including signal.h and use NSIG, but this paper-thin abstraction may be
   considered more elegant. */
#define JITTER_SIGNAL_NO  \
  NSIG




/* Notifying about a signal and handling notifications.
 * ************************************************************************** */

/* This functionality is in templates/vm.h . */




/* Initialization and finalization.
 * ************************************************************************** */

/* This functionality is used at state initialization and finalization.  Not
   meant for the user. */

/* Allocate and initialize the pointed notifications field within an Array. */
void
jitter_initialize_pending_signal_notifications
   (struct jitter_signal_notification * volatile * signal_notifications);

/* Free the pointed notification array.  It is intentional that ths is only a
   simple pointer instead that a pointer-to-pointer as in the initialization
   case. */
void
jitter_finalize_pending_signal_notifications
   (struct jitter_signal_notification * volatile signal_notifications);


#endif // #ifndef JITTER_SIGNALS_H_
