#ifndef S1SAMPLE_H
#define S1SAMPLE_H

#include "../core/AmigaSample.h"

//fixed
#define S1SAMPLE_MAX_ARPEGGIOS 16

struct S1Sample {
	struct AmigaSample super;
	int waveform;
	//arpeggio     : Vector.<int>,
	int attackSpeed;
	int attackMax;
	int decaySpeed;
	int decayMin;
	int sustain;
	int releaseSpeed;
	int releaseMin;
	int phaseShift;
	int phaseSpeed;
	int finetune;
	int pitchFall;
	int arpeggio[S1SAMPLE_MAX_ARPEGGIOS];
	char sample_name[20];
};

void S1Sample_defaults(struct S1Sample* self);
void S1Sample_ctor(struct S1Sample* self);
struct S1Sample* S1Sample_new(void);

#endif
