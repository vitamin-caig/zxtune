#ifndef S2PLAYER_H
#define S2PLAYER_H

#include "../core/AmigaPlayer.h"
#include "S2Step.h"
#include "S2Instrument.h"
#include "S2Sample.h"
#include "S2Voice.h"

#define S2PLAYER_MAX_TRACKS 396
#define S2PLAYER_MAX_PATTERNS 24412
#define S2PLAYER_MAX_INSTRUMENTS 104
#define S2PLAYER_MAX_SAMPLES 63
#define S2PLAYER_MAX_ARPEGGIOS 528
#define S2PLAYER_MAX_VIBRATOS 608
#define S2PLAYER_MAX_WAVES 1008

//fixed
#define S2PLAYER_MAX_VOICES 4
#define S2PLAYER_MAX_ARPEGGIOFX 4

struct S2Player {
	struct AmigaPlayer super;
	//tracks      : Vector.<S2Step>,
	struct S2Step tracks[S2PLAYER_MAX_TRACKS];
	//patterns    : Vector.<SMRow>,
	struct SMRow patterns[S2PLAYER_MAX_PATTERNS];
	//instruments : Vector.<S2Instrument>,
	struct S2Instrument instruments [S2PLAYER_MAX_INSTRUMENTS];
	//samples     : Vector.<S2Sample>,
	struct S2Sample samples[S2PLAYER_MAX_SAMPLES];
	//arpeggios   : Vector.<int>,
	signed char arpeggios[S2PLAYER_MAX_ARPEGGIOS];
	//vibratos    : Vector.<int>,
	signed char vibratos[S2PLAYER_MAX_VIBRATOS];
	//waves       : Vector.<int>,
	unsigned char waves[S2PLAYER_MAX_WAVES];
	int length;
	int speedDef;
	//voices      : Vector.<S2Voice>,
	struct S2Voice voices[S2PLAYER_MAX_VOICES];
	int trackPos;
	int patternPos;
	int patternLen;
	//arpeggioFx  : Vector.<int>,
	int arpeggioFx[S2PLAYER_MAX_ARPEGGIOFX];
	int arpeggioPos;
};

void S2Player_defaults(struct S2Player* self);
void S2Player_ctor(struct S2Player* self, struct Amiga *amiga);
struct S2Player* S2Player_new(struct Amiga *amiga);

#endif
