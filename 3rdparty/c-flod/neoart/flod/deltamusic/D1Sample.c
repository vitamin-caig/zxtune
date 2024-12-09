#include "D1Sample.h"
#include "../flod_internal.h"

void D1Sample_defaults(struct D1Sample* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void D1Sample_ctor(struct D1Sample* self) {
	CLASS_CTOR_DEF(D1Sample);
	// original constructor code goes here
	//arpeggio = new Vector.<int>( 8, true);
	//table    = new Vector.<int>(48, true);
}

struct D1Sample* D1Sample_new(void) {
	CLASS_NEW_BODY(D1Sample);
}

