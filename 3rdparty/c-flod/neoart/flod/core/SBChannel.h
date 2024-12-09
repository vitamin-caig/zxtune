#ifndef SBCHANNEL_H
#define SBCHANNEL_H

#include "../flod.h"
#include "SBSample.h"

struct SBChannel {
	struct SBChannel *next;
	int mute;
	int enabled;
	struct SBSample* sample;
	int length;
	int index;
	int pointer;
	int delta;
	Number fraction;
	Number speed;
	int dir;
	struct SBSample* oldSample;
	int oldLength;
	int oldPointer;
	Number oldFraction;
	Number oldSpeed;
	int oldDir;
	Number volume;
	Number lvol;
	Number rvol;
	int panning;
	Number lpan;
	Number rpan;
	Number ldata;
	Number rdata;
	int mixCounter;
	Number lmixRampU;
	Number lmixDeltaU;
	Number rmixRampU;
	Number rmixDeltaU;
	Number lmixRampD;
	Number lmixDeltaD;
	Number rmixRampD;
	Number rmixDeltaD;
	int volCounter;
	Number lvolDelta;
	Number rvolDelta;
	int panCounter;
	Number lpanDelta;
	Number rpanDelta;
};

void SBChannel_defaults(struct SBChannel* self);

void SBChannel_ctor(struct SBChannel* self);

struct SBChannel* SBChannel_new(void);

void SBChannel_initialize(struct SBChannel* self);

#pragma RcB2 DEP "SBChannel.c"

#endif
