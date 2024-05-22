/***************************************************************************
             exsid.cpp  -  exSID support interface.
                             -------------------
   Based on hardsid.cpp (C) 2001 Jarno Paananen

    copyright            : (C) 2015-2017,2021 Thibaut VARENE
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

#include "exsid-emu.h"

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sstream>

#ifdef HAVE_EXSID
#  include <exSID.h>
#else
#  include "driver/exSID.h"
#endif

#include "sidcxx11.h"

namespace libsidplayfp
{

unsigned int exSID::sid = 0;

const char* exSID::getCredits()
{
    return
        "exSID V" VERSION " Engine:\n"
        "\t(C) 2015-2017,2021 Thibaut VARENE\n";
}

exSID::exSID(sidbuilder *builder) :
    sidemu(builder),
    m_status(false),
    readflag(false),
    busValue(0)
{
    exsid = exSID_new();
    if (!exsid)
    {
        m_error = "out of memory";
        return;
    }

    if (exSID_init(exsid) < 0)
    {
        m_error = exSID_error_str(exsid);
        return;
    }

    m_status = true;
    sid++;
  
    muted[0] = muted[1] = muted[2] = false;
}

exSID::~exSID()
{
    sid--;
    if (m_status)
        exSID_audio_op(exsid, XS_AU_MUTE);
    exSID_exit(exsid);
    exSID_free(exsid);
}

void exSID::reset(uint8_t volume)
{
    exSID_reset(exsid);
    exSID_clkdwrite(exsid, 0, 0x18, volume);	// this will offset the internal clock
    m_accessClk = 0;
    readflag = false;
}

unsigned int exSID::delay()
{
    event_clock_t cycles = eventScheduler->getTime(EVENT_CLOCK_PHI1) - m_accessClk;
    m_accessClk += cycles;

    while (cycles > 0xffff)
    {
        exSID_delay(exsid, 0xffff);
        cycles -= 0xffff;
    }

    return static_cast<unsigned int>(cycles);
}

void exSID::clock()
{
    const unsigned int cycles = delay();

    if (cycles)
        exSID_delay(exsid, cycles);
}

uint8_t exSID::read(uint_least8_t addr)
{
    if ((addr < 0x19) || (addr > 0x1C))
    {
        return busValue;
    }

    if (!readflag)
    {
#ifdef DEBUG
        printf("WARNING: Read support is limited. This file may not play correctly!\n");
#endif
        readflag = true;

        // Here we implement the "safe" detection routine return values
        if (0x1b == addr) {	// we could implement a commandline-chosen return byte here
            return (SidConfig::MOS8580 == runmodel) ? 0x02 : 0x03;
        }
    }

    const unsigned int cycles = delay();

    exSID_clkdread(exsid, cycles, addr, &busValue);	// busValue is updated on valid reads
    return busValue;
}

void exSID::write(uint_least8_t addr, uint8_t data)
{
    busValue = data;

    if (addr > 0x18)
        return;

    const unsigned int cycles = delay();

    if (addr % 7 == 4 && muted[addr / 7])
        data = 0;

    exSID_clkdwrite(exsid, cycles, addr, data);
}

void exSID::voice(unsigned int num, bool mute)
{
    muted[num] = mute;
}

void exSID::model(SidConfig::sid_model_t model, MAYBE_UNUSED bool digiboost)
{
    runmodel = model;
    // currently no support for stereo mode: output the selected SID to both L and R channels
    exSID_audio_op(exsid, model == SidConfig::MOS8580 ? XS_AU_8580_8580 : XS_AU_6581_6581);	// mutes output
    //exSID_audio_op(exsid, XS_AU_UNMUTE);	// sampling is set after model, no need to unmute here and cause pops
    exSID_chipselect(exsid, model == SidConfig::MOS8580 ? XS_CS_CHIP1 : XS_CS_CHIP0);
}

void exSID::flush() {}

bool exSID::lock(EventScheduler* env)
{
    return sidemu::lock(env);
}

void exSID::unlock()
{
    sidemu::unlock();
}

void exSID::sampling(float systemclock, MAYBE_UNUSED float freq,
        MAYBE_UNUSED SidConfig::sampling_method_t method, bool)
{
    exSID_audio_op(exsid, XS_AU_MUTE);
    if (systemclock < 1000000.0F)
        exSID_clockselect(exsid, XS_CL_PAL);
    else
        exSID_clockselect(exsid, XS_CL_NTSC);
    exSID_audio_op(exsid, XS_AU_UNMUTE);
}

}
