#ifndef AMIGAPLAYER_H
#define AMIGAPLAYER_H

#include "CorePlayer.h"
#include "Amiga.h"

/*
inheritance
??
	->CorePlayer
		->AmigaPlayer
*/
struct AmigaPlayer {
	struct CorePlayer super;
	struct Amiga *amiga;
	//protected
	int standard;
};

void AmigaPlayer_defaults(struct AmigaPlayer* self);
void AmigaPlayer_ctor(struct AmigaPlayer* self, struct Amiga* amiga);
struct AmigaPlayer* AmigaPlayer_new(struct Amiga* amiga);
void AmigaPlayer_set_ntsc(struct AmigaPlayer* self, int value);
void AmigaPlayer_set_stereo(struct AmigaPlayer* self, Number value);
void AmigaPlayer_set_volume(struct AmigaPlayer* self, Number value);
void AmigaPlayer_toggle(struct AmigaPlayer* self, int index);

#pragma RcB2 DEP "AmigaPlayer.c"

#endif
