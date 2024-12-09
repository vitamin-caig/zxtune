#ifndef AMIGAROW_H
#define AMIGAROW_H

/*
inheritance
Object
	-> AmigaRow
*/
struct AmigaRow {
	int note;
	int sample;
	int effect;
	int param;
};

void AmigaRow_defaults(struct AmigaRow* self);

void AmigaRow_ctor(struct AmigaRow* self);

struct AmigaRow* AmigaRow_new(void);

#endif
