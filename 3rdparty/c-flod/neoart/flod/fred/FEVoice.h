#ifndef FEVOICE_H
#define FEVOICE_H

#include "../core/AmigaChannel.h"
#include "FESample.h"

struct FEVoice {
	//next          : FEVoice,
	struct FEVoice *next;
	//channel       : AmigaChannel,
	struct AmigaChannel *channel;
	//sample        : FESample,
	struct FESample *sample;

	int index;
	int bitFlag;
	int trackPos;
	int patternPos;
	int tick;
	int busy;
	int synth;
	int note;
	int period;
	int volume;
	int envelopePos;
	int sustainTime;
	int arpeggioPos;
	int arpeggioSpeed;
	int portamento;
	int portaCounter;
	int portaDelay;
	int portaFlag;
	int portaLimit;
	int portaNote;
	int portaPeriod;
	int portaSpeed;
	int vibrato;
	int vibratoDelay;
	int vibratoDepth;
	int vibratoFlag;
	int vibratoSpeed;
	int pulseCounter;
	int pulseDelay;
	int pulseDir;
	int pulsePos;
	int pulseSpeed;
	int blendCounter;
	int blendDelay;
	int blendDir;
	int blendPos;
};

void FEVoice_defaults(struct FEVoice* self);
void FEVoice_ctor(struct FEVoice* self, int index, int bitFlag);
struct FEVoice* FEVoice_new(int index, int bitFlag);

void FEVoice_initialize(struct FEVoice* self);

#endif
