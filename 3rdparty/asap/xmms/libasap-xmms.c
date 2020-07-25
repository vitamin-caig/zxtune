/*
 * libasap-xmms.c - ASAP plugin for XMMS
 *
 * Copyright (C) 2006-2011  Piotr Fusik
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

#include <pthread.h>
#include <string.h>
#ifdef USE_STDIO
#include <stdio.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

#include <xmms/plugin.h>
#include <xmms/titlestring.h>
#include <xmms/util.h>

#include "asap.h"

#define BITS_PER_SAMPLE  16
#define BUFFERED_BLOCKS  512

static int channels;

static InputPlugin mod;

static unsigned char module[ASAPInfo_MAX_MODULE_LENGTH];
static int module_len;
static ASAP *asap;

static volatile int seek_to;
static pthread_t thread_handle;
static volatile cibool thread_run = FALSE;
static volatile cibool generated_eof = FALSE;

static char *asap_stpcpy(char *dest, const char *src)
{
	size_t len = strlen(src);
	memcpy(dest, src, len);
	return dest + len;
}

static void asap_show_message(gchar *title, gchar *text)
{
	xmms_show_message(title, text, "Ok", FALSE, NULL, NULL);
}

static void asap_init(void)
{
	asap = ASAP_New();
}

static void asap_about(void)
{
	asap_show_message("About ASAP XMMS plugin " ASAPInfo_VERSION,
		ASAPInfo_CREDITS "\n" ASAPInfo_COPYRIGHT);
}

static int asap_is_our_file(char *filename)
{
	return ASAPInfo_IsOurFile(filename);
}

static cibool asap_load_file(const char *filename)
{
#ifdef USE_STDIO
	FILE *fp;
	fp = fopen(filename, "rb");
	if (fp == NULL)
		return FALSE;
	module_len = (int) fread(module, 1, sizeof(module), fp);
	fclose(fp);
#else
	int fd;
	fd = open(filename, O_RDONLY);
	if (fd == -1)
		return FALSE;
	module_len = read(fd, module, sizeof(module));
	close(fd);
	if (module_len < 0)
		return FALSE;
#endif
	return TRUE;
}

static ASAPInfo *asap_get_info(const char *filename)
{
	ASAPInfo *info;
	if (!asap_load_file(filename))
		return NULL;
	info = ASAPInfo_New();
	if (info == NULL)
		return NULL;
	if (!ASAPInfo_Load(info, filename, module, module_len)) {
		ASAPInfo_Delete(info);
		return NULL;
	}
	return info;
}

static char *asap_get_title(const char *filename, const ASAPInfo *info)
{
	char *path;
	char *filepart;
	char *ext;
	TitleInput *title_input;
	const char *p;
	int year;
	char *title;

	path = g_strdup(filename);
	filepart = strrchr(path, '/');
	if (filepart != NULL) {
		filepart[1] = '\0';
		filepart += 2;
	}
	else
		filepart = path;
	ext = strrchr(filepart, '.');
	if (ext != NULL)
		ext++;

	XMMS_NEW_TITLEINPUT(title_input);
	p = ASAPInfo_GetAuthor(info);
	if (p[0] != '\0')
		title_input->performer = (gchar *) p;
	title_input->track_name = (gchar *) ASAPInfo_GetTitleOrFilename(info);
	year = ASAPInfo_GetYear(info);
	if (year > 0)
		title_input->year = year;
	p = ASAPInfo_GetDate(info);
	if (p[0] != '\0')
		title_input->date = (gchar *) p;
	title_input->file_name = g_basename(filename);
	title_input->file_ext = ext;
	title_input->file_path = path;
	title = xmms_get_titlestring(xmms_get_gentitle_format(), title_input);
	if (title == NULL)
		title = g_strdup(ASAPInfo_GetTitleOrFilename(info));

	g_free(path);
	return title;
}

static void *asap_play_thread(void *arg)
{
	while (thread_run) {
		static unsigned char buffer[BUFFERED_BLOCKS * (BITS_PER_SAMPLE / 8) * 2];
		int buffered_bytes;
		if (generated_eof) {
			xmms_usleep(10000);
			continue;
		}
		if (seek_to >= 0) {
			mod.output->flush(seek_to);
			ASAP_Seek(asap, seek_to);
			seek_to = -1;
		}
		buffered_bytes = BUFFERED_BLOCKS * channels * (BITS_PER_SAMPLE / 8);
		buffered_bytes = ASAP_Generate(asap, buffer, buffered_bytes,
			BITS_PER_SAMPLE == 8 ? ASAPSampleFormat_U8 : ASAPSampleFormat_S16_L_E);
		if (buffered_bytes == 0) {
			generated_eof = TRUE;
			mod.output->buffer_free();
			mod.output->buffer_free();
			continue;
		}
		mod.add_vis_pcm(mod.output->written_time(),
			BITS_PER_SAMPLE == 8 ? FMT_U8 : FMT_S16_LE,
			channels, buffered_bytes, buffer);
		while (thread_run && mod.output->buffer_free() < buffered_bytes)
			xmms_usleep(20000);
		if (thread_run)
			mod.output->write_audio(buffer, buffered_bytes);
	}
	pthread_exit(NULL);
}

static void asap_play_file(char *filename)
{
	const ASAPInfo *info;
	int song;
	int duration;
	char *title;
	if (asap == NULL)
		return;
	if (!asap_load_file(filename))
		return;
	if (!ASAP_Load(asap, filename, module, module_len))
		return;
	info = ASAP_GetInfo(asap);
	song = ASAPInfo_GetDefaultSong(info);
	duration = ASAPInfo_GetDuration(info, song);
	if (!ASAP_PlaySong(asap, song, duration))
		return;
	channels = ASAPInfo_GetChannels(info);
	if (!mod.output->open_audio(BITS_PER_SAMPLE == 8 ? FMT_U8 : FMT_S16_LE, ASAP_SAMPLE_RATE, channels))
		return;
	title = asap_get_title(filename, info);
	mod.set_info(title, duration, BITS_PER_SAMPLE * 1000, ASAP_SAMPLE_RATE, channels);
	g_free(title);
	seek_to = -1;
	thread_run = TRUE;
	generated_eof = FALSE;
	pthread_create(&thread_handle, NULL, asap_play_thread, NULL);
}

static void asap_seek(int time)
{
	seek_to = time * 1000;
	generated_eof = FALSE;
	while (thread_run && seek_to >= 0)
		xmms_usleep(10000);
}

static void asap_pause(short paused)
{
	mod.output->pause(paused);
}

static void asap_stop(void)
{
	if (thread_run) {
		thread_run = FALSE;
		pthread_join(thread_handle, NULL);
		mod.output->close_audio();
	}
}

static int asap_get_time(void)
{
	if (!thread_run || (generated_eof && !mod.output->buffer_playing()))
		return -1;
	return mod.output->output_time();
}

static void asap_get_song_info(char *filename, char **title, int *length)
{
	ASAPInfo *info = asap_get_info(filename);
	if (info == NULL)
		return;
	*title = asap_get_title(filename, info);
	*length = ASAPInfo_GetDuration(info, ASAPInfo_GetDefaultSong(info));
	ASAPInfo_Delete(info);
}

static void asap_file_info_box(char *filename)
{
	ASAPInfo *info = asap_get_info(filename);
	char s[3 * ASAPInfo_MAX_TEXT_LENGTH + 100];
	char *p;
	if (info == NULL)
		return;
	p = asap_stpcpy(s, "Author: ");
	p = asap_stpcpy(p, ASAPInfo_GetAuthor(info));
	p = asap_stpcpy(p, "\nName: ");
	p = asap_stpcpy(p, ASAPInfo_GetTitle(info));
	p = asap_stpcpy(p, "\nDate: ");
	p = asap_stpcpy(p, ASAPInfo_GetDate(info));
	*p = '\0';
	ASAPInfo_Delete(info);
	asap_show_message("File information", s);
}

static InputPlugin mod = {
	NULL, NULL,
	"ASAP " ASAPInfo_VERSION,
	asap_init,
	asap_about,
	NULL,
	asap_is_our_file,
	NULL,
	asap_play_file,
	asap_stop,
	asap_pause,
	asap_seek,
	NULL,
	asap_get_time,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	asap_get_song_info,
	asap_file_info_box,
	NULL
};

InputPlugin *get_iplugin_info(void)
{
	return &mod;
}
