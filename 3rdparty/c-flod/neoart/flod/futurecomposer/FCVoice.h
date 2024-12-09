#ifndef FCVOICE_H
#define FCVOICE_H

#include "../core/AmigaChannel.h"
#include "../core/AmigaSample.h"

/*
inheritance:
object
	-> FCVoice
*/
struct FCVoice {
	int index;
	struct FCVoice *next;
	struct AmigaChannel *channel;
	struct AmigaSample *sample;
	int enabled;
	int pattern;
	int soundTranspose;
	int transpose;
	int patStep;
	int frqStep;
	int frqPos;
	int frqSustain;
	int frqTranspose;
	int volStep;
	int volPos;
	int volCtr;
	int volSpeed;
	int volSustain;
	int note;
	int pitch;
	int volume;
	int pitchBendFlag;
	int pitchBendSpeed;
	int pitchBendTime;
	int portamentoFlag;
	int portamento;
	int volBendFlag;
	int volBendSpeed;
	int volBendTime;
	int vibratoFlag;
	int vibratoSpeed;
	int vibratoDepth;
	int vibratoDelay;
	int vibrato;
};

void FCVoice_defaults(struct FCVoice* self);
void FCVoice_ctor(struct FCVoice* self, int index);
struct FCVoice* FCVoice_new(int index);

void FCVoice_initialize(struct FCVoice* self);
void FCVoice_volumeBend(struct FCVoice* self);

#endif
