#ifndef AOWRITER_H
#define AOWRITER_H

#include <stddef.h>
#include <unistd.h>
#include <ao/ao.h>

#include "backend.h"

struct AoWriter {
	struct Backend super;
	ao_device *device;
	ao_sample_format format;
	int aodriver;
};

int AoWriter_init(struct AoWriter *self, void* data);
int AoWriter_write(struct AoWriter *self, void* buffer, size_t bufsize);
int AoWriter_close(struct AoWriter *self);

#pragma RcB2 DEP "aowriter.c"
#pragma RcB2 LINK "-lao"

#endif
