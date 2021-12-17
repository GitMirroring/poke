/* Jitter: fatal error functions.

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
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTER_FATAL_H_
#define JITTER_FATAL_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>


/* Fatal error reporting.
 * ************************************************************************** */

#define jitter_fatal(...)                                           \
  do                                                                \
    {                                                               \
      /* Ignore any failures in printing a fatal error message. */  \
      printf ("FATAL ERROR: " __VA_ARGS__);                         \
      printf ("\n");                                                \
      exit (EXIT_FAILURE);                                          \
    }                                                               \
  while (false)

#define jitter_unimplemented(message) \
  do                                                                          \
    {                                                                         \
      jitter_fatal ("%s:%i: %s: unimplemented", __FILE__, __LINE__, message); \
    }                                                                         \
  while (false)

#endif // #ifndef JITTER_FATAL_H_
