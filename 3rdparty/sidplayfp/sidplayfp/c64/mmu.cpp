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

#include "mmu.h"

class Bank;

MMU::MMU(EventContext *context, Bank* ioBank) :
    context(*context),
    loram(false),
    hiram(false),
    charen(false),
    ioBank(ioBank),
    zeroRAMBank(this, &ramBank)
{
    setReadFunc(0, 65536, &MMU::RAM_Read);
    setWriteBank(0, 65536, &ramBank);

    // fill later for cover cases when GRANULARITY > 2
    setReadFunc(0, 2, &MMU::ZeroRAM_Read);
    setWriteBank(0, 2, &zeroRAMBank);
}

void MMU::setReadFunc(uint_least16_t start, int size, MMU::ReadFunc func)
{
  //aggressive loop optimization sometimes break cycle
  for (int idx = getReadBankIndex(start), rest = size; rest > 0; ++idx, rest -= READ_BANK_GRANULARITY)
  {
    cpuReadMap[idx] = func;
  }
}

void MMU::setWriteBank(uint_least16_t start, int size, Bank* bank)
{
  for (int idx = getWriteBankIndex(start), rest = size; rest > 0; ++idx, rest -= WRITE_BANK_GRANULARITY)
  {
    cpuWriteMap[idx] = bank;
  }
}

void MMU::setCpuPort (int state)
{
    loram = (state & 1) != 0;
    hiram = (state & 2) != 0;
    charen = (state & 4) != 0;
    updateMappingPHI2();
}

void MMU::updateMappingPHI2()
{
    setReadFunc(0xe000, 0x2000, hiram ? &MMU::KernalROM_Read : &MMU::RAM_Read);
    setReadFunc(0xa000, 0x2000, (loram && hiram) ? &MMU::BasicROM_Read : &MMU::RAM_Read);

    if (charen && (loram || hiram))
    {
        setReadFunc(0xd000, 0x1000, &MMU::IO_Read);
        setWriteBank(0xd000, 0x1000, ioBank);
    }
    else
    {
        setReadFunc(0xd000, 0x1000, (!charen && (loram || hiram)) ? &MMU::CharROM_Read : &MMU::RAM_Read);
        setWriteBank(0xd000, 0x1000, &ramBank);
    }
}

void MMU::reset()
{
    ramBank.reset();
    zeroRAMBank.reset();

    // Reset the ROMs to undo the hacks applied
    kernalRomBank.reset();
    basicRomBank.reset();

    updateMappingPHI2();
}
