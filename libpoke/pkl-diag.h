/* pkl-diag.h - Functions to emit compiler diagnostics.  */

/* Copyright (C) 2020, 2021, 2022, 2023, 2024 Jose E. Marchesi */

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

#ifndef PKL_DIAG_H
#define PKL_DIAG_H

#include <config.h>
#include <stdarg.h>

#include "pkl-ast.h"
#include "pkl-parser.h"

void pkl_error (pkl_compiler compiler, pkl_ast ast, pkl_ast_loc loc,
                const char *fmt, ...);

void pkl_warning (pkl_compiler compiler, pkl_ast ast,
                  pkl_ast_loc loc, const char *fmt, ...);

void pkl_ice (pkl_compiler compiler, pkl_ast ast, pkl_ast_loc loc,
              const char *fmt, ...);


/* Given a PKL parser and a location, allocate and return a
   NULL-terminated string with the source code corresponding to the
   location.

   MAX_CHARS determines the maximum number of characters to allocate
   in the string.  */

char *pkl_loc_to_source (struct pkl_parser *parser, pkl_ast_loc loc,
                         size_t max_chars);

#endif /* ! PKL_DIAG_H */
