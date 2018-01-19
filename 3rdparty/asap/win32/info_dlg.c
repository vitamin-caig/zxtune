/*
 * info_dlg.c - file information dialog box
 *
 * Copyright (C) 2007-2014  Piotr Fusik
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
#include <stdio.h>
#include <string.h>
#include <tchar.h>
#ifndef _WIN32_IE
#define _WIN32_IE 0x400
#endif
#include <commctrl.h>

#ifndef MCM_SETCURRENTVIEW
/* missing in MinGW */
#define MCMV_MONTH      0
#define MCMV_YEAR       1
#define MCMV_DECADE     2
#define MCM_SETCURRENTVIEW 0x1020
#define MonthCal_SetCurrentView(hmc, dwNewView)  (BOOL) SNDMSG(hmc, MCM_SETCURRENTVIEW, 0, (LPARAM)(dwNewView))
#endif

#include "asap.h"
#include "astil.h"
#include "info_dlg.h"
#ifdef WINAMP
#include "aatr-stdio.h"
#endif

void combineFilenameExt(LPTSTR dest, LPCTSTR filename, LPCTSTR ext)
{
	int filenameChars = _tcsrchr(filename, '.') + 1 - filename;
	memcpy(dest, filename, filenameChars * sizeof(_TCHAR));
	_tcscpy(dest + filenameChars, ext);
}

#ifdef WINAMP
LPCTSTR atrFilenameHash(LPCTSTR filename)
{
	for ( ; *filename != '\0'; filename++) {
		if (_tcsnicmp(filename, _T(".atr#"), 5) == 0)
			return filename + 4;
	}
	return NULL;
}
#endif

BOOL loadModule(LPCTSTR filename, BYTE *module, int *module_len)
{
	HANDLE fh;
	BOOL ok;
#ifdef WINAMP
	LPCTSTR hash = atrFilenameHash(filename);
	if (hash != NULL && hash < filename + MAX_PATH) {
		_TCHAR atr_filename[MAX_PATH];
		memcpy(atr_filename, filename, (hash - filename) * sizeof(_TCHAR));
		atr_filename[hash - filename] = '\0';
		*module_len = AATRStdio_ReadFile(atr_filename, hash + 1, module, ASAPInfo_MAX_MODULE_LENGTH);
		return *module_len >= 0;
	}
#endif
	fh = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE)
		return FALSE;
	ok = ReadFile(fh, module, ASAPInfo_MAX_MODULE_LENGTH, (LPDWORD) module_len, NULL);
	CloseHandle(fh);
	return ok;
}

HWND infoDialog = NULL;

#ifndef _UNICODE /* TODO */

static _TCHAR playing_filename[MAX_PATH];
static int playing_song = 0;
BOOL playing_info = FALSE;
static byte saved_module[ASAPInfo_MAX_MODULE_LENGTH];
static int saved_module_len;
static ASAPInfo *edited_info = NULL;
static int edited_song;
static _TCHAR saved_author[ASAPInfo_MAX_TEXT_LENGTH + 1];
static _TCHAR saved_title[ASAPInfo_MAX_TEXT_LENGTH + 1];
static _TCHAR saved_date[ASAPInfo_MAX_TEXT_LENGTH + 1];
static int saved_durations[ASAPInfo_MAX_SONGS];
static cibool saved_loops[ASAPInfo_MAX_SONGS];
static BOOL can_save;
static int invalid_fields;
#define INVALID_FIELD_AUTHOR      1
#define INVALID_FIELD_NAME        2
#define INVALID_FIELD_DATE        4
#define INVALID_FIELD_TIME        8
#define INVALID_FIELD_TIME_SHOW  16
static HWND monthcal = NULL;
static WNDPROC monthcalOriginalWndProc;
static ASTIL *astil = NULL;

static char *appendStilString(char *p, const char *s)
{
	for (;;) {
		char c = *s++;
		switch (c) {
		case '\0':
			return p;
		case '\n':
			c = ' ';
			break;
		default:
			break;
		}
		*p++ = c;
	}
}

static char *appendStil(char *p, const char *prefix, const char *value)
{
	if (value[0] != '\0') {
		p = appendStilString(p, prefix);
		p = appendStilString(p, value);
		*p++ = '\r';
		*p++ = '\n';
	}
	return p;
}

static char *appendAddress(char *p, const char *format, int value)
{
	if (value >= 0)
		p += sprintf(p, format, value);
	return p;
}

static void chomp(char *s)
{
	size_t i = strlen(s);
	if (i >= 2 && s[i - 2] == '\r' && s[i - 1] == '\n')
		s[i - 2] = '\0';
}

static void updateTech(void)
{
	char buf[16000];
	char *p = buf;
	const char *ext;
	int type;
	int i;
	ext = ASAPInfo_GetOriginalModuleExt(edited_info, saved_module, saved_module_len);
	if (ext != NULL)
		p += sprintf(p, "Composed in %s\r\n", ASAPInfo_GetExtDescription(ext));
	i = ASAPInfo_GetSongs(edited_info);
	if (i > 1) {
		p += sprintf(p, "SONGS %d\r\n", i);
		i = ASAPInfo_GetDefaultSong(edited_info);
		if (i > 0)
			p += sprintf(p, "DEFSONG %d (song %d)\r\n", i, i + 1);
	}
	p += sprintf(p, ASAPInfo_GetChannels(edited_info) > 1 ? "STEREO\r\n" : "MONO\r\n");
	p += sprintf(p, ASAPInfo_IsNtsc(edited_info) ? "NTSC\r\n" : "PAL\r\n");
	type = ASAPInfo_GetTypeLetter(edited_info);
	if (type != 0)
		p += sprintf(p, "TYPE %c\r\n", type);
	p += sprintf(p, "FASTPLAY %d (%d Hz)\r\n", ASAPInfo_GetPlayerRateScanlines(edited_info), ASAPInfo_GetPlayerRateHz(edited_info));
	if (type == 'C')
		p += sprintf(p, "MUSIC %04X\r\n", ASAPInfo_GetMusicAddress(edited_info));
	if (type != 0) {
		p = appendAddress(p, "INIT %04X\r\n", ASAPInfo_GetInitAddress(edited_info));
		p = appendAddress(p, "PLAYER %04X\r\n", ASAPInfo_GetPlayerAddress(edited_info));
		p = appendAddress(p, "COVOX %04X\r\n", ASAPInfo_GetCovoxAddress(edited_info));
	}
	i = ASAPInfo_GetSapHeaderLength(edited_info);
	if (i >= 0) {
		while (p < buf + sizeof(buf) - 17 && i + 4 < saved_module_len) {
			int start = saved_module[i] + (saved_module[i + 1] << 8);
			int end;
			if (start == 0xffff) {
				i += 2;
				start = saved_module[i] + (saved_module[i + 1] << 8);
			}
			end = saved_module[i + 2] + (saved_module[i + 3] << 8);
			p += sprintf(p, "LOAD %04X-%04X\r\n", start, end);
			i += 5 + end - start;
		}
	}
	chomp(buf);
	SendDlgItemMessage(infoDialog, IDC_TECHINFO, WM_SETTEXT, 0, (LPARAM) buf);
}

static void updateStil(void)
{
	char buf[16000];
	char *p = buf;
	int i;
	p = appendStil(p, "", ASTIL_GetTitle(astil));
	p = appendStil(p, "by ", ASTIL_GetAuthor(astil));
	p = appendStil(p, "Directory comment: ", ASTIL_GetDirectoryComment(astil));
	p = appendStil(p, "File comment: ", ASTIL_GetFileComment(astil));
	p = appendStil(p, "Song comment: ", ASTIL_GetSongComment(astil));
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
		p = appendStil(p, "overs: ", s[0] != '\0' ? s : "<?>");
		p = appendStil(p, "by ", ASTILCover_GetArtist(cover));
		p = appendStil(p, "Comment: ", ASTILCover_GetComment(cover));
	}
	*p = '\0';
	chomp(buf);
#if 1
	/* not compatible with Windows 9x */
	if (ASTIL_IsUTF8(astil)) {
		WCHAR wBuf[16000];
		if (MultiByteToWideChar(CP_UTF8, 0, buf, -1, wBuf, 16000) > 0) {
			SendDlgItemMessageW(infoDialog, IDC_STILINFO, WM_SETTEXT, 0, (LPARAM) wBuf);
			return;
		}
	}
#endif
	SendDlgItemMessage(infoDialog, IDC_STILINFO, WM_SETTEXT, 0, (LPARAM) buf);
}

static void setEditedSong(int song)
{
	_TCHAR filename[MAX_PATH];
	unsigned char str[ASAPWriter_MAX_DURATION_LENGTH + 1];
	int len;

	edited_song = song;
	len = ASAPWriter_DurationToString(str, ASAPInfo_GetDuration(edited_info, song));
	str[len] = '\0';
	SendDlgItemMessage(infoDialog, IDC_TIME, WM_SETTEXT, 0, (LPARAM) str);
	CheckDlgButton(infoDialog, IDC_LOOP, ASAPInfo_GetLoop(edited_info, song) ? BST_CHECKED : BST_UNCHECKED);
	EnableWindow(GetDlgItem(infoDialog, IDC_LOOP), len > 0);

	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_GETTEXT, MAX_PATH, (LPARAM) filename);
	ASTIL_Load(astil, filename, song);
	SendDlgItemMessage(infoDialog, IDC_STILFILE, WM_SETTEXT, 0, (LPARAM) ASTIL_GetStilFilename(astil));
	updateStil();
}

static void showEditTip(int nID, LPCTSTR title, LPCTSTR message)
{
#ifndef _UNICODE

#ifndef EM_SHOWBALLOONTIP
/* missing in MinGW */
typedef struct
{
	DWORD cbStruct;
	LPCWSTR pszTitle;
	LPCWSTR pszText;
	INT ttiIcon;
} EDITBALLOONTIP;
#define TTI_ERROR  3
#define EM_SHOWBALLOONTIP  0x1503
#endif
	WCHAR wTitle[64];
	WCHAR wMessage[64];
	EDITBALLOONTIP ebt = { sizeof(EDITBALLOONTIP), wTitle, wMessage, TTI_ERROR };
	if (MultiByteToWideChar(CP_ACP, 0, title, -1, wTitle, 64) <= 0
	 || MultiByteToWideChar(CP_ACP, 0, message, -1, wMessage, 64) <= 0
	 || !SendDlgItemMessage(infoDialog, nID, EM_SHOWBALLOONTIP, 0, (LPARAM) &ebt))

#endif /* _UNICODE */

		/* Windows before XP don't support balloon tips */
		MessageBox(infoDialog, message, title, MB_OK | MB_ICONERROR);
}

static BOOL isExt(LPCTSTR filename, LPCTSTR ext)
{
	return _tcsicmp(filename + _tcslen(filename) - 4, ext) == 0;
}

static void setSaved(void)
{
	int i;
	_tcscpy(saved_author, ASAPInfo_GetAuthor(edited_info));
	_tcscpy(saved_title, ASAPInfo_GetTitle(edited_info));
	_tcscpy(saved_date, ASAPInfo_GetDate(edited_info));
	for (i = 0; i < ASAPInfo_GetSongs(edited_info); i++) {
		saved_durations[i] = ASAPInfo_GetDuration(edited_info, i);
		saved_loops[i] = ASAPInfo_GetLoop(edited_info, i);
	}
}

static BOOL infoChanged(void)
{
	int i;
	if (_tcscmp(ASAPInfo_GetAuthor(edited_info), saved_author) != 0)
		return TRUE;
	if (_tcscmp(ASAPInfo_GetTitle(edited_info), saved_title) != 0)
		return TRUE;
	if (_tcscmp(ASAPInfo_GetDate(edited_info), saved_date) != 0)
		return TRUE;
	for (i = 0; i < ASAPInfo_GetSongs(edited_info); i++) {
		if (ASAPInfo_GetDuration(edited_info, i) != saved_durations[i])
			return TRUE;
		if (saved_durations[i] >= 0 && ASAPInfo_GetLoop(edited_info, i) != saved_loops[i])
			return TRUE;
	}
	return FALSE;
}

static void updateSaveButtons(int mask, BOOL ok)
{
	if (ok) {
		invalid_fields &= ~mask;
		ok = invalid_fields == 0;
	}
	else
		invalid_fields |= mask;
	if (can_save)
		EnableWindow(GetDlgItem(infoDialog, IDC_SAVE), ok && infoChanged());
	EnableWindow(GetDlgItem(infoDialog, IDC_SAVEAS), ok);
}

static void updateInfoString(HWND hDlg, int nID, int mask, cibool (*func)(ASAPInfo *, LPCTSTR))
{
	_TCHAR str[ASAPInfo_MAX_TEXT_LENGTH + 1];
	BOOL ok;
	SendDlgItemMessage(hDlg, nID, WM_GETTEXT, ASAPInfo_MAX_TEXT_LENGTH + 1, (LPARAM) str);
	ok = func(edited_info, str);
	updateSaveButtons(mask, ok);
	if (!ok)
		showEditTip(nID, _T("Invalid characters"), _T("Avoid national characters and quotation marks"));
}

static LRESULT CALLBACK MonthCalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_KILLFOCUS:
		ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			ShowWindow(hWnd, SW_HIDE);
		break;
	case WM_DESTROY:
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR) monthcalOriginalWndProc);
		break;
	default:
		break;
	}
	return CallWindowProc(monthcalOriginalWndProc, hWnd, uMsg, wParam, lParam);
}

static void toggleCalendar(HWND hDlg)
{
	if (monthcal == NULL) {
		INITCOMMONCONTROLSEX icex;
		icex.dwSize = sizeof(icex);
		icex.dwICC = ICC_DATE_CLASSES;
		InitCommonControlsEx(&icex);
		monthcal = CreateWindowEx(0, MONTHCAL_CLASS, _T(""), WS_BORDER | WS_POPUP, 0, 0, 0, 0, hDlg, NULL, NULL, NULL);
		/* subclass month calendar, so that it hides when looses focus */
		monthcalOriginalWndProc = (WNDPROC) SetWindowLongPtr(monthcal, GWLP_WNDPROC, (LONG_PTR) MonthCalWndProc);
	}
	if (IsWindowVisible(monthcal))
		ShowWindow(monthcal, SW_HIDE);
	else {
		RECT rc;
		int x;
		int y;
		SYSTEMTIME st;
		GetWindowRect(GetDlgItem(hDlg, IDC_PICKDATE), &rc);
		x = rc.left;
		y = rc.bottom;
		MonthCal_GetMinReqRect(monthcal, &rc);
		SetWindowPos(monthcal, NULL, x, y, rc.right, rc.bottom, SWP_SHOWWINDOW | SWP_NOZORDER);
		y = ASAPInfo_GetYear(edited_info);
		if (y > 0) {
			DWORD view;
			int month = ASAPInfo_GetMonth(edited_info);
			st.wYear = y;
			if (month > 0) {
				int day = ASAPInfo_GetDayOfMonth(edited_info);
				st.wMonth = month;
				if (day > 0) {
					st.wDay = day;
					view = MCMV_MONTH;
				}
				else {
					st.wDay = 1;
					view = MCMV_YEAR;
				}
			}
			else {
				st.wMonth = 1;
				st.wDay = 1;
				view = MCMV_DECADE;
			}
			(void) MonthCal_SetCurSel(monthcal, &st);
			(void) MonthCal_SetCurrentView(monthcal, view);
		}
		SetFocus(monthcal);
	}
}

static void saveByte(void *obj, int data)
{
	putc(data, (FILE *) obj);
}

static BOOL doSaveFile(LPCTSTR filename, BOOL tag)
{
	FILE *fp = _tfopen(filename, _T("wb"));
	ByteWriter bw;
	if (fp == NULL)
		return FALSE;
	bw.obj = fp;
	bw.func = saveByte;
	if (!ASAPWriter_Write(filename, bw, edited_info, saved_module, saved_module_len, tag)) {
		fclose(fp);
		DeleteFile(filename);
		return FALSE;
	}
	if (fclose(fp) != 0) {
		DeleteFile(filename);
		return FALSE;
	}
	return TRUE;
}

static BOOL saveFile(LPCTSTR filename, BOOL tag)
{
	BOOL isSap = isExt(filename, _T(".sap"));
	if (isSap) {
		int song = ASAPInfo_GetSongs(edited_info);
		while (--song >= 0 && ASAPInfo_GetDuration(edited_info, song) < 0);
		while (--song >= 0) {
			if (ASAPInfo_GetDuration(edited_info, song) < 0) {
				MessageBox(infoDialog, _T("Cannot save file because time not set for all songs"), _T("Error"), MB_OK | MB_ICONERROR);
				return FALSE;
			}
		}
	}
	if (!doSaveFile(filename, tag)) {
		MessageBox(infoDialog, _T("Cannot save file"), _T("Error"), MB_OK | MB_ICONERROR);
		// FIXME: delete file
		return FALSE;
	}
	if (isSap)
		setSaved();
	return TRUE;
}

static BOOL saveInfo(void)
{
	_TCHAR filename[MAX_PATH];
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_GETTEXT, MAX_PATH, (LPARAM) filename);
	return saveFile(filename, FALSE);
}

static void addFilterExt(void *obj, const char *ext)
{
	LPTSTR p = *(LPTSTR *) obj;
	p += _stprintf(p, _T("%s (*.%s)"), ASAPInfo_GetExtDescription(ext), ext) + 1;
	p += _stprintf(p, _T("*.%s"), ext) + 1;
	*p = '\0';
	*(LPTSTR *) obj = p;
}

static void setSaveFilters(LPTSTR p)
{
	StringConsumer sc = { &p, addFilterExt };
	ASAPWriter_EnumSaveExts(sc, edited_info, saved_module, saved_module_len);
}

static BOOL saveInfoAs(void)
{
	_TCHAR filename[MAX_PATH];
	_TCHAR filter[1024];
	OPENFILENAME ofn = {
		sizeof(OPENFILENAME),
		infoDialog,
		0,
		filter,
		NULL,
		0,
		1,
		filename,
		MAX_PATH,
		NULL,
		0,
		NULL,
		_T("Select output file"),
		OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT,
		0,
		0,
		NULL,
		0,
		NULL,
		NULL
	};
	LPTSTR ext;
	BOOL tag = FALSE;
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_GETTEXT, MAX_PATH, (LPARAM) filename);
	ext = _tcsrchr(filename, '.');
	if (ext != NULL) {
		*ext = '\0';
		ofn.lpstrDefExt = ext + 1;
	}
	setSaveFilters(filter);
	if (!GetSaveFileName(&ofn))
		return FALSE;
	if (isExt(filename, _T(".xex"))) {
		switch (MessageBox(infoDialog, _T("Do you want to display information during playback?"), _T("ASAP"), MB_YESNOCANCEL | MB_ICONQUESTION)) {
		case IDYES:
			tag = TRUE;
			break;
		case IDCANCEL:
			return FALSE;
		default:
			break;
		}
	}
	return saveFile(filename, tag);
}

static void closeInfoDialog(void)
{
	DestroyWindow(infoDialog);
	infoDialog = NULL;
	ASAPInfo_Delete(edited_info);
	edited_info = NULL;
	ASTIL_Delete(astil);
	astil = NULL;
}

static INT_PTR CALLBACK infoDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
#ifdef PLAYING_INFO
		CheckDlgButton(hDlg, IDC_PLAYING, playing_info ? BST_CHECKED : BST_UNCHECKED);
#endif
		SendDlgItemMessage(hDlg, IDC_AUTHOR, EM_LIMITTEXT, ASAPInfo_MAX_TEXT_LENGTH, 0);
		SendDlgItemMessage(hDlg, IDC_NAME, EM_LIMITTEXT, ASAPInfo_MAX_TEXT_LENGTH, 0);
		SendDlgItemMessage(hDlg, IDC_DATE, EM_LIMITTEXT, ASAPInfo_MAX_TEXT_LENGTH, 0);
		SendDlgItemMessage(hDlg, IDC_TIME, EM_LIMITTEXT, ASAPWriter_MAX_DURATION_LENGTH, 0);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
#ifdef PLAYING_INFO
		case MAKEWPARAM(IDC_PLAYING, BN_CLICKED):
			playing_info = (IsDlgButtonChecked(hDlg, IDC_PLAYING) == BST_CHECKED);
			if (playing_info && playing_filename[0] != '\0')
				updateInfoDialog(playing_filename, playing_song);
			onUpdatePlayingInfo();
			return TRUE;
#endif
		case MAKEWPARAM(IDC_AUTHOR, EN_CHANGE):
			updateInfoString(hDlg, IDC_AUTHOR, INVALID_FIELD_AUTHOR, ASAPInfo_SetAuthor);
			return TRUE;
		case MAKEWPARAM(IDC_NAME, EN_CHANGE):
			updateInfoString(hDlg, IDC_NAME, INVALID_FIELD_NAME, ASAPInfo_SetTitle);
			return TRUE;
		case MAKEWPARAM(IDC_DATE, EN_CHANGE):
			updateInfoString(hDlg, IDC_DATE, INVALID_FIELD_DATE, ASAPInfo_SetDate);
			return TRUE;
		case MAKEWPARAM(IDC_PICKDATE, BN_CLICKED):
			toggleCalendar(hDlg);
			return TRUE;
		case MAKEWPARAM(IDC_TIME, EN_CHANGE):
			{
				char str[ASAPWriter_MAX_DURATION_LENGTH + 1];
				int duration;
				SendDlgItemMessage(hDlg, IDC_TIME, WM_GETTEXT, ASAPWriter_MAX_DURATION_LENGTH + 1, (LPARAM) str);
				duration = ASAPInfo_ParseDuration(str);
				ASAPInfo_SetDuration(edited_info, edited_song, duration);
				EnableWindow(GetDlgItem(infoDialog, IDC_LOOP), str[0] != '\0');
				updateSaveButtons(INVALID_FIELD_TIME | INVALID_FIELD_TIME_SHOW, duration >=0 || str[0] == '\0');
			}
			return TRUE;
		case MAKEWPARAM(IDC_TIME, EN_KILLFOCUS):
			if ((invalid_fields & INVALID_FIELD_TIME_SHOW) != 0) {
				invalid_fields &= ~INVALID_FIELD_TIME_SHOW;
				showEditTip(IDC_TIME, _T("Invalid format"), _T("Please type MM:SS.mmm"));
			}
			return TRUE;
		case MAKEWPARAM(IDC_LOOP, BN_CLICKED):
			ASAPInfo_SetLoop(edited_info, edited_song, IsDlgButtonChecked(hDlg, IDC_LOOP) == BST_CHECKED);
			updateSaveButtons(0, TRUE);
			return TRUE;
		case MAKEWPARAM(IDC_SONGNO, CBN_SELCHANGE):
			setEditedSong(SendDlgItemMessage(hDlg, IDC_SONGNO, CB_GETCURSEL, 0, 0));
			updateSaveButtons(INVALID_FIELD_TIME | INVALID_FIELD_TIME_SHOW, TRUE);
			return TRUE;
		case MAKEWPARAM(IDC_SAVE, BN_CLICKED):
			saveInfo();
			return TRUE;
		case MAKEWPARAM(IDC_SAVEAS, BN_CLICKED):
			saveInfoAs();
			return TRUE;
		case MAKEWPARAM(IDCANCEL, BN_CLICKED):
			if (invalid_fields == 0 && infoChanged()) {
				switch (MessageBox(hDlg, _T("Save changes?"), _T("ASAP"), MB_YESNOCANCEL | MB_ICONQUESTION)) {
				case IDYES:
					if (!saveInfoAs())
						return TRUE;
					break;
				case IDCANCEL:
					return TRUE;
				default:
					break;
				}
			}
			closeInfoDialog();
			return TRUE;
		}
		break;
	case WM_NOTIFY: {
			LPNMSELCHANGE psc = (LPNMSELCHANGE) lParam;
			if (psc->nmhdr.hwndFrom == monthcal && psc->nmhdr.code == MCN_SELECT) {
				_TCHAR str[32];
				ShowWindow(monthcal, SW_HIDE);
				_stprintf(str, _T("%02d/%02d/%d"), psc->stSelStart.wDay, psc->stSelStart.wMonth, psc->stSelStart.wYear);
				SetDlgItemText(hDlg, IDC_DATE, str);
				ASAPInfo_SetDate(edited_info, str);
				updateSaveButtons(INVALID_FIELD_DATE, TRUE);
			}
		}
		break;
	case WM_DESTROY:
		if (monthcal != NULL) {
			DestroyWindow(monthcal);
			monthcal = NULL;
		}
		return 0;
	default:
		break;
	}
	return FALSE;
}

void showInfoDialog(HINSTANCE hInstance, HWND hwndParent, LPCTSTR filename, int song)
{
	if (infoDialog == NULL)
		infoDialog = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_INFO), hwndParent, infoDialogProc);
	if (playing_info || filename == NULL)
		updateInfoDialog(playing_filename, playing_song);
	else
		updateInfoDialog(filename, song);
}

void updateInfoDialog(LPCTSTR filename, int song)
{
	int songs;
	int i;
	if (infoDialog == NULL)
		return;
	if (edited_info == NULL) {
		edited_info = ASAPInfo_New();
		if (astil == NULL)
			astil = ASTIL_New();
		if (edited_info == NULL || astil == NULL) {
			closeInfoDialog();
			return;
		}
	}
	else if (infoChanged())
		return;
	if (!loadModule(filename, saved_module, &saved_module_len)
	 || !ASAPInfo_Load(edited_info, filename, saved_module, saved_module_len)) {
		closeInfoDialog();
		return;
	}
	setSaved();
	can_save = isExt(filename, _T(".sap"));
	invalid_fields = 0;
	SendDlgItemMessage(infoDialog, IDC_FILENAME, WM_SETTEXT, 0, (LPARAM) filename);
	SendDlgItemMessage(infoDialog, IDC_AUTHOR, WM_SETTEXT, 0, (LPARAM) saved_author);
	SendDlgItemMessage(infoDialog, IDC_NAME, WM_SETTEXT, 0, (LPARAM) saved_title);
	SendDlgItemMessage(infoDialog, IDC_DATE, WM_SETTEXT, 0, (LPARAM) saved_date);
	SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_RESETCONTENT, 0, 0);
	songs = ASAPInfo_GetSongs(edited_info);
	EnableWindow(GetDlgItem(infoDialog, IDC_SONGNO), songs > 1);
	for (i = 1; i <= songs; i++) {
		_TCHAR str[16];
		_stprintf(str, _T("%d"), i);
		SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_ADDSTRING, 0, (LPARAM) str);
	}
	if (song < 0)
		song = ASAPInfo_GetDefaultSong(edited_info);
	SendDlgItemMessage(infoDialog, IDC_SONGNO, CB_SETCURSEL, song, 0);
	setEditedSong(song);
	EnableWindow(GetDlgItem(infoDialog, IDC_SAVE), FALSE);
	updateTech();
}

void setPlayingSong(LPCTSTR filename, int song)
{
	if (filename != NULL && _tcslen(filename) < MAX_PATH - 1)
		_tcscpy(playing_filename, filename);
	playing_song = song;
	if (playing_info)
		updateInfoDialog(playing_filename, song);
}

const ASTIL *getPlayingASTIL(void)
{
	if (astil == NULL)
		astil = ASTIL_New();
	ASTIL_Load(astil, playing_filename, playing_song);
	return astil;
}

#endif /* _UNICODE */
