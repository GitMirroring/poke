/* Jitter: jitterc identifier mangling.

   Copyright (C) 2017, 2018 Luca Saiu
   Updated in 2019 by Luca Saiu
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


/* Include the Gnulib header. */
#include <config.h>

#include <ctype.h>
#include <c-ctype.h>
#include <string.h>

#include <xalloc.h>

#include <jitter/jitter-fatal.h>
#include "jitterc-mangle.h"


/* Identifier mangling.
 * ************************************************************************** */

/* The highest possible growth factor of a mangled string compared to the
   original.  This must be updated if the mangling defined below in
   jitter_mangle changes to require more space. */
#define JITTERC_MANGLE_MAX_GROWTH 2

char *
jitterc_mangle (const char *original)
{
  size_t length = strlen (original);
  /* Allocate space very generously, in order not to worry about allocation
     during the actual translation: make space for two characters more than how
     many we have in the input (the initial digit prefix and the final '\0'),
     all scaled by the maximum growth factor.  Memory usage will be modest in
     practice, and we are going to trim the buffer at the end. */
  char *big_buffer = xmalloc ((1 + length + 1) * JITTERC_MANGLE_MAX_GROWTH);
  const char *in;
  char *out = big_buffer;

/* Add one character to the output, and advance the output pointer. */
#define JITTERC_ADD_CHARACTER(c)  \
  do                              \
    {                             \
      * (out ++) = (c);           \
    }                             \
  while (false)

/* Add one string to the output, and advance the output pointer. */
#define JITTERC_ADD_STRING(s)              \
  do                                       \
    {                                      \
      strcpy (out, s);                     \
      out += strlen (s);                   \
    }                                      \
  while (false)

  /* Start with an underscore and a q if and only if the first character is a
     digit, so that the mangled version alone, even withot a prefix, is a valid
     C identifier.  Then every character including the first, even if it's a
     digit, is encoded the same way. */
  if (length > 0 && c_isdigit (original [0]))
    JITTERC_ADD_STRING("_q");

  /* Translate each character. */
  for (in = original; * in != '\0'; in ++)
    {
      char c = *in;
      switch (c)
        {
        /* Replace occurrences of a few special characters with fixed strings,
           always starting with single underscores to keep the mapping
           bijective.  */
        case '@':
          JITTERC_ADD_STRING("_a");
          break;
        case '\\':
          JITTERC_ADD_STRING("_b");
          break;
        case '.':
          JITTERC_ADD_STRING("_d");
          break;
        case '!':
          /* The '!' character is not allowed in user identifiers, but is used
             (in order to make them visually distinct) in special specialized
             instructions; therefore we need an encoding for it. */
          JITTERC_ADD_STRING("_e");
          break;
        case '*':
          /* Again the asterisk is not allowed in user identifiers, but occurs
             (as a prefix) in replacement specialized instruction names. */
          JITTERC_ADD_STRING("_A");
          break;
        case '-':
          JITTERC_ADD_STRING("_m");
          break;
        case '+':
          JITTERC_ADD_STRING("_p");
          break;
        case '%':
          /* The percent sign does not occur in user identifiers, but since it
             does as a register prefix we need to encode in specialized
             instruction names. */
          JITTERC_ADD_STRING("_r");
          break;
        case '~':
          JITTERC_ADD_STRING("_t");
          break;
        case '_':
          JITTERC_ADD_STRING("_u");
          break;
        case '/':
          /* A double underscore cannot occur in a mangled string, other than by
             this translation.  This makes arguments easier to recognize at a
             glance in specialized instruction mangled names. */
          JITTERC_ADD_STRING("__");
          break;

        /* Every other character can be reproduced as is... */
        default:
          /* ...Except that it's better to play it safe.  Here I'm using
             Gnulib's c_isalnum function, which only works on ASCII chracters
             independently from the locale -- which is what I want. */
          if (! c_isalnum (c))
            jitter_fatal ("mangling: non-alphanumeric character '%c' "
                          "(0x%x) in %s", c, c, original);
          JITTERC_ADD_CHARACTER(c);
        }
    }

  /* The translation is done.  Add a string terminator. */
  JITTERC_ADD_CHARACTER('\0');

  /* We no longer need the temporary macros. */
#undef JITTERC_ADD_CHARACTER
#undef JITTERC_ADD_STRING

  /* Trim the allocated string to make it just large enough. */
  return xrealloc (big_buffer, out - big_buffer);
}
