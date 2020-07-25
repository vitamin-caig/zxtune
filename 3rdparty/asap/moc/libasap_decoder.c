/*
 * libasap_decoder.c - ASAP plugin for MOC (Music On Console)
 *
 * Copyright (C) 2007-2013  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "common.h"
#include "decoder.h"
#include "files.h"

#include "asap.h"

#define BITS_PER_SAMPLE      16
#define DEFAULT_SONG_LENGTH  -1

typedef struct {
	ASAP *asap;
	int duration;
	struct decoder_error error;
} ASAP_Decoder;

static cibool asap_load(ASAP_Decoder *d, const char *filename)
{
	struct io_stream *s;
	ssize_t module_len;
	unsigned char *module;
	cibool ok;
	const ASAPInfo *info;
	int song;
	int duration;

	d->asap = NULL;
	decoder_error_init(&d->error);
	s = io_open(filename, 0);
	if (s == NULL) {
		decoder_error(&d->error, ERROR_FATAL, 0, "Can't open %s", filename);
		return FALSE;
	}
	module_len = io_file_size(s);
	module = (unsigned char *) xmalloc(module_len);
	module_len = io_read(s, module, module_len);
	io_close(s);

	d->asap = ASAP_New();
	if (d->asap == NULL) {
		decoder_error(&d->error, ERROR_FATAL, 0, "Out of memory");
		return FALSE;
	}

	ok = ASAP_Load(d->asap, filename, module, module_len);
	free(module);
	if (!ok) {
		decoder_error(&d->error, ERROR_FATAL, 0, "Unsupported file format");
		return FALSE;
	}
	info = ASAP_GetInfo(d->asap);
	song = ASAPInfo_GetDefaultSong(info);
	duration = ASAPInfo_GetDuration(info, song);
	if (duration < 0)
		duration = DEFAULT_SONG_LENGTH * 1000;
	d->duration = duration;
	return TRUE;
}

static void asap_close(void *data)
{
	ASAP_Decoder *d = (ASAP_Decoder *) data;
	ASAP_Delete(d->asap);
	free(d);
}

static void *asap_open(const char *uri)
{
	ASAP_Decoder *d = (ASAP_Decoder *) xmalloc(sizeof(ASAP_Decoder));
	if (!asap_load(d, uri) || !ASAP_PlaySong(d->asap, ASAPInfo_GetDefaultSong(ASAP_GetInfo(d->asap)), d->duration)) {
		asap_close(d);
		return NULL;
	}
	return d;
}

static int asap_decode(void *data, char *buf, int buf_len, struct sound_params *sound_params)
{
	ASAP_Decoder *d = (ASAP_Decoder *) data;
	sound_params->channels = ASAPInfo_GetChannels(ASAP_GetInfo(d->asap));
	sound_params->rate = ASAP_SAMPLE_RATE;
	sound_params->fmt = BITS_PER_SAMPLE == 8 ? SFMT_U8 : (SFMT_S16 | SFMT_LE);
	return ASAP_Generate(d->asap, (unsigned char *) buf, buf_len, BITS_PER_SAMPLE == 8 ? ASAPSampleFormat_U8 : ASAPSampleFormat_S16_L_E);
}

static int asap_seek(void *data, int sec)
{
	ASAP_Decoder *d = (ASAP_Decoder *) data;
	ASAP_Seek(d->asap, sec * 1000);
	return sec;
}

static void asap_info(const char *file, struct file_tags *tags, const int tags_sel)
{
	ASAP_Decoder d;
	if (asap_load(&d, file)) {
		if ((tags_sel & TAGS_COMMENTS) != 0) {
			const ASAPInfo *info = ASAP_GetInfo(d.asap);
			tags->title = xstrdup(ASAPInfo_GetTitleOrFilename(info));
			tags->artist = xstrdup(ASAPInfo_GetAuthor(info));
			tags->filled |= TAGS_COMMENTS;
		}
		if ((tags_sel & TAGS_TIME) != 0) {
			tags->time = d.duration / 1000;
			tags->filled |= TAGS_TIME;
		}
	}
	ASAP_Delete(d.asap);
}

static int asap_get_bitrate(void *data)
{
	return -1;
}

static int asap_get_duration(void *data)
{
	const ASAP_Decoder *d = (const ASAP_Decoder *) data;
	return d->duration / 1000;
}

static void asap_get_error(void *data, struct decoder_error *error)
{
	ASAP_Decoder *d = (ASAP_Decoder *) data;
	decoder_error_copy(error, &d->error);
}

static int asap_our_format_ext(const char *ext)
{
	return ASAPInfo_IsOurExt(ext);
}

static void asap_get_name(const char *file, char buf[4])
{
	const char *ext = ext_pos(file);
	int i = 0;
	while (ext[i] != '\0' && i < 3) {
		int c = ext[i];
		if (c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		buf[i++] = c;
	}
	buf[i] = '\0';
}

struct decoder *plugin_init()
{
	static struct decoder asap_decoder = {
		DECODER_API_VERSION,
		NULL,
		NULL,
		asap_open,
		NULL,
		NULL,
		asap_close,
		asap_decode,
		asap_seek,
		asap_info,
		asap_get_bitrate,
		asap_get_duration,
		asap_get_error,
		asap_our_format_ext,
		NULL,
		asap_get_name,
		NULL,
		NULL
#if DECODER_API_VERSION >= 7
		, NULL
#endif
	};
	return &asap_decoder;
}
