/*
 * sap2ntsc.c - convert PAL SAP files to NTSC
 *
 * Copyright (C) 2012  Piotr Fusik
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* public API doesn't allow us to set NTSC or FASTPLAY */
#include "asap.c"

static void fatal_error(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "sap2ntsc: ");
	vfprintf(stderr, format, args);
	fputc('\n', stderr);
	va_end(args);
	exit(1);
}

static void write_byte(void *obj, int data)
{
	putc(data, (FILE *) obj);
}

static void process_file(const char *filename)
{
	const char *ext;
	FILE *fp;
	static unsigned char module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len;
	ASAPInfo *info;
	ByteWriter bw;
	int i;
	int warnings;

	/* check filename */
	ext = strrchr(filename, '.');
	if (ext == NULL || strcasecmp(ext, ".sap") != 0)
		fatal_error("%s: filename must be *.sap", filename);

	/* read file */
	fp = fopen(filename, "rb");
	if (fp == NULL)
		fatal_error("cannot open %s", filename);
	module_len = fread(module, 1, sizeof(module), fp);
	fclose(fp);

	/* parse file */
	info = ASAPInfo_New();
	if (info == NULL)
		fatal_error("out of memory");
	if (!ASAPInfo_Load(info, filename, module, module_len))
		fatal_error("%s: unsupported file", filename);

	/* check if conversion possible */
	if (ASAPInfo_IsNtsc(info))
		fatal_error("%s: is already NTSC", filename);
	if (ASAPInfo_GetPlayerRateScanlines(info) != 312)
		fatal_error("%s: uses FASTPLAY", filename);

	/* do the conversion */
	info->ntsc = TRUE;
	info->fastplay = 262;
	for (i = 0; i < ASAPInfo_GetSongs(info); i++) {
		int duration = ASAPInfo_GetDuration(info, i);
		if (duration > 0)
			ASAPInfo_SetDuration(info, i, (int) (duration * (1773447 / 1789772.5 * 262 / 312)));
	}

	/* write file */
	fp = fopen(filename, "wb");
	if (fp == NULL)
		fatal_error("cannot write %s", filename);
	bw.obj = fp;
	bw.func = write_byte;
	if (!ASAPWriter_Write(filename, bw, info, module, module_len, TRUE)) {
		fclose(fp);
		remove(filename); /* "unlink" is less portable */
		fatal_error("%s: write error", filename);
	}
	fclose(fp);

	/* print summary */
	printf("%s: ", filename);
	warnings = 0;

	/* issue a warning for samples - they may break on NTSC */
	/* TYPE S has FASTPLAY!=312, so it has been rejected earlier */
	if (ASAPInfo_GetTypeLetter(info) == 'D') {
		printf("WARNING: TYPE D");
		warnings++;
	}
	
	/* issue a warning if the 6502 code possibly reads the PAL/NTSC flag of GTIA
	   (LDA/LDX/LDY $D014, e.g. Ghostbusters.sap).
	   This is just a guess - false positives are possible and other code may be used for NTSC detection. */
	for (i = 0; i < module_len - 2; i++) {
		if (module[i] >= 0xac && module[i] <= 0xae && module[i + 1] == 0x14 && module[i + 2] == 0xd0) {
			if (warnings++ > 0)
				printf(", ");
			printf("WARNING: possible PAL/NTSC detection code");
			break;
		}
	}

	ASAPInfo_Delete(info);
	if (warnings == 0)
		printf("ok");
	printf("\n");
}

int main(int argc, char **argv)
{
	cibool usage = TRUE;
	int i;
	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (strcmp(arg, "--help") == 0) {
			usage = TRUE;
			break;
		}
		if (strcmp(arg, "--version") == 0) {
			printf("sap2ntsc " ASAPInfo_VERSION "\n");
			return 0;
		}
		process_file(arg);
		usage = FALSE;
	}
	if (usage) {
		printf(
			"Usage: sap2ntsc FILE.sap...\n"
			"Replaces FILE.sap with an NTSC version\n"
		);
	}
	return 0;
}
