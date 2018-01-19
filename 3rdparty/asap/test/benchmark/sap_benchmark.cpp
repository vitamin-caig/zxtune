/*
 * sap_benchmark.cpp - SAP Library benchmark
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
#include "sapLib.h"

#define BUF_SIZE 4096

static int get_samples_count(const char *filename)
{
	/* SAP Library doesn't understand TIME so ask ASAP */
	FILE *fp;
	unsigned char content[ASAPInfo_MAX_MODULE_LENGTH];
	int content_len;
	ASAPInfo *info;

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
	/* milliseconds to samples */
	return ASAPInfo_GetDuration(info, 0) * (44100 / 100) / 10;
}

int main(int argc, char **argv)
{
	sapMUSICstrc *sap;
	int samples;
	if (argc != 2) {
		fprintf(stderr, "Usage: sap_benchmark SAPFILE\n");
		return 1;
	}
	sap = sapLoadMusicFile(argv[1]);
	if (sap == NULL) {
		fprintf(stderr, "SAP doesn't understand %s\n", argv[1]);
		return 1;
	}
	sapPlaySong(0);
	samples = get_samples_count(argv[1]);
	while (samples > 0) {
		short buf[BUF_SIZE * 2];
		int len = samples >= BUF_SIZE ? BUF_SIZE : samples;
		/* for stereo, sapRenderBuffer fills len*2 samples */
		sapRenderBuffer(buf, len);
		samples -= len;
	}
	return 0;
}
