#ifndef JHPLAYER_H
#define JHPLAYER_H

#include "../core/Amiga.h"
#include "../core/AmigaPlayer.h"
#include "../core/AmigaSample.h"
#include "JHSong.h"
#include "JHVoice.h"

// there is only one tune that needs 17 songs: the_seven_gates_of_jambala__ingame_2_.hip
// all others work with 8
#define JHPLAYER_MAX_SONGS 17
#define JHPLAYER_MAX_SAMPLES 52

//fixed
#define JHPLAYER_MAX_VOICES 4

struct JHPlayer {
	struct AmigaPlayer super;
	struct JHSong songs[JHPLAYER_MAX_SONGS];//       : Vector.<JHSong>,
	struct AmigaSample samples[JHPLAYER_MAX_SAMPLES];//     : Vector.<AmigaSample>,
	char sample_names[JHPLAYER_MAX_SAMPLES][18];
	struct ByteArray *stream;
	int base;
	int patterns;
	int patternLen;
	int periods;
	int frqseqs;
	int volseqs;
	int samplesData;
	struct JHSong *song;
	struct JHVoice voices[JHPLAYER_MAX_VOICES];//      : Vector.<JHVoice>,
	int coso;
};

void JHPlayer_defaults(struct JHPlayer* self);
void JHPlayer_ctor(struct JHPlayer* self, struct Amiga *amiga);
struct JHPlayer* JHPlayer_new(struct Amiga *amiga);

#endif
