/*  SPU.h

	Copyright 2006 Theo Berkau
    Copyright (C) 2006-2009 DeSmuME team

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef SPU_H
#define SPU_H

#include <string>
#include "types.h"
#include <math.h>
#include <assert.h>

#include "resampler.h"

#ifdef _MSC_VER
#define FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define FORCEINLINE __inline__ __attribute__((always_inline))
#else
#define FORCEINLINE
#endif

static FORCEINLINE s32 spumuldiv7fast(s32 val, u8 multiplier) { return (val * multiplier) >> 7; }
static FORCEINLINE s32 spumuldiv7(s32 val, u8 multiplier) {
	assert(multiplier <= 127);
	return (multiplier == 127) ? val : spumuldiv7fast(val, multiplier);
}

#define CHANSTAT_STOPPED          0
#define CHANSTAT_PLAY             1
#define CHANSTAT_EMPTYBUFFER      2

enum SPUInterpolationMode
{
	SPUInterpolation_None = 0,
    SPUInterpolation_Blep = 1,
	SPUInterpolation_Linear = 2,
    SPUInterpolation_Cubic = 3,
	SPUInterpolation_Sinc = 4
};

typedef struct NDS_state NDS_state;

static bool resampler_initialized = false;

struct channel_struct
{
	channel_struct()
	{
		resampler = 0;
	}
	~channel_struct()
	{
		if (resampler)
			resampler_delete(resampler);
	}
	void init_resampler()
	{
		if (!resampler)
		{
			if (!resampler_initialized)
			{
				resampler_init();
				resampler_initialized = true;
			}
			resampler = resampler_create();
		}
	}
	u32 num;
   u8 vol;
   u8 datashift;
   u8 hold;
   u8 pan;
   u8 waveduty;
   u8 repeat;
   u8 format;
   u8 status;
   u32 addr;
   u16 timer;
   u16 loopstart;
   u32 length;

   union {
     s8* buf8;
     s16* buf16;
   };
   u32 samppos;
   float sampinc;
   u32 samploop;
   u32 samplimit;
   // ADPCM specific
   u32 lastsamppos;
   s16 pcm16b;
   s16 loop_pcm16b;
   int index;
   int loop_index;
   // PSG specific
   u16 x;
   s16 psgnoise_last;
   void *resampler;
} ;

struct SPU_struct
{
	SPU_struct(NDS_state *state, int buffersize);
   u32 bufpos;
   u32 buflength;
   s32 *sndbuf;
   s16 *outbuf;
   u32 bufsize;
   NDS_state *state;
   channel_struct channels[16];

   void reset();
   ~SPU_struct();
   void KeyOn(int channel);
   void WriteByte(u32 addr, u8 val);
   void WriteWord(u32 addr, u16 val);
   void WriteLong(u32 addr, u32 val);
   
   //kills all channels but leaves SPU otherwise running normally
   void ShutUp();
};

#endif
