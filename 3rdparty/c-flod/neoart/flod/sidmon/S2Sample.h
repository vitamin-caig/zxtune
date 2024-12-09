#ifndef S2SAMPLE_H
#define S2SAMPLE_H

#include "../core/AmigaSample.h"

struct S2Sample {
	struct AmigaSample super;
	char sample_name[32];
	int negStart;
	int negLen;
	int negPos;
	int negToggle;
	unsigned short negSpeed;
	unsigned short negDir;
	signed short negOffset;
	unsigned short negCtr;
};

void S2Sample_defaults(struct S2Sample* self);
void S2Sample_ctor(struct S2Sample* self);
struct S2Sample* S2Sample_new(void);

#endif
