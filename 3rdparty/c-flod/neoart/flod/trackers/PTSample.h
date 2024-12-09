#ifndef PTSAMPLE_H
#define PTSAMPLE_H

#include "../core/AmigaSample.h"

/*
inheritance
object
      -> AmigaSample
		->PTSample
*/
struct PTSample {
	struct AmigaSample super;
	int finetune;
	int realLen;
};

void PTSample_defaults(struct PTSample* self);
void PTSample_ctor(struct PTSample* self);
struct PTSample* PTSample_new(void);

#endif
