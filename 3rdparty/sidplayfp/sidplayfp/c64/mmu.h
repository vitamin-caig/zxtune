/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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

#ifndef MMU_H
#define MMU_H

#include <stdint.h>

#include "sidplayfp/event.h"
#include "sidplayfp/sidendian.h"
#include "sidplayfp/sidmemory.h"

#include "Banks/Bank.h"
#include "Banks/SystemRAMBank.h"
#include "Banks/SystemROMBanks.h"
#include "Banks/ZeroRAMBank.h"

#include <string.h>

/**
 * The C64 MMU chip.
*/
class MMU : public PLA, public sidmemory
{
private:
    EventContext &context;

    /** CPU port signals */
    bool loram, hiram, charen;

    /** CPU read memory mapping in READ_BANK_GRANULARITY chunks */
    Bank* cpuReadMap[65536 / READ_BANK_GRANULARITY];

    /** CPU write memory mapping in WRITE_BANK_GRANULARITY chunks */
    Bank* cpuWriteMap[65536 / WRITE_BANK_GRANULARITY];

    /** IO region handler */
    Bank* ioBank;

    /** Kernal ROM */
    KernalRomBank kernalRomBank;

    /** BASIC ROM */
    BasicRomBank basicRomBank;

    /** Character ROM */
    CharacterRomBank characterRomBank;

    /** RAM */
    SystemRAMBank ramBank;

    ZeroRAMBank zeroRAMBank;
private:
    inline unsigned getReadBankIndex(uint_least16_t addr) const
    {
      return addr / READ_BANK_GRANULARITY;
    }

    inline unsigned getWriteBankIndex(uint_least16_t addr) const
    {
      return addr / WRITE_BANK_GRANULARITY;
    }

    void setReadBank(uint_least16_t start, int size, Bank* bank);

    void setWriteBank(uint_least16_t start, int size, Bank* bank);
private:
    void setCpuPort(int state);
    void updateMappingPHI2();
    uint8_t getLastReadByte() const { return 0; }
    event_clock_t getPhi2Time() const { return context.getTime(EVENT_CLOCK_PHI2); }

public:
    MMU(EventContext *context, Bank* ioBank);
    ~MMU () {}

    void reset();

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
    {
        kernalRomBank.set(kernal);
        basicRomBank.set(basic);
        characterRomBank.set(character);
    }

    // RAM access methods
    uint8_t readMemByte(uint_least16_t addr) { return ramBank.peek(addr); }
    uint_least16_t readMemWord(uint_least16_t addr) { return ramBank.peekWord(addr); }

    void writeMemByte(uint_least16_t addr, uint8_t value) { ramBank.poke(addr, value); }
    void writeMemWord(uint_least16_t addr, uint_least16_t value) { ramBank.pokeWord(addr, value); }

    void fillRam(uint_least16_t start, uint8_t value, unsigned int size)
    {
        ramBank.fill(start, value, size);
    }

    void fillRam(uint_least16_t start, const uint8_t* source, unsigned int size)
    {
        ramBank.fill(start, source, size);
    }

    // SID specific hacks
    void installResetHook(uint_least16_t addr) { kernalRomBank.installResetHook(addr); }

    void installBasicTrap(uint_least16_t addr) { basicRomBank.installTrap(addr); }

    void setBasicSubtune(uint8_t tune) { basicRomBank.setSubtune(tune); }

    uint8_t cpuRead(uint_least16_t addr) const { return cpuReadMap[getReadBankIndex(addr)]->peek(addr); }

    void cpuWrite(uint_least16_t addr, uint8_t data) { cpuWriteMap[getWriteBankIndex(addr)]->poke(addr, data); }
};

#endif
