#ifndef SDLAUDIOWRITER_H
#define SDLAUDIOWRITER_H

#include <stddef.h>
#include <unistd.h>
#include <pthread.h>
#include <SDL/SDL_audio.h>

#include "backend.h"

struct SdlWriter {
	struct Backend super;
	pthread_mutex_t cb_lock;
	char* buffer;
	size_t buffer_used;
	size_t buffer_size;
	SDL_AudioSpec fmt;
};

int SdlWriter_init(struct SdlWriter *self, void* data);
int SdlWriter_write(struct SdlWriter *self, void* buffer, size_t bufsize);
int SdlWriter_close(struct SdlWriter *self);

#pragma RcB2 DEP "sdlwriter.c"
#pragma RcB2 LINK "-lSDL_sound" "-lpthread"

#endif
