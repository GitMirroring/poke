/* pkt.h - Terminal utilities for libpoke.  */

/* Copyright (C) 2020, 2021, 2022, 2023, 2024, 2025, 2026 Jose E.
 * Marchesi */

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

#ifndef PKT_H
#define PKT_H

#include <config.h>

#include "libpoke.h"  /* For struct pk_term_if, pk_compiler.  */

struct pk_term_if_internal
{
  struct pk_term_if term_if;
  pk_compiler pkc;
};

extern struct pk_term_if_internal libpoke_term_if;

#define PKT_IF (&libpoke_term_if.term_if)
#define PKT_PKC (libpoke_term_if.pkc)

/* Terminal interface for Poke compiler.  */

#define pk_puts(STR) (PKT_IF)->puts_fn ((PKT_PKC), (STR))
#define pk_printf(...) (PKT_IF)->printf_fn ((PKT_PKC), __VA_ARGS__)
#define pk_term_flush() (PKT_IF)->flush_fn (PKT_PKC)
#define pk_term_indent(LVL, STEP)                                             \
  (PKT_IF)->indent_fn ((PKT_PKC), (LVL), (STEP))
#define pk_term_class(CLS) (PKT_IF)->class_fn ((PKT_PKC), (CLS))
#define pk_term_end_class(CLS) (PKT_IF)->end_class_fn ((PKT_PKC), (CLS))
#define pk_term_hyperlink(URL, ID)                                            \
  (PKT_IF)->hyperlink_fn ((PKT_PKC), (URL), ID)
#define pk_term_end_hyperlink() (PKT_IF)->end_hyperlink_fn (PKT_PKC)
#define pk_term_get_color() (PKT_IF)->get_color_fn (PKT_PKC)
#define pk_term_set_color(COLOR) (PKT_IF)->set_color_fn ((PKT_PKC), (COLOR))
#define pk_term_get_bgcolor() (PKT_IF)->get_bgcolor_fn (PKT_PKC)
#define pk_term_set_bgcolor(COLOR)                                            \
  (PKT_IF)->set_bgcolor_fn ((PKT_PKC), (COLOR))

#endif /* ! PKT_H */
