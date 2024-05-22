/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2022 Leandro Nini <drfiemost@users.sourceforge.net>
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


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// Redirection to private version of sidplayer (This method is called Cheshire Cat)
// [ms: which is J. Carolan's name for a degenerate 'bridge']
// This interface can be directly replaced with a libsidplay1 or C interface wrapper.
//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

#include "sidplayfp.h"

#include "player.h"

sidplayfp::sidplayfp() :
    sidplayer(*(new libsidplayfp::Player)) {}

sidplayfp::~sidplayfp()
{
    delete &sidplayer;
}

bool sidplayfp::config(const SidConfig &cfg)
{
    return sidplayer.config(cfg);
}

const SidConfig &sidplayfp::config() const
{
    return sidplayer.config();
}

void sidplayfp::stop()
{
    sidplayer.stop();
}

uint_least32_t sidplayfp::play(short *buffer, uint_least32_t count)
{
    return sidplayer.play(buffer, count);
}

bool sidplayfp::load(SidTune *tune)
{
    return sidplayer.load(tune);
}

const SidInfo &sidplayfp::info() const
{
    return sidplayer.info();
}

uint_least32_t sidplayfp::time() const
{
    return sidplayer.timeMs() / 1000;
}

uint_least32_t sidplayfp::timeMs() const
{
    return sidplayer.timeMs();
}

const char *sidplayfp::error() const
{
    return sidplayer.error();
}

bool  sidplayfp::fastForward(unsigned int percent)
{
    return sidplayer.fastForward(percent);
}

void sidplayfp::mute(unsigned int sidNum, unsigned int voice, bool enable)
{
    sidplayer.mute(sidNum, voice, enable);
}

void sidplayfp::debug(bool enable, FILE *out)
{
    sidplayer.debug(enable, out);
}

bool sidplayfp::isPlaying() const
{
    return sidplayer.isPlaying();
}

void sidplayfp::setKernal(const uint8_t* rom) { sidplayer.setKernal(rom); }
void sidplayfp::setBasic(const uint8_t* rom) { sidplayer.setBasic(rom); }
void sidplayfp::setChargen(const uint8_t* rom) { sidplayer.setChargen(rom); }

void sidplayfp::setRoms(const uint8_t* kernal, const uint8_t* basic, const uint8_t* character)
{
    setKernal(kernal);
    setBasic(basic);
    setChargen(character);
}

uint_least16_t sidplayfp::getCia1TimerA() const
{
    return sidplayer.getCia1TimerA();
}

bool sidplayfp::getSidStatus(unsigned int sidNum, uint8_t regs[32])
{
    return sidplayer.getSidStatus(sidNum, regs);
}
