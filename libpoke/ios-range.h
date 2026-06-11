/* ios-range.h - IO mapped value range tracking for poke.  */

/* Copyright (C) 2026 Free Software Foundation, Inc.  */

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

#ifndef IOS_RANGE_H
#define IOS_RANGE_H

#include "ios.h"
#include "pvm-val.h"

/* See definition in ios-range.c.  */
struct ios_rangetbl;

/* Allocate and initialize a new rangetbl.  */
struct ios_rangetbl *ios_rangetbl_create (void);

/* Empty and free a rangetbl.  */
void ios_rangetbl_destroy (struct ios_rangetbl *);


/* Track VAL in TBL with interval [BEGIN, END].  */
int  ios_rangetbl_insert (struct ios_rangetbl *tbl, pvm_val val,
			  ios_off begin, ios_off end);

/* Remove VAL from TBL, given that it is mapped at offset OFFS.  */
void ios_rangetbl_remove (struct ios_rangetbl *tbl, pvm_val val,
			  ios_off offs);

/* Mark any values stored in the table which overlap the interval
   [BEGIN,END] as dirty.  */
void ios_rangetbl_dirty (struct ios_rangetbl *, ios_off begin, ios_off end);

/* Mark all values stored in the table as dirty.
   For example, if the ios changes endianness, all currently-mapped
   values must be marked dirty and remapped.  */
void ios_rangetbl_dirty_all (struct ios_rangetbl *);

/* Return the number of entries currently in the given table.  */
size_t ios_rangetbl_nentries (struct ios_rangetbl *);

/* Notify the range table that the corresponding ios is being closed,
   so that values tracked in that table will not attempt to deregister
   themselves from a closed ios when they get GC'd.  */
void ios_rangetbl_notify_close (struct ios_rangetbl *);

#endif /* ! IOS_RANGE_H */
