/*
 * asapconv.c - converter of ASAP-supported formats
 *
 * Copyright (C) 2005-2014  Piotr Fusik
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
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
#include <fcntl.h>
#ifdef _MSC_VER
#define strcasecmp _stricmp
#include <io.h>
#endif
#endif

#ifdef HAVE_LIBMP3LAME
#define SAMPLE_FORMATS "WAV, RAW or MP3"
#ifndef HAVE_LIBMP3LAME_DLL
#include <lame.h>
#endif
#else
#define SAMPLE_FORMATS "WAV or RAW"
#endif

#include "asap.h"

/* parsed command line */
static const char *arg_output = NULL;
static int arg_song = -1;
static ASAPSampleFormat arg_sample_format = ASAPSampleFormat_S16_L_E;
static int arg_duration = -1;
static int arg_mute_mask = 0;
static const char *arg_author = NULL;
static const char *arg_name = NULL;
static const char *arg_date = NULL;
static cibool arg_tag = FALSE;
static int arg_music_address = -1;

static int current_song;
static char output_file[FILENAME_MAX];

static void print_help(void)
{
	printf(
		"Usage: asapconv [OPTIONS] INPUTFILE...\n"
		"Each INPUTFILE must be in a supported format:\n"
		"SAP, CMC, CM3, CMR, CMS, DMC, DLT, MPT, MPD, RMT, TMC, TM8, TM2 or FC.\n"
		"Output EXT must be one of the above or XEX, " SAMPLE_FORMATS ".\n"
		"Options:\n"
		"-o FILE.EXT --output=FILE.EXT  Write to the specified file\n"
		"-o .EXT     --output=.EXT      Use input file path and name\n"
		"-o DIR/.EXT --output=DIR/.EXT  Write to the specified directory\n"
		"-o -.EXT    --output=-.EXT     Write to standard output\n"
		"-a \"TEXT\"   --author=\"TEXT\"    Set author name\n"
		"-n \"TEXT\"   --name=\"TEXT\"      Set music name\n"
		"-d \"TEXT\"   --date=\"TEXT\"      Set music creation date (DD/MM/YYYY format)\n"
		"-h          --help             Display this information\n"
		"-v          --version          Display version information\n"
		"In FILE, DIR and EXT you may use the following placeholders:\n"
		"%%a                             Music author\n"
		"%%n                             Music name\n"
		"%%d                             Music creation date\n"
		"%%e                             Original extension (e.g. \"cmc\")\n"
		"%%s                             Subsong number (1-based), one file per subsong\n"
		"Options for XEX, " SAMPLE_FORMATS " output:\n"
		"-s SONG     --song=SONG        Select subsong number (zero-based)\n"
		"-t TIME     --time=TIME        Set output length (MM:SS format)\n"
		"Options for XEX, WAV "
#ifdef HAVE_LIBMP3LAME
		                "or MP3 "
#endif
		                       "output:\n"
		"            --tag              Include metadata in the output file\n"
		"Options for " SAMPLE_FORMATS " output:\n"
		"-m CHANNELS --mute=CHANNELS    Mute POKEY channels (1-8, comma-separated)\n"
#ifdef HAVE_LIBMP3LAME
		"Options for WAV or RAW output:\n"
#endif
		"-b          --byte-samples     Output 8-bit samples\n"
		"-w          --word-samples     Output 16-bit samples (default)\n"
		"Options for SAP output:\n"
		"-s SONG     --song=SONG        Select subsong to set length of\n"
		"-t TIME     --time=TIME        Set subsong length (MM:SS format)\n"
		"Options for native modules (output format same as input format):\n"
		"            --address=HEXNUM   Relocate music to the given address\n"
	);
}

static void fatal_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "asapconv: ");
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);
	exit(1);
}

static int parse_int(const char *s, int base, const char *description, int max_value)
{
	char *e;
	int result;
	if (s[0] == '\0')
		fatal_error("invalid %s", description);
	result = (int) strtol(s, &e, base);
	if (e[0] != '\0' || result < 0 || result > max_value)
		fatal_error("invalid %s", description);
	return result;
}

static void set_song(const char *s)
{
	arg_song = parse_int(s, 10, "subsong number", ASAPInfo_MAX_SONGS - 1);
}

static void set_time(const char *s)
{
	arg_duration = ASAPInfo_ParseDuration(s);
	if (arg_duration <= 0)
		fatal_error("invalid time format");
}

static void set_mute_mask(const char *s)
{
	while (*s != '\0') {
		if (*s >= '1' && *s <= '8')
			arg_mute_mask |= 1 << (*s - '1');
		s++;
	}
}

static void set_music_address(const char *s)
{
	if (s[0] == '$')
		s++;
	arg_music_address = parse_int(s, 16, "music address", 0xffff);
}

static void apply_tags(const char *input_file, ASAPInfo *info)
{
	if (arg_author != NULL) {
		if (!ASAPInfo_SetAuthor(info, arg_author))
			fatal_error("invalid author");
	}
	if (arg_name != NULL) {
		if (!ASAPInfo_SetTitle(info, arg_name))
			fatal_error("invalid music name");
		arg_name = NULL;
	}
	if (arg_date != NULL) {
		if (!ASAPInfo_SetDate(info, arg_date))
			fatal_error("invalid date");
	}
}

static ASAP *load_module(const char *input_file, const unsigned char *module, int module_len)
{
	ASAP *asap = ASAP_New();
	ASAPInfo *info;
	if (asap == NULL)
		fatal_error("out of memory");
	if (!ASAP_Load(asap, input_file, module, module_len))
		fatal_error("%s: unsupported file", input_file);
	info = (ASAPInfo *) ASAP_GetInfo(asap); /* FIXME: avoid cast */
	apply_tags(input_file, info);
	return asap;
}

static FILE *open_output_file(const char *input_file, const unsigned char *module, int module_len, const ASAPInfo *info, cibool allow_file_per_song)
{
	const char *output_ext = strrchr(arg_output, '.');
	cibool file_per_song = FALSE;
	const char *pattern_ptr;
	char *output_ptr;
	FILE *fp;
	if (output_ext == arg_output) {
		/* .EXT */
	}
	else if (output_ext == arg_output + 1 && arg_output[0] == '-') {
		/* -.EXT */
		strcpy(output_file, "stdout");
#ifdef _WIN32
		_setmode(_fileno(stdout), _O_BINARY);
#endif
		return stdout;
	}
	else if (output_ext[-1] == '/' || output_ext[-1] == '\\') {
		/* DIR/.EXT */
		const char *p;
		for (p = input_file; *p != '\0'; p++)
			if (*p == '/' || *p == '\\')
				input_file = p + 1;
	}
	else {
		/* FILE.EXT */
		output_ext = NULL; /* don't insert input_file */
	}

	output_ptr = output_file;
	for (pattern_ptr = arg_output; *pattern_ptr != '\0'; pattern_ptr++) {
		char c;
		if (pattern_ptr == output_ext) {
			/* insert input_file without the extension */
			size_t len = strrchr(input_file, '.') - input_file;
			if (output_ptr + len >= output_file + sizeof(output_file))
				fatal_error("filename too long");
			memcpy(output_ptr, input_file, len);
			output_ptr += len;
		}
		c = *pattern_ptr;
		if (c == '%') {
			const char *tag;
			char song_tag[16];
			c = *++pattern_ptr;
			switch (c) {
			case 'a':
				tag = ASAPInfo_GetAuthor(info);
				break;
			case 'n':
				tag = ASAPInfo_GetTitleOrFilename(info);
				break;
			case 'd':
				tag = ASAPInfo_GetDate(info);
				break;
			case 'e':
				tag = ASAPInfo_GetOriginalModuleExt(info, module, module_len);
				if (tag == NULL)
					tag = "unknown";
				break;
			case 's':
				if (!file_per_song) {
					if (!allow_file_per_song)
						fatal_error("%%s is invalid for this output format");
					if (arg_song >= 0)
						fatal_error("-s and %%s are mutually exclusive");
					if (++current_song >= ASAPInfo_GetSongs(info))
						return NULL;
					sprintf(song_tag, "%d", current_song + 1);
					file_per_song = TRUE;
				}
				tag = song_tag;
				break;
			case '%':
				tag = "%";
				break;
			default:
				fatal_error("unrecognized %%%c", c);
				return NULL;
			}
			while (*tag != '\0') {
				if (output_ptr >= output_file + sizeof(output_file) - 1)
					fatal_error("filename too long");
				c = *tag++;
				switch (c) {
				case '<':
				case '>':
				case ':':
				case '/':
				case '\\':
				case '|':
				case '?':
				case '*':
					c = '_';
					break;
				default:
					break;
				}
				*output_ptr++ = c;
			}
		}
		else {
			if (output_ptr >= output_file + sizeof(output_file) - 1)
				fatal_error("filename too long");
			*output_ptr++ = c;
		}
	}
	*output_ptr = '\0';

	if (!file_per_song) {
		if (current_song >= 0)
			return NULL;
		if (arg_song < 0)
			current_song = ASAPInfo_GetDefaultSong(info);
		else if (arg_song >= ASAPInfo_GetSongs(info)) {
			fatal_error("you have requested subsong %d ...\n"
				"... but %s contains only %d subsongs",
				arg_song, input_file, ASAPInfo_GetSongs(info));
		}
		else
			current_song = arg_song;
	}

	fp = fopen(output_file, "wb");
	if (fp == NULL)
		fatal_error("cannot write %s", output_file);
	return fp;
}

static int play_song(const char *input_file, ASAP *asap)
{
	int duration = arg_duration;
	if (duration < 0) {
		duration = ASAPInfo_GetDuration(ASAP_GetInfo(asap), current_song);
		if (duration < 0)
			duration = 180 * 1000;
	}
	if (!ASAP_PlaySong(asap, current_song, duration))
		fatal_error("%s: PlaySong failed", input_file);
	ASAP_MutePokeyChannels(asap, arg_mute_mask);
	return duration;
}

static void write_output_file(FILE *fp, unsigned char *buffer, int n_bytes)
{
	if (fwrite(buffer, 1, n_bytes, fp) != n_bytes) {
		fclose(fp);
		fatal_error("error writing to %s", output_file);
	}
}

static void close_output_file(FILE *fp)
{
	if (fp != stdout) {
		if (fclose(fp) != 0)
			fatal_error("error closing %s", output_file);
	}
}

static void convert_to_wav(const char *input_file, const unsigned char *module, int module_len, cibool output_header)
{
	ASAP *asap = load_module(input_file, module, module_len);
	FILE *fp;

	while ((fp = open_output_file(input_file, module, module_len, ASAP_GetInfo(asap), TRUE)) != NULL) {
		int n_bytes;
		unsigned char buffer[8192];

		play_song(input_file, asap);
		if (output_header) {
			n_bytes = ASAP_GetWavHeader(asap, buffer, arg_sample_format, arg_tag);
			fwrite(buffer, 1, n_bytes, fp);
		}
		do {
			n_bytes = ASAP_Generate(asap, buffer, sizeof(buffer), arg_sample_format);
			write_output_file(fp, buffer, n_bytes);
		} while (n_bytes == sizeof(buffer));
		close_output_file(fp);
	}

	ASAP_Delete(asap);
}

#ifdef HAVE_LIBMP3LAME

#ifdef HAVE_LIBMP3LAME_DLL

#include <windows.h>

typedef struct lame_global_struct *lame_global_flags;
typedef lame_global_flags *(*plame_init)(void);
typedef int (*plame_set_num_samples)(lame_global_flags *, unsigned long);
typedef int (*plame_set_in_samplerate)(lame_global_flags *, int);
typedef int (*plame_set_num_channels)(lame_global_flags *, int);
typedef int (*plame_init_params)(lame_global_flags *);
typedef int (*plame_encode_buffer_interleaved)(lame_global_flags *, short int[], int, unsigned char *, int);
typedef int (*plame_encode_flush)(lame_global_flags *, unsigned char *, int);
typedef int (*plame_close)(lame_global_flags *);
typedef int (*pid3tag_init)(lame_global_flags *);
typedef int (*pid3tag_set_title)(lame_global_flags *, const char *);
typedef int (*pid3tag_set_artist)(lame_global_flags *, const char *);
typedef int (*pid3tag_set_year)(lame_global_flags *, const char *);
typedef int (*pid3tag_set_genre)(lame_global_flags *, const char *);
#define LAME_OKAY 0

static HMODULE lame_load(void)
{
	HMODULE lame_dll = LoadLibrary("libmp3lame.dll");
	if (lame_dll != NULL)
		return lame_dll;
	lame_dll = LoadLibrary("lame_enc.dll");
	if (lame_dll != NULL)
		return lame_dll;
	fatal_error("libmp3lame.dll and lame_enc.dll not found");
	return NULL;
}

static FARPROC lame_proc(HMODULE lame_dll, const char *name)
{
	FARPROC proc = GetProcAddress(lame_dll, name);
	if (proc == NULL) {
		char dll_name[FILENAME_MAX];
		GetModuleFileName(lame_dll, dll_name, FILENAME_MAX);
		fatal_error("%s not found in %s", name, dll_name);
	}
	return proc;
}

#define LAME_FUNC(name) p##name name = (p##name) lame_proc(lame_dll, #name)

#endif

static void convert_to_mp3(const char *input_file, const unsigned char *module, int module_len)
{
	ASAP *asap = load_module(input_file, module, module_len);
	const ASAPInfo *info = ASAP_GetInfo(asap);
	int channels = ASAPInfo_GetChannels(info);
	FILE *fp;

#ifdef HAVE_LIBMP3LAME_DLL
	HMODULE lame_dll = lame_load();
	LAME_FUNC(lame_init);
	LAME_FUNC(lame_set_num_samples);
	LAME_FUNC(lame_set_in_samplerate);
	LAME_FUNC(lame_set_num_channels);
	LAME_FUNC(lame_init_params);
	LAME_FUNC(lame_encode_buffer_interleaved);
	LAME_FUNC(lame_encode_flush);
	LAME_FUNC(lame_close);
#endif

	while ((fp = open_output_file(input_file, module, module_len, info, TRUE)) != NULL) {
		int duration = play_song(input_file, asap);
		lame_global_flags *lame = lame_init();
		unsigned char buffer[8192];
		int n_bytes;
		unsigned char mp3buf[4096 * 5 / 4 + 7200]; /* it would be possible to reuse "buffer" instead */
		int mp3_bytes;

		if (lame == NULL)
			fatal_error("lame_init failed");
		if (lame_set_num_samples(lame, duration * (ASAP_SAMPLE_RATE / 100) / 10) != LAME_OKAY
		 || lame_set_in_samplerate(lame, ASAP_SAMPLE_RATE) != LAME_OKAY
		 || lame_set_num_channels(lame, channels) != LAME_OKAY)
			fatal_error("lame_set_* failed");
		if (lame_init_params(lame) != LAME_OKAY)
			fatal_error("lame_init_params failed");

		if (arg_tag) {
#ifdef HAVE_LIBMP3LAME_DLL
			LAME_FUNC(id3tag_init);
			LAME_FUNC(id3tag_set_title);
			LAME_FUNC(id3tag_set_artist);
			LAME_FUNC(id3tag_set_year);
			LAME_FUNC(id3tag_set_genre);
#endif
			const char *s;
			int year;
			id3tag_init(lame);
			s = ASAPInfo_GetTitle(info);
			if (s[0] != '\0')
				id3tag_set_title(lame, s);
			s = ASAPInfo_GetAuthor(info);
			if (s[0] != '\0')
				id3tag_set_artist(lame, s);
			year = ASAPInfo_GetYear(info);
			if (year > 0) {
				char year_string[16];
				sprintf(year_string, "%d", year);
				id3tag_set_year(lame, year_string);
			}
			id3tag_set_genre(lame, "Electronic");
		}

		do {
			short pcm[8192];
			int i;
			short *p = pcm;
			n_bytes = ASAP_Generate(asap, buffer, sizeof(buffer), ASAPSampleFormat_S16_L_E);
			for (i = 0; i < n_bytes; i += 2) {
				*p++ = buffer[i] + (buffer[i + 1] << 8);
				if (channels == 1)
					p++;
			}
			mp3_bytes = lame_encode_buffer_interleaved(lame, pcm, n_bytes >> channels, mp3buf, sizeof(mp3buf));
			if (mp3_bytes < 0)
				fatal_error("lame_encode_buffer_interleaved failed");
			write_output_file(fp, mp3buf, mp3_bytes);
		} while (n_bytes == sizeof(buffer));

		mp3_bytes = lame_encode_flush(lame, mp3buf, sizeof(mp3buf));
		if (mp3_bytes < 0)
			fatal_error("lame_encode_flush failed");
		write_output_file(fp, mp3buf, mp3_bytes);
		lame_close(lame);
		close_output_file(fp);
	}

	ASAP_Delete(asap);
}

#endif /* HAVE_LIBMP3LAME */

static void write_byte(void *obj, int data)
{
	putc(data, (FILE *) obj);
}

static void convert_to_module(const char *input_file, const unsigned char *module, int module_len, cibool output_xex)
{
	const char *input_ext = strrchr(input_file, '.');
	ASAPInfo *info;
	FILE *fp;

	if (input_ext == NULL)
		fatal_error("%s: missing extension", input_file);
	input_ext++;
	info = ASAPInfo_New();
	if (info == NULL)
		fatal_error("out of memory");
	if (!ASAPInfo_Load(info, input_file, module, module_len))
		fatal_error("%s: unsupported file", input_file);
	apply_tags(input_file, info);
	if (arg_music_address >= 0)
		ASAPInfo_SetMusicAddress(info, arg_music_address);

	while ((fp = open_output_file(input_file, module, module_len, info, output_xex)) != NULL) {
		ByteWriter bw;
		if (output_xex)
			ASAPInfo_SetDefaultSong(info, current_song);
		if (arg_duration >= 0)
			ASAPInfo_SetDuration(info, current_song, arg_duration);
		bw.obj = fp;
		bw.func = write_byte;
		/* FIXME: stdout */
		if (!ASAPWriter_Write(output_file, bw, info, module, module_len, arg_tag)) {
			if (fp != stdout) {
				fclose(fp);
				remove(output_file); /* "unlink" is less portable */
			}
			fatal_error("%s: conversion error", input_file);
		}
		close_output_file(fp);
	}

	ASAPInfo_Delete(info);
}

static void process_file(const char *input_file)
{
	FILE *fp;
	static unsigned char module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len;
	const char *output_ext;

	if (arg_output == NULL)
		fatal_error("the -o/--output option is mandatory");
	fp = fopen(input_file, "rb");
	if (fp == NULL)
		fatal_error("cannot open %s", input_file);
	module_len = fread(module, 1, sizeof(module), fp);
	fclose(fp);

	output_ext = strrchr(arg_output, '.');
	if (output_ext == NULL)
		fatal_error("missing .EXT in -o/--output");
	output_ext++;
	current_song = -1;
	if (strcasecmp(output_ext, "wav") == 0)
		convert_to_wav(input_file, module, module_len, TRUE);
	else if (strcasecmp(output_ext, "raw") == 0)
		convert_to_wav(input_file, module, module_len, FALSE);
	else if (strcasecmp(output_ext, "mp3") == 0) {
#ifdef HAVE_LIBMP3LAME
		convert_to_mp3(input_file, module, module_len);
#else
		fatal_error("this build of asapconv doesn't support MP3");
#endif
	}
	else
		convert_to_module(input_file, module, module_len, strcasecmp(output_ext, "xex") == 0);
}

int main(int argc, char *argv[])
{
	const char *options_error = "no input files";
	int i;
	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (arg[0] != '-') {
			process_file(arg);
			options_error = NULL;
			continue;
		}
		options_error = "options must be specified before the input file";
#define is_opt(c)  (arg[1] == c && arg[2] == '\0')
		if (is_opt('o'))
			arg_output = argv[++i];
		else if (strncmp(arg, "--output=", 9) == 0)
			arg_output = arg + 9;
		else if (is_opt('s'))
			set_song(argv[++i]);
		else if (strncmp(arg, "--song=", 7) == 0)
			set_song(arg + 7);
		else if (is_opt('t'))
			set_time(argv[++i]);
		else if (strncmp(arg, "--time=", 7) == 0)
			set_time(arg + 7);
		else if (is_opt('b') || strcmp(arg, "--byte-samples") == 0)
			arg_sample_format = ASAPSampleFormat_U8;
		else if (is_opt('w') || strcmp(arg, "--word-samples") == 0)
			arg_sample_format = ASAPSampleFormat_S16_L_E;
		else if (is_opt('m'))
			set_mute_mask(argv[++i]);
		else if (strncmp(arg, "--mute=", 7) == 0)
			set_mute_mask(arg + 7);
		else if (is_opt('a'))
			arg_author = argv[++i];
		else if (strncmp(arg, "--author=", 9) == 0)
			arg_author = arg + 9;
		else if (is_opt('n'))
			arg_name = argv[++i];
		else if (strncmp(arg, "--name=", 7) == 0)
			arg_name = arg + 7;
		else if (is_opt('d'))
			arg_date = argv[++i];
		else if (strncmp(arg, "--date=", 7) == 0)
			arg_date = arg + 7;
		else if (strcmp(arg, "--tag") == 0)
			arg_tag = TRUE;
		else if (strncmp(arg, "--address=", 10) == 0)
			set_music_address(arg + 10);
		else if (is_opt('h') || strcmp(arg, "--help") == 0) {
			print_help();
			options_error = NULL;
		}
		else if (is_opt('v') || strcmp(arg, "--version") == 0) {
			printf("asapconv " ASAPInfo_VERSION "\n");
			options_error = NULL;
		}
		else
			fatal_error("unknown option: %s", arg);
	}
	if (options_error != NULL) {
		fprintf(stderr, "asapconv: %s\n", options_error);
		print_help();
		return 1;
	}
	return 0;
}
