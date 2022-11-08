/* JitterLisp: utility function header.

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


#ifndef JITTERLISP_UTILITY_H_
#define JITTERLISP_UTILITY_H_

#include "jitterlisp-sexpression.h"




/* S-expression shape checking.
 * ************************************************************************** */

/* Return non-false iff the given s-expression is a list. */
bool
jitterlisp_is_list (jitterlisp_object o);

/* Return non-false iff the given s-expression is a list having exactly the
   given length. */
bool
jitterlisp_is_list_of_length (jitterlisp_object o, size_t length);

/* Return non-false iff the given s-expression is a list of symbols. */
bool
jitterlisp_is_list_of_symbols (jitterlisp_object o);

/* Return non-false iff the given s-expression is a list of symbols all
   different from one another.  This is O(n log(n)). */
bool
jitterlisp_is_list_of_distinct_symbols (jitterlisp_object o);

/* Return non-false iff the given s-expression is a non-global environment of
   the shape defined below, as used by the AST interpreter and by eval and
   macroexpand in Lisp.
   A well-formed environment is an a-list with symbols as keys and boxes as
   values.  There is no restriction on the box content (which may contain other
   boxes), and the symbols are not required to be unique. */
bool
jitterlisp_is_environment (jitterlisp_object o);

/* A macro wrapper around jitterlisp_is_list , convenient for type-checking
   primitive arguments.  This calls jitterlisp_is_list . */
#define JITTERLISP_IS_LIST(x)  \
  (jitterlisp_is_list (x))

/* A macro wrapper around jitterlisp_is_list_of_symbols , convenient for
   type-checking primitive arguments.  This calls jitterlisp_is_list_of_symbols
   . */
#define JITTERLISP_IS_SYMBOLS(x)  \
  (jitterlisp_is_list_of_symbols (x))

/* A macro wrapper around jitterlisp_is_environment , convenient for
   type-checking primitive arguments.  This calls jitterlisp_is_environment . */
#define JITTERLISP_IS_ENVIRONMENT(x)  \
  (jitterlisp_is_environment (x))




/* S-expression shape validation.
 * ************************************************************************** */

/* The functions in this section do nothing on success, and error out in case of
   failure.  They all take an encoded s-expression. */

/* Error out if the given encoded s-expression is not a symbol. */
void
jitterlisp_validate_empty_list (jitterlisp_object o);

/* Error out if the given encoded s-expression is not a symbol. */
void
jitterlisp_validate_symbol (jitterlisp_object o);

/* Error out if the given encoded s-expression is not a box. */
void
jitterlisp_validate_box (jitterlisp_object o);

/* Error out if the given encoded s-expression is not a primitive. */
void
jitterlisp_validate_primitive (jitterlisp_object o);

/* Error out if the given encoded s-expression is not an AST. */
void
jitterlisp_validate_ast (jitterlisp_object o);

/* Error out if the given encoded s-expression is not a list of symbols. */
void
jitterlisp_validate_symbols (jitterlisp_object list);

/* Error out if the given encoded s-expression is not a list of symbols all
   different from one another.  This is O(n log(n)). */
void
jitterlisp_validate_distinct_symbols (jitterlisp_object list);

/* Error out if the given encoded s-expression is not a list of ASTs. */
void
jitterlisp_validate_asts (jitterlisp_object list);

/* Error out if the given encoded s-expression is not a non-global environment
   as per jitterlisp_is_environment .  This is meant for validating the
   arguments of eval and macroexpand , as used from Lisp. */
void
jitterlisp_validate_environment (jitterlisp_object o);




/* List functions.
 * ************************************************************************** */

/* Return the length of the given encoded list, unencoded.  Error out if the
   argument is not a list. */
size_t
jitterlisp_length (jitterlisp_object list);




/* S-expression constructors and accessors.
 * ************************************************************************** */

/* Return a fresh cons of the given car and cdr. */
jitterlisp_object
jitterlisp_cons (jitterlisp_object car, jitterlisp_object cdr);

/* Return the encoded car of the given encoded s-expression, if the argument is
   a cons; error out otherwise. */
jitterlisp_object
jitterlisp_car (jitterlisp_object cons);

/* Return the encoded cdr of the given encoded s-expression, if the argument is
   a cons; error out otherwise. */
jitterlisp_object
jitterlisp_cdr (jitterlisp_object cons);

/* Composed selectors. */
jitterlisp_object
jitterlisp_caar (jitterlisp_object cons);
jitterlisp_object
jitterlisp_cadr (jitterlisp_object cons);
jitterlisp_object
jitterlisp_cdar (jitterlisp_object cons);
jitterlisp_object
jitterlisp_cddr (jitterlisp_object cons);
jitterlisp_object
jitterlisp_caaar (jitterlisp_object cons);
jitterlisp_object
jitterlisp_caadr (jitterlisp_object cons);
jitterlisp_object
jitterlisp_cadar (jitterlisp_object cons);
jitterlisp_object
jitterlisp_caddr (jitterlisp_object cons);
jitterlisp_object
jitterlisp_cdaar (jitterlisp_object cons);
jitterlisp_object
jitterlisp_cdadr (jitterlisp_object cons);
jitterlisp_object
jitterlisp_cddar (jitterlisp_object cons);
jitterlisp_object
jitterlisp_cdddr (jitterlisp_object cons);

/* Return a fresh list with the given objects as elements. */
jitterlisp_object
jitterlisp_list_1 (jitterlisp_object o0);
jitterlisp_object
jitterlisp_list_2 (jitterlisp_object o0, jitterlisp_object o1);
jitterlisp_object
jitterlisp_list_3 (jitterlisp_object o0, jitterlisp_object o1,
                   jitterlisp_object o2);
jitterlisp_object
jitterlisp_list_4 (jitterlisp_object o0, jitterlisp_object o1,
                   jitterlisp_object o2, jitterlisp_object o3);

/* Return a fresh encoded box containing the given encoded object. */
jitterlisp_object
jitterlisp_box (jitterlisp_object o);

/* Return the encoded content of the given encoded box.  Error out if the
   argument is not a box. */
jitterlisp_object
jitterlisp_box_get (jitterlisp_object box);

/* Destructively update the given encoded box, setting it to contain the given
   encoded object.  Error out if the box argument is not actually box.*/
void
jitterlisp_box_setb (jitterlisp_object box, jitterlisp_object new_content);




/* Non-global environments (for the AST interpreter only).
 * ************************************************************************** */

/* This data structure holds a binding from variable to value representing a
   non-global environment for the AST interpreter.

   Non-global means local (procedure arguments, let) plus non-local (locals from
   outer static contexts) variables.  Global variables are handled differently,
   with a value directly stored in the symbol data structure.  Non-global
   bindings have precedence over global bindings.

   Variables are encoded as symbols and compared by identity.  Each associated
   value is a box, whose content is updated by set! operations on non-global
   variables.  This indirection thru a box has a cost and may seem overkill
   as conses are already mutable and may be shared, but the box becomes
   necessary when assigned variables are shared between the interpreter and
   compiled procedures, which don't use this data structure to represent
   environments; compiled procedures in fact avoid boxes when possible, but
   their more efficient representation relies on a code analysis which the
   interpreter cannot afford at every variable binding.

   The functions in this section assume non-global environments to be
   well-formed, and don't validate them.  The assumption is correct if the
   functions are called from eval which is in its turn called from C code or
   from primitives, after validation has already been perforemd. */

/* The empty non-global environment. */
extern const jitterlisp_object
jitterlisp_empty_environment;

/* Return an expanded non-global environment, sharing structure with the given
   one, binding the given name to the given value.  The given environment is
   not modified. */
jitterlisp_object
jitterlisp_environment_bind (jitterlisp_object env, jitterlisp_object name,
                             jitterlisp_object value);

/* Return the value bound to the given name in the local environment and,
   failing that, in the global environment.  Error out if the name is not bound
   in the global environment either.  This returns the box content, not the box
   -- of course the box content may be another box, and this function only
   performes *one* unboxing operation. */
jitterlisp_object
jitterlisp_environment_lookup (jitterlisp_object env, jitterlisp_object name);

/* Return non-false iff the given symbol is bound in either the given non-global
   environment or the global environment. */
bool
jitterlisp_environment_has (jitterlisp_object env, jitterlisp_object name);

/* Destructively update the first binding for the given name in the given
   non-global environment, or the global binding if no non-global environment
   exists (subject to the error conditions of jitterlisp_global_set, below).
   Error out if the name is not bound. */
void
jitterlisp_environment_setb (jitterlisp_object env, jitterlisp_object name,
                             jitterlisp_object new_value);

/* Set the global binding for the given symbol, replacing the previous global
   binding in case one was present.  Error out if the symbol is constant. */
void
jitterlisp_define (jitterlisp_object name, jitterlisp_object new_value);




#endif // #ifndef JITTERLISP_UTILITY_H_
