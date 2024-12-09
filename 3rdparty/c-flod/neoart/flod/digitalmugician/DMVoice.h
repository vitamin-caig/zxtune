#ifndef DMVOICE_H
#define DMVOICE_H

#include "../core/AmigaChannel.h"
#include "../core/AmigaStep.h"
#include "DMSample.h"

/*
inheritance
object
	-> DMVoice
*/
struct DMVoice {
	struct AmigaChannel *channel;
	struct DMSample *sample;
	struct AmigaStep *step;
	int note;
	int period;
	int val1;
	int val2;
	int finalPeriod;
	int arpeggioStep;
	int effectCtr;
	int pitch;
	int pitchCtr;
	int pitchStep;
	int portamento;
	int volume;
	int volumeCtr;
	int volumeStep;
	int mixMute;
	int mixPtr;
	int mixEnd;
	int mixSpeed;
	int mixStep;
	int mixVolume;
};

void DMVoice_defaults(struct DMVoice* self);
void DMVoice_ctor(struct DMVoice* self);
struct DMVoice* DMVoice_new(void);

void DMVoice_initialize(struct DMVoice *self);

#endif
