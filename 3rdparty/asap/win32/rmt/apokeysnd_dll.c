/*
 * apokeysnd_dll.c - POKEY sound emulator for Raster Music Tracker
 *
 * Copyright (C) 2008-2014  Piotr Fusik
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

#include <stddef.h>
#include "pokey.h"

static PokeyPair *pokeys = NULL;

__declspec(dllexport) void APokeySound_Initialize(bool stereo)
{
	PokeyPair_Delete(pokeys);
	pokeys = PokeyPair_New();
	PokeyPair_Initialize(pokeys, false, stereo);
	PokeyPair_StartFrame(pokeys);
}

__declspec(dllexport) void APokeySound_PutByte(int addr, int data)
{
	PokeyPair_Poke(pokeys, addr, data, 0);
}

__declspec(dllexport) int APokeySound_GetRandom(int addr, int cycle)
{
	return PokeyPair_GetRandom(pokeys, addr, cycle);
}

__declspec(dllexport) int APokeySound_Generate(int cycles, unsigned char *buffer, int depth)
{
	int len = PokeyPair_EndFrame(pokeys, cycles);
	len = PokeyPair_Generate(pokeys, buffer, 0, len, depth == 16 ? ASAPSampleFormat_S16_L_E : ASAPSampleFormat_U8);
	PokeyPair_StartFrame(pokeys);
	return len;
}

__declspec(dllexport) void APokeySound_About(const char **name, const char **author, const char **description)
{
	*name = "Another POKEY Sound Emulator, v3.2.0";
	*author = "Piotr Fusik, (C) 2007-2014";
	*description = "Part of ASAP, http://asap.sourceforge.net";
}
