#ifndef DWSONG_H
#define DWSONG_H

#define DWSONG_MAXTRACKS 16


struct DWSong {
	int speed;
	int delay;
	//tracks : Vector.<int>;
	//int *tracks;
	int tracks[DWSONG_MAXTRACKS];
	unsigned int vector_count_tracks;
};

void DWSong_defaults(struct DWSong* self);
void DWSong_ctor(struct DWSong* self);
struct DWSong* DWSong_new(void);

#pragma RcB2 DEP "DWSong.c"

#endif
