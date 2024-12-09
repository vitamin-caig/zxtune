#include "FEVoice.h"
#include "../flod_internal.h"

void FEVoice_defaults(struct FEVoice* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FEVoice_ctor(struct FEVoice* self, int index, int bitFlag) {
	CLASS_CTOR_DEF(FEVoice);
	// original constructor code goes here
	self->index = index;
	self->bitFlag = bitFlag;
}

struct FEVoice* FEVoice_new(int index, int bitFlag) {
	CLASS_NEW_BODY(FEVoice, index, bitFlag);
}


void FEVoice_initialize(struct FEVoice* self) {
	self->channel       = null;
	self->sample        = null;
	self->trackPos      = 0;
	self->patternPos    = 0;
	self->tick          = 1;
	self->busy          = 1;
	self->note          = 0;
	self->period        = 0;
	self->volume        = 0;
	self->envelopePos   = 0;
	self->sustainTime   = 0;
	self->arpeggioPos   = 0;
	self->arpeggioSpeed = 0;
	self->portamento    = 0;
	self->portaCounter  = 0;
	self->portaDelay    = 0;
	self->portaFlag     = 0;
	self->portaLimit    = 0;
	self->portaNote     = 0;
	self->portaPeriod   = 0;
	self->portaSpeed    = 0;
	self->vibrato       = 0;
	self->vibratoDelay  = 0;
	self->vibratoDepth  = 0;
	self->vibratoFlag   = 0;
	self->vibratoSpeed  = 0;
	self->pulseCounter  = 0;
	self->pulseDelay    = 0;
	self->pulseDir      = 0;
	self->pulsePos      = 0;
	self->pulseSpeed    = 0;
	self->blendCounter  = 0;
	self->blendDelay    = 0;
	self->blendDir      = 0;
	self->blendPos      = 0;
}
