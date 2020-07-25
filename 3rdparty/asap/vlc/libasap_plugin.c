/*
 * libasap_plugin.c - ASAP plugin for VLC
 *
 * Copyright (C) 2012  Piotr Fusik
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

#define __PLUGIN__
#define MODULE_NAME  asap
#define MODULE_STRING  "asap"

#include <stdarg.h>

#include <vlc_common.h>
#include <vlc_input.h>
#include <vlc_demux.h>
#include <vlc_plugin.h>

#include "asap.h"

#define BITS_PER_SAMPLE  16
#define BUFFER_BYTES  4096

struct demux_sys_t
{
	ASAP *asap;
	es_out_id_t *es;
	date_t pts;
	int bytes_per_frame;
	int duration;
};

static int Demux(demux_t *demux)
{
	demux_sys_t *sys = demux->p_sys;

	block_t *block = block_Alloc(BUFFER_BYTES);
	if (unlikely(block == NULL))
		return 0;

	int len = ASAP_Generate(sys->asap, block->p_buffer, BUFFER_BYTES,
		BITS_PER_SAMPLE == 16 ? ASAPSampleFormat_S16_L_E : ASAPSampleFormat_U8);
	if (len <= 0) {
		block_Release(block);
		return 0;
	}
	block->i_buffer = len;
	block->i_pts = block->i_dts = VLC_TS_0 + date_Get(&sys->pts);
	es_out_Control(demux->out, ES_OUT_SET_PCR, block->i_pts);
	es_out_Send(demux->out, sys->es, block);
	date_Increment(&sys->pts, len / sys->bytes_per_frame);
	return 1;
}

static cibool PlaySong(demux_t *demux, demux_sys_t *sys, int song)
{
	const ASAPInfo *info = ASAP_GetInfo(sys->asap);
	int duration = ASAPInfo_GetDuration(info, song);
	if (!ASAP_PlaySong(sys->asap, song, duration))
		return FALSE;
	sys->duration = duration;
	demux->info.i_title = song;
	demux->info.i_update |= INPUT_UPDATE_TITLE;
	return TRUE;
}

static int Control(demux_t *demux, int query, va_list args)
{
	demux_sys_t *sys = demux->p_sys;

	switch (query) {
		case DEMUX_GET_POSITION: {
			if (sys->duration <= 0)
				return VLC_EGENERIC;
			double *v = va_arg(args, double *);
			*v = (double) ASAP_GetPosition(sys->asap) / sys->duration;
			return VLC_SUCCESS;
		}
		case DEMUX_SET_POSITION: {
			if (sys->duration <= 0)
				return VLC_EGENERIC;
			double v = va_arg(args, double);
			if (v < 0 || v > 1 || !ASAP_Seek(sys->asap, (int) (v * sys->duration)))
				return VLC_EGENERIC;
			return VLC_SUCCESS;
		}
		case DEMUX_GET_LENGTH: {
			if (sys->duration <= 0)
				return VLC_EGENERIC;
			int64_t *v = va_arg(args, int64_t *);
			*v = sys->duration * INT64_C(1000);
			return VLC_SUCCESS;
		}
		case DEMUX_GET_TIME: {
			int64_t *v = va_arg(args, int64_t *);
			*v = ASAP_GetPosition(sys->asap) * INT64_C(1000);
			return VLC_SUCCESS;
		}
		case DEMUX_SET_TIME: {
			int64_t v = va_arg(args, int64_t) / 1000;
			if (v > 0x7fffffff || !ASAP_Seek(sys->asap, v))
				return VLC_EGENERIC;
			return VLC_SUCCESS;
		}
		case DEMUX_GET_META: {
			const ASAPInfo *info = ASAP_GetInfo(sys->asap);
			vlc_meta_t *p_meta = (vlc_meta_t *) va_arg(args, vlc_meta_t *);
			const char *s = ASAPInfo_GetTitle(info);
			if (s[0] != '\0')
				vlc_meta_SetTitle(p_meta, s);
			s = ASAPInfo_GetAuthor(info);
			if (s[0] != '\0')
				vlc_meta_SetArtist(p_meta, s);
			int year = ASAPInfo_GetYear(info);
			if (year > 0) {
				char s[16];
				sprintf(s, "%d", year);
				vlc_meta_SetDate(p_meta, s);
			}
			return VLC_SUCCESS;
		}
		case DEMUX_GET_TITLE_INFO: {
			int songs = ASAPInfo_GetSongs(ASAP_GetInfo(sys->asap));
			if (songs <= 1)
				return VLC_EGENERIC;
			input_title_t ***titlev = va_arg(args, input_title_t ***);
			int *titlec = va_arg(args, int *);
			*titlev = malloc(sizeof(**titlev) * songs);
			if (unlikely(*titlev == NULL)) {
				*titlec = 0;
				return VLC_ENOMEM;
			}
			*titlec = songs;
			for (int i = 0; i < songs; i++)
				(*titlev)[i] = vlc_input_title_New();
			return VLC_SUCCESS;
		}
		case DEMUX_SET_TITLE: {
			int song = (int) va_arg(args, int);
			if (!PlaySong(demux, sys, song))
				return VLC_EGENERIC;
			return VLC_SUCCESS;
		}
		default:
			return VLC_EGENERIC;
	}
}

static int Open(vlc_object_t *obj)
{
	demux_t *demux = (demux_t *) obj;

	/* read module */
	int64_t module_len = stream_Size(demux->s);
	if (module_len > ASAPInfo_MAX_MODULE_LENGTH)
		return VLC_EGENERIC;
	uint8_t *module = (uint8_t *) malloc(module_len);
	if (unlikely(module == NULL))
		return VLC_ENOMEM;
	if (stream_Read(demux->s, module, module_len) < module_len) {
		free(module);
		return VLC_EGENERIC;
	}

	/* load into ASAP and start */
	demux_sys_t *sys = malloc(sizeof(demux_sys_t));
	if (unlikely(sys == NULL)) {
		free(module);
		return VLC_ENOMEM;
	}
	sys->asap = ASAP_New();
	if (unlikely(sys->asap == NULL)) {
		free(sys);
		free(module);
		return VLC_ENOMEM;
	}
	if (!ASAP_Load(sys->asap, NULL, module, module_len)) {
		ASAP_Delete(sys->asap);
		free(sys);
		free(module);
		return VLC_EGENERIC;
	}
	free(module);
	const ASAPInfo *info = ASAP_GetInfo(sys->asap);
	int song = ASAPInfo_GetDefaultSong(info);
	if (!PlaySong(demux, sys, song)) {
		ASAP_Delete(sys->asap);
		free(sys);
		return VLC_EGENERIC;
	}

	/* setup VLC */
	es_format_t fmt;
	es_format_Init(&fmt, AUDIO_ES, BITS_PER_SAMPLE == 16 ? VLC_CODEC_S16L : VLC_CODEC_U8);
	fmt.audio.i_channels = ASAPInfo_GetChannels(info);
	fmt.audio.i_bitspersample = BITS_PER_SAMPLE;
	fmt.audio.i_rate = ASAP_SAMPLE_RATE;
	fmt.audio.i_blockalign = fmt.audio.i_frame_length = fmt.audio.i_bytes_per_frame = sys->bytes_per_frame = fmt.audio.i_channels * (BITS_PER_SAMPLE / 8);
	fmt.i_bitrate = ASAP_SAMPLE_RATE * 8 * fmt.audio.i_bytes_per_frame;
	sys->es = es_out_Add(demux->out, &fmt);
	date_Init(&sys->pts, ASAP_SAMPLE_RATE, 1);
	date_Set(&sys->pts, 0);
	demux->pf_demux = Demux;
	demux->pf_control = Control;
	demux->p_sys = sys;

	return VLC_SUCCESS;
}


static void Close(vlc_object_t *obj)
{
	demux_t *demux = (demux_t *) obj;
	demux_sys_t *sys = demux->p_sys;

	ASAP_Delete(sys->asap);
	free(sys);
}

vlc_module_begin()
	set_shortname("ASAP")
	set_description("Another Slight Atari Player")
	set_category(CAT_INPUT)
	set_subcategory(SUBCAT_INPUT_DEMUX)
	set_capability("demux", 100)
	set_callbacks(Open, Close)
vlc_module_end()
