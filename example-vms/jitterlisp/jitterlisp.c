/* JitterLisp: main file.

   Copyright (C) 2017, 2018 Luca Saiu
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
   along with GNU Jitter.  If not, see <https://www.gnu.org/licenses/>. */


#include "jitterlisp.h"

/* Include the GNU Readline header for initialization.  This may not be the most
   appropriate place for this, but it's just one line. */
#ifdef JITTER_HAVE_READLINE
# include <readline/readline.h>
#endif // #ifdef JITTER_HAVE_READLINE


void
jitterlisp_initialize (void)
{
  /* Provide default values for the global settings. */
  jitterlisp_settings_set_default ();

  /* Initialize every subsystem. */
  jitterlisp_memory_initialize ();
  jitterlisp_sexpression_initialize ();
  jitterlisp_constants_initialize ();
  jitterlisp_primitives_initialize ();
  jitterlisp_error_initialize ();
  jitterlisp_vm_initialize ();

#ifdef JITTER_HAVE_READLINE
  /* Enable blink-matching-paren if using GNU Readline.  Some people might find
     this default somewhat too Lispy to be the default in Jitter, which is why
     it's not in jitter-readline.c . */
  rl_variable_bind ("blink-matching-paren", "on");
#endif // #ifdef JITTER_HAVE_READLINE
}

void
jitterlisp_finalize (void)
{
  /* Finalize every subsystem, in the opposite order of initialization. */
  jitterlisp_vm_finalize ();
  jitterlisp_error_finalize ();
  jitterlisp_primitives_finalize ();
  jitterlisp_constants_finalize ();
  jitterlisp_sexpression_finalize ();
  jitterlisp_memory_finalize ();
  jitterlisp_settings_finalize (); /* There's a dynamic buffer to finalize. */
  jitterlisp_printer_finalize ();
}
