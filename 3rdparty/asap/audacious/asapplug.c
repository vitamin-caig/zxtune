/*
 * asapplug.c - ASAP plugin for Audacious
 *
 * Copyright (C) 2010-2013  Piotr Fusik
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

#include <stdio.h>
#include <pthread.h>
#include <glib.h>
#include <audacious/plugin.h>
#include <libaudcore/audstrings.h>

#if !defined(_AUD_PLUGIN_VERSION) && defined(__AUDACIOUS_PLUGIN_API__)
#define _AUD_PLUGIN_VERSION  __AUDACIOUS_PLUGIN_API__
#define _AUD_PLUGIN_VERSION_MIN  __AUDACIOUS_PLUGIN_API__
#endif
#if _AUD_PLUGIN_VERSION_MIN < 40
#include <gtk/gtk.h>
#endif

#include "asap.h"

#define BITS_PER_SAMPLE  16

static pthread_mutex_t control_mutex = PTHREAD_MUTEX_INITIALIZER;
static ASAP *asap;

#if _AUD_PLUGIN_VERSION >= 18
static gboolean playing;
#else
#define playing playback->playing
#endif

static
#if _AUD_PLUGIN_VERSION >= 18
	gboolean
#else
	void
#endif
	plugin_init(void)
{
	asap = ASAP_New();
#if _AUD_PLUGIN_VERSION >= 18
	return asap != NULL;
#endif
}

static void plugin_cleanup(void)
{
	ASAP_Delete(asap);
}

#if _AUD_PLUGIN_VERSION_MIN < 40
static void plugin_about(void)
{
	static GtkWidget *aboutbox = NULL;
	if (aboutbox == NULL) {
		aboutbox = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s",
			ASAPInfo_CREDITS "\n" ASAPInfo_COPYRIGHT);
		gtk_window_set_title((GtkWindow *) aboutbox, "About ASAP plugin " ASAPInfo_VERSION);
		g_signal_connect(aboutbox, "response", (GCallback) gtk_widget_destroy, NULL);
		g_signal_connect(aboutbox, "destroy", (GCallback) gtk_widget_destroyed, &aboutbox);
	}
	gtk_window_present((GtkWindow *) aboutbox);
}
#endif

static gboolean is_our_file_from_vfs(const char *filename, VFSFile *file)
{
	return ASAPInfo_IsOurFile(filename);
}

static int load_module(const char *filename, VFSFile *file, unsigned char *module)
{
	int module_len;
	if (file != NULL)
		return vfs_fread(module, 1, ASAPInfo_MAX_MODULE_LENGTH, file);
	file = vfs_fopen(filename, "rb");
	if (file == NULL)
		return -1;
	module_len = vfs_fread(module, 1, ASAPInfo_MAX_MODULE_LENGTH, file);
	vfs_fclose(file);
	return module_len;
}

#if _AUD_PLUGIN_VERSION >= 37
static char *filename_split_subtune(const char *filename, int *song)
{
	const char *sub;
	uri_parse(filename, NULL, NULL, &sub, song);
	return g_strndup(filename, sub - filename);
}
#else
#define tuple_set_int  tuple_associate_int
#define tuple_set_str  tuple_associate_string
#endif

static void tuple_set_nonblank(Tuple *tuple, int nfield, const char *value)
{
	if (value[0] != '\0')
		tuple_set_str(tuple, nfield, NULL, value);
}

static Tuple *probe_for_tuple(const char *filename, VFSFile *file)
{
	int song = -1;
	unsigned char module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len;
	ASAPInfo *info = NULL;
	Tuple *tuple = NULL;
	int songs;
	int duration;
	int year;

#if _AUD_PLUGIN_VERSION >= 10
	char *real_filename = filename_split_subtune(filename, &song);
	if (real_filename != NULL)
		filename = real_filename;
#endif
	module_len = load_module(filename, file, module);
	if (module_len > 0) {
		info = ASAPInfo_New();
		if (info != NULL && ASAPInfo_Load(info, filename, module, module_len))
			tuple = tuple_new_from_filename(filename);
	}
#if _AUD_PLUGIN_VERSION >= 10
	g_free(real_filename);
#endif
	if (tuple == NULL) {
		ASAPInfo_Delete(info);
		return NULL;
	}

	tuple_set_nonblank(tuple, FIELD_ARTIST, ASAPInfo_GetAuthor(info));
	tuple_set_nonblank(tuple, FIELD_TITLE, ASAPInfo_GetTitleOrFilename(info));
	tuple_set_nonblank(tuple, FIELD_DATE, ASAPInfo_GetDate(info));
	tuple_set_str(tuple, FIELD_CODEC, NULL, "ASAP");
	songs = ASAPInfo_GetSongs(info);
	if (song > 0) {
		tuple_set_int(tuple, FIELD_SUBSONG_ID, NULL, song);
		tuple_set_int(tuple, FIELD_SUBSONG_NUM, NULL, songs);
		song--;
	}
	else {
		if (songs > 1) {
#if _AUD_PLUGIN_VERSION >= 37
			tuple_set_subtunes(tuple, songs, NULL);
#else
			tuple->nsubtunes = songs;
#endif
		}
		song = ASAPInfo_GetDefaultSong(info);
	}
	duration = ASAPInfo_GetDuration(info, song);
	if (duration > 0)
		tuple_set_int(tuple, FIELD_LENGTH, NULL, duration);
	year = ASAPInfo_GetYear(info);
	if (year > 0)
		tuple_set_int(tuple, FIELD_YEAR, NULL, year);
	ASAPInfo_Delete(info);
	return tuple;
}

#if _AUD_PLUGIN_VERSION < 18
static Tuple *get_song_tuple(const char *filename)
{
	return probe_for_tuple(filename, NULL);
}
#endif

static gboolean play_start(InputPlayback *playback, const char *filename, VFSFile *file, int start_time, int stop_time, gboolean pause)
{
	int song = -1;
	unsigned char module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len;
	gboolean ok;
	const ASAPInfo *info;
	int channels;

#if _AUD_PLUGIN_VERSION >= 10
	char *real_filename = filename_split_subtune(filename, &song);
	if (real_filename != NULL)
		filename = real_filename;
#endif
	module_len = load_module(filename, file, module);
	ok = module_len > 0 && ASAP_Load(asap, filename, module, module_len);
#if _AUD_PLUGIN_VERSION >= 10
	g_free(real_filename);
#endif
	if (!ok)
		return FALSE;

	info = ASAP_GetInfo(asap);
	channels = ASAPInfo_GetChannels(info);
	if (song > 0)
		song--;
	else
		song = ASAPInfo_GetDefaultSong(info);
	if (stop_time < 0)
		stop_time = ASAPInfo_GetDuration(info, song);
	if (!ASAP_PlaySong(asap, song, stop_time))
		return FALSE;
	if (start_time > 0)
		ASAP_Seek(asap, start_time);

	if (!playback->output->open_audio(BITS_PER_SAMPLE == 8 ? FMT_U8 : FMT_S16_LE, ASAP_SAMPLE_RATE, channels))
		return FALSE;
	playback->set_params(playback,
#if _AUD_PLUGIN_VERSION < 18
		NULL, 0,
#endif
		0, ASAP_SAMPLE_RATE, channels);
	if (pause)
		playback->output->pause(TRUE);
	playing = TRUE;
	playback->set_pb_ready(playback);

	for (;;) {
		static unsigned char buffer[4096];
		int len;
		pthread_mutex_lock(&control_mutex);
		if (!playing) {
			pthread_mutex_unlock(&control_mutex);
			break;
		}
		len = ASAP_Generate(asap, buffer, sizeof(buffer), BITS_PER_SAMPLE == 8 ? ASAPSampleFormat_U8 : ASAPSampleFormat_S16_L_E);
		pthread_mutex_unlock(&control_mutex);
		if (len <= 0) {
#if _AUD_PLUGIN_VERSION < 18
			playback->eof = TRUE;
#endif
			break;
		}
#if _AUD_PLUGIN_VERSION >= 14
		playback->output->write_audio(buffer, len);
#else
		playback->pass_audio(playback, BITS_PER_SAMPLE == 8 ? FMT_U8 : FMT_S16_LE, channels, len, buffer, NULL);
#endif
	}

#if _AUD_PLUGIN_VERSION_MIN < 40
	while (playing && playback->output->buffer_playing())
		g_usleep(10000);
#endif
	pthread_mutex_lock(&control_mutex);
	playing = FALSE;
	pthread_mutex_unlock(&control_mutex);
#if _AUD_PLUGIN_VERSION_MIN < 40
	playback->output->close_audio();
#endif
	return TRUE;
}

#if _AUD_PLUGIN_VERSION < 18
static void play_file(InputPlayback *playback)
{
	play_start(playback, playback->filename, NULL, 0, -1, FALSE);
}
#endif

static void play_pause(InputPlayback *playback,
#if _AUD_PLUGIN_VERSION >= 18
	gboolean
#else
	gshort
#endif
	pause)
{
	pthread_mutex_lock(&control_mutex);
	if (playing)
		playback->output->pause(pause);
	pthread_mutex_unlock(&control_mutex);
}

static void play_mseek(InputPlayback *playback,
#if _AUD_PLUGIN_VERSION >= 18
	int
#else
	gulong
#endif
	time)
{
	pthread_mutex_lock(&control_mutex);
	if (playing) {
		ASAP_Seek(asap, time);
#if _AUD_PLUGIN_VERSION >= 15
		playback->output->abort_write();
#endif
	}
	pthread_mutex_unlock(&control_mutex);
}

static void play_stop(InputPlayback *playback)
{
	pthread_mutex_lock(&control_mutex);
	if (playing) {
#if _AUD_PLUGIN_VERSION >= 15
		playback->output->abort_write();
#endif
		playing = FALSE;
	}
	pthread_mutex_unlock(&control_mutex);
}

#pragma GCC diagnostic ignored "-Wunused-result"
static void write_byte(void *obj, int data)
{
	VFSFile *file = (VFSFile *) obj;
	const char buf[1] = { data };
	vfs_fwrite(buf, 1, 1, file);
}

static gboolean update_song_tuple(const Tuple * tuple, VFSFile *file)
{
#if _AUD_PLUGIN_VERSION < 38
#define vfs_get_filename(file)  g_filename_from_uri(file->uri, NULL, NULL)
#define tuple_get_str  tuple_get_string
#define str_unref(s)
	const char *s;
#else
	char *s;
#endif
	/* read file */
	const char *filename = vfs_get_filename(file);
	unsigned char module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len = load_module(filename, file, module);
	ASAPInfo *info;
	int year;
	ByteWriter bw;
	gboolean ok;
	if (module_len <= 0)
		return FALSE;
	info = ASAPInfo_New();
	if (info == NULL)
		return FALSE;
	if (!ASAPInfo_Load(info, filename, module, module_len)) {
		ASAPInfo_Delete(info);
		return FALSE;
	}

	/* apply new tags */
	s = tuple_get_str(tuple, FIELD_ARTIST, NULL);
	if (s != NULL) {
		if (!ASAPInfo_SetAuthor(info, s)) {
			str_unref(s);
			ASAPInfo_Delete(info);
			return FALSE;
		}
		str_unref(s);
	}
	else
		ASAPInfo_SetAuthor(info, "");
	s = tuple_get_str(tuple, FIELD_TITLE, NULL);
	if (s != NULL) {
		if (!ASAPInfo_SetTitle(info, s)) {
			str_unref(s);
			ASAPInfo_Delete(info);
			return FALSE;
		}
		str_unref(s);
	}
	else
		ASAPInfo_SetTitle(info, "");
	year = tuple_get_int(tuple, FIELD_YEAR, NULL);
	if (year == 0)
		year = -1;
	/* check if year changed so that we don't lose other date parts */
	if (year != ASAPInfo_GetYear(info)) {
		if (year <= 0)
			ASAPInfo_SetDate(info, "");
		else {
			char d[16];
			sprintf(d, "%d", year);
			ASAPInfo_SetDate(info, d);
		}
	}

	/* write file */
	vfs_rewind(file);
	bw.obj = file;
	bw.func = write_byte;
	ok = ASAPWriter_Write(filename, bw, info, module, module_len, TRUE) && vfs_ftruncate(file, vfs_ftell(file)) == 0;
	ASAPInfo_Delete(info);
	return ok;
}

static
#if _AUD_PLUGIN_VERSION >= 16
	const
#endif
	char *exts[] = { "sap", "cmc", "cm3", "cmr", "cms", "dmc", "dlt", "mpt", "mpd", "rmt", "tmc", "tm8", "tm2", "fc", NULL };

#ifdef _WIN32
/* For some strange reason, it doesn't get exported without it. It wasn't necessary for Audacious 3.2... */
__declspec(dllexport) InputPlugin *get_plugin_info(AudAPITable *);
#endif

#if _AUD_PLUGIN_VERSION >= 30

AUD_INPUT_PLUGIN
(
	.name = "ASAP",
	.init = plugin_init,
	.cleanup = plugin_cleanup,
#if _AUD_PLUGIN_VERSION_MIN >= 40
	.about_text = "ASAP " ASAPInfo_VERSION "\n" ASAPInfo_CREDITS "\n" ASAPInfo_COPYRIGHT,
#else
	.about = plugin_about,
#endif
	.have_subtune = TRUE,
	.extensions = exts,
	.is_our_file_from_vfs = is_our_file_from_vfs,
	.probe_for_tuple = probe_for_tuple,
	.play = play_start,
	.pause = play_pause,
	.mseek = play_mseek,
	.stop = play_stop,
	.update_song_tuple = update_song_tuple
);

#else

static InputPlugin asap_ip = {
	.description = "ASAP Plugin",
	.init = plugin_init,
	.cleanup = plugin_cleanup,
	.about = plugin_about,
	.have_subtune = TRUE,
	.vfs_extensions = exts,
	.is_our_file_from_vfs = is_our_file_from_vfs,
#if _AUD_PLUGIN_VERSION >= 16
	.probe_for_tuple = probe_for_tuple,
	.play = play_start,
#endif
#if _AUD_PLUGIN_VERSION < 18
	.get_song_tuple = get_song_tuple,
	.play_file = play_file,
#endif
	.pause = play_pause,
	.mseek = play_mseek,
	.stop = play_stop,
	.update_song_tuple = update_song_tuple
};

static InputPlugin *asap_iplist[] = { &asap_ip, NULL };

SIMPLE_INPUT_PLUGIN(ASAP, asap_iplist)

#endif
