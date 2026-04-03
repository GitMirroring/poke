#include "ios.h"

struct ios_rangetbl;

/* Allocate and initialize a new rangetbl.  */
struct ios_rangetbl *ios_rangetbl_create (void);

/* Empty and free a rangetbl.  */
void ios_rangetbl_destroy (struct ios_rangetbl *tbl);


/* Insert pvm_val VAL into TBL with interval [begin, end].  */
int  ios_rangetbl_insert (struct ios_rangetbl *tbl, uint64_t val,
			  ios_off begin, ios_off end);

/* Remove VAL mapped at OFFS from TBL.  */
void ios_rangetbl_remove (struct ios_rangetbl *tbl, uint64_t val,
			  ios_off offs);

void ios_rangetbl_dirty (struct ios_rangetbl *, ios_off begin, ios_off end);
void ios_rangetbl_dirty_all (struct ios_rangetbl *);

size_t ios_rangetbl_nentries (struct ios_rangetbl *);

void ios_rangetbl_notify_close (struct ios_rangetbl *);

void ios_rangetbl_debug (struct ios_rangetbl *);
