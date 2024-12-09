#ifndef FEPLAYER_H
#define FEPLAYER_H

#include "../core/Amiga.h"
#include "../core/AmigaPlayer.h"
#include "FESample.h"
#include "FESong.h"
#include "FEVoice.h"

#define FEPLAYER_MAX_SONGS 7
#define FEPLAYER_MAX_SAMPLES 64
#define FEPLAYER_PATTERNS_MEMORY_MAX 5090

//fixed
#define FEPLAYER_MAX_VOICES 4

struct FEPlayer {
	struct AmigaPlayer super;
	//songs    : Vector.<FESong>,
	struct FESong songs[FEPLAYER_MAX_SONGS];
	//samples  : Vector.<FESample>,
	struct FESample samples[FEPLAYER_MAX_SAMPLES];
	//patterns : ByteArray,
	struct ByteArray *patterns;
	struct ByteArray patterns_buf;
	char patterns_memory[FEPLAYER_PATTERNS_MEMORY_MAX];
	struct FESong *song;
	//voices   : Vector.<FEVoice>,
	struct FEVoice voices[FEPLAYER_MAX_VOICES];
	int complete;
	int sampFlag;
};

void FEPlayer_defaults(struct FEPlayer* self);
void FEPlayer_ctor(struct FEPlayer* self, struct Amiga* amiga);
struct FEPlayer* FEPlayer_new(struct Amiga* amiga);

#endif
