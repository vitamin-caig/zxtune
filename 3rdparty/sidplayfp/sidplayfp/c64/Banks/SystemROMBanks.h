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

#ifndef SYSTEMROMBANKS_H
#define SYSTEMROMBANKS_H

#include <stdint.h>
#include <cstring>

#include "Bank.h"
#include "sidplayfp/c64/CPU/opcodes.h"

/**
 * ROM bank base class
 * N must be a power of two
 */
template <int N>
class romBank : public Bank
{
protected:
    /// The ROM array
    uint8_t rom[N];

protected:
    /// Set value at memory address
    void setVal(uint_least16_t address, uint8_t val) { rom[address & (N-1)]=val; }

    /// Return value from memory address
    uint8_t getVal(uint_least16_t address) const { return rom[address & (N-1)]; }

    /// Return pointer to memory address
    void* getPtr(uint_least16_t address) const { return (void*)&rom[address & (N-1)]; }

public:
    /// Copy content from source buffer
    void set(const uint8_t* source) { if (source) memcpy(rom, source, N); }

    /// Writing to ROM is a no-op
    void poke(uint_least16_t address SID_UNUSED, uint8_t value SID_UNUSED) {}

    uint8_t peek(uint_least16_t address) { return rom[address & (N-1)]; }
};

/**
 * Kernal ROM
 */
class KernalRomBank : public romBank<0x2000>
{
private:
    uint8_t resetVectorLo;  // 0xfffc
    uint8_t resetVectorHi;  // 0xfffd

public:
    void set(const uint8_t* kernal)
    {
        romBank<0x2000>::set(kernal);

        if (kernal)
        {
            // Apply Kernal hacks
            // FIXME these are tailored to the original kernals
            //       may not work as intended on other roms
            setVal(0xfd69, 0x9f); // Bypass memory check
            setVal(0xe55f, 0x00); // Bypass screen clear
            setVal(0xfdc4, 0xea); // Ignore sid volume reset to avoid DC
            setVal(0xfdc5, 0xea); //   click (potentially incompatibility)!!
            setVal(0xfdc6, 0xea);
        }
        else
        {
            // Normal IRQ interrupt
            setVal(0xea31, LDAa); // Ack IRQs
            setVal(0xea32, 0x0D);
            setVal(0xea33, 0xDC);
            setVal(0xea34, PLAn); // Restore regs
            setVal(0xea35, TAYn);
            setVal(0xea36, PLAn);
            setVal(0xea37, TAXn);
            setVal(0xea38, PLAn);
            setVal(0xea39, RTIn); // Return

            // Hardware setup routine
            setVal(0xff84, LDAa); // Set CIA 1 Timer A
            setVal(0xff85, 0xa6);
            setVal(0xff86, 0x02);
            setVal(0xff87, BEQr);
            setVal(0xff88, 0x06);
            setVal(0xff89, LDAb); // PAL
            setVal(0xff8a, 0x25);
            setVal(0xff8b, LDXb);
            setVal(0xff8c, 0x40);
            setVal(0xff8d, BNEr);
            setVal(0xff8e, 0x04);
            setVal(0xff8f, LDAb); // NTSC
            setVal(0xff90, 0x95);
            setVal(0xff91, LDXb);
            setVal(0xff92, 0x42);
            setVal(0xff93, STAa);
            setVal(0xff94, 0x04);
            setVal(0xff95, 0xdc);
            setVal(0xff96, STXa);
            setVal(0xff97, 0x05);
            setVal(0xff98, 0xdc);
            setVal(0xff99, LDAb); // Set SID to maximum volume
            setVal(0xff9a, 0x0f);
            setVal(0xff9b, STAa);
            setVal(0xff9c, 0x18);
            setVal(0xff9d, 0xd4);
            setVal(0xff9e, RTSn);

            // IRQ entry point
            setVal(0xffa0, PHAn); // Save regs
            setVal(0xffa1, TXAn);
            setVal(0xffa2, PHAn);
            setVal(0xffa3, TYAn);
            setVal(0xffa4, PHAn);
            setVal(0xffa5, JMPi); // Jump to IRQ routine
            setVal(0xffa6, 0x14);
            setVal(0xffa7, 0x03);

            // Hardware vectors
            setVal(0xfffa, 0x39); // NMI vector
            setVal(0xfffb, 0xea);
            setVal(0xfffc, 0x39); // RESET vector
            setVal(0xfffd, 0xea);
            setVal(0xfffe, 0xa0); // IRQ/BRK vector
            setVal(0xffff, 0xff);
        }

        // Backup Reset Vector
        resetVectorLo = getVal(0xfffc);
        resetVectorHi = getVal(0xfffd);
    }

    void reset()
    {
        // Restore original Reset Vector
        setVal(0xfffc, resetVectorLo);
        setVal(0xfffd, resetVectorHi);
    }

    /**
    * Change the RESET vector
    *
    * @param addr the new addres to point to
    */
    void installResetHook(uint_least16_t addr)
    {
        setVal(0xfffc, endian_16lo8(addr));
        setVal(0xfffd, endian_16hi8(addr));
    }
};

/**
 * BASIC ROM
 */
class BasicRomBank : public romBank<0x2000>
{
private:
    uint8_t trap[3];
    uint8_t subTune[11];

public:
    void set(const uint8_t* basic)
    {
        romBank<0x2000>::set(basic);

        // Backup BASIC Warm Start
        memcpy(trap, getPtr(0xa7ae), 3);

        memcpy(subTune, getPtr(0xbf53), 11);
    }

    void reset()
    {
        // Restore original BASIC Warm Start
        memcpy(getPtr(0xa7ae), trap, 3);

        memcpy(getPtr(0xbf53), subTune, 11);
    }

    /**
    * Set BASIC Warm Start address
    *
    * @param addr
    */
    void installTrap(uint_least16_t addr)
    {
        setVal(0xa7ae, JMPw);
        setVal(0xa7af, endian_16lo8(addr));
        setVal(0xa7b0, endian_16hi8(addr));
    }

    void setSubtune(uint8_t tune)
    {
        setVal(0xbf53, LDAb);
        setVal(0xbf54, tune);
        setVal(0xbf55, STAa);
        setVal(0xbf56, 0x0c);
        setVal(0xbf57, 0x03);
        setVal(0xbf58, JSRw);
        setVal(0xbf59, 0x2c);
        setVal(0xbf5a, 0xa8);
        setVal(0xbf5b, JMPw);
        setVal(0xbf5c, 0xb1);
        setVal(0xbf5d, 0xa7);
    }
};

/**
 * Character ROM
 */
class CharacterRomBank : public romBank<0x1000> {};

#endif
