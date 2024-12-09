#include "sdlwriter.h"
#include <SDL/SDL.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "../neoart/flod/core/CoreMixerMaxBuffer.h"


#define lock() pthread_mutex_lock(&self->cb_lock)
#define unlock() pthread_mutex_unlock(&self->cb_lock)

static void sdl_callback(void *user, Uint8 *stream, int len) {
	struct SdlWriter *self = user;
	do {
		lock();
		if((size_t) len <= self->buffer_used) {
			memcpy(stream, self->buffer, len);
			size_t diff = self->buffer_used - len;
			memmove(self->buffer, (char*)self->buffer + len, diff);
			self->buffer_used = diff;
			unlock();
			return;
		} else {
			unlock();
			usleep(1000);
		}

	} while(1);
}

int SdlWriter_write(struct SdlWriter *self, void* buffer, size_t bufsize) {
	do {
		lock();
		if(self->buffer_used + bufsize > self->buffer_size) {
			unlock();
			usleep(1000);
		} else {
			memcpy((char*)self->buffer + self->buffer_used, buffer, bufsize);
			self->buffer_used += bufsize;
			unlock();
			break;
		}
	} while(1);
	return 1;
}

#define NUM_CHANNELS 2
int SdlWriter_init(struct SdlWriter *self, void* data) {
	(void) data;
	SDL_Init(SDL_INIT_AUDIO);
	SDL_AudioSpec obtained;
	self->fmt.freq = 44100;
	self->fmt.format = AUDIO_S16;
	self->fmt.channels = NUM_CHANNELS;
	self->fmt.samples = COREMIXER_MAX_BUFFER;
	self->fmt.callback = sdl_callback;
	self->fmt.userdata = self;
	pthread_mutex_init(&self->cb_lock, 0);
	if(SDL_OpenAudio(&self->fmt, &obtained) < 0) {
		printf("sdl_openaudio: %s\n", SDL_GetError());
		return 0;
	}
	size_t max = obtained.samples > self->fmt.samples ? obtained.samples : self->fmt.samples;
	self->fmt = obtained;
	/* the buffer must be twice as big as the biggest number of samples processed/consumed */
	self->buffer_size = max * sizeof(int16_t) * NUM_CHANNELS * 2;
	if(!(self->buffer = malloc(self->buffer_size))) return 0;
	self->buffer_used = 0;
	SDL_PauseAudio(0);
	return 1;
}

int SdlWriter_close(struct SdlWriter *self) {
	SDL_CloseAudio();
	free(self->buffer);
	self->buffer = 0;
	self->buffer_size = 0;
	pthread_mutex_destroy(&self->cb_lock);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	return 1;
}
