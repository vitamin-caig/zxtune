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

#ifndef PSIDDRV_H
#define PSIDDRV_H

#include <stdint.h>

class SidTuneInfo;
class sidmemory;

class psiddrv
{
private:
    const SidTuneInfo *m_tuneInfo;
    const char *m_errorString;

    uint8_t *reloc_driver;
    int      reloc_size;

    uint_least16_t m_driverAddr;
    uint_least16_t m_driverLength;
    uint_least16_t m_powerOnDelay;

private:
    static uint8_t psid_driver[];

private:
    /**
    * @param addr a 16-bit effective address
    * @return a default bank-select value for $01
    */
    uint8_t iomap(uint_least16_t addr) const;

public:
    psiddrv(const SidTuneInfo *tuneInfo) :
        m_tuneInfo(tuneInfo),
        m_powerOnDelay(0) {}

    /**
    * Set the power on delay cycles.
    *
    * @param delay the delay
    */
    void powerOnDelay(uint_least16_t delay) { m_powerOnDelay = delay; }

    /**
    * Relocate the driver.
    *
    * @param mem the c64 memory interface
    */
    bool drvReloc(sidmemory *mem);

    /**
     * Install the driver.
     * Must be called after the tune has been placed in memory.
     *
     * @param mem the c64 memory interface
     */
    void install(sidmemory *mem) const;

    /**
    * Get a detailed error message.
    *
    * @return a pointer to the string
    */
    const char* errorString() const { return m_errorString; }

    uint_least16_t driverAddr() const { return m_driverAddr; }
    uint_least16_t driverLength() const { return m_driverLength; }
};

#endif
