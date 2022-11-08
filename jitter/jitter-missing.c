/* Jitter: header declaring standard functions that some stupid systems lack.

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


#include <stdint.h>
#include <stdbool.h>

#include <jitter/jitter-missing.h>
#include <jitter/jitter-fatal.h>

/* This is not the Gnulib file: the Jitter runtime does not depend on Gnulib. */
#include <jitter/jitter-config.h>


/* Do nothing. */
#define JITTER_DO_NOTHING  \
  do                       \
    {                      \
    }                      \
  while (false)

/* Fail fatally, printing the name of the function in the error message. */
#define JITTER_FAIL                                        \
  do                                                       \
    {                                                      \
      jitter_fatal ("the standard function "               \
                    __func__                               \
                    "is unimplemented on this platform");  \
    }                                                      \
  while (false)


#ifndef JITTER_HAVE_FLOCKFILE
  void
  flockfile (FILE *f)
  {
    JITTER_DO_NOTHING;
  }

  void
  funlockfile (FILE *f)
  {
    JITTER_DO_NOTHING;
  }
#endif // #ifndef JITTER_HAVE_FLOCKFILE
