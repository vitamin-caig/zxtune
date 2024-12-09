#ifndef AMIGASAMPLE_H
#define AMIGASAMPLE_H

#include "../flod.h"
/*
inheritance
object
      -> AmigaSample
*/
struct AmigaSample {
	char* name;
	int length;
	int loop;
	int repeat;
	int volume;
	int pointer;
	int loopPtr;
};

void AmigaSample_defaults(struct AmigaSample* self);
void AmigaSample_ctor(struct AmigaSample* self);
struct AmigaSample* AmigaSample_new(void);

#endif