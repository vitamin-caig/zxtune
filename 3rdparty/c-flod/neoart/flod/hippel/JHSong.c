#include "JHSong.h"
#include "../flod_internal.h"

void JHSong_defaults(struct JHSong* self) {
	CLASS_DEF_INIT();
	// static initializers go here
}

void JHSong_ctor(struct JHSong* self) {
	CLASS_CTOR_DEF(JHSong);
	// original constructor code goes here
}

struct JHSong* JHSong_new(void) {
	CLASS_NEW_BODY(JHSong);
}

