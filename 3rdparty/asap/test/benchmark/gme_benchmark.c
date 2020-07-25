/*
 * gme_benchmark.c - Game_Music_Emu benchmark
 *
 * Copyright (C) 2013  Piotr Fusik
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

#include "asap.h"
#include "gme.h"

#define BUF_SIZE 4096

static int get_stereo_samples_count(const char *filename)
{
	/* GME doesn't understand TIME so ask ASAP */
	FILE *fp;
	unsigned char content[ASAPInfo_MAX_MODULE_LENGTH];
	int content_len;
	ASAPInfo *info;
	int duration;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open %s\n", filename);
		exit(1);
	}
	content_len = fread(content, 1, sizeof(content), fp);
	fclose(fp);
	
	info = ASAPInfo_New();
	if (!ASAPInfo_Load(info, filename, content, content_len)) {
		fprintf(stderr, "ASAP doesn't understand %s\n", filename);
		exit(1);
	}
	duration = ASAPInfo_GetDuration(info, 0);
	return duration * (44100 / 100) / 10 /* milliseconds to samples */
		* 2; /* always stereo for gme */
}

int main(int argc, char **argv)
{
	Music_Emu *gme;
	gme_err_t err;
	int samples;
	if (argc != 2) {
		fprintf(stderr, "Usage: gme_benchmark SAPFILE\n");
		return 1;
	}
	err = gme_open_file(argv[1], &gme, 44100);
	if (err != NULL) {
		fprintf(stderr, "%s\n", err);
		return 1;
	}
	err = gme_start_track(gme, 0);
	if (err != NULL) {
		fprintf(stderr, "%s\n", err);
		return 1;
	}
	samples = get_stereo_samples_count(argv[1]);
	while (samples > 0) {
		short buf[BUF_SIZE];
		int len = samples >= BUF_SIZE ? BUF_SIZE : samples;
		gme_play(gme, len, buf);
		samples -= len;
	}
	return 0;
}
