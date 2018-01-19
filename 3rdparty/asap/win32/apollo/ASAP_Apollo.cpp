/*
 * ASAP_Apollo.cpp - ASAP plugin for Apollo
 *
 * Copyright (C) 2008-2012  Piotr Fusik
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

#include <string.h>

#include "InputPlugin.h"

#include "asap.h"
#include "info_dlg.h"
#include "settings_dlg.h"

#define BITS_PER_SAMPLE    16

ASAP *asap;

class CASAPDecoder : public CInputDecoder
{
	BYTE buf[8192];

public:

	int __cdecl GetNextAudioChunk(void **buffer)
	{
		*buffer = buf;
		return ASAP_Generate(asap, buf, sizeof(buf), BITS_PER_SAMPLE == 8 ? ASAPSampleFormat_U8 : ASAPSampleFormat_S16_L_E);
	}

	int __cdecl SetPosition(int newPosition)
	{
		ASAP_Seek(asap, newPosition);
		return newPosition;
	}

	void __cdecl SetEqualizer(BOOL equalize)
	{
	}

	void __cdecl AdjustEqualizer(int equalizerValues[16])
	{
	}

	void __cdecl Close()
	{
		delete this;
	}
};

class CASAPPlugin : public CInputPlugin
{
	HINSTANCE hInstance;

public:

	CASAPPlugin(HINSTANCE hInst) : hInstance(hInst)
	{
		if (asap == NULL)
			asap = ASAP_New();
	}

	char * __cdecl GetDescription()
	{
		return const_cast<char *> ("Apollo ASAP Decoder version " ASAPInfo_VERSION);
	}

	CInputDecoder * __cdecl Open(char *filename, int audioDataOffset)
	{
		BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
		int module_len;
		if (!loadModule(filename, module, &module_len))
			return NULL;
		if (!ASAP_Load(asap, filename, module, module_len))
			return NULL;
		playSong(ASAPInfo_GetDefaultSong(ASAP_GetInfo(asap)));
		return new CASAPDecoder();
	}

	BOOL __cdecl GetInfo(char *filename, int audioDataOffset, TrackInfo *trackInfo)
	{
		BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
		int module_len;
		if (!loadModule(filename, module, &module_len))
			return FALSE;
		ASAPInfo *info = ASAPInfo_New();
		if (info == NULL)
			return FALSE;
		if (!ASAPInfo_Load(info, filename, module, module_len)) {
			ASAPInfo_Delete(info);
			return FALSE;
		}
		strcpy(trackInfo->suggestedTitle, ASAPInfo_GetTitle(info));
		trackInfo->fileSize = module_len;
		trackInfo->seekable = TRUE;
		trackInfo->hasEqualizer = FALSE;
		int duration = getSongDuration(info, ASAPInfo_GetDefaultSong(info));
		trackInfo->playingTime = duration >= 0 ? duration : -1;
		trackInfo->bitRate = 0;
		trackInfo->sampleRate = ASAP_SAMPLE_RATE;
		trackInfo->numChannels = ASAPInfo_GetChannels(info);
		trackInfo->bitResolution = BITS_PER_SAMPLE;
		strcpy(trackInfo->fileTypeDescription, "Atari 8-bit music");
		ASAPInfo_Delete(info);
		return TRUE;
	}

	void __cdecl AdditionalInfo(char *filename, int audioDataOffset)
	{
		showInfoDialog(hInstance, GetActiveWindow(), filename, -1);
	}

	BOOL __cdecl IsSuitableFile(char *filename, int audioDataOffset)
	{
		return ASAPInfo_IsOurFile(filename);
	}

	char * __cdecl GetExtensions()
	{
		return const_cast<char *>(
			"Slight Atari Player\0*.SAP\0"
			"Chaos Music Composer\0*.CMC;*.CM3;*.CMR;*.CMS;*.DMC\0"
			"Delta Music Composer\0*.DLT\0"
			"Music ProTracker\0*.MPT;*.MPD\0"
			"Raster Music Tracker\0*.RMT\0"
			"Theta Music Composer 1.x\0*.TMC;*.TM8\0"
			"Theta Music Composer 2.x\0*.TM2\0"
			"Future Composer\0*.FC\0"
			"\0");
	}

	void __cdecl Config()
	{
		settingsDialog(hInstance, GetActiveWindow());
	}

	void __cdecl About()
	{
		MessageBox(GetActiveWindow(), ASAPInfo_CREDITS "\n" ASAPInfo_COPYRIGHT,
			"About ASAP plugin " ASAPInfo_VERSION, MB_OK);
	}

	void __cdecl Close()
	{
		delete this;
	}
};

extern "C" __declspec(dllexport) CInputPlugin *Apollo_GetInputModule2(HINSTANCE hDLLInstance, HWND hApolloWnd)
{
	return new CASAPPlugin(hDLLInstance);
}
