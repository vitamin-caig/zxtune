#ifndef D1VOICE_H
#define D1VOICE_H

#include "D1Sample.h"
#include "../core/AmigaRow.h"
#include "../core/AmigaStep.h"
#include "../core/AmigaChannel.h"

struct D1Voice {
	struct D1Voice *next;
	struct AmigaChannel *channel;
	struct D1Sample *sample;
	struct AmigaStep *step;
	struct AmigaRow *row;
	int index;
	int trackPos;
	int patternPos;
	int status;
	int speed;
	int note;
	int period;
	int arpeggioPos;
	int pitchBend;
	int tableCtr;
	int tablePos;
	int vibratoCtr;
	int vibratoDir;
	int vibratoPos;
	int vibratoPeriod;
	int volume;
	int attackCtr;
	int decayCtr;
	int releaseCtr;
	int sustain;
};

void D1Voice_defaults(struct D1Voice* self);
void D1Voice_ctor(struct D1Voice* self, int index);
struct D1Voice* D1Voice_new(int index);

void D1Voice_initialize(struct D1Voice* self);

#endif
