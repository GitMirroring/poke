/* pk-cmd.h - terminal related stuff.  */

/* Copyright (C) 2019, 2020, 2021, 2022, 2023, 2024 Jose E. Marchesi */

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

#ifndef PK_TERM_H
#define PK_TERM_H

#include <config.h>

#include <textstyle.h>

/* From libpoke.h.  */
typedef struct _pk_compiler *pk_compiler;

/* Defined in poke.c.  */
extern pk_compiler poke_compiler;

/* Initialize and finalize the terminal subsystem.  */
void pk_term_init (int argc, char *argv[]);
void pk_term_shutdown (void);

/* Return 1 if the terminal supports colors/hyperlinks.  Return 0
   otherwise.  */
extern int pk_term_color_p (void);

/* Flush the terminal output.  */
extern void pk_term_flush_1 (pk_compiler pkc);
#define pk_term_flush() pk_term_flush_1 (poke_compiler)

/* Print a string to the terminal.  */
extern void pk_puts_1 (pk_compiler pkc, const char *str);
#define pk_puts(STR) pk_puts_1 (poke_compiler, (STR))

/* Print a formatted string to the terminal.  */
extern void pk_printf_1 (pk_compiler pkc, const char *format, ...)
  __attribute__ ((format (printf, 2, 3)));
extern void pk_vprintf (const char *format, va_list ap);
#define pk_printf(...) pk_printf_1 (poke_compiler, __VA_ARGS__)

/* Print indentation.  */
extern void pk_term_indent_1 (pk_compiler pkc, unsigned int lvl,
                              unsigned int step);
#define pk_term_indent(LVL, STEP) pk_term_indent_1 (poke_compiler, (LVL), (STEP))

/* Class handling.  */
extern void pk_term_class_1 (pk_compiler pkc, const char *class);
extern int pk_term_end_class_1 (pk_compiler pkc, const char *class);
#define pk_term_class(CLS) pk_term_class_1 (poke_compiler, (CLS))
#define pk_term_end_class(CLS) pk_term_end_class_1 (poke_compiler, (CLS))

/* Hyperlinks.  */
extern void pk_term_hyperlink_1 (pk_compiler pkc, const char *url, const char *id);
extern int pk_term_end_hyperlink_1 (pk_compiler pkc);
#define pk_term_hyperlink(URL, ID) pk_term_hyperlink_1 (poke_compiler, (URL), (ID))
#define pk_term_end_hyperlink() pk_term_end_hyperlink_1 (poke_compiler)

/* Color handling.  */
extern struct pk_color pk_term_get_color_1 (pk_compiler pkc);
extern struct pk_color pk_term_get_bgcolor_1 (pk_compiler pkc);
extern void pk_term_set_color_1 (pk_compiler pkc, struct pk_color color);
extern void pk_term_set_bgcolor_1 (pk_compiler pkc, struct pk_color color);
#define pk_term_get_color() pk_term_get_color_1 (poke_compiler)
#define pk_term_get_bgcolor() pk_term_get_bgcolor_1 (poke_compiler)
#define pk_term_set_color(COLOR) pk_term_set_color_1 (COLOR)
#define pk_term_set_bgcolor(COLOR) pk_term_set_bgcolor_1 (COLOR)

/* Paging.  */
extern void pk_term_start_pager (void);
extern void pk_term_stop_pager (void);

#endif /* PK_TERM_H */
