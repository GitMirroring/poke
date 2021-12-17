/* JitterLisp: Lisp macro header.

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
   along with GNU Jitter.  If not, see <http://www.gnu.org/licenses/>. */


#ifndef JITTERLISP_MACROS_H_
#define JITTERLISP_MACROS_H_

#include "jitterlisp-sexpression.h"




/* Macroexpansion.
 * ************************************************************************** */

/* Return the macroexpansion of the given s-expression in the given
   expansion-time environment [FIXME: encoded how?  Decide, document, make a
   wrapper in the .c file].

   The result will be a tagged AST. */
jitterlisp_object
jitterlisp_macroexpand (jitterlisp_object o, jitterlisp_object env);

/* Return the macroexpansion of the given s-expression using the global
   environment only.  The result will be a tagged AST. */
jitterlisp_object
jitterlisp_macroexpand_globally (jitterlisp_object o);




/* Macro primitives.
 * ************************************************************************** */

/* These are the C functions used in the implementation of the primitive macros
   building ASTs.  Like every primitive macro function they take exactly two
   argument, the unexpanded cdr of the macro call and a non-global environment.
   The functions check that their argument is well-formed, then either error out
   in case of problems or return a fresh AST as an encoded s-expression.  Those
   primitive macros take care of recursively macroexpanding subs where
   needed. */
jitterlisp_object
jitterlisp_primitive_macro_function_define (jitterlisp_object cdr,
                                            jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_define_constant (jitterlisp_object cdr,
                                                     jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_if (jitterlisp_object cdr,
                                        jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_cond (jitterlisp_object cdr,
                                          jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_setb (jitterlisp_object cdr,
                                          jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_while (jitterlisp_object cdr,
                                           jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_primitive (jitterlisp_object cdr,
                                               jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_call (jitterlisp_object cdr,
                                          jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_lambda (jitterlisp_object cdr,
                                            jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_let_star (jitterlisp_object cdr,
                                              jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_begin (jitterlisp_object cdr,
                                           jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_quote (jitterlisp_object cdr,
                                           jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_undefined (jitterlisp_object cdr,
                                               jitterlisp_object env);
jitterlisp_object
jitterlisp_primitive_macro_function_low_level_macro
   (jitterlisp_object cdr, /* Args: . body-forms */
    jitterlisp_object env);

#endif // #ifndef JITTERLISP_MACROS_H_
