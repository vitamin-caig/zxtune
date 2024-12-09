#ifndef AMIGASTEP_H
#define AMIGASTEP_H

struct AmigaStep {
	int pattern;
	int transpose;
};

void AmigaStep_defaults(struct AmigaStep* self);

void AmigaStep_ctor(struct AmigaStep* self);

struct AmigaStep* AmigaStep_new(void);

#endif