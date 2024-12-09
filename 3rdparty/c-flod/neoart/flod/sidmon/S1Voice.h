#ifndef S1VOICE_H
#define S1VOICE_H

#include "../core/AmigaChannel.h"

struct S1Voice {
	struct S1Voice *next;
	struct AmigaChannel *channel;
	int index;
	int step;
	int row;
	int sample;
	int samplePtr;
	int sampleLen;
	int note;
	int noteTimer;
	int period;
	int volume;
	int bendTo;
	int bendSpeed;
	int arpeggioCtr;
	int envelopeCtr;
	int pitchCtr;
	int pitchFallCtr;
	int sustainCtr;
	int phaseTimer;
	int phaseSpeed;
	int wavePos;
	int waveList;
	int waveTimer;
	int waitCtr;
};

void S1Voice_defaults(struct S1Voice* self);
void S1Voice_ctor(struct S1Voice* self, int index);
struct S1Voice* S1Voice_new(int index);

void S1Voice_initialize(struct S1Voice* self);

#endif
