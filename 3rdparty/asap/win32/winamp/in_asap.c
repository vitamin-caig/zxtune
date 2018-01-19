/*
 * in_asap.c - ASAP plugin for Winamp
 *
 * Copyright (C) 2005-2012  Piotr Fusik
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
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

#include "in2.h"
#include "ipc_pe.h"
#include "wa_ipc.h"

#include "aatr-stdio.h"
#include "asap.h"
#include "info_dlg.h"
#include "settings_dlg.h"

LPCTSTR atrFilenameHash(LPCTSTR filename);

// Winamp's equalizer works only with 16-bit samples
#define SUPPORT_EQUALIZER  1

#define BITS_PER_SAMPLE    16
// 576 is a magic number for Winamp, better do not modify it
#define BUFFERED_BLOCKS    576

static In_Module mod;

// configuration
#define INI_SECTION  "in_asap"
static const char *ini_file;

// current file
ASAP *asap;
static char playing_filename_with_song[MAX_PATH + 3] = "";
static BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
static int module_len;
static int duration;

static int playlistLength;

static HANDLE thread_handle = NULL;
static volatile int thread_run = FALSE;
static int paused = 0;
static int seek_needed;

static ASAPInfo *title_info;
static int title_song;

static void writeIniInt(const char *name, int value)
{
	char str[16];
	sprintf(str, "%d", value);
	WritePrivateProfileString(INI_SECTION, name, str, ini_file);
}

void onUpdatePlayingInfo(void)
{
	writeIniInt("playing_info", playing_info);
}

static void config(HWND hwndParent)
{
	if (settingsDialog(mod.hDllInstance, hwndParent)) {
		writeIniInt("song_length", song_length);
		writeIniInt("silence_seconds", silence_seconds);
		writeIniInt("play_loops", play_loops);
		writeIniInt("mute_mask", mute_mask);
	}
}

static void about(HWND hwndParent)
{
	MessageBox(hwndParent, ASAPInfo_CREDITS "\n" ASAPInfo_COPYRIGHT,
		"About ASAP Winamp plugin " ASAPInfo_VERSION, MB_OK);
}

static int extractSongNumber(const char *s, char *filename)
{
	int i = strlen(s);
	int song = -1;
	if (i > 6 && s[i - 1] >= '0' && s[i - 1] <= '9') {
		if (s[i - 2] == '#') {
			song = s[i - 1] - '1';
			i -= 2;
		}
		else if (s[i - 2] >= '0' && s[i - 2] <= '9' && s[i - 3] == '#') {
			song = (s[i - 2] - '0') * 10 + s[i - 1] - '1';
			i -= 3;
		}
	}
	memcpy(filename, s, i);
	filename[i] = '\0';
	return song;
}

static BOOL isATR(const char *filename)
{
	size_t len = strlen(filename);
	return len >= 4 && _stricmp(filename + len - 4, ".atr") == 0;
}

static void addFileSongs(HWND playlistWnd, fileinfo *fi, const ASAPInfo *info, int *index)
{
	char *p = fi->file + strlen(fi->file);
	int song;
	for (song = 0; song < ASAPInfo_GetSongs(info); song++) {
		COPYDATASTRUCT cds;
		sprintf(p, "#%d", song + 1);
		fi->index = (*index)++;
		cds.dwData = IPC_PE_INSERTFILENAME;
		cds.lpData = fi;
		cds.cbData = sizeof(fileinfo);
		SendMessage(playlistWnd, WM_COPYDATA, 0, (LPARAM) &cds);
	}
}

static void expandFileSongs(HWND playlistWnd, int index, ASAPInfo *info)
{
	const char *fn = (const char *) SendMessage(mod.hMainWindow, WM_WA_IPC, index, IPC_GETPLAYLISTFILE);
	fileinfo fi;
	int song = extractSongNumber(fn, fi.file);
	if (song >= 0)
		return;
	if (ASAPInfo_IsOurFile(fi.file)) {
		if (loadModule(fi.file, module, &module_len)
		 && ASAPInfo_Load(info, fi.file, module, module_len)) {
			SendMessage(playlistWnd, WM_WA_IPC, IPC_PE_DELETEINDEX, index);
			addFileSongs(playlistWnd, &fi, info, &index);
		}
	}
	else if (isATR(fi.file)) {
		FILE *fp = fopen(fi.file, "rb");
		if (fp != NULL) {
			AATR *aatr = AATRStdio_New(fp);
			if (aatr != NULL) {
				size_t atr_fn_len = strlen(fi.file);
				BOOL found = FALSE;
				fi.file[atr_fn_len++] = '#';
				for (;;) {
					const char *inside_fn = AATR_NextFile(aatr);
					if (inside_fn == NULL)
						break;
					if (ASAPInfo_IsOurFile(inside_fn)) {
						module_len = AATR_ReadCurrentFile(aatr, module, sizeof(module));
						if (ASAPInfo_Load(info, inside_fn, module, module_len)) {
							size_t inside_fn_len = strlen(inside_fn);
							if (atr_fn_len + inside_fn_len + 4 <= sizeof(fi.file)) {
								memcpy(fi.file + atr_fn_len, inside_fn, inside_fn_len + 1);
								if (!found) {
									found = TRUE;
									SendMessage(playlistWnd, WM_WA_IPC, IPC_PE_DELETEINDEX, index);
								}
								addFileSongs(playlistWnd, &fi, info, &index);
							}
						}
					}
				}
				AATR_Delete(aatr);
				/* Prevent Winamp crash:
				   1. Play anything.
				   2. Open an ATR with no songs.
				   3. If the ATR deletes itself, Winamp crashes (tested with 5.581).
				   Solution: leave the ATR on playlist. */
				if (!found && SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GETLISTLENGTH) > 1)
					SendMessage(playlistWnd, WM_WA_IPC, IPC_PE_DELETEINDEX, index);
			}
			fclose(fp);
		}
	}
}

static INT_PTR CALLBACK progressDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, playlistLength));
	return FALSE;
}

static void expandPlaylistSongs(void)
{
	static BOOL processing = FALSE;
	HWND playlistWnd;
	ASAPInfo *info;
	HWND progressWnd;
	int index;
	if (processing)
		return;
	playlistWnd = (HWND) SendMessage(mod.hMainWindow, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND);
	if (playlistWnd == NULL)
		return;
	processing = TRUE;
	info = ASAPInfo_New();
	playlistLength = SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GETLISTLENGTH);
	progressWnd = CreateDialog(mod.hDllInstance, MAKEINTRESOURCE(IDD_PROGRESS), mod.hMainWindow, progressDialogProc);
	index = playlistLength;
	while (--index >= 0) {
		if ((index & 15) == 0)
			SendDlgItemMessage(progressWnd, IDC_PROGRESS, PBM_SETPOS, playlistLength - index, 0);
		expandFileSongs(playlistWnd, index, info);
	}
	DestroyWindow(progressWnd);
	ASAPInfo_Delete(info);
	processing = FALSE;
}

static void init(void)
{
	asap = ASAP_New();
	title_info = ASAPInfo_New();
	ini_file = (const char *) SendMessage(mod.hMainWindow, WM_WA_IPC, 0, IPC_GETINIFILE);
	song_length = GetPrivateProfileInt(INI_SECTION, "song_length", song_length, ini_file);
	silence_seconds = GetPrivateProfileInt(INI_SECTION, "silence_seconds", silence_seconds, ini_file);
	play_loops = GetPrivateProfileInt(INI_SECTION, "play_loops", play_loops, ini_file);
	mute_mask = GetPrivateProfileInt(INI_SECTION, "mute_mask", mute_mask, ini_file);
	playing_info = GetPrivateProfileInt(INI_SECTION, "playing_info", playing_info, ini_file);
}

static void quit(void)
{
	ASAP_Delete(asap);
}

static void getTitle(char *title)
{
	if (ASAPInfo_GetSongs(title_info) > 1)
		sprintf(title, "%s (song %d)", ASAPInfo_GetTitleOrFilename(title_info), title_song + 1);
	else
		strcpy(title, ASAPInfo_GetTitleOrFilename(title_info));
}

static char *tagFunc(char *tag, void *p)
{
	if (stricmp(tag, "artist") == 0) {
		const char *author = ASAPInfo_GetAuthor(title_info);
		return author[0] != '\0' ? strdup(author) : NULL;
	}
	if (stricmp(tag, "title") == 0) {
		char *title = malloc(strlen(ASAPInfo_GetTitleOrFilename(title_info)) + 11);
		if (title != NULL)
			getTitle(title);
		return title;
	}
	return NULL;
}

static void tagFreeFunc(char *tag, void *p)
{
	free(tag);
}

static void getFileInfo(char *file, char *title, int *length_in_ms)
{
	char filename[MAX_PATH];
	const char *hash;
	if (file == NULL || file[0] == '\0')
		file = playing_filename_with_song;
	title_song = extractSongNumber(file, filename);
	if (title_song < 0)
		expandPlaylistSongs();
	if (!loadModule(filename, module, &module_len))
		return;
	hash = atrFilenameHash(filename);
	if (!ASAPInfo_Load(title_info, hash != NULL ? hash + 1 : filename, module, module_len))
		return;
	if (title_song < 0)
		title_song = ASAPInfo_GetDefaultSong(title_info);
	if (title != NULL) {
		waFormatTitle fmt_title = {
			NULL, NULL, title, 512, tagFunc, tagFreeFunc
		};
		getTitle(title); // in case IPC_FORMAT_TITLE doesn't work...
		SendMessage(mod.hMainWindow, WM_WA_IPC, (WPARAM) &fmt_title, IPC_FORMAT_TITLE);
	}
	if (length_in_ms != NULL)
		*length_in_ms = getSongDuration(title_info, title_song);
}

static int infoBox(char *file, HWND hwndParent)
{
	char filename[MAX_PATH];
	int song;
	song = extractSongNumber(file, filename);
	showInfoDialog(mod.hDllInstance, hwndParent, filename, song);
	return 0;
}

static int isOurFile(char *fn)
{
	char filename[MAX_PATH];
	extractSongNumber(fn, filename);
	return ASAPInfo_IsOurFile(filename);
}

static DWORD WINAPI playThread(LPVOID dummy)
{
	int channels = ASAPInfo_GetChannels(ASAP_GetInfo(asap));
	while (thread_run) {
		static BYTE buffer[BUFFERED_BLOCKS * 2 * (BITS_PER_SAMPLE / 8)
#if SUPPORT_EQUALIZER
			* 2
#endif
			];
		int buffered_bytes = BUFFERED_BLOCKS * channels * (BITS_PER_SAMPLE / 8);
		if (seek_needed >= 0) {
			mod.outMod->Flush(seek_needed);
			ASAP_Seek(asap, seek_needed);
			seek_needed = -1;
		}
		if (mod.outMod->CanWrite() >= buffered_bytes
#if SUPPORT_EQUALIZER
			<< mod.dsp_isactive()
#endif
		) {
			int t;
			buffered_bytes = ASAP_Generate(asap, buffer, buffered_bytes, BITS_PER_SAMPLE == 8 ? ASAPSampleFormat_U8 : ASAPSampleFormat_S16_L_E);
			if (buffered_bytes <= 0) {
				mod.outMod->CanWrite();
				if (!mod.outMod->IsPlaying()) {
					PostMessage(mod.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
					return 0;
				}
				Sleep(10);
				continue;
			}
			t = mod.outMod->GetWrittenTime();
			mod.SAAddPCMData(buffer, channels, BITS_PER_SAMPLE, t);
			mod.VSAAddPCMData(buffer, channels, BITS_PER_SAMPLE, t);
#if SUPPORT_EQUALIZER
			t = buffered_bytes / (channels * (BITS_PER_SAMPLE / 8));
			t = mod.dsp_dosamples((short *) buffer, t, BITS_PER_SAMPLE, channels, ASAP_SAMPLE_RATE);
			t *= channels * (BITS_PER_SAMPLE / 8);
			mod.outMod->Write((char *) buffer, t);
#else
			mod.outMod->Write((char *) buffer, buffered_bytes);
#endif
		}
		else
			Sleep(20);
	}
	return 0;
}

static int play(char *fn)
{
	char filename[MAX_PATH];
	int song;
	const ASAPInfo *info;
	int channels;
	int maxlatency;
	DWORD threadId;
	strcpy(playing_filename_with_song, fn);
	song = extractSongNumber(fn, filename);
	if (!loadModule(filename, module, &module_len))
		return -1;
	if (!ASAP_Load(asap, filename, module, module_len))
		return 1;
	info = ASAP_GetInfo(asap);
	if (song < 0)
		song = ASAPInfo_GetDefaultSong(info);
	duration = playSong(song);
	channels = ASAPInfo_GetChannels(info);
	maxlatency = mod.outMod->Open(ASAP_SAMPLE_RATE, channels, BITS_PER_SAMPLE, -1, -1);
	if (maxlatency < 0)
		return 1;
	mod.SetInfo(BITS_PER_SAMPLE, ASAP_SAMPLE_RATE / 1000, channels, 1);
	mod.SAVSAInit(maxlatency, ASAP_SAMPLE_RATE);
	// the order of VSASetInfo's arguments in in2.h is wrong!
	// http://forums.winamp.com/showthread.php?postid=1841035
	mod.VSASetInfo(ASAP_SAMPLE_RATE, channels);
	mod.outMod->SetVolume(-666);
	seek_needed = -1;
	thread_run = TRUE;
	thread_handle = CreateThread(NULL, 0, playThread, NULL, 0, &threadId);
	setPlayingSong(filename, song);
	return thread_handle != NULL ? 0 : 1;
}

static void pause(void)
{
	paused = 1;
	mod.outMod->Pause(1);
}

static void unPause(void)
{
	paused = 0;
	mod.outMod->Pause(0);
}

static int isPaused(void)
{
	return paused;
}

static void stop(void)
{
	if (thread_handle != NULL) {
		thread_run = FALSE;
		// wait max 10 seconds
		if (WaitForSingleObject(thread_handle, 10 * 1000) == WAIT_TIMEOUT)
			TerminateThread(thread_handle, 0);
		CloseHandle(thread_handle);
		thread_handle = NULL;
	}
	mod.outMod->Close();
	mod.SAVSADeInit();
}

static int getLength(void)
{
	return duration;
}

static int getOutputTime(void)
{
	return mod.outMod->GetOutputTime();
}

static void setOutputTime(int time_in_ms)
{
	seek_needed = time_in_ms;
}

static void setVolume(int volume)
{
	mod.outMod->SetVolume(volume);
}

static void setPan(int pan)
{
	mod.outMod->SetPan(pan);
}

static void eqSet(int on, char data[10], int preamp)
{
}

static In_Module mod = {
	IN_VER,
	"ASAP " ASAPInfo_VERSION,
	0, 0, // filled by Winamp
	"SAP\0Slight Atari Player (*.SAP)\0"
	"CMC;CM3;CMR;CMS;DMC\0Chaos Music Composer (*.CMC;*.CM3;*.CMR;*.CMS;*.DMC)\0"
	"DLT\0Delta Music Composer (*.DLT)\0"
	"MPT;MPD\0Music ProTracker (*.MPT;*.MPD)\0"
	"RMT\0Raster Music Tracker (*.RMT)\0"
	"TMC;TM8\0Theta Music Composer 1.x (*.TMC;*.TM8)\0"
	"TM2\0Theta Music Composer 2.x (*.TM2)\0"
	"FC\0FutureComposer (*.FC)\0"
	"ATR\0Atari 8-bit disk image (*.ATR)\0"
	,
	1,    // is_seekable
	1,    // UsesOutputPlug
	config,
	about,
	init,
	quit,
	getFileInfo,
	infoBox,
	isOurFile,
	play,
	pause,
	unPause,
	isPaused,
	stop,
	getLength,
	getOutputTime,
	setOutputTime,
	setVolume,
	setPan,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // filled by Winamp
	eqSet,
	NULL, // SetInfo
	NULL  // filled by Winamp
};

__declspec(dllexport) In_Module *winampGetInModule2(void)
{
	return &mod;
}
