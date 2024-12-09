#include "D2Sample.h"
#include "../flod_internal.h"

void D2Sample_defaults(struct D2Sample* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void D2Sample_ctor(struct D2Sample* self) {
	CLASS_CTOR_DEF(D2Sample);
	// original constructor code goes here
	//table    = new Vector.<int>(48, true);
	//vibratos = new Vector.<int>(15, true);
	//volumes  = new Vector.<int>(15, true);
	
}

struct D2Sample* D2Sample_new(void) {
	CLASS_NEW_BODY(D2Sample);
}

