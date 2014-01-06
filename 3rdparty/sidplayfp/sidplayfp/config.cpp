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

#include "player.h"

#include "sidemu.h"
#include "SidTune.h"
#include "sidbuilder.h"

SIDPLAYFP_NAMESPACE_START

const char TXT_PAL_VBI[]        = "50 Hz VBI (PAL)";
const char TXT_PAL_VBI_FIXED[]  = "60 Hz VBI (PAL FIXED)";
const char TXT_PAL_CIA[]        = "CIA (PAL)";
const char TXT_PAL_UNKNOWN[]    = "UNKNOWN (PAL)";
const char TXT_NTSC_VBI[]       = "60 Hz VBI (NTSC)";
const char TXT_NTSC_VBI_FIXED[] = "50 Hz VBI (NTSC FIXED)";
const char TXT_NTSC_CIA[]       = "CIA (NTSC)";
const char TXT_NTSC_UNKNOWN[]   = "UNKNOWN (NTSC)";

// Error Strings
const char ERR_UNSUPPORTED_FREQ[]      = "SIDPLAYER ERROR: Unsupported sampling frequency.";

bool Player::config(const SidConfig &cfg)
{
    const SidTuneInfo* tuneInfo = 0;

    // Check for base sampling frequency
    if (cfg.frequency < 8000)
    {
        m_errorString = ERR_UNSUPPORTED_FREQ;
        return false;
    }

    uint_least16_t secondSidAddress = cfg.secondSidAddress;

    // Only do these if we have a loaded tune
    if (m_tune)
    {
        tuneInfo = m_tune->getInfo();

        if (tuneInfo->sidChipBase2() != 0)
            secondSidAddress = tuneInfo->sidChipBase2();

        try
        {
            // SID emulation setup (must be performed before the
            // environment setup call)
            sidRelease();
            const int channels = (secondSidAddress != 0) ? 2 : 1;
            sidCreate(cfg.sidEmulation, cfg.defaultSidModel, cfg.forceSidModel, channels);

            // Determine clock speed
            const c64::model_t model = c64model(cfg.defaultC64Model, cfg.forceC64Model);

            m_c64.setModel(model);

            sidParams(m_c64.getMainCpuSpeed(), cfg.frequency, cfg.samplingMethod, cfg.fastSampling);

            // Configure, setup and install C64 environment/events
            initialise();
        }
        catch (configError const &e)
        {
            m_errorString = e.message();
            m_cfg.sidEmulation = 0;
            if (&m_cfg != &cfg)
            {
                config(m_cfg);
            }
            return false;
        }
    }

    if (secondSidAddress)
    {
        // Assumed to be in d420-d7ff or de00-dfff range
        m_c64.setSecondSIDAddress(secondSidAddress);
        m_info.m_channels = 2;
    }
    else
    {
        m_c64.setSecondSIDAddress(0);
        m_info.m_channels = 1;
    }

    m_mixer.setStereo(cfg.playback == SidConfig::STEREO);
    m_mixer.setVolume(cfg.leftVolume, cfg.rightVolume);

    // Update Configuration
    m_cfg = cfg;

    return true;
}

// Clock speed changes due to loading a new song
c64::model_t Player::c64model(SidConfig::c64_model_t defaultModel, bool forced)
{
    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    SidTuneInfo::clock_t clockSpeed = tuneInfo->clockSpeed();

    c64::model_t model;

    // Use preferred speed if forced or if song speed is unknown
    if (forced || clockSpeed == SidTuneInfo::CLOCK_UNKNOWN || clockSpeed == SidTuneInfo::CLOCK_ANY)
    {
        switch (defaultModel)
        {
        case SidConfig::PAL:
            clockSpeed = SidTuneInfo::CLOCK_PAL;
            model = c64::PAL_B;
            break;
        case SidConfig::DREAN:
            clockSpeed = SidTuneInfo::CLOCK_PAL;
            model = c64::PAL_N;
            break;
        case SidConfig::NTSC:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            model = c64::NTSC_M;
            break;
        case SidConfig::OLD_NTSC:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            model = c64::OLD_NTSC_M;
            break;
        }
    }
    else
    {
        switch (clockSpeed)
        {
        case SidTuneInfo::CLOCK_PAL:
            model = c64::PAL_B;
            break;
        case SidTuneInfo::CLOCK_NTSC:
            model = c64::NTSC_M;
            break;
        }
    }

    switch (clockSpeed)
    {
    case SidTuneInfo::CLOCK_PAL:
        if (tuneInfo->songSpeed() == SidTuneInfo::SPEED_CIA_1A)
            m_info.m_speedString = TXT_PAL_CIA;
        else if (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_NTSC)
            m_info.m_speedString = TXT_PAL_VBI_FIXED;
        else
            m_info.m_speedString = TXT_PAL_VBI;
        break;
    case SidTuneInfo::CLOCK_NTSC:
        if (tuneInfo->songSpeed() == SidTuneInfo::SPEED_CIA_1A)
            m_info.m_speedString = TXT_NTSC_CIA;
        else if (tuneInfo->clockSpeed() == SidTuneInfo::CLOCK_PAL)
            m_info.m_speedString = TXT_NTSC_VBI_FIXED;
        else
            m_info.m_speedString = TXT_NTSC_VBI;
        break;
    }

    return model;
}

SidConfig::sid_model_t Player::getModel(SidTuneInfo::model_t sidModel, SidConfig::sid_model_t defaultModel, bool forced)
{
    SidTuneInfo::model_t tuneModel = sidModel;

    // Use preferred speed if forced or if song speed is unknown
    if (forced || tuneModel == SidTuneInfo::SIDMODEL_UNKNOWN || tuneModel == SidTuneInfo::SIDMODEL_ANY)
    {
        switch (defaultModel)
        {
        case SidConfig::MOS6581:
            tuneModel = SidTuneInfo::SIDMODEL_6581;
            break;
        case SidConfig::MOS8580:
            tuneModel = SidTuneInfo::SIDMODEL_8580;
            break;
        }
    }

    SidConfig::sid_model_t newModel;

    switch (tuneModel)
    {
    case SidTuneInfo::CLOCK_PAL:
        newModel = SidConfig::MOS6581;
        break;
    case SidTuneInfo::CLOCK_NTSC:
        newModel = SidConfig::MOS8580;
        break;
    }

    return newModel;
}

void Player::sidRelease()
{
    unsigned int i=0;
    while (sidemu *s = m_mixer.getSid(i))
    {
        if (sidbuilder *b = s->builder())
        {
            b->unlock(s);
        }
        m_c64.setSid(i, 0);
        i++;
    }

    m_mixer.clearSids();
}

void Player::sidCreate(sidbuilder *builder, SidConfig::sid_model_t defaultModel,
                        bool forced, unsigned int channels)
{
    if (builder)
    {
        // Detect the Correct SID model
        // Determine model when unknown
        SidConfig::sid_model_t userModels[c64::MAX_SIDS];

        const SidTuneInfo* tuneInfo = m_tune->getInfo();

        userModels[0] = getModel(tuneInfo->sidModel1(), defaultModel, forced);
        // If bits 6-7 are set to Unknown then the second SID will be set to the same SID
        // model as the first SID.
        userModels[1] = getModel(tuneInfo->sidModel2(), userModels[0], forced);

        for (unsigned int i = 0; i < channels; i++)
        {
            sidemu *s = builder->lock(m_c64.getEventScheduler(), userModels[i]);
            // Get at least one SID emulation
            if ((i == 0) && !builder->getStatus())
            {
                throw new configError(builder->error());
            }

            m_c64.setSid(i, s);
            m_mixer.addSid(s);
        }
    }
}

void Player::sidParams(double cpuFreq, int frequency,
                        SidConfig::sampling_method_t sampling, bool fastSampling)
{
    unsigned int i = 0;

    while (sidemu *s = m_mixer.getSid(i))
    {
        s->sampling((float)cpuFreq, frequency, sampling, fastSampling);
        i++;
    }
}

SIDPLAYFP_NAMESPACE_STOP
