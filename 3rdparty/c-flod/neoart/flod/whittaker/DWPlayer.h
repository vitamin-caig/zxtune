#ifndef DWPLAYER_H
#define DWPLAYER_H

#include "../core/AmigaPlayer.h"
#include "../core/Amiga.h"
#include "DWSong.h"
#include "DWSample.h"
#include "DWVoice.h"

#define DWPLAYER_MAX_VOICES 4
#define DWPLAYER_MAX_SONGS 16
#define DWPLAYER_MAX_SAMPLES 19

/*
inheritance
??
	->CorePlayer
		->AmigaPlayer
			->DWPlayer
*/
struct DWPlayer {
	struct AmigaPlayer super;
	//songs         : Vector.<DWSong>,
	//samples       : Vector.<DWSample>,
	//struct DWSong* songs;
	//struct DWSample* samples;
	struct DWSong songs[DWPLAYER_MAX_SONGS];
	struct DWSample samples[DWPLAYER_MAX_SAMPLES];
	
	unsigned int vector_count_songs;
	unsigned int vector_count_samples;
	
	struct ByteArray *stream;
	struct DWSong *song;
	int songvol;
	int master;
	int periods;
	int frqseqs;
	int volseqs;
	int transpose;
	int slower;
	int slowerCounter;
	int delaySpeed;
	int delayCounter;
	int fadeSpeed;
	int fadeCounter;
	struct DWSample *wave;
	int waveCenter;
	int waveLo;
	int waveHi;
	int waveDir;
	int waveLen;
	int wavePos;
	int waveRateNeg;
	int waveRatePos;
	struct DWVoice voices[DWPLAYER_MAX_VOICES];
	//struct DWVoice *voices;
	//voices        : Vector.<DWVoice>,
	int active;
	int complete;
	int base;
	int com2;
	int com3;
	int com4;
	int readLen;
};

void DWPlayer_defaults(struct DWPlayer* self);
void DWPlayer_ctor(struct DWPlayer* self, struct Amiga* amiga);
struct DWPlayer* DWPlayer_new(struct Amiga* amiga);

void DWPlayer_process(struct DWPlayer* self);
void DWPlayer_initialize(struct DWPlayer* self);
void DWPlayer_loader(struct DWPlayer* self, struct ByteArray *stream);

#pragma RcB2 DEP "DWPlayer.c"

#endif
