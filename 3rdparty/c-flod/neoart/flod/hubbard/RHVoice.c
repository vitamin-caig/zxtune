#include "RHVoice.h"
#include "../flod_internal.h"

void RHVoice_defaults(struct RHVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void RHVoice_ctor(struct RHVoice* self, int index, int bitFlag) {
	CLASS_CTOR_DEF(RHVoice);
	// original constructor code goes here
	self->index = index;
	self->bitFlag = bitFlag;
}

struct RHVoice* RHVoice_new(int index, int bitFlag) {
	CLASS_NEW_BODY(RHVoice, index, bitFlag);
}


void RHVoice_initialize(struct RHVoice* self) {
	self->channel    = null;
	self->sample     = null;
	self->trackPtr   = 0;
	self->trackPos   = 0;
	self->patternPos = 0;
	self->tick       = 1;
	self->busy       = 1;
	self->flags      = 0;
	self->note       = 0;
	self->period     = 0;
	self->volume     = 0;
	self->portaSpeed = 0;
	self->vibratoPtr = 0;
	self->vibratoPos = 0;
	self->synthPos   = 0;
}
