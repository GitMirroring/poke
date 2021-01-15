/* JitterLisp: s-expression implementation.

   Copyright (C) 2017, 2018, 2021 Luca Saiu
   Updated in 2019 by Luca Saiu
   Written by Luca Saiu

   This file is part of the JitterLisp language implementation, distributed as
   an example along with Jitter under the same license.

   Jitter is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Jitter is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Jitter.  If not, see <http://www.gnu.org/licenses/>. */


/* Include the Gnulib header. */
#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <jitter/jitter.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-malloc.h>

#include "jitterlisp-sexpression.h"
#include "jitterlisp-allocator.h" /* For globally named objects. */


/* Compiler sanity checks.
 * ************************************************************************** */

/* We currently rely on some behavior which is very common across C compilers
   but not mandated by the C standard.  Some of this logic should probably be
   moved to configure. */

/* Did we already perform the sanity check?  We only need to do it once. */
static bool
jitterlisp_platform_sanity_check_performed = false;

/* Perform compiler sanity checks on the C compiler and hardware and set
   jitterlisp_compiler_sanity_check_performed to true.  Fail fatally if any
   check fails. */
static void
jitterlisp_platform_sanity_check (void)
{
  /* These checks are all based on constant expressions, and a sensible C
     compiler will not generate any conditional to be executed at run time. */

  /* Check that the C implementation uses two's complement arithmetic. */
  jitter_int signed_minus_one = (jitter_int) -1;
  jitter_uint bitwise_negated_unsigned_zero = ~ (jitter_uint) 0;
  if ((jitter_uint) signed_minus_one
      != (jitter_uint) bitwise_negated_unsigned_zero)
    jitter_fatal ("this machine doesn't seem to use two's complement");

  /* Check that the C implementation sign-extends on signed >> operands. */
  if (! JITTER_RIGHT_SHIFT_SIGN_EXTENDS (jitter_uint, jitter_int))
    jitter_fatal ("this compiler doesn't sign-extend on signed >> .  "
                  "You can comment out this fatal error and everything "
                  "should still work, but performance will suffer.  "
                  "Write me if you have constructive suggestions on how "
                  "to improve this.");

  /* We've checked everything, and we can proceed.  There's no need to do this
     ever again. */
  jitterlisp_platform_sanity_check_performed = true;
}




/* S-expression initialization and finalization.
 * ************************************************************************** */

/* A forward-declaration. */
static void
jitterlisp_initialize_globally_named_objects (void);

void
jitterlisp_sexpression_initialize (void)
{
  /* Perform sanity checks, unless we've already done it before. */
  if (! jitterlisp_platform_sanity_check_performed)
    jitterlisp_platform_sanity_check ();

  jitterlisp_initialize_globally_named_objects ();
}

void
jitterlisp_sexpression_finalize (void)
{
  /* Do nothing.  There is no need to destroy each globally named object, as the
     symbol table finalization will deal with them. */
}




/* S-expression representation: unique object names.
 * ************************************************************************** */

/* This must exactly follow the order in jitterlisp-sexpression.h . */
const char * const
jitterlisp_unique_object_names []
  = {
      "#f",                             /* The unique object with index 0. */
      "#t",                             /* The unique object with index 1. */
      "()",                             /* The unique object with index 2. */
      "#<eof>",                         /* The unique object with index 3. */
      "#<nothing>",                     /* The unique object with index 4. */
      "#<undefined>",                   /* The unique object with index 5. */
      "#<uninitialized-register>",      /* The unique object with index 6. */
      "#<uninitialized-main-stack>",    /* The unique object with index 7. */
      "#<uninitialized-return-stack>",  /* The unique object with index 8. */
      "#<uninitialized-heap>"           /* The unique object with index 9. */
    };




/* Globally named objects.
 * ************************************************************************** */

/* Return an interned symbol with the given name as a tagged s-expression. */
static jitterlisp_object
jitterlisp_make_interned (const char *name)
{
  struct jitterlisp_symbol *untagged_res
    = jitterlisp_symbol_make_interned (name);
  return JITTERLISP_SYMBOL_ENCODE(untagged_res);
}

/* Globally named object variables. */
jitterlisp_object jitterlisp_else;
jitterlisp_object jitterlisp_label;
jitterlisp_object jitterlisp_low_level_macro_args;
jitterlisp_object jitterlisp_primitive_make_constantb;

/* Initialize globally named object variables. */
static void
jitterlisp_initialize_globally_named_objects (void)
{
  jitterlisp_else = jitterlisp_make_interned ("else");;
  jitterlisp_label = jitterlisp_make_interned ("label");;
  jitterlisp_low_level_macro_args
    = jitterlisp_make_interned ("low-level-macro-args");
  jitterlisp_primitive_make_constantb
    = jitterlisp_make_interned ("primitive-make-constant!");;
}
