#include "FETrack.h"
#include "../flod_internal.h"

void FETrack_defaults(struct FETrack* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FETrack_ctor(struct FETrack* self) {
	CLASS_CTOR_DEF(FETrack);
	// original constructor code goes here
}

struct FETrack* FETrack_new(void) {
	CLASS_NEW_BODY(FETrack);
}
