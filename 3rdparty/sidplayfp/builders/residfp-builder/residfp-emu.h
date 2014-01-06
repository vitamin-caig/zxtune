/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#ifndef RESIDFP_EMU_H
#define RESIDFP_EMU_H

#include <stdint.h>

#include <string>

#include "residfp/SID.h"
#include "sidplayfp/SidConfig.h"
#include "sidplayfp/sidemu.h"
#include "sidplayfp/event.h"

class sidbuilder;

#define RESID_NAMESPACE reSIDfp

class ReSIDfp: public sidemu
{
private:
    EventContext *m_context;
    RESID_NAMESPACE::SID &m_sid;
    event_clock_t m_accessClk;
    const  char  *m_error;
    bool          m_status;
    bool          m_locked;

    static std::string m_credit;

public:
    static const char* getCredits();

public:
    ReSIDfp(sidbuilder *builder);
    ~ReSIDfp();

    // Standard component functions
    const char *credits() const { return getCredits(); }
    
    void reset() { sidemu::reset (); }
    void reset(uint8_t volume);

    uint8_t read(uint_least8_t addr);
    void write(uint_least8_t addr, uint8_t data);

    const char *error() const { return m_error; }

    // Standard SID functions
    void clock();
    void filter(bool enable);
    void voice(unsigned int num, bool mute) { m_sid.mute(num, mute); }

    bool getStatus() const { return m_status; }

    // Specific to resid
    void sampling(float systemclock, float freq,
        SidConfig::sampling_method_t method, bool fast);

    void filter6581Curve(double filterCurve);
    void filter8580Curve(double filterCurve);
    void model(SidConfig::sid_model_t model);

    // Must lock the SID before using the standard functions.
    bool lock(EventContext *env);
    void unlock();
};

#endif // RESIDFP_EMU_H
