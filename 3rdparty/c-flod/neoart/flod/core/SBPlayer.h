#ifndef SBPLAYER_H
#define SBPLAYER_H

#include "CorePlayer.h"
#include "Soundblaster.h"

#define SBPLAYER_MAX_TRACKS 64

/*
inheritance
??
	->CorePlayer
		->SBPlayer
*/
struct SBPlayer {
	struct CorePlayer super;
	struct Soundblaster *mixer;
	int length;
	int restart;
	//track   : Vector.<int>;
	int track[SBPLAYER_MAX_TRACKS];
	//protected 
	int timer;
	int master;
};

void SBPlayer_defaults(struct SBPlayer* self);

void SBPlayer_ctor(struct SBPlayer* self, struct Soundblaster* mixer);

struct SBPlayer* SBPlayer_new(struct Soundblaster* mixer);

void SBPlayer_set_volume(struct SBPlayer* self, Number value);
void SBPlayer_toggle(struct SBPlayer* self, int index);
void SBPlayer_setup(struct SBPlayer* self);
void SBPlayer_initialize(struct SBPlayer* self);


#endif