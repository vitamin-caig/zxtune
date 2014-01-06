/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

#include <ctime>

#include "SidTune.h"
#include "sidemu.h"
#include "psiddrv.h"
#include "romCheck.h"

SIDPLAYFP_NAMESPACE_START


const char TXT_NA[]             = "NA";


Player::Player () :
    // Set default settings for system
    m_mixer(m_c64.getEventScheduler()),
    m_tune(0),
    m_errorString(TXT_NA),
    m_isPlaying(false),
    m_rand((unsigned int)::time(0))
{
#ifdef PC64_TESTSUITE
    m_c64.setTestEnv(this);
#endif

    m_c64.setRoms(0, 0, 0);
    config(m_cfg);

    // Get component credits
    m_info.m_credits.push_back(m_c64.cpuCredits());
    m_info.m_credits.push_back(m_c64.ciaCredits());
    m_info.m_credits.push_back(m_c64.vicCredits());
}

void Player::setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
{
    if (kernal)
    {
        kernalCheck k(kernal);
        m_info.m_kernalDesc = k.info();
    }

    if (basic)
    {
        basicCheck b(basic);
        m_info.m_basicDesc = b.info();
    }

    if (character)
    {
        chargenCheck c(character);
        m_info.m_chargenDesc = c.info();
    }

    m_c64.setRoms(kernal, basic, character);
}

bool Player::fastForward(unsigned int percent)
{
    if (!m_mixer.setFastForward(percent / 100))
    {
        m_errorString = "SIDPLAYER ERROR: Percentage value out of range.";
        return false;
    }

    return true;
}

void Player::initialise()
{
    m_isPlaying = false;

    m_c64.reset();

    const SidTuneInfo* tuneInfo = m_tune->getInfo();

    {
        const uint_least32_t size = (uint_least32_t)tuneInfo->loadAddr() + tuneInfo->c64dataLen() - 1;
        if (size > 0xffff)
        {
            throw new configError("SIDPLAYER ERROR: Size of music data exceeds C64 memory.");
        }
    }

    uint_least16_t powerOnDelay = m_cfg.powerOnDelay;
    // Delays above MAX result in random delays
    if (powerOnDelay > SidConfig::MAX_POWER_ON_DELAY)
    {   // Limit the delay to something sensible.
        powerOnDelay = (uint_least16_t)((m_rand.next() >> 3) & SidConfig::MAX_POWER_ON_DELAY);
    }

    psiddrv driver(m_tune->getInfo());
    driver.powerOnDelay(powerOnDelay);
    if (!driver.drvReloc(m_c64.getMemInterface()))
    {
        throw new configError(driver.errorString());
    }

    m_info.m_driverAddr = driver.driverAddr();
    m_info.m_driverLength = driver.driverLength();
    m_info.m_powerOnDelay = powerOnDelay;

    if (!m_tune->placeSidTuneInC64mem(m_c64.getMemInterface()))
    {
        throw new configError(m_tune->statusString());
    }

    driver.install(m_c64.getMemInterface());

    m_c64.resetCpu();

    m_mixer.reset();
}

bool Player::load(SidTune *tune)
{
    m_tune = tune;
    if (!tune)
    {   // Unload tune
        return true;
    }

    {   // Must re-configure on fly for stereo support!
        if (!config(m_cfg))
        {
            // Failed configuration with new tune, reject it
            m_tune = 0;
            return false;
        }
    }
    return true;
}

void Player::mute(unsigned int sidNum, unsigned int voice, bool enable)
{
    sidemu *s = m_mixer.getSid(sidNum);
    if (s)
        s->voice(voice, enable);
}

uint_least32_t Player::play(short *buffer, uint_least32_t count)
{
    // Make sure a tune is loaded
    if (!m_tune)
        return 0;

    if (count)
    {
        m_mixer.begin(buffer, count);

        // Start the player loop
        m_isPlaying = true;

        while (m_isPlaying && m_mixer.notFinished())
            m_c64.getEventScheduler()->clock();

        if (!m_isPlaying)
        {
            try
            {
                initialise();
            }
            catch (configError const &e) {}
        }

        return m_mixer.samplesGenerated();
    }
    else
    {
        count = OUTPUTBUFFERSIZE;
        while (count--)
            m_c64.getEventScheduler()->clock();

        return 0;
    }
}

void Player::stop()
{   // Re-start song
    if (m_tune && m_isPlaying)
    {
        if (m_mixer.notFinished())
        {
            m_isPlaying = false;
        }
        else
        {
            try
            {
                initialise();
            }
            catch (configError const &e) {}
        }
    }
}

SIDPLAYFP_NAMESPACE_STOP
