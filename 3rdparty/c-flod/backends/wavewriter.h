#ifndef WAVEWRITER_H
#define WAVEWRITER_H

#include <stddef.h>
#include <unistd.h>

#include "backend.h"

struct WaveWriter {
	struct Backend super;
	int fd;
	size_t written;
};

int WaveWriter_init(struct WaveWriter *self, char* filename);
int WaveWriter_write(struct WaveWriter *self, void* buffer, size_t bufsize);
int WaveWriter_close(struct WaveWriter *self);

#endif
