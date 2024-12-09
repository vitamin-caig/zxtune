#ifndef BPSAMPLE_H
#define BPSAMPLE_H

#include "../core/AmigaSample.h"

struct BPSample {
	struct AmigaSample super;
	int synth;
	int table;
	int adsrControl;
	int adsrTable;
	int adsrLen;
	int adsrSpeed;
	int lfoControl;
	int lfoTable;
	int lfoDepth;
	int lfoLen;
	int lfoDelay;
	int lfoSpeed;
	int egControl;
	int egTable;
	int egLen;
	int egDelay;
	int egSpeed;
	int fxControl;
	int fxDelay;
	int fxSpeed;
	int modControl;
	int modTable;
	int modLen;
	int modDelay;
	int modSpeed;
	char sample_name[24];
};

void BPSample_defaults(struct BPSample* self);
void BPSample_ctor(struct BPSample* self);
struct BPSample* BPSample_new(void);

#endif
