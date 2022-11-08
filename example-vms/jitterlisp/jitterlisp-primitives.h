/* JitterLisp: primitives header.

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


#ifndef JITTERLISP_PRIMITIVES_H_
#define JITTERLISP_PRIMITIVES_H_

#include "jitterlisp-sexpression.h"




/* Not for the user: initialization/finalization of the primitives subsystem.
 * ************************************************************************** */

/* The functions here are not for the user to call directly.  The user is
   supposed to initialize and finalize every JitterLisp subsystem by calling
   jitterlisp_initialize and jitterlisp_finalize , which in their turn will call
   these functions in the appropriate order. */

/* Initialize the primitives subsystem. */
void
jitterlisp_primitives_initialize (void);

/* Finalize the primitives subsystem. */
void
jitterlisp_primitives_finalize (void);


#endif // #ifndef JITTERLISP_PRIMITIVES_H_
