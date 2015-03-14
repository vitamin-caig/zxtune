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
const char ERR_UNSUPPORTED_FREQ[]     = "SIDPLAYER ERROR: Unsupported sampling frequency.";
const char ERR_UNSUPPORTED_SID_ADDR[] = "SIDPLAYER ERROR: Unsupported SID address.";

bool Player::config(const SidConfig &cfg)
{
    // Check for base sampling frequency
    if (cfg.frequency < 8000)
    {
        m_errorString = ERR_UNSUPPORTED_FREQ;
        return false;
    }

    const bool forceReconfig = &cfg == &m_cfg;
    uint_least16_t secondSidAddress = cfg.secondSidAddress;

    // Only do these if we have a loaded tune
    if (m_tune != 0)
    {
        const SidTuneInfo* tuneInfo = m_tune->getInfo();

        if (tuneInfo->sidChipBase2() != 0)
            secondSidAddress = tuneInfo->sidChipBase2();

        try
        {
            const bool needReinit = forceReconfig
                || cfg.sidEmulation != m_cfg.sidEmulation
                || cfg.defaultSidModel != m_cfg.defaultSidModel
                || cfg.forceSidModel != m_cfg.forceSidModel;
            if (needReinit)
            {
              sidRelease();
              sidCreate(cfg.sidEmulation, cfg.defaultSidModel, cfg.forceSidModel, secondSidAddress);
            }

            if (forceReconfig
                || cfg.defaultC64Model != m_cfg.defaultC64Model
                || cfg.forceC64Model != m_cfg.forceC64Model)
            {
              // Determine clock speed
              const c64::model_t model = c64model(cfg.defaultC64Model, cfg.forceC64Model);

              m_c64.setModel(model);
            }

            if (forceReconfig
                || cfg.frequency != m_cfg.frequency
                || cfg.samplingMethod != m_cfg.samplingMethod
                || cfg.fastSampling != m_cfg.fastSampling)
            {
              sidParams(m_c64.getMainCpuSpeed(), cfg.frequency, cfg.samplingMethod, cfg.fastSampling);
            }

            if (needReinit)
            {
              // Configure, setup and install C64 environment/events
              initialise();
            }
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

    m_info.m_channels = secondSidAddress ? 2 : 1;

    m_mixer.setStereo(cfg.playback == SidConfig::STEREO);
    m_mixer.setVolume(cfg.leftVolume, cfg.rightVolume);

    // Update Configuration
    m_cfg = cfg;
    m_cfg.secondSidAddress = secondSidAddress;

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
            videoSwitch = 1;
            break;
        case SidConfig::DREAN:
            clockSpeed = SidTuneInfo::CLOCK_PAL;
            model = c64::PAL_N;
            videoSwitch = 1; // TODO verify
            break;
        case SidConfig::NTSC:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            model = c64::NTSC_M;
            videoSwitch = 0;
            break;
        case SidConfig::OLD_NTSC:
            clockSpeed = SidTuneInfo::CLOCK_NTSC;
            model = c64::OLD_NTSC_M;
            videoSwitch = 0;
            break;
        }
    }
    else
    {
        switch (clockSpeed)
        {
        case SidTuneInfo::CLOCK_PAL:
            model = c64::PAL_B;
            videoSwitch = 1;
            break;
        case SidTuneInfo::CLOCK_NTSC:
            model = c64::NTSC_M;
            videoSwitch = 0;
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
    case SidTuneInfo::SIDMODEL_6581:
        newModel = SidConfig::MOS6581;
        break;
    case SidTuneInfo::SIDMODEL_8580:
        newModel = SidConfig::MOS8580;
        break;
    }

    return newModel;
}

void Player::sidRelease()
{
    m_c64.clearSids();

    for (unsigned int i = 0; ; i++)
    {
        sidemu *s = m_mixer.getSid(i);
        if (s == 0)
            break;

        if (sidbuilder *b = s->builder())
        {
            b->unlock(s);
        }
    }

    m_mixer.clearSids();
}

void Player::sidCreate(sidbuilder *builder, SidConfig::sid_model_t defaultModel,
                        bool forced, const unsigned int secondSidAddresses)
{
    if (builder != 0)
    {
        const SidTuneInfo* tuneInfo = m_tune->getInfo();

        // Setup base SID
        const SidConfig::sid_model_t userModel = getModel(tuneInfo->sidModel1(), defaultModel, forced);
        sidemu *s = builder->lock(m_c64.getEventScheduler(), userModel);
        if (!builder->getStatus())
        {
            throw configError(builder->error());
        }

        m_c64.setBaseSid(s);
        m_mixer.addSid(s);

        // Setup extra SIDs if needed
        if (secondSidAddresses != 0)
        {
            // If bits 6-7 are set to Unknown then the second SID will be set to the same SID
            // model as the first SID.
            const SidConfig::sid_model_t secondSidModel = getModel(tuneInfo->sidModel2(), userModel, forced);

            sidemu *s = builder->lock(m_c64.getEventScheduler(), secondSidModel);

            if (!m_c64.addExtraSid(s, secondSidAddresses))
                throw configError(ERR_UNSUPPORTED_SID_ADDR);

            m_mixer.addSid(s);
        }
    }
}

void Player::sidParams(double cpuFreq, int frequency,
                        SidConfig::sampling_method_t sampling, bool fastSampling)
{
    for (unsigned int i = 0; ; i++)
    {
        sidemu *s = m_mixer.getSid(i);
        if (s == 0)
            break;

        s->sampling((float)cpuFreq, frequency, sampling, fastSampling);
    }
}

SIDPLAYFP_NAMESPACE_STOP
