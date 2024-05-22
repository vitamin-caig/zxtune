/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2021 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef PLA_H
#define PLA_H

#include <stdint.h>

#include "Event.h"

namespace libsidplayfp
{

/**
 * Interface to PLA functions.
 */
class PLA
{
public:
    virtual void setCpuPort(uint8_t state) =0;
    virtual uint8_t getLastReadByte() const =0;
    virtual event_clock_t getPhi2Time() const =0;

protected:
    ~PLA() {}
};

}

#endif
