#ifndef FXVOICE_H
#define FXVOICE_H

#include "../core/AmigaChannel.h"
#include "../core/AmigaSample.h"

struct FXVoice {
	int index;
	struct FXVoice *next;
	struct AmigaChannel *channel;
	struct AmigaSample *sample;
	int enabled;
	int period;
	int effect;
	int param;
	int volume;
	int last;
	int slideCtr;
	int slideDir;
	int slideParam;
	int slidePeriod;
	int slideSpeed;
	int stepPeriod;
	int stepSpeed;
	int stepWanted;
};

void FXVoice_defaults(struct FXVoice* self);
void FXVoice_ctor(struct FXVoice* self, int index);
struct FXVoice* FXVoice_new(int index);

void FXVoice_initialize(struct FXVoice* self);


#endif
