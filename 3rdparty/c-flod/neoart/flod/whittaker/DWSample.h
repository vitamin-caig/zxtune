#ifndef DWSAMPLE_H
#define DWSAMPLE_H

#include "../core/AmigaSample.h"

/* 

inheritance:

object
      -> AmigaSample
                     -> DWSample
*/
struct DWSample {
	struct AmigaSample super;
	int relative;
	int finetune;
};

void DWSample_defaults(struct DWSample* self);

void DWSample_ctor(struct DWSample* self);

struct DWSample* DWSample_new(void);

#pragma RcB2 DEP "DWSample.c"

#endif
