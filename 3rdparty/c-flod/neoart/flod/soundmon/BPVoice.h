#ifndef BPVOICE_H
#define BPVOICE_H

#include "../core/AmigaChannel.h"

struct BPVoice {
	int index;
	struct BPVoice *next;
	struct AmigaChannel *channel;
	int enabled;
	int restart;
	int note;
	int period;
	int sample;
	int samplePtr;
	int sampleLen;
	int synth;
	int synthPtr;
	int arpeggio;
	int autoArpeggio;
	int autoSlide;
	int vibrato;
	int volume;
	int volumeDef;
	int adsrControl;
	int adsrPtr;
	int adsrCtr;
	int lfoControl;
	int lfoPtr;
	int lfoCtr;
	int egControl;
	int egPtr;
	int egCtr;
	int egValue;
	int fxControl;
	int fxCtr;
	int modControl;
	int modPtr;
	int modCtr;
};

void BPVoice_defaults(struct BPVoice* self);
void BPVoice_ctor(struct BPVoice* self, int index);
struct BPVoice* BPVoice_new(int index);

void BPVoice_initialize(struct BPVoice* self);

#endif
