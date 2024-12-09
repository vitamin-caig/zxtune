#ifndef FESONG_H
#define FESONG_H

#include "FETrack.h"

//fixed
#define FESONG_MAX_TRACKS 4

struct FESong {
	int speed;
	int length;
	//tracks : Vector.<Vector.<int>>;
	struct FETrack tracks[FESONG_MAX_TRACKS];
};

void FESong_defaults(struct FESong* self);
void FESong_ctor(struct FESong* self);
struct FESong* FESong_new(void);


#endif
