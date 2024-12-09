#ifndef PTPLAYER_H
#define PTPLAYER_H

#include "PTRow.h"
#include "PTSample.h"
#include "PTVoice.h"

#include "../core/Amiga.h"
#include "../core/AmigaPlayer.h"

#define PTPLAYER_MAX_TRACKS 128
#define PTPLAYER_MAX_SAMPLES 32
#define PTPLAYER_MAX_VOICES 4

#define PTPLAYER_MAX_PATTERNS ((12 * 1024) + 256)

enum Protracker_Versions {
      PROTRACKER_10 = 1,
      PROTRACKER_11 = 2,
      PROTRACKER_12 = 3,
};

/*
inheritance
??
	->CorePlayer
		->AmigaPlayer
			->PTPlayer
*/
struct PTPlayer {
	struct AmigaPlayer super;
	unsigned char track[PTPLAYER_MAX_TRACKS];//Vector.<int>,
	struct PTRow patterns[PTPLAYER_MAX_PATTERNS]; //Vector.<PTRow>,
	struct PTSample samples[PTPLAYER_MAX_SAMPLES];//Vector.<PTSample>,
	char sample_used[PTPLAYER_MAX_SAMPLES];
	int length;
	struct PTVoice voices[PTPLAYER_MAX_VOICES];//Vector.<PTVoice>,
	int trackPos;
	int patternPos;
	int patternBreak;
	int patternDelay;
	int breakPos;
	int jumpFlag;
	int vibratoDepth;
	char title_buffer[24];
	char sample_names[PTPLAYER_MAX_SAMPLES][24];
	unsigned char chans;
};

void PTPlayer_defaults(struct PTPlayer* self);
void PTPlayer_ctor(struct PTPlayer* self, struct Amiga *amiga);
struct PTPlayer* PTPlayer_new(struct Amiga *amiga);

void PTPlayer_set_force(struct PTPlayer* self, int value);
void PTPlayer_process(struct PTPlayer* self);
void PTPlayer_initialize(struct PTPlayer* self);
void PTPlayer_loader(struct PTPlayer* self, struct ByteArray *stream);

#endif
