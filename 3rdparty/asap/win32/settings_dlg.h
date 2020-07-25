/*
 * settings_dlg.h - settings dialog box
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

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SONG_LENGTH      180
#define DEFAULT_SILENCE_SECONDS  2

#define IDD_SETTINGS   400
#define IDC_UNLIMITED  401
#define IDC_LIMITED    402
#define IDC_MINUTES    403
#define IDC_SECONDS    404
#define IDC_SILENCE    405
#define IDC_SILSECONDS 406
#define IDC_LOOPS      407
#define IDC_NOLOOPS    408
#define IDC_MUTE1      411

#ifdef FOOBAR2000
void enableTimeInput(HWND hDlg, BOOL enable);
void setFocusAndSelect(HWND hDlg, int nID);
void settingsDialogSet(HWND hDlg, int song_length, int silence_seconds, BOOL play_loops, int mute_mask);
#else
extern ASAP *asap;
extern int song_length;
extern int silence_seconds;
extern BOOL play_loops;
extern int mute_mask;
BOOL settingsDialog(HINSTANCE hInstance, HWND hwndParent);
int getSongDurationInternal(const ASAPInfo *module_info, int song, ASAP *asap);
#define getSongDuration(module_info, song)  getSongDurationInternal(module_info, song, NULL)
int playSong(int song);
#endif

#ifdef __cplusplus
}
#endif
