/* JitterLisp: main header.

   Copyright (C) 2017, 2018 Luca Saiu
   Updated in 2019 by Luca Saiu
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


#ifndef JITTERLISP_H_
#define JITTERLISP_H_


/* Header transitive inclusion.
 * ************************************************************************** */

/* JitterLisp does rely on Gnulib for portability. */
#include <config.h>

/* Include the other JitterLisp headers, so that the user has to deal with this
   one only. */
#include "jitterlisp-config.h"
#include "jitterlisp-constant-strings.h"
#include "jitterlisp-sexpression.h"
#include "jitterlisp-allocator.h"
#include "jitterlisp-ast.h"
#include "jitterlisp-banner.h"
#include "jitterlisp-code-generator.h"
#include "jitterlisp-constants.h"
#include "jitterlisp-error.h"
#include "jitterlisp-operations.h"
#include "jitterlisp-reader.h"
#include "jitterlisp-run-input.h"
#include "jitterlisp-primitives.h"
#include "jitterlisp-printer.h"
#include "jitterlisp-settings.h"
#include "jitterlisp-eval.h"
#include "jitterlisp-eval-interpreter.h"
#include "jitterlisp-eval-vm.h"
#include "jitterlisp-macros.h"
#include "jitterlisp-utility.h"




/* Initialization and finalization.
 * ************************************************************************** */

/* Initialise JitterLisp, except for the printer API which must be initialised
   separately, after we decide whether styling is enabled.  It is in general
   unsafe to call any function or macro from the JitterLisp headers before
   this. */
void
jitterlisp_initialize (void);

/* Finalize JitterLisp (including the printer API).  It is in general unsafe to
   call any function or macro from the JitterLisp headers after this, until
   jitterlisp_initialize is called again. */
void
jitterlisp_finalize (void);

#endif // #ifndef JITTERLISP_H_
