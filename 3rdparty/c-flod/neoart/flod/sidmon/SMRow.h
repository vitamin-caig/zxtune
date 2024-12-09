#ifndef SMROW_H
#define SMROW_H

#include "../core/AmigaRow.h"

struct SMRow {
	struct AmigaRow super;
	signed char speed;
};

void SMRow_defaults(struct SMRow* self);
void SMRow_ctor(struct SMRow* self);
struct SMRow* SMRow_new(void);

#endif
