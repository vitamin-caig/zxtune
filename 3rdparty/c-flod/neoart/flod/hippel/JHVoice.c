#include "JHVoice.h"
#include "../flod_internal.h"
void JHVoice_defaults(struct JHVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void JHVoice_ctor(struct JHVoice* self, int index) {
	CLASS_CTOR_DEF(JHVoice);
	// original constructor code goes here
	self->index = index;
}

struct JHVoice* JHVoice_new(int index) {
	CLASS_NEW_BODY(JHVoice, index);
}

void JHVoice_initialize(struct JHVoice* self) {
	self->channel     = null;
	self->enabled     = 0;
	self->cosoCounter = 0;
	self->cosoSpeed   = 0;
	self->trackPtr    = 0;
	self->trackPos    = 12;
	self->trackTransp = 0;
	self->patternPtr  = 0;
	self->patternPos  = 0;
	self->frqseqPtr   = 0;
	self->frqseqPos   = 0;
	self->volseqPtr   = 0;
	self->volseqPos   = 0;
	self->sample      = -1;
	self->loopPtr     = 0;
	self->repeat      = 0;
	self->tick        = 0;
	self->note        = 0;
	self->transpose   = 0;
	self->info        = 0;
	self->infoPrev    = 0;
	self->volume      = 0;
	self->volCounter  = 1;
	self->volSpeed    = 1;
	self->volSustain  = 0;
	self->volTransp   = 0;
	self->volFade     = 100;
	self->portaDelta  = 0;
	self->vibrato     = 0;
	self->vibDelay    = 0;
	self->vibDelta    = 0;
	self->vibDepth    = 0;
	self->vibSpeed    = 0;
	self->slide       = 0;
	self->sldActive   = 0;
	self->sldDone     = 0;
	self->sldCounter  = 0;
	self->sldSpeed    = 0;
	self->sldDelta    = 0;
	self->sldPointer  = 0;
	self->sldLen      = 0;
	self->sldEnd      = 0;
	self->sldLoopPtr  = 0;
}
