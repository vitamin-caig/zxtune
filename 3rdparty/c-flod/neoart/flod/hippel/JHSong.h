#ifndef JHSONG_H
#define JHSONG_H

struct JHSong {
	int pointer;
	int speed;
	int length;
};

void JHSong_defaults(struct JHSong* self);
void JHSong_ctor(struct JHSong* self);
struct JHSong* JHSong_new(void);

#endif
