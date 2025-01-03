/* pvm-program.h - Internal header for PVM programs.  */

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

#ifndef PVM_PROGRAM_H
#define PVM_PROGRAM_H

#include "pvm-program-point.h"
#include "pvm-vm.h"

/* Initialize and finalize the pvm-program subsystem.  */

void pvm_program_init (void);
void pvm_program_fini (void);

/* Return the program point corresponding to the beginning of the
   given program.  */
pvm_program_program_point pvm_program_beginning (pvm_program program);

/* Get the jitter routine associated with the program PROGRAM.  */
pvm_routine pvm_program_routine (pvm_program program);

/* Expand the given PVM assembler template to a form that is
   acceptable for pvm_program_parse_from_string.

   XXX handle parse errors.  */

char *pvm_program_expand_asm_template (const char *str);

/* Parse PVM instructions from the given string and append them to
   the given program.

   If there is a parse error, the function returns an allocated string
   with the name of the offending token.  The caller is expected to
   free the memory occupied by that string.  In absence of errors, this
   function returns NULL.  */

char *pvm_program_parse_from_string (const char *str, pvm_program program);

#endif /* ! PVM_PROGRAM_H */
