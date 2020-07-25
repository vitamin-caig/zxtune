/*
 * astil.c - another SID/SAP Tune Information List parser
 *
 * Copyright (C) 2011-2012  Piotr Fusik
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
#include "astil.h"

#define ASTIL_MAX_TEXT_LENGTH  100
#define ASTIL_MAX_COMMENT_LENGTH  1000
#define ASTIL_MAX_COVERS  10

struct ASTILCover
{
	int startSeconds;
	int endSeconds;
	char titleAndSource[ASTIL_MAX_TEXT_LENGTH + 1];
	char artist[ASTIL_MAX_TEXT_LENGTH + 1];
	char comment[ASTIL_MAX_COMMENT_LENGTH + 1];
};

struct ASTIL
{
	int nCovers;
	cibool isUTF8;
	char stilFilename[FILENAME_MAX];
	char title[ASTIL_MAX_TEXT_LENGTH + 1];
	char author[ASTIL_MAX_TEXT_LENGTH + 1];
	char directoryComment[ASTIL_MAX_COMMENT_LENGTH + 1];
	char fileComment[ASTIL_MAX_COMMENT_LENGTH + 1];
	char songComment[ASTIL_MAX_COMMENT_LENGTH + 1];
	ASTILCover covers[ASTIL_MAX_COVERS];
};

ASTIL *ASTIL_New(void)
{
	ASTIL *self = (ASTIL *) malloc(sizeof(ASTIL));
	return self;
}

void ASTIL_Delete(ASTIL *self)
{
	free(self);
}

static int ASTIL_FindPreviousSlash(const char *filename, int pos)
{
	while (pos > 0) {
		pos--;
		if (filename[pos] == '/' || filename[pos] == '\\')
			return pos;
	}
	return -1;
}

static cibool ASTIL_ReadUTF8BOM(FILE *fp)
{
	if (getc(fp) == 0xef && getc(fp) == 0xbb && getc(fp) == 0xbf)
		return TRUE;
	fseek(fp, 0, SEEK_SET);
	return FALSE;
}

static int ASTIL_ReadLine(FILE *fp, char *result)
{
	int len = 0;
	for (;;) {
		int c = getc(fp);
		switch (c) {
		case EOF:
			result[0] = '\0';
			return -1;
		case '\n':
			while (len > 0 && (result[len - 1] == ' ' || result[len - 1] == '\t'))
				len--;
			result[len] = '\0';
			return len;
		case '\r':
			break;
		default:
			if (len < ASTIL_MAX_TEXT_LENGTH)
				result[len++] = c;
			break;
		}
	}
}

static int ASTIL_MatchFilename(const char *line, const char *filename)
{
	int len = 0;
	for (;; len++) {
		int c = line[len];
		if (c == filename[len]) {
			if (c == '\0')
				return -1;
		}
		else if (c == '/' && filename[len] == '\\') {
		}
		else
			return len;
	}
}

static void ASTIL_ReadComment(FILE *fp, char *line, char *comment)
{
	int len = ASTIL_ReadLine(fp, line);
	if (len >= 9 && memcmp(line, "COMMENT: ", 9) == 0) {
		len -= 9;
		memcpy(comment, line + 9, len + 1);
		for (;;) {
			int len2 = ASTIL_ReadLine(fp, line);
			if (len2 >= 9 && memcmp(line, "         ", 9) == 0) {
				if (len < ASTIL_MAX_COMMENT_LENGTH - ASTIL_MAX_TEXT_LENGTH) {
					comment[len++] = '\n';
					len2 -= 9;
					memcpy(comment + len, line + 9, len2 + 1);
					len += len2;
				}
			}
			else
				break;
		}
	}
}

static int ASTIL_ParseInt(const char *s, int *pos)
{
	int result = 0;
	int i = *pos;
	while (s[i] >= '0' && s[i] <= '9') {
		if (result >= 10)
			return -1;
		result = 10 * result + s[i++] - '0';
	}
	if (i == *pos)
		return -1;
	*pos = i;
	return result;
}

static cibool ASTIL_ReadUntilSongNo(FILE *fp, char *line)
{
	while (ASTIL_ReadLine(fp, line) > 0) {
		if (line[0] == '(' && line[1] == '#')
			return TRUE;
	}
	return FALSE;
}

static int ASTILCover_ParseTimestamp(const char *s, int *pos)
{
	int minutes = ASTIL_ParseInt(s, pos);
	if (minutes >= 0 && s[(*pos)++] == ':') {
		int seconds = ASTIL_ParseInt(s, pos);
		if (seconds >= 0)
			return 60 * minutes + seconds;
	}
	return -1;
}

static void ASTILCover_SetText(char *result, const char *src)
{
	if (strcmp(src, "<?>") == 0)
		result[0] = '\0';
	else
		strcpy(result, src);
}

static void ASTILCover_Load(ASTILCover *self, FILE *fp, char *line)
{
	const char *ts;
	self->startSeconds = -1;
	self->endSeconds = -1;
	ts = strrchr(line, '(');
	if (ts != NULL) {
		int i = ts - line + 1;
		int startSeconds = ASTILCover_ParseTimestamp(line, &i);
		int endSeconds = -1;
		if (startSeconds >= 0 && line[i] == '-') {
			i++;
			endSeconds = ASTILCover_ParseTimestamp(line, &i);
		}
		if (line[i] == ')' && line[i + 1] == '\0') {
			int len = ts - line;
			if (line[len - 1] == ' ')
				len--;
			line[len] = '\0';
			self->startSeconds = startSeconds;
			self->endSeconds = endSeconds;
		}
	}
	ASTILCover_SetText(self->titleAndSource, line + 9);
	self->artist[0] = '\0';
	self->comment[0] = '\0';
	if (ASTIL_ReadLine(fp, line) >= 9 && memcmp(line, " ARTIST: ", 9) == 0) {
		ASTILCover_SetText(self->artist, line + 9);
		ASTIL_ReadComment(fp, line, self->comment);
	}
}

static void ASTIL_ReadStilBlock(ASTIL *self, FILE *fp, char *line)
{
	for (;;) {
		if (memcmp(line, "   NAME: ", 9) == 0)
			strcpy(self->title, line + 9);
		else if (memcmp(line, " AUTHOR: ", 9) == 0)
			strcpy(self->author, line + 9);
		else
			break;
		if (ASTIL_ReadLine(fp, line) < 9)
			return;
	}
	while (self->nCovers < ASTIL_MAX_COVERS && memcmp(line, "  TITLE: ", 9) == 0) {
		ASTILCover *cover = self->covers + self->nCovers++;
		ASTILCover_Load(cover, fp, line);
	}
}

cibool ASTIL_Load(ASTIL *self, const char *filename, int song)
{
	int lastSlash = ASTIL_FindPreviousSlash(filename, strlen(filename));
	int rootSlash;
	FILE *fp;
	char line[ASTIL_MAX_TEXT_LENGTH + 1];
	self->nCovers = 0;
	self->stilFilename[0] = '\0';
	self->title[0] = '\0';
	self->author[0] = '\0';
	self->directoryComment[0] = '\0';
	self->fileComment[0] = '\0';
	self->songComment[0] = '\0';
	if (lastSlash < 0 || lastSlash >= FILENAME_MAX - 14) /* strlen("/Docs/STIL.txt") */
		return FALSE;
	memcpy(self->stilFilename, filename, lastSlash + 1);
	for (rootSlash = lastSlash; ; rootSlash = ASTIL_FindPreviousSlash(filename, rootSlash)) {
		if (rootSlash < 0) {
			self->stilFilename[0] = '\0';
			return FALSE;
		}
		strcpy(self->stilFilename + rootSlash + 1, "Docs/STIL.txt");
		self->stilFilename[rootSlash + 5] = self->stilFilename[rootSlash]; /* copy dir separator - slash or backslash */
		fp = fopen(self->stilFilename, "rb");
		if (fp != NULL)
			break;
		strcpy(self->stilFilename + rootSlash + 1, "STIL.txt");
		fp = fopen(self->stilFilename, "rb");
		if (fp != NULL)
			break;
	}
	self->isUTF8 = ASTIL_ReadUTF8BOM(fp);
	while (ASTIL_ReadLine(fp, line) >= 0) {
		int len = ASTIL_MatchFilename(line, filename + rootSlash);
		if (len == -1) {
			ASTIL_ReadComment(fp, line, self->fileComment);
			if (line[0] == '(' && line[1] == '#') {
				do {
					int i = 2;
					if (ASTIL_ParseInt(line, &i) - 1 == song && line[i] == ')' && line[i + 1] == '\0') {
						ASTIL_ReadComment(fp, line, self->songComment);
						ASTIL_ReadStilBlock(self, fp, line);
						break;
					}
				} while (ASTIL_ReadUntilSongNo(fp, line));
			}
			else
				ASTIL_ReadStilBlock(self, fp, line);
			break;
		}
		if (len == lastSlash - rootSlash + 1 && line[len] == '\0')
			ASTIL_ReadComment(fp, line, self->directoryComment);
	}
	fclose(fp);
	return TRUE;
}

const char *ASTIL_GetStilFilename(const ASTIL *self)
{
	return self->stilFilename;
}

cibool ASTIL_IsUTF8(const ASTIL *self)
{
	return self->isUTF8;
}

const char *ASTIL_GetTitle(const ASTIL *self)
{
	return self->title;
}

const char *ASTIL_GetAuthor(const ASTIL *self)
{
	return self->author;
}

const char *ASTIL_GetDirectoryComment(const ASTIL *self)
{
	return self->directoryComment;
}

const char *ASTIL_GetFileComment(const ASTIL *self)
{
	return self->fileComment;
}

const char *ASTIL_GetSongComment(const ASTIL *self)
{
	return self->songComment;
}

const ASTILCover *ASTIL_GetCover(const ASTIL *self, int i)
{
	if (i < 0 || i >= self->nCovers)
		return NULL;
	return &self->covers[i];
}

int ASTILCover_GetStartSeconds(const ASTILCover *self)
{
	return self->startSeconds;
}

int ASTILCover_GetEndSeconds(const ASTILCover *self)
{
	return self->endSeconds;
}

const char *ASTILCover_GetTitleAndSource(const ASTILCover *self)
{
	return self->titleAndSource;
}

const char *ASTILCover_GetArtist(const ASTILCover *self)
{
	return self->artist;
}

const char *ASTILCover_GetComment(const ASTILCover *self)
{
	return self->comment;
}
