/* JitterLisp: eval wrapper.

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


#include "jitterlisp-eval.h"

#include <jitter/jitter-fatal.h>

#include "jitterlisp.h"
#include "jitterlisp-eval-interpreter.h"
#include "jitterlisp-settings.h"


/* Eval wrapper.
 * ************************************************************************** */

// FIXME: remove the commented-out part.  This idea of having two interpreters
// selectable by the user is fundamentally wrong.

jitterlisp_object
jitterlisp_eval_globally (jitterlisp_object form)
{
  /* /\* Just call the appropriate jitterlisp_eval_globally_* function. *\/ */
  /* if (jitterlisp_settings.vm) */
  /*   return jitterlisp_eval_globally_vm (form); */
  /* else */
    return jitterlisp_eval_globally_interpreter (form);
}

jitterlisp_object
jitterlisp_eval (jitterlisp_object form, jitterlisp_object env)
{
  /* /\* Just call the appropriate jitterlisp_eval_* function. *\/ */
  /* if (jitterlisp_settings.vm) */
  /*   return jitterlisp_eval_vm (form, env); */
  /* else */
    return jitterlisp_eval_interpreter (form, env);
}

jitterlisp_object
jitterlisp_apply (jitterlisp_object closure_value,
                  jitterlisp_object operands_as_list)
{
  /* /\* Just call the appropriate jitterlisp_apply_* function. *\/ */
  /* if (jitterlisp_settings.vm) */
  /*   return jitterlisp_apply_vm (closure_value, operands_as_list); */
  /* else */
    return jitterlisp_apply_interpreter (closure_value, operands_as_list);
}
