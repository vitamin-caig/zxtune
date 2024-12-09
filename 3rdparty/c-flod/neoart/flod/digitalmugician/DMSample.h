#ifndef DMSAMPLE_H
#define DMSAMPLE_H

#include "../core/AmigaSample.h"

/*
inheritance
object
	-> AmigaSample
		-> DMSample
*/
struct DMSample {
	struct AmigaSample super;
	int wave;
	int waveLen;
	int finetune;
	int arpeggio;
	int pitch;
	int pitchDelay;
	int pitchLoop;
	int pitchSpeed;
	int effect;
	int effectDone;
	int effectStep;
	int effectSpeed;
	int source1;
	int source2;
	int volumeLoop;
	int volumeSpeed;
	char sample_name[16];
};

void DMSample_defaults(struct DMSample* self);
void DMSample_ctor(struct DMSample* self);
struct DMSample* DMSample_new(void);

#endif
