/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2014 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef RESID_EMU_H
#define RESID_EMU_H

#include <stdint.h>

#include "sidplayfp/SidConfig.h"
#include "sidemu.h"
#include "sidplayfp/event.h"
#include "resid/sid.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

//FIXME
#define RESID_NAMESPACE reSID

#ifdef RESID_NAMESPACE
#   define RESID_NS ::RESID_NAMESPACE
#else
#   define RESID_NS
#endif

class ReSID: public sidemu
{
private:
    RESID_NS::SID &m_sid;
    uint8_t       m_voiceMask;

public:
    static const char* getCredits();

public:
    ReSID(sidbuilder *builder);
    ~ReSID();

    // Standard component functions
    void reset() { sidemu::reset (); }
    void reset(uint8_t volume);

    uint8_t read(uint_least8_t addr);
    void write(uint_least8_t addr, uint8_t data);

    // Standard SID functions
    void clock();
    void clockSilent();
    void filter(bool enable);
    void voice(unsigned int num, bool mute);

    bool getStatus() const { return m_status; }

    // Specific to resid
    void sampling(float systemclock, float freq,
        SidConfig::sampling_method_t method, bool fast);

    void bias(double dac_bias);
    void model(SidConfig::sid_model_t model);
    void analyze(unsigned int tone[3], unsigned int level[3]);
};

#endif // RESID_EMU_H
