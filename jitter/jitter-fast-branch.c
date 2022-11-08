/* Jitter: fast branch utilities.

   Copyright (C) 2017 Luca Saiu
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


#include <jitter/jitter-fast-branch.h>


/* Do nothing if not using patch-ins.
 * ************************************************************************** */

/* This whole source file expands to nothing if patch-ins are not supported
   in this configuration.  The CPP inclusion above suffices to make the CPP
   definition of JITTER_HAVE_PATCH_IN visible, if it exists. */
#ifdef JITTER_HAVE_PATCH_IN

/* Do nothing even if we have patch-ins, at least right now. */


#endif // #ifdef JITTER_HAVE_PATCH_IN
