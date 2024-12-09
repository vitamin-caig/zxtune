#include <stdint.h>
#include <fcntl.h>
#include <stddef.h>
#include <unistd.h>

#include "wavewriter.h"
#include "wave_format.h"
#include "../include/endianness.h"

static const WAVE_HEADER_COMPLETE wave_hdr = {
	{
		{ 'R', 'I', 'F', 'F'},
		0,
		{ 'W', 'A', 'V', 'E'},
	},
	{
		{ 'f', 'm', 't', ' '},
		16,
		{1, 0},
		0,
		44100,
		0,
		0,
		16,
	},
	{
		{ 'd', 'a', 't', 'a' },
		0
	},
};

static int do_write(struct WaveWriter *self, void* buffer, size_t bufsize) {
	if(self->fd == -1) return 0;
	if(write(self->fd, buffer, bufsize) != (ssize_t) bufsize) {
		close(self->fd);
		self->fd = -1;
		return 0;
	}
	return 1;
}

int WaveWriter_init(struct WaveWriter *self, char* filename) {
	self->fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0660);
	return do_write(self, (void*) &wave_hdr, sizeof(wave_hdr));
}

int WaveWriter_write(struct WaveWriter *self, void* buffer, size_t bufsize) {
	int success = 0;
	if(do_write(self, buffer, bufsize)) {
		self->written += bufsize;
		success = 1;
	}
	return success;
}

int WaveWriter_close(struct WaveWriter *self) {
	const int channels = 2;
	const int samplerate = 44100;
	const int bitwidth = 16;
	
	WAVE_HEADER_COMPLETE hdr = wave_hdr;
	size_t size = self->written;
	
	if(self->fd == -1) return 0;
	if(lseek(self->fd, 0, SEEK_SET) == -1) {
		close(self->fd);
		return 0;
	}
	
	hdr.wave_hdr.channels = le16(channels);
	hdr.wave_hdr.samplerate = le32(samplerate);
	hdr.wave_hdr.bitwidth = le16(bitwidth);
	
	hdr.wave_hdr.blockalign = le16(channels * (bitwidth / 8));
	hdr.wave_hdr.bytespersec = le32(samplerate * channels * (bitwidth / 8));
	hdr.riff_hdr.filesize_minus_8 = le32(sizeof(WAVE_HEADER_COMPLETE) + size - 8);
	hdr.sub2.data_size = le32(size);
	
	if(!do_write(self, &hdr, sizeof(hdr))) return 0;
	
	close(self->fd);
	self->fd = -1;
	return 1;
}
