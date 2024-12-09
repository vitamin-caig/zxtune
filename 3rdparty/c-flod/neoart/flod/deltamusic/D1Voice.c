#include "D1Voice.h"
#include "../flod_internal.h"

void D1Voice_defaults(struct D1Voice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void D1Voice_ctor(struct D1Voice* self, int index) {
	CLASS_CTOR_DEF(D1Voice);
	// original constructor code goes here
	self->index = index;
}

struct D1Voice* D1Voice_new(int index) {
	CLASS_NEW_BODY(D1Voice, index);
}

void D1Voice_initialize(struct D1Voice* self) {
	self->sample        = null;
	self->trackPos      = 0;
	self->patternPos    = 0;
	self->status        = 0;
	self->speed         = 1;
	self->step          = null;
	self->row           = null;
	self->note          = 0;
	self->period        = 0;
	self->arpeggioPos   = 0;
	self->pitchBend     = 0;
	self->tableCtr      = 0;
	self->tablePos      = 0;
	self->vibratoCtr    = 0;
	self->vibratoDir    = 0;
	self->vibratoPos    = 0;
	self->vibratoPeriod = 0;
	self->volume        = 0;
	self->attackCtr     = 0;
	self->decayCtr      = 0;
	self->releaseCtr    = 0;
	self->sustain       = 1;
}
