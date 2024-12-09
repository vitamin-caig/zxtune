#ifndef BPSTEP_H
#define BPSTEP_H

#include "../core/AmigaStep.h"

struct BPStep {
	struct AmigaStep super;
	int soundTranspose;
};

void BPStep_defaults(struct BPStep* self);
void BPStep_ctor(struct BPStep* self);
struct BPStep* BPStep_new(void);

#endif
