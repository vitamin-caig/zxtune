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

#ifndef ZERORAMBANK_H
#define ZERORAMBANK_H

#include <stdint.h>

#include "Bank.h"

#include "sidplayfp/event.h"

/**
 * Interface to PLA functions.
 */
class PLA
{
public:
    virtual void setCpuPort(int state) =0;
    virtual uint8_t getLastReadByte() const =0;
    virtual event_clock_t getPhi2Time() const =0;
};

/**
 * Area backed by RAM, including cpu port addresses 0 and 1.
 *
 * This is bit of a fake. We know that the CPU port is an internal
 * detail of the CPU, and therefore CPU should simply pay the price
 * for reading/writing to 0/1.
 *
 * However, that would slow down all accesses, which is suboptimal. Therefore
 * we install this little hook to the 4k 0 region to deal with this.
 *
 * Implementation based on VICE code.
 *
 * @author Antti Lankila
 */
class ZeroRAMBank : public Bank
{
private:
/*
    NOTE: fall-off cycles are heavily chip- and temperature dependent. as a
          consequence it is very hard to find suitable realistic values that
          always work and we can only tweak them based on testcases. (unless we
          want to make it configurable or emulate temperature over time =))

          it probably makes sense to tweak the values for a warmed up CPU, since
          this is likely how (old) programs were coded and tested :)
*/

/* $01 bits 6 and 7 fall-off cycles (1->0), average is about 350 msec for a 6510
   and about 1500 msec for a 8500 */
/* NOTE: the unused bits of the 6510 seem to be much more temperature dependant
         and the fall-off time decreases quicker and more drastically than on a
         8500
*/
    static const event_clock_t C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES = 350000;
    //static const event_clock_t C64_CPU8500_DATA_PORT_FALL_OFF_CYCLES = 1500000;
/*
   cpuports.prg from the lorenz testsuite will fail when the falloff takes more
   than 1373 cycles. this suggests that he tested on a well warmed up c64 :)
   he explicitly delays by ~1280 cycles and mentions capacitance, so he probably
   even was aware of what happens.
*/

    static const bool tape_sense = false;

private:
    PLA* pla;

    /** C64 RAM area. Use static types to avoid unneeded polymorphism */
    SystemRAMBank* ramBank;

    /** Value written to processor port.  */
    //@{
    unsigned dir;
    unsigned data;
    //@}

    /** Value read from processor port.  */
    unsigned dataRead;

    /** State of processor port pins.  */
    unsigned procPortPins;

    /// Cycle that should invalidate the unused bits of the data port.
    //@{
    event_clock_t dataSetClkBit6;
    event_clock_t dataSetClkBit7;
    //@}

    /// Indicates if the unused bits of the data port are in the process of falling off.
    //@{
    bool dataFalloffBit6;
    bool dataFalloffBit7;
    //@}

    /// Value of the unused bit of the processor port.
    //@{
    uint8_t dataSetBit6;
    uint8_t dataSetBit7;
    //@}

private:
    void updateCpuPort()
    {
        // Update data pins for which direction is OUTPUT
        procPortPins = (procPortPins & ~dir) | (data & dir);

        dataRead = (data | ~dir) & (procPortPins | 0x17);

        pla->setCpuPort((data | ~dir) & 0x07);

        if ((dir & 0x20) == 0)
        {
            dataRead &= ~0x20;
        }
        if (tape_sense && (dir & 0x10) == 0)
        {
            dataRead &= ~0x10;
        }
    }

private:
    // prevent copying
    ZeroRAMBank(const ZeroRAMBank&);
    ZeroRAMBank& operator=(const ZeroRAMBank&);

public:
    ZeroRAMBank(PLA* pla, SystemRAMBank* ramBank) :
        pla(pla),
        ramBank(ramBank) {}

    void reset()
    {
        dataFalloffBit6 = false;
        dataFalloffBit7 = false;
        dataSetBit6 = 0;
        dataSetBit7 = 0;
        dir = 0;
        data = 0x3f;
        dataRead = 0x3f;
        procPortPins = 0x3f;
        updateCpuPort();
    }

/*
    $00/$01 unused bits emulation, as investigated by groepaz:

    - There are 2 different unused bits, 1) the output bits, 2) the input bits
    - The output bits can be (re)set when the data-direction is set to output
      for those bits and the output bits will not drop-off to 0.
    - When the data-direction for the unused bits is set to output then the
      unused input bits can be (re)set by writing to them, when set to 1 the
      drop-off timer will start which will cause the unused input bits to drop
      down to 0 in a certain amount of time.
    - When an unused input bit already had the drop-off timer running, and is
      set to 1 again, the drop-off timer will restart.
    - when a an unused bit changes from output to input, and the current output
      bit is 1, the drop-off timer will restart again
*/

    uint8_t peek(uint_least16_t address)
    {
        if (sidmemory::READ_BANK_GRANULARITY > 2 && address > 1)
        {
            return ramBank->peekByte(address);
        }
        else if (0 == address)
        {
            return dir;
        }
        else
        {
#ifdef FAST_AND_ROUGH
            return dataRead;
#else
            /* discharge the "capacitor" */
            if (dataFalloffBit6 || dataFalloffBit7)
            {
                const event_clock_t phi2time = pla->getPhi2Time();

                /* set real value of read bit 6 */
                if (dataFalloffBit6 && dataSetClkBit6 < phi2time)
                {
                    dataFalloffBit6 = false;
                    dataSetBit6 = 0;
                }

                /* set real value of read bit 7 */
                if (dataFalloffBit7 && dataSetClkBit7 < phi2time)
                {
                    dataFalloffBit7 = false;
                    dataSetBit7 = 0;
                }
            }

            uint8_t retval = dataRead;

            /* for unused bits in input mode, the value comes from the "capacitor" */

            /* set real value of bit 6 */
            if (!(dir & 0x40))
            {
                retval &= ~0x40;
                retval |= dataSetBit6;
            }

            /* set real value of bit 7 */
            if (!(dir & 0x80))
            {
                retval &= ~0x80;
                retval |= dataSetBit7;
            }
            return retval;
#endif
        }
    }

    void poke(uint_least16_t address, uint8_t value)
    {
        if (sidmemory::WRITE_BANK_GRANULARITY > 2 && address > 1)
        {
            ramBank->pokeByte(address, value);
        }
        else if (0 == address)
        {
#ifndef FAST_AND_ROUGH
            /* when switching an unused bit from output (where it contained a
             * stable value) to input mode (where the input is floating), some
             * of the charge is transferred to the floating input */

            /* check if bit 6 has flipped from 1 to 0 */
            if ((dir & 0x40) && !(value & 0x40))
            {
                dataSetClkBit6 = pla->getPhi2Time() + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES;
                dataSetBit6 = data & 0x40;
                dataFalloffBit6 = true;
            }

            /* check if bit 7 has flipped from 1 to 0 */
            if ((dir & 0x80) && !(value & 0x80))
            {
                dataSetClkBit7 = pla->getPhi2Time() + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES;
                dataSetBit7 = data & 0x80;
                dataFalloffBit7 = true;
            }
#endif
            if (dir != value)
            {
                dir = value;
                updateCpuPort();
            }
            ramBank->pokeByte(address, pla->getLastReadByte());
        }
        else
        {
#ifndef FAST_AND_ROUGH
          /* when writing to an unused bit that is output, charge the "capacitor",
           * otherwise don't touch it */
            if (dir & 0x40)
            {
                dataSetBit6 = value & 0x40;
                dataSetClkBit6 = pla->getPhi2Time() + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES;
                dataFalloffBit6 = true;
            }

            if (dir & 0x80)
            {
                dataSetBit7 = value & 0x80;
                dataSetClkBit7 = pla->getPhi2Time() + C64_CPU6510_DATA_PORT_FALL_OFF_CYCLES;
                dataFalloffBit7 = true;
            }
#endif
            if (data != value)
            {
                data = value;
                updateCpuPort();
            }
            ramBank->pokeByte(address, value);
        }
    }
};

#endif
