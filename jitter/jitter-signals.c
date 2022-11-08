/* Jitter: signal support: implementation.

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


#include <signal.h>

#include <jitter/jitter-signals.h>
#include <jitter/jitter-malloc.h>




/* Initialization and finalization.
 * ************************************************************************** */

/* Initialize the one pointed signal notification array element. */
static
void
jitter_initialize_pending_signal_notification
  (struct jitter_signal_notification * volatile s)
{
  s->pending = false;
  s->user_data = NULL;
}

/* Finalize the one pointed signal notification array element. */
static
void
jitter_finalize_pending_signal_notification
  (struct jitter_signal_notification * volatile s)
{
  /* Do nothing. */
}

void
jitter_initialize_pending_signal_notifications
   (struct jitter_signal_notification * volatile * signal_notifications)
{
  struct jitter_signal_notification * volatile array
    = jitter_xmalloc (NSIG * sizeof (struct jitter_signal_notification));
  int i;
  for (i = 0; i < NSIG; i ++)
    jitter_initialize_pending_signal_notification (array + i);
  * signal_notifications = array;
}

void
jitter_finalize_pending_signal_notifications
   (struct jitter_signal_notification * volatile signal_notifications)
{
  int i;
  for (i = 0; i < NSIG; i ++)
    jitter_finalize_pending_signal_notification (signal_notifications + i);
  free (signal_notifications);
}
