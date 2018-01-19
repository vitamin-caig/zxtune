/*
 * sap2txt.c - write plain text summary of a SAP file
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

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#ifdef _MSC_VER
#include <io.h>
#endif
#endif
#include <zlib.h>

static void usage(void)
{
	fprintf(stderr,
		"Usage:\n"
		"sap2txt INPUT.sap             - dump to stdout\n"
		"sap2txt INPUT.txt OUTPUT.sap  - replace header in SAP\n");
}

static int get_word(FILE *fp)
{
	int lo = getc(fp);
	int hi;
	if (lo == EOF)
		return -1;
	hi = getc(fp);
	if (hi == EOF)
		return -2;
	return lo | (hi << 8);
}

static int sap2txt(const char *sap_file)
{
	FILE *fp = fopen(sap_file, "rb");
	if (fp == NULL) {
		fprintf(stderr, "sap2txt: cannot open %s\n", sap_file);
		return 1;
	}
#ifdef _WIN32
	_setmode(_fileno(stdout), _O_BINARY);
#endif

	/* copy header */
	for (;;) {
		int c = getc(fp);
		if (c == EOF)
			return 0;
		else if (c == 0xff)
			break;
		putchar(c);
	}
	if (getc(fp) != 0xff)
		return 0;

	for (;;) {
		int ffff = 0;
		int start_address = get_word(fp);
		int end_address;
		Byte buffer[65536];
		int len;
		uLong crc;

		switch (start_address) {
		case -1:
			/* ok */
			return 0;
		case -2:
			printf("Unexpected end of file in a binary header\n");
			return 0;
		case 0xffff:
			ffff = 1;
			start_address = get_word(fp);
			break;
		default:
			break;
		}
		end_address = get_word(fp);
		if (end_address < 0) {
			printf("Unexpected end of file in a binary header\n");
			return 0;
		}
		if (end_address < start_address) {
			printf("Invalid binary header\n");
			return 0;
		}
		len = end_address - start_address + 1;
		if (fread(buffer, 1, len, fp) != len) {
			printf("Unexpected end of file in a binary block\n");
			return 0;
		}
		crc = crc32(0, Z_NULL, 0);
		crc = crc32(crc, buffer, len);
		if (ffff)
			printf("FFFF ");
		printf("LOAD %04X-%04X CRC32=%08lX\r\n", start_address, end_address, crc);
	}
}

static size_t slurp(const char *input_file, Byte *buffer, size_t len)
{
	FILE *fp = fopen(input_file, "rb");
	size_t result;
	if (fp == NULL) {
		fprintf(stderr, "sap2txt: cannot open %s\n", input_file);
		return 0;
	}
	result = fread(buffer, 1, len, fp);
	fclose(fp);
	if (result == len) {
		fprintf(stderr, "sap2txt: %s: file too long\n", input_file);
		return 0;
	}
	return result;
}

static int txt2sap(const char *txt_file, const char *sap_file)
{
	Byte txt_buf[65536];
	size_t txt_len;
	Byte sap_buf[65536];
	size_t sap_len;
	const void *bin_ptr;
	size_t i;
	FILE *fp;

	txt_len = slurp(txt_file, txt_buf, sizeof(txt_buf));
	if (txt_len == 0)
		return 1;
	sap_len = slurp(sap_file, sap_buf, sizeof(sap_buf));
	if (sap_len == 0)
		return 1;

	bin_ptr = memchr(sap_buf, 0xff, sap_len);
	if (bin_ptr == NULL) {
		fprintf(stderr, "sap2txt: missing binary part in %s\n", sap_file);
		return 1;
	}

	for (i = 0; i < txt_len; ) {
		if (memcmp(txt_buf + i, "LOAD ", 5) == 0)
			break;
		if (txt_buf[i] == 0xff && txt_buf[i + 1] == 0xff)
			break;
		while (i < txt_len && txt_buf[i++] != 0x0a) { }
	}

	if (i == (const Byte *) bin_ptr - sap_buf && memcmp(txt_buf, sap_buf, i) == 0)
		return 0; /* same */

	fp = fopen(sap_file, "wb");
	if (fp == NULL) {
		fprintf(stderr, "sap2txt: cannot write %s\n", sap_file);
		return 1;
	}
	fwrite(txt_buf, 1, i, fp);
	fwrite(bin_ptr, 1, sap_len - ((const Byte *) bin_ptr - sap_buf), fp);
	fclose(fp);
	return 0;
}

int main(int argc, char **argv)
{
	switch (argc) {
	case 2:
		if (strcmp(argv[1], "--help") == 0) {
			usage();
			return 0;
		}
		return sap2txt(argv[1]);
	case 3:
		return txt2sap(argv[1], argv[2]);
	default:
		usage();
		return 1;
	}
}
