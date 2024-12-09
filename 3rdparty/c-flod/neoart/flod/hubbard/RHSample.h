#ifndef RHSAMPLE_H
#define RHSAMPLE_H

#include "../core/AmigaSample.h"

#define RHSAMPLE_MAX_WAVE 16

struct RHSample {
	struct AmigaSample super;
	unsigned short relative;
	unsigned short divider;
	unsigned short vibrato;
	unsigned short hiPos;
	unsigned short loPos;
	//wave     : Vector.<int>;
	int wave[RHSAMPLE_MAX_WAVE];
	int got_wave:1;
};

void RHSample_defaults(struct RHSample* self);
void RHSample_ctor(struct RHSample* self);
struct RHSample* RHSample_new(void);

#endif
