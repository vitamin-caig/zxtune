#ifndef D2VOICE_H
#define D2VOICE_H

#include "../core/AmigaChannel.h"
#include "../core/AmigaStep.h"
#include "../core/AmigaRow.h"
#include "D2Sample.h"

struct D2Voice {
	struct D2Voice *next;
	struct AmigaChannel *channel;
	struct D2Sample *sample;
	struct AmigaStep *step;
	struct AmigaRow *row;
	int index;
	int trackPtr;
	int trackPos;
	int trackLen;
	int patternPos;
	int restart;
	int note;
	int period;
	int finalPeriod;
	int arpeggioPtr;
	int arpeggioPos;
	int pitchBend;
	int portamento;
	int tableCtr;
	int tablePos;
	int vibratoCtr;
	int vibratoDir;
	int vibratoPos;
	int vibratoPeriod;
	int vibratoSustain;
	int volume;
	int volumeMax;
	int volumePos;
	int volumeSustain;
};

void D2Voice_defaults(struct D2Voice* self) ;
void D2Voice_ctor(struct D2Voice* self, int index);
struct D2Voice* D2Voice_new(int index);

void D2Voice_initialize(struct D2Voice* self);

#endif
