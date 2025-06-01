/* pvm-program.c - PVM programs.  */

/* Copyright (C) 2020, 2021, 2022, 2023, 2024, 2025 Jose E. Marchesi */

/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h> /* For stdout. */
#include <xalloc.h> /* For xstrdup.  */

#include "jitter/jitter-print.h"

#include "pk-utils.h"
#include "pkt.h"

#include "pvm.h"
#include "pvm-alloc.h"
#include "pvm-val.h"

#include "pvm-vm.h"


/* Jitter print context to use when disassembling PVM programs.  */
jitter_print_context_kind jitter_context_kind = NULL;
jitter_print_context jitter_context = NULL;

/* Jitter print context.  */

static int
pvm_jitter_print_flush (jitter_print_context_data data)
{
  pk_term_flush ();
  return 0;
}

static int
pvm_jitter_print_char (jitter_print_context_data d, char c)
{
  char str[2];

  str[0] = c;
  str[1] = '\0';
  pk_puts ((const char*) &str);
  return 0;
}

static int
pvm_jitter_begin_decoration (jitter_print_context_data d,
                             const jitter_print_decoration_name name,
                             enum jitter_print_decoration_type type,
                             const union jitter_print_decoration_value *value)
{
  if (STREQ (name, JITTER_PRINT_DECORATION_NAME_CLASS))
    pk_term_class (value->string);
  else
    pk_term_hyperlink (value->string, NULL);

  return 0;
}

static int
pvm_jitter_end_decoration (jitter_print_context_data data,
                           const jitter_print_decoration_name name,
                           enum jitter_print_decoration_type type,
                           const union jitter_print_decoration_value *value)
{
  if (STREQ (name, JITTER_PRINT_DECORATION_NAME_CLASS))
    pk_term_end_class (value->string);
  else
    pk_term_end_hyperlink ();

  return 0;
}

void
pvm_disassemble_program_nat (pvm_val program)
{
  pvm_routine routine;

  assert (PVM_IS_PRG (program));

  routine = PVM_VAL_PRG_ROUTINE (program);
  pvm_routine_disassemble (jitter_context, routine, true, JITTER_OBJDUMP,
                           NULL);
}

char *
pvm_program_expand_asm_template (const char *str)
{
  /* XXX translate str to handle immediates:
       "foo"
       u?int<N>M
       E_inval, etc.
     then pass the resulting pointer in the string.
     but beware of 32-bit: pushlo + push32.  */

  size_t expanded_size = 0, q;
  const char *p;
  char *expanded;

  /* First, calculate the size of the expanded string.  */
  for (p = str; *p != '\0'; ++p)
    {
      ++expanded_size;
    }

  /* Allocate the expanded string.  */
  expanded = xmalloc (expanded_size + 1);

  /* Now build the expanded string.  */
  for (p = str, q = 0; *p != '\0'; ++p)
    {
      assert (q < expanded_size);

      /* ; -> \n */
      if (*p == ';')
        expanded[q++] = '\n';
      else if (*p == '.')
        expanded[q++] = '$';
      else
        expanded[q++] = *p;
    }
  expanded[expanded_size] = '\0';

  return expanded;
}

char *
pvm_program_parse_from_string (const char *str, pvm_val program)
{
  struct pvm_routine_parse_error *err;

  assert (PVM_IS_PRG (program));
  err = pvm_parse_mutable_routine_from_string (str,
                                               PVM_VAL_PRG_ROUTINE (program));
  if (err != NULL)
    {
      char *invalid_token = xstrdup (err->error_token_text);
      pvm_routine_parse_error_destroy (err);
      return invalid_token;
    }

  /* No error.  */
  return NULL;
}

void
pvm_disassemble_program (pvm_val program)
{
  pvm_routine routine;

  assert (PVM_IS_PRG (program));
  routine = PVM_VAL_PRG_ROUTINE (program);
  pvm_routine_print (jitter_context, routine,
                     /*user_data*/ (void *)(uintptr_t)routine);
}

void
pvm_program_init (void)
{
  jitter_context_kind = jitter_print_context_kind_make_trivial ();

  jitter_context_kind->print_char = pvm_jitter_print_char;
  jitter_context_kind->flush = pvm_jitter_print_flush;
  jitter_context_kind->begin_decoration = pvm_jitter_begin_decoration;
  jitter_context_kind->end_decoration = pvm_jitter_end_decoration;

  jitter_context = jitter_print_context_make (jitter_context_kind,
                                              NULL);
}

void
pvm_program_fini (void)
{
  jitter_print_context_destroy (jitter_context);
  jitter_print_context_kind_destroy (jitter_context_kind);
}
