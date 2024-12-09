#ifndef RHSONG_H
#define RHSONG_H

//fixed
#define RHSONG_MAX_TRACKS 4

struct RHSong {
	int speed;
	//tracks : Vector.<int>;
	int tracks[RHSONG_MAX_TRACKS];
};

void RHSong_defaults(struct RHSong* self);
void RHSong_ctor(struct RHSong* self);
struct RHSong* RHSong_new(void);

#endif
