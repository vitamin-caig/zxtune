/*
 * aatr-stdio.c - another ATR file extractor
 *
 * Copyright (C) 2012-2013  Piotr Fusik
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
#include "aatr-stdio.h"

static cibool AATRStdio_Read(void *obj, int offset, const unsigned char *buffer, int length)
{
	FILE *fp = (FILE *) obj;
	return fseek(fp, offset, SEEK_SET) == 0
		&& fread((void *) buffer, length, 1, fp) == 1;
}

AATR *AATRStdio_New(FILE *fp)
{
	AATR *aatr = AATR_New();
	RandomAccessInputStream content;
	if (aatr == NULL)
		return NULL;
	content.obj = fp;
	content.func = AATRStdio_Read;
	if (!AATR_Open(aatr, content)) {
		AATR_Delete(aatr);
		return NULL;
	}
	return aatr;
}

int AATRStdio_ReadFile(const char *atr_filename, const char *inside_filename, unsigned char *buffer, int length)
{
	FILE *fp = fopen(atr_filename, "rb");
	AATR *aatr;
	if (fp == NULL)
		return -1;
	aatr = AATRStdio_New(fp);
	if (aatr == NULL) {
		fclose(fp);
		return -1;
	}
	length = AATR_ReadFile(aatr, inside_filename, buffer, length);
	AATR_Delete(aatr);
	fclose(fp);
	return length;
}

#if 0
int main(int argc, char **argv)
{
	FILE *fp = fopen("C:\\0\\a8\\asap\\TMC2.atr", "rb");
	AATR *aatr;
	if (fp == NULL)
		return 1;
	aatr = AATRStdio_New(fp);
	if (aatr == NULL)
		return 1;
	for (;;) {
		const char *current_filename = AATR_NextFile(aatr);
		int length;
		if (current_filename == NULL)
			break;
		length = AATR_ReadCurrentFile(aatr, NULL, 60000);
		printf("%s (%d)\n", current_filename, length);
	}
	return 0;
}
#endif
