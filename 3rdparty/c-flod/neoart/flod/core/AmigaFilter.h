#ifndef AMIGAFILTER_H
#define AMIGAFILTER_H

#include "Sample.h"
#include "../flod.h"

enum AmigaFilterForce {
	AUTOMATIC =  0,
	FORCE_ON  =  1,
	FORCE_OFF = -1,
};
/*
inheritance
object
	-> AmigaFilter
*/
struct AmigaFilter {
	int active;
	enum AmigaFilterForce forced;
	//private
	Number l0;
	Number l1;
	Number l2;
	Number l3;
	Number l4;
	Number r0;
	Number r1;
	Number r2;
	Number r3;
	Number r4;
};

void AmigaFilter_defaults(struct AmigaFilter* self);
void AmigaFilter_ctor(struct AmigaFilter* self);
struct AmigaFilter* AmigaFilter_new(void);
void AmigaFilter_initialize(struct AmigaFilter* self);
void AmigaFilter_process(struct AmigaFilter* self, int model, struct Sample* sample);

#pragma RcB2 DEP "AmigaFilter.c"

#endif
