/*
 * xmp-asap.c - ASAP plugin for XMPlay
 *
 * Copyright (C) 2010-2012  Piotr Fusik
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

#include "xmpin.h"

#include "asap.h"
#include "astil.h"
#include "info_dlg.h"
#include "settings_dlg.h"

#define BITS_PER_SAMPLE  16

static HINSTANCE hInst;
static const XMPFUNC_MISC *xmpfmisc;
static const XMPFUNC_REGISTRY *xmpfreg;
static const XMPFUNC_FILE *xmpffile;
static const XMPFUNC_IN *xmpfin;

static BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
static int module_len;
ASAP *asap;

void onUpdatePlayingInfo(void)
{
	xmpfreg->SetInt("ASAP", "PlayingInfo", &playing_info);
}

static void WINAPI ASAP_ShowInfo()
{
	showInfoDialog(hInst, xmpfmisc->GetWindow(), NULL, -1);
}

static void WINAPI ASAP_About(HWND win)
{
	MessageBox(win, ASAPInfo_CREDITS "\n" ASAPInfo_COPYRIGHT,
		"About ASAP XMPlay plugin " ASAPInfo_VERSION, MB_OK);
}

static void WINAPI ASAP_Config(HWND win)
{
	if (settingsDialog(hInst, win)) {
		xmpfreg->SetInt("ASAP", "SongLength", &song_length);
		xmpfreg->SetInt("ASAP", "SilenceSeconds", &silence_seconds);
		xmpfreg->SetInt("ASAP", "PlayLoops", &play_loops);
		xmpfreg->SetInt("ASAP", "MuteMask", &mute_mask);
	}
}

static BOOL WINAPI ASAP_CheckFile(const char *filename, XMPFILE file)
{
	return ASAPInfo_IsOurFile(filename);
}

static void GetTag(const char *src, char **dest)
{
	int len;
	if (src[0] == '\0')
		return;
	len = strlen(src) + 1;
	*dest = xmpfmisc->Alloc(len);
	memcpy(*dest, src, len);
}

static void GetTags(const ASAPInfo *info, char *tags[8])
{
	GetTag(ASAPInfo_GetTitle(info), tags);
	GetTag(ASAPInfo_GetAuthor(info), tags + 1);
	GetTag(ASAPInfo_GetDate(info), tags + 3);
	GetTag("ASAP", tags + 7);
}

static void GetTotalLength(const ASAPInfo *info, float *length)
{
	int total_duration = 0;
	int song;
	for (song = 0; song < ASAPInfo_GetSongs(info); song++) {
		int duration = getSongDuration(info, song);
		if (duration < 0) {
			*length = 0;
			return;
		}
		total_duration += duration;
	}
	*length = total_duration / 1000.0;
}

static void PlaySong(int song)
{
	int duration = playSong(song);
	if (duration >= 0)
		xmpfin->SetLength(duration / 1000.0, TRUE);
	setPlayingSong(NULL, song);
}

static BOOL WINAPI ASAP_GetFileInfo(const char *filename, XMPFILE file, float *length, char *tags[8])
{
	BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len = xmpffile->Read(file, module, sizeof(module));
	ASAPInfo *info = ASAPInfo_New();
	if (info == NULL)
		return FALSE;
	if (!ASAPInfo_Load(info, filename, module, module_len)) {
		ASAPInfo_Delete(info);
		return FALSE;
	}
	if (length != NULL)
		GetTotalLength(info, length);
	GetTags(info, tags);
	ASAPInfo_Delete(info);
	return TRUE;
}

static DWORD WINAPI ASAP_Open(const char *filename, XMPFILE file)
{
	module_len = xmpffile->Read(file, module, sizeof(module));
	if (!ASAP_Load(asap, filename, module, module_len))
		return 0;
	setPlayingSong(filename, 0);
	PlaySong(0);
	return 2; /* close file */
}

static void WINAPI ASAP_Close()
{
}

static void WINAPI ASAP_SetFormat(XMPFORMAT *form)
{
	form->rate = ASAP_SAMPLE_RATE;
	form->chan = ASAPInfo_GetChannels(ASAP_GetInfo(asap));
	form->res = BITS_PER_SAMPLE / 8;
}

static BOOL WINAPI ASAP_GetTags(char *tags[8])
{
	GetTags(ASAP_GetInfo(asap), tags);
	return TRUE;
}

static void WINAPI ASAP_GetInfoText(char *format, char *length)
{
	if (format != NULL)
		strcpy(format, "ASAP");
}

static void WINAPI ASAP_GetGeneralInfo(char *buf)
{
	const ASAPInfo *info = ASAP_GetInfo(asap);
	const char *s = ASAPInfo_GetDate(info);
	if (s[0] != '\0')
		buf += sprintf(buf, "Date\t%s\r", s);
	s = ASAPInfo_GetOriginalModuleExt(info, module, module_len);
	if (s != NULL)
		buf += sprintf(buf, "Composed in\t%s\r", ASAPInfo_GetExtDescription(s));
	*buf = '\0';
}

static char *appendStilString(char *p, const char *s)
{
	for (;;) {
		char c = *s++;
		switch (c) {
		case '\0':
			return p;
		case '\n':
			*p++ = '\r';
			*p++ = '\n';
			*p++ = '\t';
			break;
		default:
			*p++ = c;
			break;
		}
	}
}

static char *appendStil(char *p, const char *prefix, const char *value)
{
	if (value[0] != '\0') {
		p = appendStilString(p, prefix);
		*p++ = '\t';
		p = appendStilString(p, value);
		*p++ = '\r';
		*p++ = '\n';
	}
	return p;
}

static void WINAPI ASAP_GetMessage(char *buf)
{
	const ASTIL *astil = getPlayingASTIL();
	char *p = buf;
	int i;
	p = appendStil(p, "STIL file", ASTIL_GetStilFilename(astil));
	p = appendStil(p, "Title", ASTIL_GetTitle(astil));
	p = appendStil(p, "Author", ASTIL_GetAuthor(astil));
	p = appendStil(p, "Directory comment", ASTIL_GetDirectoryComment(astil));
	p = appendStil(p, "File comment", ASTIL_GetFileComment(astil));
	p = appendStil(p, "Song comment", ASTIL_GetSongComment(astil));
	for (i = 0; ; i++) {
		const ASTILCover *cover = ASTIL_GetCover(astil, i);
		int startSeconds;
		const char *s;
		if (cover == NULL)
			break;
		startSeconds = ASTILCover_GetStartSeconds(cover);
		if (startSeconds >= 0) {
			int endSeconds = ASTILCover_GetEndSeconds(cover);
			if (endSeconds >= 0)
				p += sprintf(p, "At %d:%02d-%d:%02d c", startSeconds / 60, startSeconds % 60, endSeconds / 60, endSeconds % 60);
			else
				p += sprintf(p, "At %d:%02d c", startSeconds / 60, startSeconds % 60);
		}
		else
			*p++ = 'C';
		s = ASTILCover_GetTitleAndSource(cover);
		p = appendStil(p, "overs", s[0] != '\0' ? s : "<?>");
		p = appendStil(p, "by", ASTILCover_GetArtist(cover));
		p = appendStil(p, "Comment", ASTILCover_GetComment(cover));
	}
	if (p > buf)
		p[-2] = '\0'; /* chomp trailing \r\n */
}

static double WINAPI ASAP_SetPosition(DWORD pos)
{
	int song = pos - XMPIN_POS_SUBSONG;
	if (song >= 0 && song < ASAPInfo_GetSongs(ASAP_GetInfo(asap))) {
		PlaySong(song);
		return 0;
	}
	// TODO: XMPIN_POS
	ASAP_Seek(asap, pos);
	return pos / 1000.0;
}

static double WINAPI ASAP_GetGranularity()
{
	return 0.001;
}

static DWORD WINAPI ASAP_Process(float *buf, DWORD count)
{
	/* Quick and dirty hack... Need to support floats directly... */
	short *buf2 = (short *) buf;
	DWORD n = ASAP_Generate(asap, (unsigned char *) buf2, count * sizeof(short), ASAPSampleFormat_S16_L_E) >> 1;
	int i;
	for (i = n; --i >= 0; )
		buf[i] = buf2[i] / 32767.0;
	return n;
}

static void WINAPI ASAP_GetSamples(char *buf)
{
	const ASAPInfo *info = ASAP_GetInfo(asap);
	const char *s = ASAPInfo_GetInstrumentName(info, module, module_len, 0);
	if (s != NULL) {
		int i = 0;
		buf += sprintf(buf, "instrument list:\r\n");
		do {
			buf += sprintf(buf, "%03d\t%s\r\n", i, s);
			s = ASAPInfo_GetInstrumentName(info, module, module_len, ++i);
		} while (s != NULL);
		buf[-2] = '\0'; /* chomp trailing \r\n */
	}
}

static DWORD WINAPI ASAP_GetSubSongs(float *length)
{
	const ASAPInfo *info = ASAP_GetInfo(asap);
	GetTotalLength(info, length);
	return ASAPInfo_GetSongs(info);
}

__declspec(dllexport) XMPIN *WINAPI XMPIN_GetInterface(DWORD face, InterfaceProc faceproc)
{
	static XMPIN xmpin = {
		0,
		"ASAP",
		"ASAP\0sap/cmc/cm3/cmr/cms/dmc/dlt/mpt/mpd/rmt/tmc/tm8/tm2/fc",
		ASAP_About,
		ASAP_Config,
		ASAP_CheckFile,
		ASAP_GetFileInfo,
		ASAP_Open,
		ASAP_Close,
		NULL,
		ASAP_SetFormat,
		ASAP_GetTags,
		ASAP_GetInfoText,
		ASAP_GetGeneralInfo,
		ASAP_GetMessage,
		ASAP_SetPosition,
		ASAP_GetGranularity,
		NULL,
		ASAP_Process,
		NULL,
		ASAP_GetSamples,
		ASAP_GetSubSongs,
		NULL,
		NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};
	static const XMPSHORTCUT info_shortcut = {
		0x10000, "ASAP - File information", ASAP_ShowInfo
	};

	if (face != XMPIN_FACE)
		return NULL;

	xmpfmisc = (const XMPFUNC_MISC *) faceproc(XMPFUNC_MISC_FACE);
	xmpfreg = (const XMPFUNC_REGISTRY *) faceproc(XMPFUNC_REGISTRY_FACE);
	xmpffile = (const XMPFUNC_FILE *) faceproc(XMPFUNC_FILE_FACE);
	xmpfin = (const XMPFUNC_IN *) faceproc(XMPFUNC_IN_FACE);

	asap = ASAP_New();

	xmpfmisc->RegisterShortcut(&info_shortcut);
	xmpfreg->GetInt("ASAP", "SongLength", &song_length);
	xmpfreg->GetInt("ASAP", "SilenceSeconds", &silence_seconds);
	xmpfreg->GetInt("ASAP", "PlayLoops", &play_loops);
	xmpfreg->GetInt("ASAP", "MuteMask", &mute_mask);
	xmpfreg->GetInt("ASAP", "PlayingInfo", &playing_info);
	return &xmpin;
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		hInst = hInstance;
	return TRUE;
}
