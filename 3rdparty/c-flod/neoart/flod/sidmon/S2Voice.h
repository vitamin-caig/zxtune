#ifndef S2VOICE_H
#define S2VOICE_H

#include "../core/AmigaChannel.h"
#include "SMRow.h"
#include "S2Step.h"
#include "S2Sample.h"
#include "S2Instrument.h"

struct S2Voice {
	struct S2Voice *next;
	struct AmigaChannel *channel;
	struct S2Step *step;
	struct SMRow *row;
	struct S2Instrument *instr;
	struct S2Sample *sample;
	int index;
	int enabled;
	int pattern;
	int instrument;
	int note;
	int period;
	int volume;
	int original;
	int adsrPos;
	int sustainCtr;
	int pitchBend;
	int pitchBendCtr;
	int noteSlideTo;
	int noteSlideSpeed;
	int waveCtr;
	int wavePos;
	int arpeggioCtr;
	int arpeggioPos;
	int vibratoCtr;
	int vibratoPos;
	int speed;
};

void S2Voice_defaults(struct S2Voice* self);
void S2Voice_ctor(struct S2Voice* self, int index);
struct S2Voice* S2Voice_new(int index);

void S2Voice_initialize(struct S2Voice* self);

#endif
