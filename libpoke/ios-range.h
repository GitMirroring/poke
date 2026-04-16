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

void ios_rangetbl_debug (struct ios_rangetbl *);

#endif /* ! IOS_RANGE_H */
