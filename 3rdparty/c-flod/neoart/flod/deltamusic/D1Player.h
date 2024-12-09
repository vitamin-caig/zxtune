#ifndef D1PLAYER_H
#define D1PLAYER_H

#include "D1Voice.h"
#include "D1Sample.h"
#include "../core/Amiga.h"
#include "../core/AmigaRow.h"
#include "../core/AmigaStep.h"
#include "../core/AmigaPlayer.h"

#define D1PLAYER_MAX_PATTERNS 560
#define D1PLAYER_MAX_TRACKS 292
//crusaders2.dm is *very* hungry
// those are the values to make it work. all others are fine with the above values.
//#define D1PLAYER_MAX_TRACKS 1884
//#define D1PLAYER_MAX_PATTERNS 1664

//fixed
#define D1PLAYER_MAX_SAMPLES 21
#define D1PLAYER_MAX_VOICES 4
#define D1PLAYER_MAX_POINTERS 4

struct D1Player {
	struct AmigaPlayer super;
	int pointers[D1PLAYER_MAX_POINTERS];
	struct AmigaStep tracks[D1PLAYER_MAX_TRACKS];
	struct AmigaRow patterns[D1PLAYER_MAX_PATTERNS];
	struct D1Sample samples[D1PLAYER_MAX_SAMPLES];
	struct D1Voice voices[D1PLAYER_MAX_VOICES];
};

void D1Player_defaults(struct D1Player* self);
void D1Player_ctor(struct D1Player* self, struct Amiga *amiga);
struct D1Player* D1Player_new(struct Amiga *amiga);

#endif
