#ifndef D1SAMPLE_H
#define D1SAMPLE_H
#include "../core/AmigaSample.h"

//fixed
#define D1SAMPLE_MAX_ARPEGGIO 8
#define D1SAMPLE_MAX_TABLE 48

struct D1Sample {
	struct AmigaSample super;
	int arpeggio[D1SAMPLE_MAX_ARPEGGIO];
	int table[D1SAMPLE_MAX_TABLE];
	int synth;
	int attackStep;
	int attackDelay;
	int decayStep;
	int decayDelay;
	int releaseStep;
	int releaseDelay;
	int sustain;
	int pitchBend;
	int portamento;
	int tableDelay;
	int vibratoWait;
	int vibratoStep;
	int vibratoLen;
};

void D1Sample_defaults(struct D1Sample* self);
void D1Sample_ctor(struct D1Sample* self);
struct D1Sample* D1Sample_new(void);

#endif
