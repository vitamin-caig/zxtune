#ifndef PTVOICE_H
#define PTVOICE_H

#include "../core/AmigaChannel.h"
#include "PTSample.h"

/*
inheritance
Object
	-> PTVoice
*/
struct PTVoice {
	int index;
	struct PTVoice *next;
	struct AmigaChannel *channel;
	struct PTSample *sample;
	int enabled;
	int loopCtr;
	int loopPos;
	int step;
	int period;
	int effect;
	int param;
	int volume;
	int pointer;
	int length;
	int loopPtr;
	int repeat;
	int finetune;
	int offset;
	int portaDir;
	int portaPeriod;
	int portaSpeed;
	int glissando;
	int tremoloParam;
	int tremoloPos;
	int tremoloWave;
	int vibratoParam;
	int vibratoPos;
	int vibratoWave;
	int funkPos;
	int funkSpeed;
	int funkWave;
};

void PTVoice_defaults(struct PTVoice* self);
void PTVoice_ctor(struct PTVoice* self, int index);
struct PTVoice* PTVoice_new(int index);

void PTVoice_initialize(struct PTVoice* self);

#endif
