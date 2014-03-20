/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef EXTRASIDBANK_H
#define EXTRASIDBANK_H

#include "Bank.h"
#include "sidplayfp/c64/c64sid.h"

#include "NullSid.h"

/**
* Extra SID bank
*/
class ExtraSidBank : public Bank
{
private:
    /**
    * Size of mapping table. Each 32 bytes another SID chip base address
    * can be assigned to.
    */
    static const int MAPPER_SIZE = 8;

private:
    /**
    * SID mapping table.
    * Maps a SID chip base address to a SID
    * or to the underlying bank.
    */
    Bank *mapper[MAPPER_SIZE];

    c64sid *sid;

private:
    static unsigned int mapperIndex(int address) { return address >> 5 & (MAPPER_SIZE - 1); }

public:
    ExtraSidBank() :
        sid(NullSid::getInstance())
    {}

    void reset()
    {
        sid->reset(0xf);
    }

    void resetSIDMapper(Bank *bank)
    {
        for (int i = 0; i < MAPPER_SIZE; i++)
            mapper[i] = bank;
    }

    /**
    * Put a SID at desired location.
    *
    * @param address the address
    */
    void setSIDMapping(int address)
    {
        mapper[mapperIndex(address)] = sid;
    }

    uint8_t peek(uint_least16_t addr)
    {
        return mapper[mapperIndex(addr)]->peek(addr);
    }

    void poke(uint_least16_t addr, uint8_t data)
    {
        mapper[mapperIndex(addr)]->poke(addr, data);
    }

    /**
    * Set SID emulation.
    *
    * @param s the emulation
    */
    void setSID(c64sid *s) { sid = (s != 0) ? s : NullSid::getInstance(); }
};

#endif
