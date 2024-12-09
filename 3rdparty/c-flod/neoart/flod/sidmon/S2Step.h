#ifndef S2STEP_H
#define S2STEP_H

#include "../core/AmigaStep.h"

struct S2Step {
	struct AmigaStep super;
	signed char soundTranspose;
};

void S2Step_defaults(struct S2Step* self);
void S2Step_ctor(struct S2Step* self);
struct S2Step* S2Step_new(void);

#endif
