/* Jitter: jitterc identifier mangling header.

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


#ifndef JITTERC_MANGLE_H_
#define JITTERC_MANGLE_H_


/* Identifier mangling.
 * ************************************************************************** */

/* Given a string, return a malloc-allocated version of it suitable to be used
   as a C identifier.  The mapping is bijective, even if I currently don't need
   the inverse version. */
char *
jitterc_mangle (const char *original)
  __attribute__ ((returns_nonnull, nonnull (1)));


#endif // #ifndef JITTERC_MANGLE_H_
