#ifndef STPLAYER_H
#define STPLAYER_H

#include "../core/AmigaPlayer.h"
#include "../core/AmigaSample.h"
#include "../core/AmigaRow.h"
#include "../core/Amiga.h"
#include "STVoice.h"

enum Soundtracker_Format {
	ULTIMATE_SOUNDTRACKER = 1,
	DOC_SOUNDTRACKER_9 = 2,
	MASTER_SOUNDTRACKER = 3,
	DOC_SOUNDTRACKER_20 = 4,
};

#define STPLAYER_MAX_TRACKS 128
#define STPLAYER_MAX_SAMPLES 16
// all tunes but Federal_Intelligent_Gamesoft1.mod work with the below buffersize
//#define STPLAYER_MAX_PATTERNS (3840 + 256)
#define STPLAYER_MAX_PATTERNS 8448


// fixed
#define STPLAYER_MAX_VOICES 4

/*
inheritance
??
	->CorePlayer
		->AmigaPlayer
			->STPlayer
*/
struct STPlayer {
	struct AmigaPlayer super;
	//track      : Vector.<int>,
	int track[STPLAYER_MAX_TRACKS];
	//patterns   : Vector.<AmigaRow>,
	struct AmigaRow patterns[STPLAYER_MAX_PATTERNS];
	//samples    : Vector.<AmigaSample>,
	struct AmigaSample samples[STPLAYER_MAX_SAMPLES];
	char sample_names[STPLAYER_MAX_SAMPLES][24];
	int length;
	//voices     : Vector.<STVoice>,
	struct STVoice voices[STPLAYER_MAX_VOICES];
	int trackPos;
	int patternPos;
	int jumpFlag;
	char title_buf[24];
};


void STPlayer_defaults(struct STPlayer* self);
void STPlayer_ctor(struct STPlayer* self, struct Amiga *amiga);
struct STPlayer* STPlayer_new(struct Amiga *amiga);

void STPlayer_arpeggio(struct STPlayer* self, struct STVoice *voice);
void STPlayer_loader(struct STPlayer* self, struct ByteArray *stream);
void STPlayer_initialize(struct STPlayer* self);
void STPlayer_process(struct STPlayer* self);
void STPlayer_set_ntsc(struct STPlayer* self, int value);
void STPlayer_set_force(struct STPlayer* self, int value);


#endif
