#include "aowriter.h"
#include <string.h>

int AoWriter_init(struct AoWriter *self, void* data) {
	(void) data;
	ao_initialize();
	memset(self, 0, sizeof(*self));
	self->format.bits = 16;
	self->format.channels = 2;
	self->format.rate = 44100;
	self->format.byte_format = AO_FMT_LITTLE;
	self->aodriver = ao_default_driver_id();
	self->device = ao_open_live(self->aodriver, &self->format, NULL);
	return self->device != NULL;
}

int AoWriter_write(struct AoWriter *self, void* buffer, size_t bufsize) {
	return ao_play(self->device, buffer, bufsize);
}

int AoWriter_close(struct AoWriter *self) {
	return ao_close(self->device);
}
