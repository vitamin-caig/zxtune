#ifndef RHVOICE_H
#define RHVOICE_H

#include "RHSample.h"
#include "../core/AmigaChannel.h"

struct RHVoice {
	struct RHVoice *next;
	struct AmigaChannel *channel;
	struct RHSample *sample;
	int index;
	int bitFlag;
	int trackPtr;
	int trackPos;
	int patternPos;
	int tick;
	int busy;
	int flags;
	int note;
	int period;
	int volume;
	int portaSpeed;
	int vibratoPtr;
	int vibratoPos;
	int synthPos;
};

void RHVoice_defaults(struct RHVoice* self);
void RHVoice_ctor(struct RHVoice* self, int index, int bitFlag);
struct RHVoice* RHVoice_new(int index, int bitFlag);

void RHVoice_initialize(struct RHVoice* self);

#endif
