/* Jitter: string utility functions: header.

   Copyright (C) 2017 Luca Saiu
   Updated in 2021 by Luca Saiu
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


#ifndef JITTER_STRING_H_
#define JITTER_STRING_H_

/* Return a fresh malloc-allocated copy of the given C string. */
char *
jitter_clone_string (const char *s)
  __attribute__ ((returns_nonnull, nonnull (1)));

/* Return a fresh malloc-allocated copy of the given C string, escaped as per
   the rules of C. */
char *
jitter_escape_string (const char *s)
  __attribute__ ((returns_nonnull, nonnull (1)));


#endif // ifndef JITTER_STRING_H_
