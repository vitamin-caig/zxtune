#ifndef PTROW_H
#define PTROW_H

#include "../core/AmigaRow.h"

/*
inheritance
Object
	-> AmigaRow
		-> PTRow
*/
struct PTRow {
	struct AmigaRow super;
	int step;
};

void PTRow_defaults(struct PTRow* self);
void PTRow_ctor(struct PTRow* self);
struct PTRow* PTRow_new(void);

#endif
