/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SIDEMU_H
#define SIDEMU_H

#include "component.h"
#include "SidConfig.h"
#include "siddefs.h"
#include "c64/c64sid.h"

class sidbuilder;
class EventContext;

/**
* Buffer size. 5000 is roughly 5 ms at 96 kHz
*/
enum
{
    OUTPUTBUFFERSIZE = 5000
};

/**
* Inherit this class to create a new SID emulation.
*/
class sidemu : public c64sid, public component
{
private:
    sidbuilder *m_builder;

protected:
    short *m_buffer;
    int m_bufferpos;

public:
    sidemu(sidbuilder *builder) :
        m_builder (builder), m_buffer(0) {}
    virtual ~sidemu() {}

    // Standard component functions
    void reset() { reset(0); }

    virtual void reset(uint8_t volume) = 0;

    virtual void clock() = 0;

    virtual bool lock(EventContext *env) = 0;
    virtual void unlock() = 0;

    // Standard SID functions
    virtual void voice(unsigned int num, bool mute) = 0;
    virtual void model(SidConfig::sid_model_t model) = 0;

    sidbuilder *builder() const { return m_builder; }

    virtual void sampling(float systemfreq SID_UNUSED, float outputfreq SID_UNUSED,
        SidConfig::sampling_method_t method SID_UNUSED, bool fast SID_UNUSED) {}

    int bufferpos() const { return m_bufferpos; }
    void bufferpos(int pos) { m_bufferpos = pos; }
    short *buffer() const { return m_buffer; }

    void poke(uint_least16_t address, uint8_t value) { write(address & 0x1f, value); }
    uint8_t peek(uint_least16_t address) { return read(address & 0x1f); }
};

#endif // SIDEMU_H
