/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright (C) 2013-2016 Leandro Nini
 * Copyright (C) 2001 Dag Lem
 * Copyright (C) 1989-1997 André Fachat (a.fachat@physik.tu-chemnitz.de)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "reloc65.h"

#include <cstring>

#include "sidplayfp/siddefs.h"

/// 16 bit header
const int HEADER_SIZE = (8 + 9 * 2);

/// Magic number
const unsigned char o65hdr[] = {1, 0, 'o', '6', '5'};

/**
 * Read a 16 bit word from a buffer at specific location.
 *
 * @param buffer
 * @param idx
 */
inline int getWord(unsigned char *buffer, int idx)
{
    return buffer[idx] | (buffer[idx+1] << 8);
}

/**
 * Write a 16 bit word into a buffer at specific location.
 *
 * @param buffer
 * @param idx
 * @param value
 */
inline void setWord(unsigned char *buffer, int idx, int value)
{
    buffer[idx] = value & 0xff;
    buffer[idx+1] = (value >> 8) & 0xff;
}

/**
 * Get the size of header options section.
 *
 * @param buf
 */
inline int read_options(unsigned char *buf)
{
    int l = 0;

    unsigned char c = buf[0];
    while (c)
    {
        l += c;
        c = buf[l];
    }
    return ++l;
}

/**
 * Get the size of undefined references list.
 *
 * @param buf
 */
inline int read_undef(unsigned char *buf)
{
    int l = 2;

    int n = getWord(buf, 0);
    while (n)
    {
        n--;
        while (!buf[l++]) {}
    }
    return l;
}

reloc65::reloc65(int addr) :
    m_tbase(addr) {}

bool reloc65::reloc(unsigned char **buf, int *fsize)
{
    unsigned char *tmpBuf = *buf;

    if (memcmp(tmpBuf, o65hdr, 5) != 0)
    {
        return false;
    }

    const int mode = getWord(tmpBuf, 6);
    if ((mode & 0x2000)     // 32 bit size not supported
        || (mode & 0x4000)) // pagewise relocation not supported
    {
        return false;
    }

    const int hlen = HEADER_SIZE + read_options(tmpBuf + HEADER_SIZE);

    const int tbase = getWord(tmpBuf, 8);
    const int tlen  = getWord(tmpBuf, 10);
    m_tdiff = m_tbase - tbase;

    const int dlen  = getWord(tmpBuf, 14);

    unsigned char *segt = tmpBuf + hlen;                    // Text segment
    unsigned char *segd = segt + tlen;                      // Data segment
    unsigned char *utab = segd + dlen;                      // Undefined references list

    unsigned char *rttab = utab + read_undef(utab);         // Text relocation table

    unsigned char *rdtab = reloc_seg(segt, tlen, rttab);    // Data relocation table
    unsigned char *extab = reloc_seg(segd, dlen, rdtab);    // Exported globals list

    reloc_globals(extab);

    setWord(tmpBuf, 8, m_tbase);

    *buf = segt;
    *fsize = tlen;
    return true;
}

int reloc65::reldiff(unsigned char s)
{
    return s==2 ? m_tdiff : 0;
}

unsigned char* reloc65::reloc_seg(unsigned char *buf, int len, unsigned char *rtab)
{
    int adr = -1;
    while (*rtab)
    {
        if ((*rtab & 255) == 255)
        {
            adr += 254;
            rtab++;
        }
        else
        {
            adr += *rtab & 255;
            rtab++;
            const unsigned char type = *rtab & 0xe0;
            unsigned char seg = *rtab & 0x07;
            rtab++;
            switch(type)
            {
            case 0x80: {
                const int oldVal = getWord(buf, adr);
                const int newVal = oldVal + reldiff(seg);
                setWord(buf, adr, newVal);
                break; }
            case 0x40: {
                const int oldVal = buf[adr] * 256 + *rtab;
                const int newVal = oldVal + reldiff(seg);
                buf[adr] = (newVal >> 8) & 255;
                *rtab = newVal & 255;
                rtab++;
                break; }
            case 0x20: {
                const int oldVal = buf[adr];
                const int newVal = oldVal + reldiff(seg);
                buf[adr] = newVal & 255;
                break; }
            }
            if (seg == 0)
            {
                rtab += 2;
            }
        }

        if (adr > len)
        {
            // Warning: relocation table entries past segment end!
        }
    }

    return ++rtab;
}

unsigned char *reloc65::reloc_globals(unsigned char *buf)
{
    int n = getWord(buf, 0);
    buf +=2;

    while (n)
    {
        while (*(buf++)) {}
        unsigned char seg = *buf;
        const int oldVal = getWord(buf, 1);
        const int newVal = oldVal + reldiff(seg);
        setWord(buf, 1, newVal);
        buf +=3;
        n--;
    }

    return buf;
}
