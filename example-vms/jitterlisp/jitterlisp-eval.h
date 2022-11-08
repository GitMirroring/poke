/* JitterLisp: eval wrapper header.

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


#ifndef JITTERLISP_EVAL_H_
#define JITTERLISP_EVAL_H_

#include "jitterlisp-sexpression.h"


/* Eval wrapper.
 * ************************************************************************** */

/* This is a wrapper, calling either the naïf C interpreter or a Jittery VM
   according to the global settings. */

// FIXME: the comment above is now wrong.

/* Macroexpand the given JitterLisp form, and return the result of its
   evaluation, using the global environment only. */
jitterlisp_object
jitterlisp_eval_globally (jitterlisp_object form);

/* Macroexpand the given JitterLisp form, and return the result of its
   evaluation, using the given non-global environment shaped as described in
   jitterlisp-utility.h . */
jitterlisp_object
jitterlisp_eval (jitterlisp_object form, jitterlisp_object env);

/* Return the result of applying closure_value, which is assumed to be an
   already evaluated closure (not checked by the function) to the operands,
   assumed to be a list (not checked for) of already evaluated operands. */
jitterlisp_object
jitterlisp_apply (jitterlisp_object closure_value,
                  jitterlisp_object operands_as_list);

#endif // #ifndef JITTERLISP_EVAL_H_
