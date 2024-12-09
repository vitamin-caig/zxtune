#ifndef F2SAMPLE_H
#define F2SAMPLE_H

#include "../core/SBSample.h"

/*
inheritance
object
	-> SBSample
		-> F2Sample
*/
struct F2Sample {
	struct SBSample super;
	int finetune;
	int panning;
	int relative;
	char name_buffer[24];
};

void F2Sample_defaults(struct F2Sample* self);
void F2Sample_ctor(struct F2Sample* self);
struct F2Sample* F2Sample_new(void);

#endif
