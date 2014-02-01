/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2010 Antti Lankila
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

#ifndef SYSTEMRAMBANK_H
#define SYSTEMRAMBANK_H

#include <stdint.h>
#include <algorithm>

#include "Bank.h"

/**
 * Area backed by RAM
 *
 * @author Antti Lankila
 */
class SystemRAMBank : public Bank
{
private:
    /** C64 RAM area */
    unsigned ram[0x10000];

public:
    /// Initialize RAM with powerup pattern
    void reset()
    {
        fill(0, uint8_t(0), 0x10000);
        for (int i = 0x07c0; i < 0x10000; i += 128)
        {
            fill(i, 0xff, 64);
        }
    }

    uint8_t peek(uint_least16_t address)
    {
        return peekByte(address);
    }

    void poke(uint_least16_t address, uint8_t value)
    {
        pokeByte(address, value);
    }

    uint8_t peekByte(uint_least16_t address)
    {
        return ram[address];
    }

    void pokeByte(uint_least16_t address, uint8_t value)
    {
        ram[address] = value;
    }

    uint16_t peekWord(uint_least16_t addr)
    {
        return endian_little16(ram+addr);
    }

    void pokeWord(uint_least16_t addr, uint_least16_t value)
    {
        endian_little16(ram+addr, value);
    }

    void fill(uint_least16_t start, uint8_t value, unsigned int size)
    {
        std::fill_n(ram+start, size, value);
    }

    void fill(uint_least16_t start, const uint8_t* source, unsigned int size)
    {
        std::copy(source, source+size, ram+start);
    }
};

#endif
