/* JitterLisp: utility functions.

   Copyright (C) 2017, 2018, 2020 Luca Saiu
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


#include "jitterlisp-utility.h"

#include <stdlib.h>
#include <jitter/jitter-malloc.h>

#include "jitterlisp.h"




/* S-expression shape checking.
 * ************************************************************************** */

bool
jitterlisp_is_list (jitterlisp_object o)
{
  while (! JITTERLISP_IS_EMPTY_LIST(o))
    {
      if (! JITTERLISP_IS_CONS(o))
        return false;

      o = JITTERLISP_EXP_C_A_CDR(o);
    }
  return true;
}

bool
jitterlisp_is_list_of_length (jitterlisp_object o, size_t required_length)
{
  size_t actual_length = 0;
  while (! JITTERLISP_IS_EMPTY_LIST(o))
    {
      if (! JITTERLISP_IS_CONS(o))
        return false;

      o = JITTERLISP_EXP_C_A_CDR(o);

      if (++ actual_length > required_length)
        return false;;
    }
  return required_length == actual_length;
}

bool
jitterlisp_is_list_of_symbols (jitterlisp_object o)
{
  while (! JITTERLISP_IS_EMPTY_LIST(o))
    {
      if (! JITTERLISP_IS_CONS(o))
        return false;
      if (! JITTERLISP_IS_SYMBOL(JITTERLISP_EXP_C_A_CAR(o)))
        return false;

      o = JITTERLISP_EXP_C_A_CDR(o);
    }
  return true;
}

/* Compare symbols as if they were untagged pointers; in practice we only care
   about identity, and any ordering based on memory order works as long as
   objects are not moved while the sorting function runs.  This is a comparison
   function suitable for qsort . */
static int
jitterlisp_compare_symbols (const void *ap, const void *bp)
{
  /* Read symbol objects from the array. */
  jitterlisp_object a = * (jitterlisp_object *) ap;
  jitterlisp_object b = * (jitterlisp_object *) bp;

  /* The symbols encoded in a and b are still tagged pointers, but we can
     compare them as they are. */
  return b - a;
}

bool
jitterlisp_is_list_of_distinct_symbols (jitterlisp_object o)
{
  /* Compute the length, returning early if the shape is not what we expect. */
  size_t length = 0;
  jitterlisp_object o_rest = o;
  while (! JITTERLISP_IS_EMPTY_LIST(o_rest))
    {
      if (! JITTERLISP_IS_CONS(o_rest))
        return false;
      if (! JITTERLISP_IS_SYMBOL(JITTERLISP_EXP_C_A_CAR(o_rest)))
        return false;

      length ++;
      o_rest = JITTERLISP_EXP_C_A_CDR(o_rest);
    }

  /* If we are here then o is actually a list of symbols.  Copy every list
     element to a temporary array. */
  jitterlisp_object *symbols
    = jitter_xmalloc (sizeof (jitterlisp_object) * length);
  jitterlisp_object *symbols_rest = symbols;
  o_rest = o;
  while (! JITTERLISP_IS_EMPTY_LIST(o_rest))
    {
      * (symbols_rest ++) = JITTERLISP_EXP_C_A_CAR(o_rest);
      o_rest = JITTERLISP_EXP_C_A_CDR(o_rest);
    }

  /* Sort the array comparing elements as pointer-sized integers.  Of course
     symbols, interned or not, can be compared by identity. */
  qsort (symbols, length, sizeof (jitterlisp_object),
         jitterlisp_compare_symbols);

  /* Check whether the array is sorted.  We can break out of the loop as soon as
     we find two equal adjacent elements but at this point we cannot return
     early, since we have to free the array independently from the result. */
  bool res = true;
  symbols_rest = symbols;
  jitterlisp_object * const symbols_limit = symbols + length;
  while (res && symbols_rest + 1 < symbols_limit)
    if (jitterlisp_compare_symbols (symbols_rest, symbols_rest + 1) == 0)
      res = false;
    else
      symbols_rest ++;

  /* Free the temporary array and return. */
  free (symbols);
  return res;
}

bool
jitterlisp_is_environment (jitterlisp_object o)
{
  while (! JITTERLISP_IS_EMPTY_LIST(o))
    {
      if (! JITTERLISP_IS_CONS(o))
        return false;
      jitterlisp_object element = JITTERLISP_EXP_C_A_CAR(o);
      if (! JITTERLISP_IS_CONS(element)
          || ! JITTERLISP_IS_SYMBOL(JITTERLISP_EXP_C_A_CAR(element))
          || ! JITTERLISP_IS_BOX(JITTERLISP_EXP_C_A_CDR(element)))
        return false;

      o = JITTERLISP_EXP_C_A_CDR(o);
    }
  return true;
}




/* S-expression shape validation.
 * ************************************************************************** */

void
jitterlisp_validate_empty_list (jitterlisp_object o)
{
  if (! JITTERLISP_IS_EMPTY_LIST(o))
    jitterlisp_error_cloned ("jitterlisp_validate_empty_list: non-null argument");
}

void
jitterlisp_validate_symbol (jitterlisp_object o)
{
  if (! JITTERLISP_IS_SYMBOL(o))
    jitterlisp_error_cloned ("jitterlisp_validate_symbol: non-symbol argument");
}

void
jitterlisp_validate_box (jitterlisp_object o)
{
  if (! JITTERLISP_IS_BOX(o))
    jitterlisp_error_cloned ("jitterlisp_validate_box: non-box argument");
}

void
jitterlisp_validate_primitive (jitterlisp_object o)
{
  if (! JITTERLISP_IS_PRIMITIVE(o))
    {
      jitterlisp_print_error_char_star ("About ");
      jitterlisp_print_error (o);
      jitterlisp_print_error_char_star ("\n"); // FIXME: add to the error message
      jitterlisp_error_cloned ("jitterlisp_validate_primitive: non-primitive argument");
    }
}

void
jitterlisp_validate_ast (jitterlisp_object o)
{
  if (! JITTERLISP_IS_AST(o))
    jitterlisp_error_cloned ("jitterlisp_validate_ast: non-AST argument");
}

void
jitterlisp_validate_symbols (jitterlisp_object o)
{
  if (! jitterlisp_is_list_of_symbols (o))
    jitterlisp_error_cloned ("jitterlisp_validate_symbols: not a list of "
                             "symbols");
}

void
jitterlisp_validate_distinct_symbols (jitterlisp_object o)
{
  if (! jitterlisp_is_list_of_distinct_symbols (o))
    jitterlisp_error_cloned ("jitterlisp_validate_distinct_symbols: not a "
                             "list of distinct symbols");
}

void
jitterlisp_validate_asts (jitterlisp_object list)
{
  while (! JITTERLISP_IS_EMPTY_LIST(list))
    {
      if (! JITTERLISP_IS_CONS(list))
        jitterlisp_error_cloned ("jitterlisp_validate_asts: non-list argument");
      if (! JITTERLISP_IS_AST(JITTERLISP_EXP_C_A_CAR(list)))
        jitterlisp_error_cloned ("jitterlisp_validate_asts: non-AST element");

      list = JITTERLISP_EXP_C_A_CDR(list);
    }
}

void
jitterlisp_validate_environment (jitterlisp_object o)
{
  if (! jitterlisp_is_environment (o))
    jitterlisp_error_cloned ("jitterlisp_validate_environment: not a "
                             "non-global environment (an alist with symbols "
                             "as keys and boxes as values)");
}




/* List functions.
 * ************************************************************************** */

size_t
jitterlisp_length (jitterlisp_object list)
{
  size_t res = 0;
  while (! JITTERLISP_IS_EMPTY_LIST(list))
    {
      if (! JITTERLISP_IS_CONS(list))
        jitterlisp_error_cloned ("jitterlisp_length: non-list argument");

      res ++;
      list = JITTERLISP_EXP_C_A_CDR(list);
    }
  return res;
}




/* S-expression constructors and selectors.
 * ************************************************************************** */

jitterlisp_object
jitterlisp_cons (jitterlisp_object car, jitterlisp_object cdr)
{
  jitterlisp_object res;
  JITTERLISP_CONS_(res, car, cdr);
  return res;
}

jitterlisp_object
jitterlisp_car (jitterlisp_object cons)
{
  if (! JITTERLISP_IS_CONS(cons))
    jitterlisp_error_cloned ("jitterlisp_car: non-cons argument");
  return JITTERLISP_EXP_C_A_CAR(cons);
}

jitterlisp_object
jitterlisp_cdr (jitterlisp_object cons)
{
  if (! JITTERLISP_IS_CONS(cons))
    jitterlisp_error_cloned ("jitterlisp_cdr: non-cons argument");
  return JITTERLISP_EXP_C_A_CDR(cons);
}




/* Composed selectors.
 * ************************************************************************** */

/* Length 2.*/
jitterlisp_object
jitterlisp_caar (jitterlisp_object cons)
{
  return jitterlisp_car (jitterlisp_car (cons));
}
jitterlisp_object
jitterlisp_cadr (jitterlisp_object cons)
{
  return jitterlisp_car (jitterlisp_cdr (cons));
}
jitterlisp_object
jitterlisp_cdar (jitterlisp_object cons)
{
  return jitterlisp_cdr (jitterlisp_car (cons));
}
jitterlisp_object
jitterlisp_cddr (jitterlisp_object cons)
{
  return jitterlisp_cdr (jitterlisp_cdr (cons));
}

/* Length 3.*/
jitterlisp_object
jitterlisp_caaar (jitterlisp_object cons)
{
  return jitterlisp_car (jitterlisp_caar (cons));
}
jitterlisp_object
jitterlisp_caadr (jitterlisp_object cons)
{
  return jitterlisp_car (jitterlisp_cadr (cons));
}
jitterlisp_object
jitterlisp_cadar (jitterlisp_object cons)
{
  return jitterlisp_car (jitterlisp_cdar (cons));
}
jitterlisp_object
jitterlisp_caddr (jitterlisp_object cons)
{
  return jitterlisp_car (jitterlisp_cddr (cons));
}
jitterlisp_object
jitterlisp_cdaar (jitterlisp_object cons)
{
  return jitterlisp_cdr (jitterlisp_caar (cons));
}
jitterlisp_object
jitterlisp_cdadr (jitterlisp_object cons)
{
  return jitterlisp_cdr (jitterlisp_cadr (cons));
}
jitterlisp_object
jitterlisp_cddar (jitterlisp_object cons)
{
  return jitterlisp_cdr (jitterlisp_cdar (cons));
}
jitterlisp_object
jitterlisp_cdddr (jitterlisp_object cons)
{
  return jitterlisp_cdr (jitterlisp_cddr (cons));
}

jitterlisp_object
jitterlisp_list_1 (jitterlisp_object o0)
{
  return jitterlisp_cons (o0, JITTERLISP_EMPTY_LIST);
}

jitterlisp_object
jitterlisp_list_2 (jitterlisp_object o0, jitterlisp_object o1)
{
  return jitterlisp_cons (o0, jitterlisp_list_1 (o1));
}

jitterlisp_object
jitterlisp_list_3 (jitterlisp_object o0, jitterlisp_object o1,
                   jitterlisp_object o2)
{
  return jitterlisp_cons (o0, jitterlisp_list_2 (o1, o2));
}

jitterlisp_object
jitterlisp_list_4 (jitterlisp_object o0, jitterlisp_object o1,
                   jitterlisp_object o2, jitterlisp_object o3)
{
  return jitterlisp_cons (o0, jitterlisp_list_3 (o1, o2, o3));
}

jitterlisp_object
jitterlisp_box (jitterlisp_object o)
{
  jitterlisp_object res;
  JITTERLISP_BOX_(res, o);
  return res;
}

jitterlisp_object
jitterlisp_box_get (jitterlisp_object box)
{
  jitterlisp_validate_box (box);
  return JITTERLISP_EXP_B_A_GET(box);
}

void
jitterlisp_box_setb (jitterlisp_object box, jitterlisp_object new_content)
{
  jitterlisp_validate_box (box);
  jitterlisp_object useless __attribute__ ((unused));
  JITTERLISP_BOX_SETB_(useless, box, new_content);
}




/* Non-global environments.
 * ************************************************************************** */

const jitterlisp_object
jitterlisp_empty_environment = JITTERLISP_EMPTY_LIST;

jitterlisp_object
jitterlisp_environment_bind (jitterlisp_object env, jitterlisp_object name,
                             jitterlisp_object value)
{
  jitterlisp_object box = jitterlisp_box (value);
  return jitterlisp_cons (jitterlisp_cons (name, box), env);
}

jitterlisp_object
jitterlisp_environment_lookup (jitterlisp_object env, jitterlisp_object name)
{
  /* First look for a binding in the local environment, which is to say look
     for the first cons in env whose car is equal-by-identity to name... */
  jitterlisp_object env_rest;
  for (env_rest = env;
       env_rest != JITTERLISP_EMPTY_LIST;
       env_rest = JITTERLISP_EXP_C_A_CDR(env_rest))
    {
      jitterlisp_object next_cons = JITTERLISP_EXP_C_A_CAR(env_rest);
      jitterlisp_object next_name = JITTERLISP_EXP_C_A_CAR(next_cons);
      if (next_name == name)
        {
          jitterlisp_object cdr = JITTERLISP_EXP_C_A_CDR(next_cons);
          return JITTERLISP_EXP_B_A_GET(cdr);
        }
    }

  /* ...The symbol is not bound in the given local environment.  Look it up as a
     global. */
  struct jitterlisp_symbol *unencoded_name = JITTERLISP_SYMBOL_DECODE(name);
  jitterlisp_object res = unencoded_name->global_value;
  if (JITTERLISP_IS_UNDEFINED(res))
    {
      jitterlisp_print_error_char_star ("About ");
      jitterlisp_print_error (name);
      jitterlisp_print_error_char_star ("\n"); // FIXME: add to the error message
      jitterlisp_error_cloned ("unbound variable");
    }
  else
    return res;
}

bool
jitterlisp_environment_has (jitterlisp_object env, jitterlisp_object name)
{
  /* First look for a binding in the local environment, which is to say look
     for the first cons in env whose car is equal-by-identity to name... */
  jitterlisp_object env_rest;
  for (env_rest = env;
       env_rest != JITTERLISP_EMPTY_LIST;
       env_rest = JITTERLISP_EXP_C_A_CDR(env_rest))
    {
      jitterlisp_object next_cons = JITTERLISP_EXP_C_A_CAR(env_rest);
      jitterlisp_object next_name = JITTERLISP_EXP_C_A_CAR(next_cons);
      if (next_name == name)
        return true;
    }

  /* ...The symbol is not bound in the given local environment.  Look it up as a
     global. */
  struct jitterlisp_symbol *unencoded_name = JITTERLISP_SYMBOL_DECODE(name);
  return ! JITTERLISP_IS_UNDEFINED(unencoded_name->global_value);
}

void
jitterlisp_define (jitterlisp_object name, jitterlisp_object new_value)
{
  struct jitterlisp_symbol *unencoded_name = JITTERLISP_SYMBOL_DECODE(name);
  if (unencoded_name->global_constant)
    {
      jitterlisp_print_error_char_star ("About ");
      jitterlisp_print_error (name);
      jitterlisp_print_error_char_star ("\n"); // FIXME: add to the error message
      jitterlisp_error_cloned ("assignment to global constant");
    }

  /* The symbol is not a constant.  We can perform the definition or
     assignment. */
  unencoded_name->global_value = new_value;
}

/* Destructively update the global binding for the given symbol, which must
   be already globally bound and non-constant, to the new value.  Error out
   if the name is not globally bound or is a global constant. */
static void
jitterlisp_global_setb (jitterlisp_object name, jitterlisp_object new_value)
{
  struct jitterlisp_symbol *unencoded_name = JITTERLISP_SYMBOL_DECODE(name);
  if (JITTERLISP_IS_UNDEFINED(unencoded_name->global_value))
    {
      /* There is no global binding for the symbol.  It's forbidden to set!
         a global which has not been define'd before. */
      jitterlisp_print_error_char_star ("About ");
      jitterlisp_print_error (name);
      jitterlisp_print_error_char_star ("\n"); // FIXME: add to the error message
      jitterlisp_error_cloned ("set! on unbound variable");
    }

  /* If there is a global binding to replace we can proceed as if this were a
     define form.  Notice that this may still fail, if the symbol is globally
     bound to a constant. */
  jitterlisp_define (name, new_value);
}

void
jitterlisp_environment_setb (jitterlisp_object env, jitterlisp_object name,
                             jitterlisp_object new_value)
{
  /* First look for a binding in the local environment, which is to say look
     for the first cons in env whose car is equal-by-identity to name... */
  jitterlisp_object env_rest;
  for (env_rest = env;
       env_rest != JITTERLISP_EMPTY_LIST;
       env_rest = JITTERLISP_EXP_C_A_CDR(env_rest))
    {
      jitterlisp_object useless __attribute__ ((unused));
      jitterlisp_object next_cons = JITTERLISP_EXP_C_A_CAR(env_rest);
      jitterlisp_object next_name = JITTERLISP_EXP_C_A_CAR(next_cons);
      if (next_name == name)
        {
          jitterlisp_object cdr = JITTERLISP_EXP_C_A_CDR(next_cons);
          JITTERLISP_BOX_SETB_(useless, cdr, new_value);
          return;
        }
    }

  /* ...The symbol is not bound in the given local environment.  Change its
     global binding (failing if there is no previous binding, or the binding is
     globally constant). */
  jitterlisp_global_setb (name, new_value);
}
