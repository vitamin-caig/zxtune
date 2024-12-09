#include "FESong.h"
#include "../flod_internal.h"

void FESong_defaults(struct FESong* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void FESong_ctor(struct FESong* self) {
	CLASS_CTOR_DEF(FESong);
	// original constructor code goes here
	//tracks = new Vector.<Vector.<int>>(4, true);
}

struct FESong* FESong_new(void) {
	CLASS_NEW_BODY(FESong);
}
