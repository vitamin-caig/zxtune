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

#ifndef C64_H
#define C64_H

#include <stdint.h>
#include <cstdio>

#include "Banks/IOBank.h"
#include "Banks/ColorRAMBank.h"
#include "Banks/DisconnectedBusBank.h"
#include "Banks/SidBank.h"
#include "Banks/ExtraSidBank.h"

#include "sidplayfp/EventScheduler.h"

#include "sidplayfp/c64/c64env.h"
#include "sidplayfp/c64/c64cpu.h"
#include "sidplayfp/c64/c64cia.h"
#include "sidplayfp/c64/c64vic.h"
#include "sidplayfp/c64/mmu.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

class c64sid;
class sidmemory;


#ifdef PC64_TESTSUITE
class testEnv
{
public:
    virtual ~testEnv() {}
    virtual void load(const char *) =0;
};
#endif

/**
* Commodore 64 emulation core.
*
* It consists of the following chips: PLA, MOS6510, MOS6526(a), VIC
* 6569(PAL)/6567(NTSC), RAM/ROM.<BR>
*
* @author Antti Lankila
* @author Ken Händel
* @author Leandro Nini
*
*/
class c64: private c64env
{
public:
    /** Maximum number of supported SIDs (mono and stereo) */
    static const unsigned int MAX_SIDS = 2;

public:
    typedef enum
    {
        PAL_B = 0     ///< PAL C64
        ,NTSC_M       ///< NTSC C64
        ,OLD_NTSC_M   ///< Old NTSC C64
        ,PAL_N        ///< C64 Drean
    } model_t;

private:
    /** System clock frequency */
    double m_cpuFreq;

    /** Number of sources asserting IRQ */
    int irqCount;

    /** BA state */
    bool oldBAState;

    /** System event context */
    EventScheduler m_scheduler;

    /** CPU */
    c64cpu cpu;

    /** CIA1 */
    c64cia1 cia1;

    /** CIA2 */
    c64cia2 cia2;

    /** VIC */
    c64vic vic;

    /** Color RAM */
    ColorRAMBank colorRAMBank;

    /** SID */
    SidBank sidBank;

    /** 2nd SID */
    ExtraSidBank extraSidBank;

    /** I/O Area #1 and #2 */
    DisconnectedBusBank disconnectedBusBank;

    /** I/O Area */
    IOBank ioBank;

    /** MMU chip */
    MMU mmu;

private:
    static double getCpuFreq(model_t model);

private:
    /**
    * Access memory as seen by CPU.
    *
    * @param addr the address where to read from
    * @return value at address
    */
    uint8_t cpuRead(uint_least16_t addr) { return mmu.cpuRead(addr); }

    /**
    * Access memory as seen by CPU.
    *
    * @param addr the address where to write to
    * @param data the value to write
    */
    void cpuWrite(uint_least16_t addr, uint8_t data) { mmu.cpuWrite(addr, data); }

    /**
    * IRQ trigger signal.
    *
    * Calls permitted any time, but normally originated by chips at PHI1.
    *
    * @param state
    */
    inline void interruptIRQ(bool state);

    /**
    * NMI trigger signal.
    *
    * Calls permitted any time, but normally originated by chips at PHI1.
    */
    inline void interruptNMI() { cpu.triggerNMI (); }

    /**
    * Reset signal.
    */
    inline void interruptRST() { cpu.triggerRST (); }

    /**
    * BA signal.
    *
    * Calls permitted during PHI1.
    *
    * @param state
    */
    inline void setBA(bool state);

    inline void lightpen() { vic.lightpen (); }

#ifdef PC64_TESTSUITE
    testEnv *m_env;

    void loadFile(const char *file)
    {
        m_env->load(file);
    }
#endif

    void resetIoBank();

public:
    c64();
    ~c64() {}

#ifdef PC64_TESTSUITE
    void setTestEnv(testEnv *env)
    {
        m_env = env;
    }
#endif

    /**
    * Get C64's event scheduler
    *
    * @return the scheduler
    */
    //@{
    EventScheduler *getEventScheduler() { return &m_scheduler; }
    const EventScheduler &getEventScheduler() const { return m_scheduler; }
    //@}

    void debug(bool enable, FILE *out) { cpu.debug(enable, out); }

    void reset();
    void resetCpu() { cpu.reset(); }

    /**
    * Set the c64 model.
    */
    void setModel(model_t model);

    void setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
    {
        mmu.setRoms(kernal, basic, character);
    }

    /**
    * Get the CPU clock speed.
    *
    * @return the speed in Hertz
    */
    double getMainCpuSpeed() const { return m_cpuFreq; }

    /**
    * Set the requested SID
    *
    * @param i sid number to set
    * @param s the sid emu to set, or 0 to remove
    */
    void setSid(unsigned int i, c64sid *s);

    /**
    * Set the base address of a stereo SID chip.<br/>
    * Valid addresses includes the SID area ($d400-$d7ff)
    * and the IO Area ($de00-$dfff).
    *
    * @param sidChipBase2
    *            base address (e.g. 0xd420)
    *            0 to remove second SID
    */
    void setSecondSIDAddress(int sidChipBase2);

    /**
    * Get the components credits
    */
    //@{
    const char* cpuCredits () const { return cpu.credits(); }
    const char* ciaCredits () const { return cia1.credits(); }
    const char* vicCredits () const { return vic.credits(); }
    //@}

    sidmemory *getMemInterface() { return &mmu; }

    uint_least16_t getCia1TimerA() const { return cia1.getTimerA(); }
};

void c64::interruptIRQ (bool state)
{
    if (state)
    {
        if (irqCount == 0)
            cpu.triggerIRQ ();

        irqCount ++;
    }
    else
    {
        irqCount --;
        if (irqCount == 0)
             cpu.clearIRQ ();
    }
}

void c64::setBA (bool state)
{
    /* only react to changes in state */
    if (state == oldBAState)
        return;

    oldBAState = state;

    /* Signal changes in BA to interested parties */
    cpu.setRDY (state);
}

#endif // C64_H
