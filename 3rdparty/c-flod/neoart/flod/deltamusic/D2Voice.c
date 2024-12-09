#include "D2Voice.h"
#include "../flod_internal.h"

void D2Voice_defaults(struct D2Voice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void D2Voice_ctor(struct D2Voice* self, int index) {
	CLASS_CTOR_DEF(D2Voice);
	// original constructor code goes here
	self->index = index;
}

struct D2Voice* D2Voice_new(int index) {
	CLASS_NEW_BODY(D2Voice, index);
}

void D2Voice_initialize(struct D2Voice* self) {
	self->sample         = null;
	self->trackPtr       = 0;
	self->trackPos       = 0;
	self->trackLen       = 0;
	self->patternPos     = 0;
	self->restart        = 0;
	self->step           = null;
	self->row            = null;
	self->note           = 0;
	self->period         = 0;
	self->finalPeriod    = 0;
	self->arpeggioPtr    = 0;
	self->arpeggioPos    = 0;
	self->pitchBend      = 0;
	self->portamento     = 0;
	self->tableCtr       = 0;
	self->tablePos       = 0;
	self->vibratoCtr     = 0;
	self->vibratoDir     = 0;
	self->vibratoPos     = 0;
	self->vibratoPeriod  = 0;
	self->vibratoSustain = 0;
	self->volume         = 0;
	self->volumeMax      = 63;
	self->volumePos      = 0;
	self->volumeSustain  = 0;
}
