#ifndef STVOICE_H
#define STVOICE_H

#include "../core/AmigaChannel.h"
#include "../core/AmigaSample.h"

/*
inheritance
object
	-> STVoice
*/
struct STVoice {
	int index;
	struct STVoice *next;
	struct AmigaChannel *channel;
	struct AmigaSample *sample;
	int enabled;
	int period;
	int last;
	int effect;
	int param;
};

void STVoice_defaults(struct STVoice* self);
void STVoice_ctor(struct STVoice* self, int index);
struct STVoice* STVoice_new(int index);

void STVoice_initialize(struct STVoice* self);

#endif
