/* Jitter: a header to be included early.

   Copyright (C) 2022 Luca Saiu
   Updated in 2023 by Luca Saiu
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


#ifndef JITTER_EARLY_HEADER_H_
#define JITTER_EARLY_HEADER_H_


/* Introduction.
 * ************************************************************************** */

/* This header should be included before standard headers.

   Should I require the user to always include this header as well?  I am not
   really sure, and am tempted to reply no.  The current solution suffices for
   Jitter-generated code.  If the user needs special provisions for using
   *standard* C features in her own code because of some stupid platform quirk,
   I would say it is her responsibility to make her code compatibile. */




/* Use Gnulib if needed.
 * ************************************************************************** */

/* Use Gnulib, if available; in particular, if Gnulib is being used for this
   compilation, #include the config.h file which then allows the user to include
   standard headers in their replaced Gnulib version.
   If there is no config.h to include do nothing: we will include the actual
   standard headers instead of Gnulib's replacements. */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif // #ifdef HAVE_CONFIG_H




/* Output conversion workadound for mingw.
 * ************************************************************************** */

/* This silly workaround is requried to be able to use "%Li", "%Lu", "%Lf" and
   friends on some mingw configurations. */
//#undef  __USE_MINGW_ANSI_STDIO  /* prevent a warning on multiple definitions */
//#define __USE_MINGW_ANSI_STDIO  1

#endif /* #ifndef JITTER_EARLY_HEADER_H_ */
