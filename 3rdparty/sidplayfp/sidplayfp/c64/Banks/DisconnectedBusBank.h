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

#ifndef DISCONNECTEDBUSBANK_H
#define DISCONNECTEDBUSBANK_H

#include "Bank.h"

/**
* IO1/IO2
* memory mapped registers or machine code routines of optional external devices 
* I/O Area #1 located at $DE00-$DEFF
* I/O Area #2 located at $DF00-$DFFF
*/
class DisconnectedBusBank : public Bank
{
    /**
    * No device is connected so this is a no-op.
    */
    void poke(uint_least16_t addr SID_UNUSED, uint8_t data SID_UNUSED) {}

    // FIXME this should actually return last byte read from VIC
    uint8_t peek(uint_least16_t addr SID_UNUSED) { return 0; }
};

#endif
