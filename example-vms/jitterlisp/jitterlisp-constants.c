/* JitterLisp: constants defined in C.

   Copyright (C) 2019 Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with GNU Jitter under the same license.

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


#include "jitterlisp.h"


/* Constant definition.
 * ************************************************************************** */

/* Define the given symbol to have the given value, and make it constant. */
static void
jitterlisp_define_constant (const char *name_as_c_string,
                            jitterlisp_object value)
{
  struct jitterlisp_symbol *name_as_untagged_symbol
    = jitterlisp_symbol_make_interned (name_as_c_string);
  jitterlisp_object name_as_tagged_symbol
    = JITTERLISP_SYMBOL_ENCODE (name_as_untagged_symbol);
  jitterlisp_define (name_as_tagged_symbol, value);
  name_as_untagged_symbol->global_constant = true;
}

/* Define every constant. */
static void
jitterlisp_define_constants (void)
{
  jitterlisp_define_constant
     ("fixnum-bit-no",
      JITTERLISP_FIXNUM_ENCODE (JITTERLISP_FIXNUM_NON_TAG_BIT_NO));
  jitterlisp_define_constant
     ("most-negative-fixnum", JITTERLISP_FIXNUM_MOST_NEGATIVE);
  jitterlisp_define_constant
     ("most-positive-fixnum", JITTERLISP_FIXNUM_MOST_POSITIVE);
}




/* Initialization and finalization of the constants subsystem.
 * ************************************************************************** */

void
jitterlisp_constants_initialize (void)
{
  /* This initialization needs to be performed only once.  If this is not the
     first time just return. */
  static bool initialized = false;
  if (initialized)
    return;

  /* Define every constants. */
  jitterlisp_define_constants ();
}

void
jitterlisp_constants_finalize (void)
{
  /* Do nothing. */
}
