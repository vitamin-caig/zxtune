#include "RHSong.h"
#include "../flod_internal.h"


void RHSong_defaults(struct RHSong* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void RHSong_ctor(struct RHSong* self) {
	CLASS_CTOR_DEF(RHSong);
	// original constructor code goes here
}

struct RHSong* RHSong_new(void) {
	CLASS_NEW_BODY(RHSong);
}

