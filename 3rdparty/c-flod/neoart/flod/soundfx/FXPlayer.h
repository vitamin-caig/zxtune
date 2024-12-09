#ifndef FXPLAYER_H
#define FXPLAYER_H

#include "../core/Amiga.h"
#include "../core/AmigaRow.h"
#include "../core/AmigaSample.h"
#include "../core/AmigaPlayer.h"

#include "FXVoice.h"

enum FXPlayerVersion {
	SOUNDFX_10 = 1,
	SOUNDFX_18 = 2,
	SOUNDFX_19 = 3,
	SOUNDFX_20 = 4,
};

/* only 2 tunes (pipes, amc-slideshow) use more than 14592 patterns 
 * (and they seem to be identical) */
#define FXPLAYER_MAX_PATTERNS 17664
#define FXPLAYER_MAX_SAMPLES 32

//fixed
#define FXPLAYER_MAX_VOICES 4
#define FXPLAYER_MAX_TRACKS 128

struct FXPlayer {
	struct AmigaPlayer super;
	//track      : Vector.<int>,
	int track[FXPLAYER_MAX_TRACKS];
	//patterns   : Vector.<AmigaRow>,
	struct AmigaRow patterns[FXPLAYER_MAX_PATTERNS];
	//samples    : Vector.<AmigaSample>,
	struct AmigaSample samples[FXPLAYER_MAX_SAMPLES];
	char sample_names[FXPLAYER_MAX_SAMPLES][22];
	int length;
	//voices     : Vector.<FXVoice>,
	struct FXVoice voices[FXPLAYER_MAX_VOICES];
	int trackPos;
	int patternPos;
	int jumpFlag;
	int delphine;
};

void FXPlayer_defaults(struct FXPlayer* self);
void FXPlayer_ctor(struct FXPlayer* self, struct Amiga *amiga);
struct FXPlayer* FXPlayer_new(struct Amiga *amiga);

#endif
