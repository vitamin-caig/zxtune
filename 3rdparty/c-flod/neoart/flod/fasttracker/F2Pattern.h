#ifndef F2PATTERN_H
#define F2PATTERN_H

#include "F2Row.h"

#define F2PATTERN_MAX_ROWS 1024

/*
inheritance
object
	-> F2Pattern
*/
struct F2Pattern {
	//rows   : Vector.<F2Row>,
	struct F2Row rows[F2PATTERN_MAX_ROWS];
	int length;
	int size;
};

void F2Pattern_defaults(struct F2Pattern* self);
void F2Pattern_ctor(struct F2Pattern* self, unsigned length, unsigned channels);
struct F2Pattern* F2Pattern_new(unsigned length, unsigned channels);

#endif
