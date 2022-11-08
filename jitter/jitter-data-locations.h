/* Jitter: data locations: header.

   Copyright (C) 2019, 2020, 2021 Luca Saiu
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


#ifndef JITTER_DATA_LOCATIONS_H_
#define JITTER_DATA_LOCATIONS_H_

#include <stdio.h>
#include <stdbool.h>

#include <jitter/jitter.h>
#if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
# include <jitter/jitter-sections.h>
#endif // #if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
#include <jitter/jitter-print.h>
#include <jitter/jitter-vm.h>


/* Low-level debugging features relying on assembly: data locations.
 * ************************************************************************** */

/* Information about where a certain datum is held at run time in the
   executor. */
struct jitter_data_location
{
  /* The datum name. */
  const char *name;

  /* The datum location, as text, in assembly notation as emitted by GCC. */
  const char *location;

  /* Non-false iff the datum is held in a register. */
  bool register_;
};

/* An array of data locations, with a field holding its size. */
struct jitter_data_locations
{
  /* An array of data, as elements of the struct above. */
  struct jitter_data_location *data_locations;

  /* How many data there are. */
  size_t data_location_no;

  /* Non-false iff the information contained in data_locations is known to be
     reliable.  The information may not be reliable if some datum is not
     accessible from a register or as a single memory operand, and instead
     requires additional loads.  That would be a symptom of some problem. */
  bool reliable;
};

/* Given a pointer to the VM data structure, return a pointer to a freshly
   allocated struct jitter_data_location_data object.  This function is used by
   machine-generated code. */
struct jitter_data_locations *
jitter_make_data_locations (const struct jitter_vm *vm)
  __attribute__ ((nonnull (1), returns_nonnull));

/* Destroy the data structure allocated from the previous function. */
void
jitter_destroy_data_locations (struct jitter_data_locations *locations)
  __attribute__ ((nonnull (1)));




/* Data locations: human-readable output.
 * ************************************************************************** */

/* Output a human-readable message about data locations for the pointed VM to
   the given print context. */
void
jitter_dump_data_locations (jitter_print_context ctx, const struct jitter_vm *vm)
  __attribute__ ((nonnull (1, 2)));




/* Data location macros.
 * ************************************************************************** */

/* The following macros provide a way of emitting information about specific
   data, as registers or register-based memory locations, *as strings* in a
   separate subsection, all as part of the definition of a global symbol visible
   from C as a global variable.  This can be useful to read from memory and
   print out to help the user follow VM instruction disassemblies.

   This functionality is used by machine-generated code. */

/* This functionality is not actually available if the host binary format is not
   supported.  In that case, define compatibility stubs. */
#if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
  /* Data locations are actually supported. */

  /* The name of the global variable holding data prefixes. */
# define JITTER_DATA_LOCATION_NAME(_jitter_vm_the_prefix)  \
    JITTER_CONCATENATE_TWO(_jitter_vm_the_prefix, _data_locations)

  /* The subsection number for data locations. */
# define JITTER_ASM_DATA_LOCATION_SUBSECTION  \
    "12"

  /* Begin the global definition for data locations for the VM with the given
     vmprefix.  This temporarily enters the appropriate subsection, emits a symbol
     definition, and pops back to the previous subsection.  Between a call to this
     macro and a call to JITTER_DATA_LOCATION_FOOTER the user is supposed to emit
     location data for every datum, in some predictable order.  The defined global
     is only one. */
# define JITTER_DATA_LOCATION_HEADER(_jitter_vm_the_prefix)                \
    asm (/* Generate the identifier definition in assembly. */             \
         JITTER_ASM_OPEN_DEFINITION (JITTER_ASM_DATA_LOCATION_SUBSECTION,  \
                                     JITTER_DATA_LOCATION_NAME             \
                                        (_jitter_vm_the_prefix)))

  /* End the global definition for data locations, for the VM with the given
     vmprefix.  This enters, and then exits, the appropriate subsection. */
# define JITTER_DATA_LOCATION_FOOTER(_jitter_vm_the_prefix)                 \
    asm (/* Emit the final empty string as "\0". */                         \
         JITTER_ASM_ENTER_SUBSECTION( JITTER_ASM_DATA_LOCATION_SUBSECTION)  \
         ".byte 0\n\t"                                                      \
         JITTER_ASM_EXIT_SUBSECTION                                         \
         /* Close the identifier definition in assembly. */                 \
         JITTER_ASM_CLOSE_DEFINITION (JITTER_ASM_DATA_LOCATION_SUBSECTION,  \
                                      JITTER_DATA_LOCATION_NAME             \
                                         (_jitter_vm_the_prefix)))

  /* Emit the location for the given datum with the given name as two
     '\0'-terminated strings, in the data location subsection.
     For example, if foo is a local variable currently kept in some hardware
     register named $r10, then the macro call
       JITTER_DATA_LOCATION_DATUM("the foo variable", foo);
     will emit
       .asciz "the foo variable"
       .asciz "$r10"
     in the data location subsection.
     Notice that:
     - name_as_string is emitted as is in an extended-asm template,
       therefore any '%' character must be escaped as "%%";
     - name_as_string_literal must never be an empty string, since an
       empty string terminates the entire data structure. */
# define JITTER_DATA_LOCATION_DATUM(name_as_string_literal, datum)             \
    asm volatile (JITTER_ASM_ENTER_SUBSECTION(                                 \
                     JITTER_ASM_DATA_LOCATION_SUBSECTION)                      \
                  "\n" JITTER_ASM_COMMENT_PREFIX                               \
                       name_as_string_literal " " JITTER_STRINGIFY(datum) "\n" \
                  ".asciz \"" name_as_string_literal "\"\n"                    \
                  ".asciz \"%[datum_from_asm]\"\n\t"                           \
                  JITTER_ASM_EXIT_SUBSECTION                                   \
                  : /* outputs */                                              \
                  : [datum_from_asm] "X" (datum) /* inputs */)
#else // ! defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)
  /* Use dummy macros emitting no data locations, for compatibility. */
# define JITTER_DATA_LOCATION_HEADER(_jitter_vm_the_prefix)         /* Nothing. */
# define JITTER_DATA_LOCATION_FOOTER(_jitter_vm_the_prefix)         /* Nothing. */
# define JITTER_DATA_LOCATION_DATUM(name_as_string_literal, datum)  /* Nothing. */
#endif // #if defined (JITTER_HAVE_KNOWN_BINARY_FORMAT)


#endif // #ifndef JITTER_DATA_LOCATIONS_H_
