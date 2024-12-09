#ifndef COREMIXER_H
#define COREMIXER_H

#include "CorePlayer.h"
#include "Sample.h"
#include "../flod.h"

#include "CoreMixerMaxBuffer.h"

enum CoreMixer_type {
	CM_AMIGA,
	CM_SOUNDBLASTER,
};

/*
inheritance
object
   -> CoreMixer
*/
struct CoreMixer {
	struct CorePlayer* player;
	int samplesTick;
	// buffer      : Vector.<Sample>,
	//struct Sample* buffer; //Vector
	struct Sample buffer[COREMIXER_MAX_BUFFER];
	unsigned vector_count_buffer;
	int samplesLeft;
	int remains;
	int completed;
	struct ByteArray* wave;
	enum CoreMixer_type type;
	void (*fast) (struct CoreMixer* self);
	void (*accurate) (struct CoreMixer* self);
};

void CoreMixer_defaults(struct CoreMixer* self);
void CoreMixer_ctor(struct CoreMixer* self);
struct CoreMixer* CoreMixer_new(void);
void CoreMixer_initialize(struct CoreMixer* self);
int CoreMixer_get_complete(struct CoreMixer* self);
void CoreMixer_set_complete(struct CoreMixer* self, int value);
int CoreMixer_get_bufferSize(struct CoreMixer* self);
void CoreMixer_set_bufferSize(struct CoreMixer* self, int value);

/* stubs */
void CoreMixer_reset(struct CoreMixer* self);
void CoreMixer_fast(struct CoreMixer* self);
void CoreMixer_accurate(struct CoreMixer* self);

#pragma RcB2 DEP "CoreMixer.c"

#endif
