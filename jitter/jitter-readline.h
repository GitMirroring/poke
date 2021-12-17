/* Readline: either a GNU Readline wrapper or a trivial emulator -- header.

   Copyright (C) 2017, 2020 Luca Saiu
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


#ifndef JITTER_READLINE_H_
#define JITTER_READLINE_H_


/* This functionality is part of a separate library.
 * ************************************************************************** */

/* In order to keep the Jitter runtime library small and minimal, the Jitter
   wrapper for readline is built as a separate library named
     libjitter-readline.la
   .  The small library is always built, but according to the configuration it
   may actually use GNU Readline and depend on it, or just provide a crude but
   compatible emulation.

   See jitter/jitter-config.h and the documentation about how to conveniently
   use the wrapper library from Jittery programs. */




/* Readline.
 * ************************************************************************** */

/* Export one function having an API similar to GNU Readline.  The
   implementation either calls the actual GNU Readline function, or implements a
   trivial compatible functionality for reading a line, but without any
   line-editing facilities. */

/* Display the pointed prompt unless it's NULL, then read one line from the
   input and return it as a malloc-allocated string, which the user will be
   responsible for freeing; if the input line contains an EOF and is otherwise
   empty return NULL instead.
   When using the actual GNU Readline in the implementation also add the
   returned line to the history, unless the line is empty. */
char *
jitter_readline (const char *prompt_or_NULL);


#endif // #ifndef JITTER_READLINE_H_
