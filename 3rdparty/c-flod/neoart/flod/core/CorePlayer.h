#ifndef COREPLAYER_H
#define COREPLAYER_H

#include "../flod.h"
#include "CoreMixer.h"
#include "../../../flashlib/ByteArray.h"

/*
inheritance
??
	->CorePlayer
*/
struct CorePlayer {
	unsigned int min_filesize;
	int quality;
	int playSong;
	int lastSong;
	int version;
	int variant;
	char* title;
	int channels;
	int loopSong;
	int speed;
	int tempo;
	//protected
	struct CoreMixer* hardware;
	Number soundPos;
	enum ByteArray_Endianess endian;
	int tick;
	void (*process) (struct CorePlayer* self);
	void (*fast) (struct CorePlayer* self);
	void (*accurate) (struct CorePlayer* self);
	void (*setup) (struct CorePlayer* self);
	void (*set_ntsc) (struct CorePlayer* self, int value);
	void (*set_stereo) (struct CorePlayer* self, Number value);
	void (*set_volume) (struct CorePlayer* self, Number value);
	void (*toggle) (struct CorePlayer* self, int index);
	void (*reset) (struct CorePlayer* self);
	void (*loader) (struct CorePlayer* self, struct ByteArray *stream);
	void (*initialize) (struct CorePlayer* self);
	void (*set_force) (struct CorePlayer* self, int value);
};

void CorePlayer_defaults(struct CorePlayer* self);
void CorePlayer_ctor(struct CorePlayer* self, struct CoreMixer *hardware);
struct CorePlayer* CorePlayer_new(struct CoreMixer *hardware);

void CorePlayer_set_force(struct CorePlayer* self, int value);
int CorePlayer_load(struct CorePlayer* self, struct ByteArray *stream);
void CorePlayer_initialize(struct CorePlayer* self);

/* stubs */
void CorePlayer_process(struct CorePlayer* self);
void CorePlayer_fast(struct CorePlayer* self);
void CorePlayer_accurate(struct CorePlayer* self);
void CorePlayer_setup(struct CorePlayer* self);
void CorePlayer_set_ntsc(struct CorePlayer* self, int value);
void CorePlayer_set_stereo(struct CorePlayer* self, Number value);
void CorePlayer_set_volume(struct CorePlayer* self, Number value);
void CorePlayer_toggle(struct CorePlayer* self, int index);
void CorePlayer_reset(struct CorePlayer* self);
void CorePlayer_loader(struct CorePlayer* self, struct ByteArray *stream);

/* defunct */
#if 0
int CorePlayer_play(struct CorePlayer* self);
void CorePlayer_stop(struct CorePlayer* self);
#endif

#pragma RcB2 DEP "CorePlayer.c"

#endif
