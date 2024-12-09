#ifndef DWVOICE_H
#define DWVOICE_H

#include "../core/AmigaChannel.h"
#include "DWSample.h"

struct DWVoice {
	int index;
	int bitFlag;
	struct DWVoice* next;
	struct AmigaChannel* channel;
	struct DWSample* sample;
	int trackPtr;
	int trackPos;
	int patternPos;
	int frqseqPtr;
	int frqseqPos;
	int volseqPtr;
	int volseqPos;
	int volseqSpeed;
	int volseqCounter;
	int halve;
	int speed;
	int tick;
	int busy;
	int flags;
	int note;
	int period;
	int transpose;
	int portaDelay;
	int portaDelta;
	int portaSpeed;
	int vibrato;
	int vibratoDelta;
	int vibratoSpeed;
	int vibratoDepth;
};

void DWVoice_defaults(struct DWVoice* self);

void DWVoice_ctor(struct DWVoice* self, int index, int bitFlag);

struct DWVoice* DWVoice_new(int index, int bitFlag);

void DWVoice_initialize(struct DWVoice* self);

#pragma RcB2 DEP "DWVoice.c"

#endif
