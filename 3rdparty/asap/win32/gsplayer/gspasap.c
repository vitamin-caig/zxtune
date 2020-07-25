/*
 * gspasap.c - ASAP plugin for GSPlayer
 *
 * Copyright (C) 2007-2013  Piotr Fusik
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

#include <windows.h>
#include <tchar.h>

#include "mapplugin.h"

#include "asap.h"
#include "settings_dlg.h"

#define BITS_PER_SAMPLE  16

#define REG_KEY _T("Software\\GreenSoftware\\GSPlayer\\Plug-ins\\gspasap")

static HINSTANCE hInstance;

static BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
static DWORD module_len;
ASAP *asap;

#ifdef _UNICODE

#define CHAR_TO_TCHAR(dest, src, destChars) \
	MultiByteToWideChar(CP_ACP, 0, src, -1, dest, destChars)
static char ansiFilename[MAX_PATH];
#define ANSI_FILENAME ansiFilename
#define CONVERT_FILENAME \
	if (WideCharToMultiByte(CP_ACP, 0, pszFile, -1, ansiFilename, MAX_PATH, NULL, NULL) <= 0) \
		return FALSE;

#else

#define CHAR_TO_TCHAR(dest, src, destChars) \
	_tcscpy(dest, src)
#define ANSI_FILENAME pszFile
#define CONVERT_FILENAME

#endif

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	hInstance = hinstDLL;
	return TRUE;
}

static void WINAPI asapInit()
{
	HKEY hKey;
	DWORD type;
	DWORD size;
	asap = ASAP_New();
	if (RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY, 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
		return;
	size = sizeof(int);
	RegQueryValueEx(hKey, _T("SongLength"), NULL, &type, (LPBYTE) &song_length, &size);
	RegQueryValueEx(hKey, _T("SilenceSeconds"), NULL, &type, (LPBYTE) &silence_seconds, &size);
	RegQueryValueEx(hKey, _T("PlayLoops"), NULL, &type, (LPBYTE) &play_loops, &size);
	RegQueryValueEx(hKey, _T("MuteMask"), NULL, &type, (LPBYTE) &mute_mask, &size);
	RegCloseKey(hKey);
}

static void WINAPI asapQuit()
{
	ASAP_Delete(asap);
}

static DWORD WINAPI asapGetFunc()
{
	return PLUGIN_FUNC_DECFILE | PLUGIN_FUNC_SEEKFILE | PLUGIN_FUNC_FILETAG;
}

static BOOL WINAPI asapGetPluginName(LPTSTR pszName)
{
	_tcscpy(pszName, _T("ASAP plug-in"));
	return TRUE;
}

static BOOL WINAPI asapSetEqualizer(MAP_PLUGIN_EQ *pEQ)
{
	return FALSE;
}

static void WINAPI asapShowConfigDlg(HWND hwndParent)
{
	if (settingsDialog(hInstance, hwndParent)) {
		HKEY hKey;
		if (RegCreateKeyEx(HKEY_CURRENT_USER, REG_KEY, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS)
			return;
		RegSetValueEx(hKey, _T("SongLength"), 0, REG_DWORD, (LPBYTE) &song_length, sizeof(int));
		RegSetValueEx(hKey, _T("SilenceSeconds"), 0, REG_DWORD, (LPBYTE) &silence_seconds, sizeof(int));
		RegSetValueEx(hKey, _T("PlayLoops"), 0, REG_DWORD, (LPBYTE) &play_loops, sizeof(int));
		RegSetValueEx(hKey, _T("MuteMask"), 0, REG_DWORD, (LPBYTE) &mute_mask, sizeof(int));
		RegCloseKey(hKey);
	}
}

static const char * const exts[] = {
	"sap", "Slight Atari Player (*.sap)",
	"cmc", "Chaos Music Composer (*.cmc)",
	"cm3", "Chaos Music Composer 3/4 (*.cm3)",
	"cmr", "Chaos Music Composer / Rzog (*.cmr)",
	"cms", "Stereo Double Chaos Music Composer (*.cms)",
	"dmc", "DoublePlay Chaos Music Composer (*.dmc)",
	"dlt", "Delta Music Composer (*.dlt)",
	"mpt", "Music ProTracker (*.mpt)",
	"mpd", "Music ProTracker DoublePlay (*.mpd)",
	"rmt", "Raster Music Tracker (*.rmt)",
	"tmc", "Theta Music Composer 1.x (*.tmc)",
	"tm8", "Theta Music Composer 1.x (*.tm8)",
	"tm2", "Theta Music Composer 2.x (*.tm2)",
	"fc", "Future Composer (*.fc)"
};
#define N_EXTS (sizeof(exts) / sizeof(exts[0]) / 2)

static int WINAPI asapGetFileExtCount()
{
	return N_EXTS;
}

static BOOL WINAPI asapGetFileExt(int nIndex, LPTSTR pszExt, LPTSTR pszExtDesc)
{
	if (nIndex >= N_EXTS)
		return FALSE;
	CHAR_TO_TCHAR(pszExt, exts[nIndex * 2], MAX_PATH);
	CHAR_TO_TCHAR(pszExtDesc, exts[nIndex * 2 + 1], MAX_PATH);
	return TRUE;
}

static BOOL WINAPI asapIsValidFile(LPCTSTR pszFile)
{
	CONVERT_FILENAME;
	return ASAPInfo_IsOurFile(ANSI_FILENAME);
}

static BOOL loadFile(LPCTSTR pszFile)
{
	HANDLE fh;
	CONVERT_FILENAME;
	if (!ASAPInfo_IsOurFile(ANSI_FILENAME))
		return FALSE;
	fh = CreateFile(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	if (!ReadFile(fh, module, sizeof(module), &module_len, NULL)) {
		CloseHandle(fh);
		return FALSE;
	}
	CloseHandle(fh);
	return TRUE;
}

static BOOL WINAPI asapOpenFile(LPCTSTR pszFile, MAP_PLUGIN_FILE_INFO *pInfo)
{
	const ASAPInfo *info = ASAP_GetInfo(asap);
	CONVERT_FILENAME;
	if (!loadFile(pszFile))
		return FALSE;
	if (!ASAP_Load(asap, ANSI_FILENAME, module, module_len))
		return FALSE;
	pInfo->nChannels = ASAPInfo_GetChannels(info);
	pInfo->nSampleRate = ASAP_SAMPLE_RATE;
	pInfo->nBitsPerSample = BITS_PER_SAMPLE;
	pInfo->nAvgBitrate = 8;
	pInfo->nDuration = getSongDuration(info, ASAPInfo_GetDefaultSong(info));
	return TRUE;
}

static long WINAPI asapSeekFile(long lTime)
{
	ASAP_Seek(asap, (int) lTime);
	return lTime;
}

static BOOL WINAPI asapStartDecodeFile()
{
	playSong(ASAPInfo_GetDefaultSong(ASAP_GetInfo(asap)));
	return TRUE;
}

static int WINAPI asapDecodeFile(WAVEHDR *pHdr)
{
	int len = ASAP_Generate(asap, (PBYTE) pHdr->lpData, pHdr->dwBufferLength, BITS_PER_SAMPLE == 8 ? ASAPSampleFormat_U8 : ASAPSampleFormat_S16_L_E);
	pHdr->dwBytesRecorded = len;
	return len < (int) pHdr->dwBufferLength ? PLUGIN_RET_EOF : PLUGIN_RET_SUCCESS;
}

static void WINAPI asapStopDecodeFile()
{
}

static void WINAPI asapCloseFile()
{
}

static void getTag(const ASAPInfo *info, MAP_PLUGIN_FILETAG *pTag)
{
	ZeroMemory(pTag, sizeof(MAP_PLUGIN_FILETAG));
	CHAR_TO_TCHAR(pTag->szTrack, ASAPInfo_GetTitle(info), MAX_PLUGIN_TAG_STR);
	CHAR_TO_TCHAR(pTag->szArtist, ASAPInfo_GetAuthor(info), MAX_PLUGIN_TAG_STR);
}

static BOOL WINAPI asapGetTag(MAP_PLUGIN_FILETAG *pTag)
{
	getTag(ASAP_GetInfo(asap), pTag);
	return TRUE;
}

static BOOL WINAPI asapGetFileTag(LPCTSTR pszFile, MAP_PLUGIN_FILETAG *pTag)
{
	ASAPInfo *info;
	CONVERT_FILENAME;
	if (!loadFile(pszFile))
		return FALSE;
	info = ASAPInfo_New();
	if (info == NULL)
		return FALSE;
	if (!ASAPInfo_Load(info, ANSI_FILENAME, module, module_len)) {
		ASAPInfo_Delete(info);
		return FALSE;
	}
	getTag(info, pTag);
	ASAPInfo_Delete(info);
	return TRUE;
}

static BOOL WINAPI asapOpenStreaming(LPBYTE pbBuf, DWORD cbBuf, MAP_PLUGIN_STREMING_INFO *pInfo)
{
	return FALSE;
}

static int WINAPI asapDecodeStreaming(LPBYTE pbInBuf, DWORD cbInBuf, DWORD *pcbInUsed, WAVEHDR *pHdr)
{
	return PLUGIN_RET_ERROR;
}

static void WINAPI asapCloseStreaming()
{
}

static MAP_DEC_PLUGIN plugin = {
	PLUGIN_DEC_VER,
	sizeof(TCHAR),
	0,
	asapInit,
	asapQuit,
	asapGetFunc,
	asapGetPluginName,
	asapSetEqualizer,
	asapShowConfigDlg,
	asapGetFileExtCount,
	asapGetFileExt,
	asapIsValidFile,
	asapOpenFile,
	asapSeekFile,
	asapStartDecodeFile,
	asapDecodeFile,
	asapStopDecodeFile,
	asapCloseFile,
	asapGetTag,
	asapGetFileTag,
	asapOpenStreaming,
	asapDecodeStreaming,
	asapCloseStreaming
};

__declspec(dllexport) MAP_DEC_PLUGIN * WINAPI mapGetDecoder()
{
	return &plugin;
}
