#ifndef DMPLAYER_H
#define DMPLAYER_H

#include "../core/AmigaPlayer.h"
#include "../core/AmigaRow.h"
#include "../core/Amiga.h"
#include "DMSong.h"
#include "DMSample.h"
#include "DMVoice.h"


#define DMPLAYER_MAX_PATTERNS 6464
#define DMPLAYER_MAX_SAMPLES 43


//fixed
#define DMPLAYER_MAX_VOICES 7
#define DMPLAYER_MAX_ARPEGGIOS 256
#define DMPLAYER_MAX_AVERAGES 1024
#define DMPLAYER_MAX_SONGS 8
#define DMPLAYER_MAX_VOLUMES 16384

enum DMPlayerVersions {
	DIGITALMUG_V1 = 1,
	DIGITALMUG_V2 = 2,
};

struct DMPlayer {
	struct AmigaPlayer super;
	//songs       : Vector.<DMSong>,
	struct DMSong songs[DMPLAYER_MAX_SONGS];
	//patterns    : Vector.<AmigaRow>,
	struct AmigaRow patterns[DMPLAYER_MAX_PATTERNS];
	//samples     : Vector.<DMSample>,
	struct DMSample samples[DMPLAYER_MAX_SAMPLES];
	//voices      : Vector.<DMVoice>,
	struct DMVoice voices[DMPLAYER_MAX_VOICES];
	int buffer1;
	int buffer2;
	struct DMSong *song1;
	struct DMSong *song2;
	int trackPos;
	int patternPos;
	int patternLen;
	int patternEnd;
	int stepEnd;
	int numChannels;
	int arpeggios[DMPLAYER_MAX_ARPEGGIOS];
	int averages[DMPLAYER_MAX_AVERAGES];
	int volumes[DMPLAYER_MAX_VOLUMES];
	struct AmigaChannel mixChannel;
	int mixPeriod;
};

void DMPlayer_defaults(struct DMPlayer* self);
void DMPlayer_ctor(struct DMPlayer* self, struct Amiga *amiga);
struct DMPlayer* DMPlayer_new(struct Amiga *amiga);

#endif