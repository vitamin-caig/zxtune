#ifndef F2PLAYER_H
#define F2PLAYER_H

#include "../core/Soundblaster.h"
#include "../core/SBPlayer.h"
#include "F2Pattern.h"
#include "F2Instrument.h"
#include "F2Voice.h"

enum F2Player_Flags {
      UPDATE_PERIOD = 1,
      UPDATE_VOLUME = 2,
      UPDATE_PANNING = 4,
      UPDATE_TRIGGER = 8,
      UPDATE_ALL = 15,
      SHORT_RAMP = 32,
};

#define F2PLAYER_MAX_PATTERNS 24
#define F2PLAYER_MAX_INSTRUMENTS 32
#define F2PLAYER_MAX_VOICES 24

/*
inheritance
object
	->CorePlayer
		->SBPlayer
			->F2Player
*/
struct F2Player {
	struct SBPlayer super;
	struct F2Pattern patterns[F2PLAYER_MAX_PATTERNS]; // Vector
	/* the instrument is what has the samples assigned (one per instr) */
	struct F2Instrument instruments[F2PLAYER_MAX_INSTRUMENTS]; // Vector
	char instrument_names[F2PLAYER_MAX_INSTRUMENTS][24];
	unsigned vector_count_instruments;
	int linear;
	struct F2Voice voices[F2PLAYER_MAX_VOICES];// Vector
	int order;
	int position;
	int nextOrder;
	int nextPosition;
	struct F2Pattern *pattern;
	int patternDelay;
	int patternOffset;
	int complete;
	char title_buffer[24];
};

void F2Player_defaults(struct F2Player* self);
void F2Player_ctor(struct F2Player* self, struct Soundblaster *mixer);
struct F2Player* F2Player_new(struct Soundblaster *mixer);

void F2Player_process(struct F2Player* self);
void F2Player_fast(struct F2Player* self);
void F2Player_accurate(struct F2Player* self);
void F2Player_initialize(struct F2Player* self);
void F2Player_loader(struct F2Player* self, struct ByteArray *stream);

#endif
