#ifndef JHVOICE_H
#define JHVOICE_H

#include "../core/AmigaChannel.h"

struct JHVoice {
	struct JHVoice *next;
	struct AmigaChannel *channel;
	int index;
	int enabled;
	int cosoCounter;
	int cosoSpeed;
	int trackPtr;
	int trackPos;
	int trackTransp;
	int patternPtr;
	int patternPos;
	int frqseqPtr;
	int frqseqPos;
	int volseqPtr;
	int volseqPos;
	int sample;
	int loopPtr;
	int repeat;
	int tick;
	int note;
	int transpose;
	int info;
	int infoPrev;
	int volume;
	int volCounter;
	int volSpeed;
	int volSustain;
	int volTransp;
	int volFade;
	int portaDelta;
	int vibrato;
	int vibDelay;
	int vibDelta;
	int vibDepth;
	int vibSpeed;
	int slide;
	int sldActive;
	int sldDone;
	int sldCounter;
	int sldSpeed;
	int sldDelta;
	int sldPointer;
	int sldLen;
	int sldEnd;
	int sldLoopPtr;
};

void JHVoice_defaults(struct JHVoice* self);
void JHVoice_ctor(struct JHVoice* self, int index);
struct JHVoice* JHVoice_new(int index);

void JHVoice_initialize(struct JHVoice* self);

#endif
