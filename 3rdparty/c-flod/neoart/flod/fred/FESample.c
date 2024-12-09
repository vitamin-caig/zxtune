#include "FESample.h"
#include "../flod_internal.h"

void FESample_defaults(struct FESample* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FESample_ctor(struct FESample* self) {
	CLASS_CTOR_DEF(FESample);
	// original constructor code goes here
	//arpeggio = new Vector.<int>(16, true);
}

struct FESample* FESample_new(void) {
	CLASS_NEW_BODY(FESample);
}

