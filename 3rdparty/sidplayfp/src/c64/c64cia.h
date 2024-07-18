/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2021 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#ifndef C64CIA_H
#define C64CIA_H

// The CIA emulations are very generic and here we need to effectively
// wire them into the computer (like adding a chip to a PCB).

#include "Banks/Bank.h"
#include "c64/c64env.h"
#include "sidendian.h"
#include "CIA/mos652x.h"

#include "sidcxx11.h"

namespace libsidplayfp
{

/**
 * CIA 1
 *
 * Generates IRQs
 *
 * Located at $DC00-$DCFF
 */
class c64cia1 final : public MOS652X, public Bank
{
private:
    c64env &m_env;
    uint_least16_t last_ta;

protected:
    void interrupt(bool state) override
    {
        m_env.interruptIRQ(state);
    }

    void portB() override
    {
        const uint8_t pb = prb | ~ddrb;
        // We should call adjustDataPort here
        // but we're only interested in bit 4
        m_env.lightpen(pb & 0x10);
    }

public:
    c64cia1(c64env &env) :
        MOS652X(env.scheduler()),
        m_env(env) {}

    void poke(uint_least16_t address, uint8_t value) override
    {
        const uint8_t addr = endian_16lo8(address);
        write(addr, value);

        // Save the value written to Timer A
        if ((addr == 0x04) || (addr == 0x05))
        {
            if (timerA.getTimer() != 0)
                last_ta = timerA.getTimer();
        }
    }

    uint8_t peek(uint_least16_t address) override
    {
        return read(endian_16lo8(address));
    }

    void reset() override
    {
        last_ta = 0;
        MOS652X::reset();
    }

    uint_least16_t getTimerA() const { return last_ta; }
};

/**
 * CIA 2
 *
 * Generates NMIs
 *
 * Located at $DD00-$DDFF
 */
class c64cia2 : public MOS652X, public Bank
{
private:
    c64env &m_env;

protected:
    void interrupt(bool state) override
    {
        if (state)
            m_env.interruptNMI();
    }

public:
    c64cia2(c64env &env) :
        MOS652X(env.scheduler()),
        m_env(env) {}

    void poke(uint_least16_t address, uint8_t value) override
    {
        write(endian_16lo8(address), value);
    }

    uint8_t peek(uint_least16_t address) override
    {
        return read(endian_16lo8(address));
    }
};

}

#endif // C64CIA_H
