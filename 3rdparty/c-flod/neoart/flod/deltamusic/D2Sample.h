#ifndef D2SAMPLE_H
#define D2SAMPLE_H
#include "../core/AmigaSample.h"

#define D2SAMPLE_MAX_TABLE 48
#define D2SAMPLE_MAX_VIBRATOS 15
#define D2SAMPLE_MAX_VOLUMES 15


struct D2Sample {
	struct AmigaSample super;
	int table[D2SAMPLE_MAX_TABLE];
	int vibratos[D2SAMPLE_MAX_VIBRATOS];
	int volumes[D2SAMPLE_MAX_VOLUMES];
	int index;
	int pitchBend;
	int synth;
};

void D2Sample_defaults(struct D2Sample* self);
void D2Sample_ctor(struct D2Sample* self);
struct D2Sample* D2Sample_new(void);

#endif
