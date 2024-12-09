#ifndef F2DATA_H
#define F2DATA_H

#include "F2Point.h"

#define F2DATA_MAX_POINTS 12

/*
inheritance
object
	-> F2Data
*/
struct F2Data {
	//points    : Vector.<F2Point>,
	struct F2Point points[F2DATA_MAX_POINTS];
	int total;
	int sustain;
	int loopStart;
	int loopEnd;
	int flags;
};

void F2Data_defaults(struct F2Data* self);
void F2Data_ctor(struct F2Data* self);
struct F2Data* F2Data_new(void);

#endif
