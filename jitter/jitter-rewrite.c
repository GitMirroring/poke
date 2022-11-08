/* Instruction rewrite functionality: non-generated part.

   Copyright (C) 2017 Luca Saiu
   Updated in 2019 by Luca Saiu
   Written by Luca Saiu

   This file is part of GNU Jitter.

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


#include <jitter/jitter.h>
#include <jitter/jitter-rewrite.h>
#include <jitter/jitter-vm.h>

#include <jitter/jitter-dynamic-buffer.h>
#include <jitter/jitter-fatal.h>

struct jitter_instruction*
jitter_last_instruction (struct jitter_mutable_routine *p)
{
  return * jitter_last_instructions (p, 1);
}

struct jitter_instruction**
jitter_last_instructions (struct jitter_mutable_routine *p, size_t how_many)
{
  // FIXME: probably useless here.  Make this always unsafe, as it's only called
  // from a safe plaace.
  if (p->stage != jitter_routine_stage_unspecialized)
    jitter_fatal ("jitter_last_instruction: non non-unspecialized program");
  if (p->expected_parameter_no != 0)
    jitter_fatal ("jitter_last_instruction: previous instruction incomplete");
  // FIXME: possibly too defensive?
  if (p->rewritable_instruction_no < how_many)
    jitter_fatal ("getting more last instructions (%i) "
                  "than we have as rewritable (%i)",
                  (int)how_many,
                  (int)p->rewritable_instruction_no);

  struct jitter_instruction **resp
    = ((struct jitter_instruction **)
       ((char *) jitter_dynamic_buffer_first_unused_char (& p->instructions)
        - sizeof (struct jitter_instruction*) * how_many));
  return resp;
}

struct jitter_instruction*
jitter_pop_instruction (struct jitter_mutable_routine *p)
{
  // FIXME: possibly too defensive?
  if (p->rewritable_instruction_no == 0)
    jitter_fatal ("popping an instruction when rewritable ones are zero");

  // FIXME: probably useless here.  Make this always unsafe, as it's only called
  // from a safe plaace.
  if (p->stage != jitter_routine_stage_unspecialized)
    jitter_fatal ("jitter_last_instruction: non non-unspecialized program");
  if (p->expected_parameter_no != 0)
    jitter_fatal ("jitter_last_instruction: previous instruction incomplete");

  /* There will be one fewer rewritable instruction after this returns. */
  p->rewritable_instruction_no --;

  return * ((struct jitter_instruction **)
            (jitter_dynamic_buffer_pop (& p->instructions,
                                        sizeof (struct jitter_instruction*))));
}

void
jitter_destroy_last_instructions (struct jitter_mutable_routine *p,
                                  size_t how_many)
{
  // FIXME: possibly too defensive?
  if (p->rewritable_instruction_no < how_many)
    jitter_fatal ("destroying more instruction than we have rewritable");

  /* Let lasts be a pointer to the pointer to the first instruction to destroy;
     the lasts + 1 will point to the second instruction to destroy, and so
     on. */
  struct jitter_instruction **lasts = jitter_last_instructions (p, how_many);

  /* Destroy instructions, left-to-right. */
  int i;
  for (i = 0; i < how_many; i ++)
    jitter_destroy_instruction (lasts [i]);

  /* There will be how_many fewer rewritable instructions after this returns. */
  p->rewritable_instruction_no -= how_many;

  /* Pop the pointers in one go.  The pointed heap memory, including parameters
     which were held in separate malloc'ed buffers, has been freed already. */
  jitter_dynamic_buffer_pop (& p->instructions,
                             sizeof (struct jitter_instruction*) * how_many);
}
