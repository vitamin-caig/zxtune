/*
 * foo_asap.cpp - ASAP plugin for foobar2000
 *
 * Copyright (C) 2006-2014  Piotr Fusik
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
#include <string.h>

#include "aatr-stdio.h"
#include "asap.h"
#include "info_dlg.h"
#include "settings_dlg.h"

#define UNICODE /* NOT for info_dlg.h */
#include "foobar2000/SDK/foobar2000.h"

#define BITS_PER_SAMPLE    16
#define BUFFERED_BLOCKS    1024

/* Configuration --------------------------------------------------------- */

static const GUID song_length_guid =
	{ 0x810e12f0, 0xa695, 0x42d2, { 0xab, 0xc0, 0x14, 0x1e, 0xe5, 0xf3, 0xb3, 0xb7 } };
static cfg_int song_length(song_length_guid, -1);

static const GUID silence_seconds_guid =
	{ 0x40170cb0, 0xc18c, 0x4f97, { 0xaa, 0x06, 0xdb, 0xe7, 0x45, 0xb0, 0x7e, 0x1d } };
static cfg_int silence_seconds(silence_seconds_guid, -1);

static const GUID play_loops_guid =
	{ 0xf08d12f8, 0x58d6, 0x49fc, { 0xae, 0xc5, 0xf4, 0xd0, 0x2f, 0xb2, 0x20, 0xaf } };
static cfg_bool play_loops(play_loops_guid, false);

static const GUID mute_mask_guid =
	{ 0x8bd94472, 0x82f1, 0x4e77, { 0x95, 0x78, 0xe9, 0x84, 0x75, 0xad, 0x17, 0x04 } };
static cfg_int mute_mask(mute_mask_guid, 0);

static const GUID playing_info_guid =
	{ 0x8a2d4509, 0x405e, 0x482e, { 0xa1, 0x30, 0x3f, 0x0c, 0xd6, 0x16, 0x98, 0x59 } };
static cfg_bool playing_info_cfg(playing_info_guid, false);

void onUpdatePlayingInfo()
{
	playing_info_cfg = playing_info != FALSE; /* avoid warning C4800 */
}


/* Decoding -------------------------------------------------------------- */

class input_asap
{
	static input_asap *head;
	input_asap *prev;
	input_asap *next;
	service_ptr_t<file> m_file;
	char *url;
	BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
	int module_len;
	ASAP *asap;
	abort_callback *p_abort;

	int get_song_duration(int song, bool play)
	{
		const ASAPInfo *info = ASAP_GetInfo(asap);
		int duration = ASAPInfo_GetDuration(info, song);
		if (duration < 0) {
			if (play)
				ASAP_DetectSilence(asap, silence_seconds);
			return 1000 * song_length;
		}
		if (play)
			ASAP_DetectSilence(asap, 0);
		if (play_loops && ASAPInfo_GetLoop(info, song))
			return 1000 * song_length;
		return duration;
	}

	static void meta_set(file_info &p_info, const char *p_name, const char *p_value)
	{
		if (p_value[0] != '\0')
			p_info.meta_set(p_name, p_value);
	}

	static const char *empty_if_null(const char *s)
	{
		return s != NULL ? s : "";
	}

	void write(int data)
	{
		BYTE b = static_cast<BYTE>(data);
		m_file->write(&b, 1, *p_abort);
	}

	static void static_write(void *obj, int data)
	{
		static_cast<input_asap *>(obj)->write(data);
	}

public:

	static void g_set_mute_mask(int mask)
	{
		input_asap *p;
		for (p = head; p != NULL; p = p->next)
			ASAP_MutePokeyChannels(p->asap, mask);
	}

	static bool g_is_our_content_type(const char *p_content_type)
	{
		return false;
	}

	static bool g_is_our_path(const char *p_path, const char *p_extension)
	{
		return ASAPInfo_IsOurFile(p_path) != 0;
	}

	input_asap()
	{
		if (head != NULL)
			head->prev = this;
		prev = NULL;
		next = head;
		head = this;
		url = NULL;
		asap = ASAP_New();
	}

	~input_asap()
	{
		ASAP_Delete(asap);
		free(url);
		if (prev != NULL)
			prev->next = next;
		if (next != NULL)
			next->prev = prev;
		if (head == this)
			head = next;
	}

	void open(service_ptr_t<file> p_filehint, const char *p_path, t_input_open_reason p_reason, abort_callback &p_abort)
	{
		switch (p_reason) {
		case input_open_info_write: {
				int len = strlen(p_path);
				if (len < 4 || _stricmp(p_path + len - 4, ".sap") != 0)
					throw exception_io_unsupported_format();
			}
			/* FALLTHROUGH */
		case input_open_decode:
			free(url);
			url = strdup(p_path);
			break;
		default:
			break;
		}
		if (p_filehint.is_empty())
			filesystem::g_open(p_filehint, p_path, filesystem::open_mode_read, p_abort);
		m_file = p_filehint;
		module_len = m_file->read(module, sizeof(module), p_abort);
		if (!ASAP_Load(asap, p_path, module, module_len))
			throw exception_io_unsupported_format();
	}

	t_uint32 get_subsong_count()
	{
		return ASAPInfo_GetSongs(ASAP_GetInfo(asap));
	}

	t_uint32 get_subsong(t_uint32 p_index)
	{
		return p_index;
	}

	void get_info(t_uint32 p_subsong, file_info &p_info, abort_callback &p_abort)
	{
		int duration = get_song_duration(p_subsong, false);
		if (duration >= 0)
			p_info.set_length(duration / 1000.0);
		const ASAPInfo *info = ASAP_GetInfo(asap);
		p_info.info_set_int("channels", ASAPInfo_GetChannels(info));
		p_info.info_set_int("subsongs", ASAPInfo_GetSongs(info));
		meta_set(p_info, "composer", ASAPInfo_GetAuthor(info));
		meta_set(p_info, "title", ASAPInfo_GetTitle(info));
		meta_set(p_info, "date", ASAPInfo_GetDate(info));
	}

	t_filestats get_file_stats(abort_callback &p_abort)
	{
		return m_file->get_stats(p_abort);
	}

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback &p_abort)
	{
		int duration = get_song_duration(p_subsong, true);
		if (!ASAP_PlaySong(asap, p_subsong, duration))
			throw exception_io_unsupported_format();
		ASAP_MutePokeyChannels(asap, mute_mask);
		const char *filename = url;
		if (foobar2000_io::_extract_native_path_ptr(filename))
			setPlayingSong(filename, p_subsong);
	}

	bool decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
	{
		int channels = ASAPInfo_GetChannels(ASAP_GetInfo(asap));
		int buffered_bytes = BUFFERED_BLOCKS * channels * (BITS_PER_SAMPLE / 8);
		BYTE buffer[BUFFERED_BLOCKS * 2 * (BITS_PER_SAMPLE / 8)];

		buffered_bytes = ASAP_Generate(asap, buffer, buffered_bytes,
			BITS_PER_SAMPLE == 8 ? ASAPSampleFormat_U8 : ASAPSampleFormat_S16_L_E);
		if (buffered_bytes == 0)
			return false;
		p_chunk.set_data_fixedpoint(buffer, buffered_bytes, ASAP_SAMPLE_RATE,
			channels, BITS_PER_SAMPLE,
			channels == 2 ? audio_chunk::channel_config_stereo : audio_chunk::channel_config_mono);
		return true;
	}

	void decode_seek(double p_seconds, abort_callback &p_abort)
	{
		ASAP_Seek(asap, static_cast<int>(p_seconds * 1000));
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info(file_info &p_out, double &p_timestamp_delta)
	{
		return false;
	}

	bool decode_get_dynamic_info_track(file_info &p_out, double &p_timestamp_delta)
	{
		return false;
	}

	void decode_on_idle(abort_callback &p_abort)
	{
		m_file->on_idle(p_abort);
	}

	void retag_set_info(t_uint32 p_subsong, const file_info &p_info, abort_callback &p_abort)
	{
		ASAPInfo *info = const_cast<ASAPInfo *>(ASAP_GetInfo(asap));
		ASAPInfo_SetAuthor(info, empty_if_null(p_info.meta_get("composer", 0)));
		ASAPInfo_SetTitle(info, empty_if_null(p_info.meta_get("title", 0)));
		ASAPInfo_SetDate(info, empty_if_null(p_info.meta_get("date", 0)));
	}

	void retag_commit(abort_callback &p_abort)
	{
		m_file.release();
		filesystem::g_open(m_file, url, filesystem::open_mode_write_new, p_abort);
		this->p_abort = &p_abort;
		ByteWriter bw = { this, static_write };
		if (!ASAPWriter_Write(url, bw, ASAP_GetInfo(asap), module, module_len, FALSE))
			throw exception_io_unsupported_format();
	}
};

input_asap *input_asap::head = NULL;
static input_factory_t<input_asap> g_input_asap_factory;


/* Configuration --------------------------------------------------------- */

static preferences_page_callback::ptr g_callback;

static INT_PTR CALLBACK settings_dialog_proc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		settingsDialogSet(hDlg, song_length, silence_seconds, play_loops, mute_mask);
		return TRUE;
	case WM_COMMAND:
		switch (wParam) {
		case MAKEWPARAM(IDC_UNLIMITED, BN_CLICKED):
			enableTimeInput(hDlg, FALSE);
			g_callback->on_state_changed();
			return TRUE;
		case MAKEWPARAM(IDC_LIMITED, BN_CLICKED):
			enableTimeInput(hDlg, TRUE);
			setFocusAndSelect(hDlg, IDC_MINUTES);
			g_callback->on_state_changed();
			return TRUE;
		case MAKEWPARAM(IDC_SILENCE, BN_CLICKED):
		{
			BOOL enabled = (IsDlgButtonChecked(hDlg, IDC_SILENCE) == BST_CHECKED);
			EnableWindow(GetDlgItem(hDlg, IDC_SILSECONDS), enabled);
			if (enabled)
				setFocusAndSelect(hDlg, IDC_SILSECONDS);
			g_callback->on_state_changed();
			return TRUE;
		}
		case MAKEWPARAM(IDC_MINUTES, EN_CHANGE):
		case MAKEWPARAM(IDC_SECONDS, EN_CHANGE):
		case MAKEWPARAM(IDC_SILSECONDS, EN_CHANGE):
		case MAKEWPARAM(IDC_LOOPS, BN_CLICKED):
		case MAKEWPARAM(IDC_NOLOOPS, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 1, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 2, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 3, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 4, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 5, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 6, BN_CLICKED):
		case MAKEWPARAM(IDC_MUTE1 + 7, BN_CLICKED):
			g_callback->on_state_changed();
			return TRUE;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

class preferences_page_instance_asap : public preferences_page_instance
{
	HWND m_parent;
	HWND m_hWnd;

	int get_time_input()
	{
		HWND hDlg = m_hWnd;
		if (IsDlgButtonChecked(hDlg, IDC_UNLIMITED) == BST_CHECKED)
			return -1;
		UINT minutes = GetDlgItemInt(hDlg, IDC_MINUTES, NULL, FALSE);
		UINT seconds = GetDlgItemInt(hDlg, IDC_SECONDS, NULL, FALSE);
		return static_cast<int>(60 * minutes + seconds);
	}

	int get_silence_input()
	{
		HWND hDlg = m_hWnd;
		if (IsDlgButtonChecked(hDlg, IDC_SILENCE) != BST_CHECKED)
			return -1;
		return GetDlgItemInt(hDlg, IDC_SILSECONDS, NULL, FALSE);
	}

	bool get_loops_input()
	{
		return IsDlgButtonChecked(m_hWnd, IDC_LOOPS) == BST_CHECKED;
	}

	int get_mute_input()
	{
		HWND hDlg = m_hWnd;
		int mask = 0;
		for (int i = 0; i < 8; i++)
			if (IsDlgButtonChecked(hDlg, IDC_MUTE1 + i) == BST_CHECKED)
				mask |= 1 << i;
		return mask;
	}

public:

	preferences_page_instance_asap(HWND parent) : m_parent(parent)
	{
		m_hWnd = CreateDialog(core_api::get_my_instance(), MAKEINTRESOURCE(IDD_SETTINGS), parent, ::settings_dialog_proc);
	}

	virtual t_uint32 get_state()
	{
		if (song_length != get_time_input()
		 || silence_seconds != get_silence_input()
		 || play_loops != get_loops_input())
			return preferences_state::changed /* | preferences_state::needs_restart_playback */ | preferences_state::resettable;
		if (mute_mask != get_mute_input())
			return preferences_state::changed | preferences_state::resettable;
		return preferences_state::resettable;
	}

	virtual HWND get_wnd()
	{
		return m_hWnd;
	}

	virtual void apply()
	{
		song_length = get_time_input();
		silence_seconds = get_silence_input();
		play_loops = get_loops_input();
		mute_mask = get_mute_input();
		input_asap::g_set_mute_mask(mute_mask);
		g_callback->on_state_changed();
	}

	virtual void reset()
	{
		settingsDialogSet(m_hWnd, -1, -1, FALSE, 0);
		g_callback->on_state_changed();
	}
};

class preferences_page_asap : public preferences_page_v3
{
public:

	virtual preferences_page_instance::ptr instantiate(HWND parent, preferences_page_callback::ptr callback)
	{
		g_callback = callback;
		return new service_impl_t<preferences_page_instance_asap>(parent);
	}

	virtual const char *get_name()
	{
		return "ASAP";
	}

	virtual GUID get_guid()
	{
		static const GUID a_guid =
			{ 0xf7c0a763, 0x7c20, 0x4b64, { 0x92, 0xbf, 0x11, 0xe5, 0x5d, 0x8, 0xe5, 0x53 } };
		return a_guid;
	}

	virtual GUID get_parent_guid()
	{
		return guid_input;
	}

	virtual bool get_help_url(pfc::string_base &p_out)
	{
		p_out = "http://asap.sourceforge.net/";
		return true;
	}
};

static service_factory_single_t<preferences_page_asap> g_preferences_page_asap_factory;


/* File types ------------------------------------------------------------ */

static const char * const names_and_masks[][2] = {
	{ "Slight Atari Player", "*.SAP" },
	{ "Chaos Music Composer", "*.CMC;*.CM3;*.CMR;*.CMS;*.DMC" },
	{ "Delta Music Composer", "*.DLT" },
	{ "Music ProTracker", "*.MPT;*.MPD" },
	{ "Raster Music Tracker", "*.RMT" },
	{ "Theta Music Composer 1.x", "*.TMC;*.TM8" },
	{ "Theta Music Composer 2.x", "*.TM2" },
	{ "Future Composer", "*.FC" }
};

#define N_FILE_TYPES (sizeof(names_and_masks) / sizeof(names_and_masks[0]))

class input_file_type_asap : public service_impl_single_t<input_file_type>
{
public:

	virtual unsigned get_count()
	{
		return N_FILE_TYPES;
	}

	virtual bool get_name(unsigned idx, pfc::string_base &out)
	{
		if (idx < N_FILE_TYPES) {
			out = ::names_and_masks[idx][0];
			return true;
		}
		return false;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base &out)
	{
		if (idx < N_FILE_TYPES) {
			out = ::names_and_masks[idx][1];
			return true;
		}
		return false;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static service_factory_single_t<input_file_type_asap> g_input_file_type_asap_factory;


/* Info window ----------------------------------------------------------- */

class info_menu : public mainmenu_commands
{
	virtual t_uint32 get_command_count()
	{
		return 1;
	}

	virtual GUID get_command(t_uint32 p_index)
	{
		static const GUID guid = { 0x15a24bcd, 0x176b, 0x4e15, { 0xbc, 0xb1, 0x24, 0xfd, 0x92, 0x67, 0xaf, 0x8f } };
		return guid;
	}

	virtual void get_name(t_uint32 p_index, pfc::string_base &p_out)
	{
		p_out = "ASAP info";
	}

	virtual bool get_description(t_uint32 p_index, pfc::string_base &p_out)
	{
		p_out = "Activates the ASAP File Information window.";
		return true;
	}

	virtual GUID get_parent()
	{
		return mainmenu_groups::view;
	}

	virtual bool get_display(t_uint32 p_index, pfc::string_base &p_text, t_uint32 &p_flags)
	{
		p_flags = 0;
		get_name(p_index, p_text);
		return true;
	}

	virtual void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback)
	{
		if (p_index == 0) {
			playing_info = playing_info_cfg;
			const char *filename = NULL;
			int song = -1;
			metadb_handle_ptr item;
			if (static_api_ptr_t<playlist_manager>()->activeplaylist_get_focus_item_handle(item) && item.is_valid()) {
				const char *url = item->get_path();
				if (foobar2000_io::_extract_native_path_ptr(url)) {
					filename = url;
					song = item->get_subsong_index();
				}
			}
			showInfoDialog(core_api::get_my_instance(), core_api::get_main_window(), filename, song);
		}
	}
};

static mainmenu_commands_factory_t<info_menu> g_info_menu_factory;


/* ATR filesystem -------------------------------------------------------- */

class archive_atr : public archive_impl
{
public:

	virtual bool supports_content_types()
	{
		return false;
	}

	virtual const char *get_archive_type()
	{
		return "atr";
	}

	virtual t_filestats get_stats_in_archive(const char *p_archive, const char *p_file, abort_callback &p_abort)
	{
		int module_len = AATRStdio_ReadFile(p_archive, p_file, NULL, ASAPInfo_MAX_MODULE_LENGTH);
		if (module_len < 0)
			throw exception_io_not_found();
		t_filestats stats = { module_len, filetimestamp_invalid };
		return stats;
	}

	virtual void open_archive(service_ptr_t<file> &p_out, const char *archive, const char *file, abort_callback &p_abort)
	{
		BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
		int module_len = AATRStdio_ReadFile(archive, file, module, sizeof(module));
		if (module_len < 0)
			throw exception_io_not_found();
		p_out = new service_impl_t<reader_membuffer_simple>(module, module_len);
	}

	virtual void archive_list(const char *path, const service_ptr_t<file> &p_reader, archive_callback &p_out, bool p_want_readers)
	{
		if (!_extract_native_path_ptr(path))
			throw exception_io_data();
		FILE *fp = fopen(path, "rb");
		if (fp == NULL)
			throw exception_io_data();
		AATR *aatr = AATRStdio_New(fp);
		if (aatr == NULL) {
			fclose(fp);
			throw exception_io_data();
		}
		pfc::string8_fastalloc url;
		for (;;) {
			const char *fn = AATR_NextFile(aatr);
			if (fn == NULL)
				break;
			t_filestats stats = { filesize_invalid, filetimestamp_invalid };
			service_ptr_t<file> p_file;
			if (p_want_readers) {
				BYTE module[ASAPInfo_MAX_MODULE_LENGTH];
				int module_len = AATR_ReadCurrentFile(aatr, module, sizeof(module));
				p_file = new service_impl_t<reader_membuffer_simple>(module, module_len);
			}
			archive_impl::g_make_unpack_path(url, path, fn, "atr");
			if (!p_out.on_entry(this, url, stats, p_file))
				break;
		}
		AATR_Delete(aatr);
		fclose(fp);
	}
};

static archive_factory_t<archive_atr> g_archive_atr_factory;

DECLARE_COMPONENT_VERSION("ASAP", ASAPInfo_VERSION, ASAPInfo_CREDITS "\n" ASAPInfo_COPYRIGHT);
