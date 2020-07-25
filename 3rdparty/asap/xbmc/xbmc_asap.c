/*
 * xbmc_asap.c - ASAP plugin for XBMC
 *
 * Copyright (C) 2008-2014  Piotr Fusik
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

#ifndef _WIN32
#define __declspec(x)
#endif

#include "asap.h"

typedef struct {
	char author[ASAPInfo_MAX_TEXT_LENGTH + 1];
	char name[ASAPInfo_MAX_TEXT_LENGTH + 1];
	int year;
	int month;
	int day;
	int channels;
	int duration;
} ASAP_SongInfo;

static ASAP *asap = NULL;

static cibool loadModule(const char *filename, unsigned char *module, int *module_len)
{
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL)
		return FALSE;
	*module_len = (int) fread(module, 1, ASAPInfo_MAX_MODULE_LENGTH, fp);
	fclose(fp);
	return TRUE;
}

static ASAPInfo *getModuleInfo(const char *filename)
{
	unsigned char module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len;
	ASAPInfo *info;
	if (!loadModule(filename, module, &module_len))
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

__declspec(dllexport) int asapGetSongs(const char *filename)
{
	ASAPInfo *info = getModuleInfo(filename);
	int songs;
	if (info == NULL)
		return 0;
	songs = ASAPInfo_GetSongs(info);
	ASAPInfo_Delete(info);
	return songs;
}

static int zeroIfNegative(int x)
{
	return x >= 0 ? x : 0;
}

__declspec(dllexport) cibool asapGetInfo(const char *filename, int song, ASAP_SongInfo *song_info)
{
	ASAPInfo *info = getModuleInfo(filename);
	if (info == NULL)
		return FALSE;
	if (song < 0)
		song = ASAPInfo_GetDefaultSong(info);
	strcpy(song_info->author, ASAPInfo_GetAuthor(info));
	strcpy(song_info->name, ASAPInfo_GetTitleOrFilename(info));
	song_info->year = zeroIfNegative(ASAPInfo_GetYear(info));
	song_info->month = zeroIfNegative(ASAPInfo_GetMonth(info));
	song_info->day = zeroIfNegative(ASAPInfo_GetDayOfMonth(info));
	song_info->channels = ASAPInfo_GetChannels(info);
	song_info->duration = ASAPInfo_GetDuration(info, song);
	ASAPInfo_Delete(info);
	return TRUE;
}

__declspec(dllexport) cibool asapLoad(const char *filename, int song, int *channels, int *duration)
{
	unsigned char module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len;
	const ASAPInfo *info;
	if (!loadModule(filename, module, &module_len))
		return FALSE;
	if (asap == NULL) {
		asap = ASAP_New();
		if (asap == NULL)
			return FALSE;
	}
	if (!ASAP_Load(asap, filename, module, module_len))
		return FALSE;
	info = ASAP_GetInfo(asap);
	*channels = ASAPInfo_GetChannels(info);
	if (song < 0)
		song = ASAPInfo_GetDefaultSong(info);
	*duration = ASAPInfo_GetDuration(info, song);
	return ASAP_PlaySong(asap, song, *duration);
}

__declspec(dllexport) void asapSeek(int position)
{
	ASAP_Seek(asap, position);
}

__declspec(dllexport) int asapGenerate(void *buffer, int buffer_len)
{
	return ASAP_Generate(asap, buffer, buffer_len, ASAPSampleFormat_S16_L_E);
}
