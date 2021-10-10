/* Jitter: data locations.

   Copyright (C) 2019, 2020, 2021 Luca Saiu
   Written by Luca Saiu

   This file is part of Jitter.

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


#include <stdlib.h>
#include <string.h>

#include <jitter/jitter-data-locations.h>
#include <jitter/jitter-malloc.h>
#include <jitter/jitter-fatal.h>
#include <jitter/jitter-specialize.h> /* For special specialized instructions. */


/* Low-level debugging features relying on assembly: data locations.
 * ************************************************************************** */

/* Given a location as a string, return non-false iff the location represents a
   register. */
static bool
jitter_data_location_is_register (const char *location)
{
  /* Take some characters which can only occur within memory operands in
     assembly notation.  This default covers every architecture I know of, but
     if some architecture requires something different it suffices to define the
     macro JITTER_MEMORY_OPERAND_DISTINGUISHING_CHARACTERS in the appropriate
     machine.h file to support it. */
  char *memory_only_characters
#ifdef JITTER_MEMORY_OPERAND_DISTINGUISHING_CHARACTERS
    = JITTER_MEMORY_OPERAND_DISTINGUISHING_CHARACTERS
#else
    = "[(@";
#endif
  size_t memory_only_characters_no = strlen (memory_only_characters);

  /* Check every memory-only character.  If the string contains any of them,
     then it represents memory.  If it contains none of them, it represents a
     register. */
  int i;
  for (i = 0; i < memory_only_characters_no; i ++)
    if (strchr (location, memory_only_characters [i]) != NULL)
      return false;
  return true;
}

struct jitter_data_locations *
jitter_make_data_locations (const struct jitter_vm *vm)
{
  const char *s;
  size_t string_length;
  size_t string_no = 0;
#ifndef JITTER_DISPATCH_SWITCH
  s = vm->data_locations;

  /* In case data location information is not available, use an empty string
     in its place: */
  if (s == NULL)
    s = "";

  /* First pass: find how many entries there are, by counting non-empty strings
     up to the final empty string used as a terminator.  They must come in an
     even number, since each entry contains one name and one location. */
  while ((string_length = strlen (s)) != 0)
    {
      s += string_length + 1;
      string_no ++;
    }
  if (string_no % 2 != 0)
    jitter_fatal ("impossible: data locations are odd in number");
#endif // #ifndef JITTER_DISPATCH_SWITCH
  size_t entry_no = string_no / 2;

  /* Allocate the result.  The actual strings point to memory from the constant
     bytes emitted by inline assembly, so there is no need to allocate them
     dynamically. */
  struct jitter_data_locations *res
    = jitter_xmalloc (sizeof (struct jitter_data_locations));
  res->data_locations
    = jitter_xmalloc (entry_no * sizeof (struct jitter_data_location));
  res->data_location_no = entry_no;

  /* Second pass: fill entries in the result array. */
  bool name = true;
  struct jitter_data_location *location = res->data_locations;
#ifndef JITTER_DISPATCH_SWITCH
  s = vm->data_locations;
#else // switch dispatch
  s = ""; /* End immediately. */
#endif // #ifndef JITTER_DISPATCH_SWITCH
  /* Again, provide a fallback solution in case information is missing. */
  if (s == NULL)
    s = "";
  while ((string_length = strlen (s)) != 0)
    {
      if (name)
        location->name = s;
      else
        {
          location->location = s;
          location->register_
            = jitter_data_location_is_register (location->location);
          location ++;
        }
      s += string_length + 1;
      string_no ++;
      name = ! name;
    }

  /* The result is reliable as long as the !DATALOCATIONS special specialized
     instruction has the same size as the !NOP special specialized instruction.
     In other words, there must be no loads or moves in the compiled code for
     !DATALOCATIONS ... */
  res->reliable
#ifndef JITTER_DISPATCH_SWITCH
    = (vm->thread_sizes [jitter_specialized_instruction_opcode_DATALOCATIONS]
       == vm->thread_sizes [jitter_specialized_instruction_opcode_NOP]);
  /* ...Except that, of course, if all the information was missing in the first
     place then we have nothing useful to show. */
  if (vm->data_locations == NULL)
    res->reliable = false;
#else  // switch dispatch
    = true;
#endif // #ifndef JITTER_DISPATCH_SWITCH


  /* Done. */
  return res;
}

void
jitter_destroy_data_locations (struct jitter_data_locations *locations)
{
  free (locations->data_locations);
  free (locations);
}




/* Data locations: human-readable output.
 * ************************************************************************** */

/* Begin using a class in the given print context, where the class name is
   formed by the concatenation of the lower-case prefix for the VM of the
   pointed executable routine, concatenated to a dash, concatenated
   to the given suffix.
   For example, if the mutable routine r belonged to a VM named "foo",
     jitter_disassemble_begin_class (ctx, r, "label")
   would open a class in the context ctx named "foo-label". */
__attribute__ ((unused))
static void
jitter_locations_begin_class (jitter_print_context ctx,
                              const struct jitter_vm *vm,
                              const char *suffix)
{
  char *prefix = vm->configuration->lower_case_prefix;
  size_t size = strlen (prefix) + 1 + strlen (suffix) + 1;
  char *buffer = jitter_xmalloc (size);
  sprintf (buffer, "%s-%s", prefix, suffix);
  jitter_print_begin_class (ctx, buffer);
  free (buffer);
}

void
jitter_dump_data_locations (jitter_print_context out,
                            const struct jitter_vm *vm)
{
  struct jitter_data_locations *locations = jitter_make_data_locations (vm);
  if (! locations->reliable)
    {
      jitter_print_char_star (out, "The following information is unreliable: at least\n");
      jitter_print_char_star (out, "one datum needs more than one load instruction to be\n");
      jitter_print_char_star (out, "accessed.\n");
      if (JITTER_ARCHITECTURE_IS ("sh"))
        {
          jitter_print_char_star (out, "This might happen, on SH, because of the\n");
          jitter_print_char_star (out, "restricted load offset ranges.\n");
        }
    }
  int i;
  size_t register_no = 0;
  for (i = 0; i < locations->data_location_no; i ++)
    {
      char s [1000];
      sprintf (s, "%2i. %24s: %-12s (%s)\n",
               i,
               locations->data_locations [i].name,
               locations->data_locations [i].location,
               locations->data_locations [i].register_ ? "register" : "memory");
      jitter_print_char_star (out, s);
      if (locations->data_locations [i].register_)
        register_no ++;
    }
  if (locations->data_location_no > 0)
    {
      int register_percentage
        = (register_no * 100) / locations->data_location_no;
      jitter_print_char_star (out, "Register ratio: ");
      jitter_print_int (out, 10, register_percentage);
      jitter_print_char_star (out, "%\n");
    }
  else
    jitter_print_char_star (out, "Register ratio: undefined\n");
  jitter_destroy_data_locations (locations);
}
