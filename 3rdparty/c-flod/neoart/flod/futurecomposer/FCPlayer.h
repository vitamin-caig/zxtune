#ifndef FCPLAYER_H
#define FCPLAYER_H
#include "../core/AmigaPlayer.h"
#include "../core/AmigaSample.h"
#include "../core/Amiga.h"
#include "../../../flashlib/ByteArray.h"
#include "FCVoice.h"

#define FCPLAYER_FC13_MAX_SAMPLES 57
#define FCPLAYER_FC14_MAX_SAMPLES 200

#ifdef SUPPORT_ONLY_FC13
#  define FCPLAYER_MAX_SAMPLES FCPLAYER_FC13_MAX_SAMPLES
#else
#  define FCPLAYER_MAX_SAMPLES FCPLAYER_FC14_MAX_SAMPLES
#endif

#define FCPLAYER_MAX_VOICES 4

enum Futurecomp_Version {
      FUTURECOMP_10 = 1,
      FUTURECOMP_14 = 2,
};

// this values were chosen to be able to play
// all fc songs in my collection
// you may encounter a "out of bounds access assertion"
// while trying to use files that use larger buffers.
// on the other hand, if you only intend to use a single
// fc module you may want to tune them down.

#define FCPLAYER_SEQS_BUFFERSIZE 3328
#define FCPLAYER_PATS_BUFFERSIZE (8192 + 16)
#define FCPLAYER_VOLS_BUFFERSIZE 4096
#define FCPLAYER_FRQS_BUFFERSIZE 4096

/*
inheritance
??
	->CorePlayer
		->AmigaPlayer
			->FCPlayer
*/
struct FCPlayer {
	struct AmigaPlayer super;
	struct ByteArray *seqs;
	struct ByteArray *pats;
	struct ByteArray *vols;
	struct ByteArray *frqs;
	struct ByteArray seqs_struct;
	struct ByteArray pats_struct;
	struct ByteArray vols_struct;
	struct ByteArray frqs_struct;
	unsigned char seqs_buffer[FCPLAYER_SEQS_BUFFERSIZE];
	unsigned char pats_buffer[FCPLAYER_PATS_BUFFERSIZE];
	unsigned char vols_buffer[FCPLAYER_VOLS_BUFFERSIZE];
	unsigned char frqs_buffer[FCPLAYER_FRQS_BUFFERSIZE];
	off_t seqs_used;
	off_t pats_used;
	off_t vols_used;
	off_t frqs_used;
	int length;
	// samples : Vector.<AmigaSample>,
	struct AmigaSample samples[FCPLAYER_MAX_SAMPLES];
	unsigned samples_max;
	// voices  : Vector.<FCVoice>;
	struct FCVoice voices[FCPLAYER_MAX_VOICES];
};

void FCPlayer_defaults(struct FCPlayer* self);
void FCPlayer_ctor(struct FCPlayer* self, struct Amiga *amiga);
struct FCPlayer* FCPlayer_new(struct Amiga *amiga);

#endif
