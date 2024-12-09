#ifndef D2PLAYER_H
#define D2PLAYER_H

#include "../core/AmigaPlayer.h"
#include "../core/Amiga.h"
#include "D2Sample.h"
#include "D2Voice.h"

#define D2PLAYER_MAX_SAMPLES 37
#define D2PLAYER_MAX_PATTERNS 2336
#define D2PLAYER_MAX_TRACKS 628

//fixed
#define D2PLAYER_MAX_ARPEGGIOS 1024
#define D2PLAYER_MAX_VOICES 4
#define D2PLAYER_MAX_DATA 12

struct D2Player {
	struct AmigaPlayer super;
	struct AmigaStep tracks[D2PLAYER_MAX_TRACKS];
	struct AmigaRow patterns[D2PLAYER_MAX_PATTERNS];
	struct D2Sample samples[D2PLAYER_MAX_SAMPLES];
	struct D2Voice voices[D2PLAYER_MAX_VOICES];
	unsigned samples_count;
	unsigned patterns_count;
	int data[D2PLAYER_MAX_DATA];
	int arpeggios[D2PLAYER_MAX_ARPEGGIOS];
	unsigned int noise;
};

void D2Player_defaults(struct D2Player* self);
void D2Player_ctor(struct D2Player* self, struct Amiga *amiga);
struct D2Player* D2Player_new(struct Amiga *amiga);

#endif
