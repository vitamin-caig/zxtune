#ifndef AMIGACHANNEL_H
#define AMIGACHANNEL_H

#include "../flod.h"

/*
inheritance
object
	-> AmigaChannel
*/
struct AmigaChannel {
	struct AmigaChannel *next;
	int mute;
	Number panning;
	int delay;
	int pointer;
	int length;
	//internal
	int audena;
	int audcnt;
	int audloc;
	int audper;
	int audvol;
	Number timer;
	Number level;
	Number ldata;
	Number rdata;
};

void AmigaChannel_defaults(struct AmigaChannel* self);
void AmigaChannel_ctor(struct AmigaChannel* self, int index);
struct AmigaChannel* AmigaChannel_new(int index);

int AmigaChannel_get_enabled(struct AmigaChannel* self);
void AmigaChannel_set_enabled(struct AmigaChannel* self, int value);
void AmigaChannel_set_period(struct AmigaChannel* self, int value);
void AmigaChannel_set_volume(struct AmigaChannel* self, int value);
void AmigaChannel_resetData(struct AmigaChannel* self);
void AmigaChannel_initialize(struct AmigaChannel* self);

#pragma RcB2 DEP "AmigaChannel.c"

#endif
 
