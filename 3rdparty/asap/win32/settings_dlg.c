/*
 * settings_dlg.c - settings dialog box
 *
 * Copyright (C) 2007-2011  Piotr Fusik
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

#include "asap.h"
#include "settings_dlg.h"

void enableTimeInput(HWND hDlg, BOOL enable)
{
	EnableWindow(GetDlgItem(hDlg, IDC_MINUTES), enable);
	EnableWindow(GetDlgItem(hDlg, IDC_SECONDS), enable);
}

void setFocusAndSelect(HWND hDlg, int nID)
{
	HWND hWnd = GetDlgItem(hDlg, nID);
	SetFocus(hWnd);
	SendMessage(hWnd, EM_SETSEL, 0, -1);
}

void settingsDialogSet(HWND hDlg, int song_length, int silence_seconds, BOOL play_loops, int mute_mask)
{
	int i;
	if (song_length <= 0) {
		CheckRadioButton(hDlg, IDC_UNLIMITED, IDC_LIMITED, IDC_UNLIMITED);
		SetDlgItemInt(hDlg, IDC_MINUTES, DEFAULT_SONG_LENGTH / 60, FALSE);
		SetDlgItemInt(hDlg, IDC_SECONDS, DEFAULT_SONG_LENGTH % 60, FALSE);
		enableTimeInput(hDlg, FALSE);
	}
	else {
		CheckRadioButton(hDlg, IDC_UNLIMITED, IDC_LIMITED, IDC_LIMITED);
		SetDlgItemInt(hDlg, IDC_MINUTES, (UINT) song_length / 60, FALSE);
		SetDlgItemInt(hDlg, IDC_SECONDS, (UINT) song_length % 60, FALSE);
		enableTimeInput(hDlg, TRUE);
	}
	if (silence_seconds <= 0) {
		CheckDlgButton(hDlg, IDC_SILENCE, BST_UNCHECKED);
		SetDlgItemInt(hDlg, IDC_SILSECONDS, DEFAULT_SILENCE_SECONDS, FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_SILSECONDS), FALSE);
	}
	else {
		CheckDlgButton(hDlg, IDC_SILENCE, BST_CHECKED);
		SetDlgItemInt(hDlg, IDC_SILSECONDS, (UINT) silence_seconds, FALSE);
		EnableWindow(GetDlgItem(hDlg, IDC_SILSECONDS), TRUE);
	}
	CheckRadioButton(hDlg, IDC_LOOPS, IDC_NOLOOPS, play_loops ? IDC_LOOPS : IDC_NOLOOPS);
	for (i = 0; i < 8; i++)
		CheckDlgButton(hDlg, IDC_MUTE1 + i, ((mute_mask >> i) & 1) != 0 ? BST_CHECKED : BST_UNCHECKED);
}

#ifndef FOOBAR2000

int song_length = -1;
int silence_seconds = -1;
BOOL play_loops = FALSE;
int mute_mask = 0;
static int saved_mute_mask;

static BOOL getDlgInt(HWND hDlg, int nID, int *result)
{
	BOOL translated;
	UINT r = GetDlgItemInt(hDlg, nID, &translated, FALSE);
	if (!translated) {
		MessageBox(hDlg, _T("Invalid number"), _T("Error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}
	*result = (int) r;
	return TRUE;
}

static INT_PTR CALLBACK settingsDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		saved_mute_mask = mute_mask;
		settingsDialogSet(hDlg, song_length, silence_seconds, play_loops, mute_mask);
		return TRUE;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) {
			WORD wCtrl = LOWORD(wParam);
			BOOL enabled;
			switch (wCtrl) {
			case IDC_UNLIMITED:
				enableTimeInput(hDlg, FALSE);
				return TRUE;
			case IDC_LIMITED:
				enableTimeInput(hDlg, TRUE);
				setFocusAndSelect(hDlg, IDC_MINUTES);
				return TRUE;
			case IDC_SILENCE:
				enabled = (IsDlgButtonChecked(hDlg, IDC_SILENCE) == BST_CHECKED);
				EnableWindow(GetDlgItem(hDlg, IDC_SILSECONDS), enabled);
				if (enabled)
					setFocusAndSelect(hDlg, IDC_SILSECONDS);
				return TRUE;
			case IDC_LOOPS:
			case IDC_NOLOOPS:
				return TRUE;
			case IDC_MUTE1:
			case IDC_MUTE1 + 1:
			case IDC_MUTE1 + 2:
			case IDC_MUTE1 + 3:
			case IDC_MUTE1 + 4:
			case IDC_MUTE1 + 5:
			case IDC_MUTE1 + 6:
			case IDC_MUTE1 + 7:
			{
				int mask = 1 << (wCtrl - IDC_MUTE1);
				if (IsDlgButtonChecked(hDlg, wCtrl) == BST_CHECKED)
					mute_mask |= mask;
				else
					mute_mask &= ~mask;
				ASAP_MutePokeyChannels(asap, mute_mask);
				return TRUE;
			}
			case IDOK:
			{
				int new_song_length;
				if (IsDlgButtonChecked(hDlg, IDC_UNLIMITED) == BST_CHECKED)
					new_song_length = -1;
				else {
					int minutes;
					int seconds;
					if (!getDlgInt(hDlg, IDC_MINUTES, &minutes)
					 || !getDlgInt(hDlg, IDC_SECONDS, &seconds))
						return TRUE;
					new_song_length = 60 * minutes + seconds;
				}
				if (IsDlgButtonChecked(hDlg, IDC_SILENCE) != BST_CHECKED)
					silence_seconds = -1;
				else if (!getDlgInt(hDlg, IDC_SILSECONDS, &silence_seconds))
					return TRUE;
				song_length = new_song_length;
				play_loops = (IsDlgButtonChecked(hDlg, IDC_LOOPS) == BST_CHECKED);
				EndDialog(hDlg, IDOK);
				return TRUE;
			}
			case IDCANCEL:
				mute_mask = saved_mute_mask;
				ASAP_MutePokeyChannels(asap, mute_mask);
				EndDialog(hDlg, IDCANCEL);
				return TRUE;
			}
		}
		break;
	default:
		break;
	}
	return FALSE;
}

BOOL settingsDialog(HINSTANCE hInstance, HWND hwndParent)
{
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_SETTINGS), hwndParent, settingsDialogProc) == IDOK;
}

int getSongDurationInternal(const ASAPInfo *info, int song, ASAP *asap)
{
	int duration = ASAPInfo_GetDuration(info, song);
	if (duration < 0) {
		if (asap != NULL)
			ASAP_DetectSilence(asap, silence_seconds);
		return 1000 * song_length;
	}
	if (asap != NULL)
		ASAP_DetectSilence(asap, 0);
	if (play_loops && ASAPInfo_GetLoop(info, song))
		return 1000 * song_length;
	return duration;
}

int playSong(int song)
{
	int duration = getSongDurationInternal(ASAP_GetInfo(asap), song, asap);
	ASAP_PlaySong(asap, song, duration); /* FIXME: check errors */
	ASAP_MutePokeyChannels(asap, mute_mask);
	return duration;
}

#endif /* FOOBAR2000 */
