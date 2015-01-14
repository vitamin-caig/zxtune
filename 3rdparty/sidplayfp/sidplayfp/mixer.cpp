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

#include "mixer.h"

#include <algorithm>

#include "sidemu.h"

void clockChip(sidemu *s) { s->clock(); }
void clockChipSilent(sidemu* s) { s->clockSilent(); }

class bufferPos
{
public:
    bufferPos(int i) : pos(i) {}
    void operator()(sidemu *s) { s->bufferpos(pos); }

private:
    int pos;
};

class bufferMove
{
public:
    bufferMove(int p, int s) : pos(p), samples(s) {}
    void operator()(short *dest)
    {
        const short* src = dest + pos;
        for (int j = 0; j < samples; j++)
        {
            dest[j] = src[j];
        }
    }

private:
    int pos;
    int samples;
};

void Mixer::clockChips()
{
    std::for_each(m_chips.begin(), m_chips.end(), m_sampleBuffer ? &clockChip : &clockChipSilent);
}

void Mixer::resetBufs()
{
    std::for_each(m_chips.begin(), m_chips.end(), bufferPos(0));
}

void Mixer::renderSamples()
{
    /* extract buffer info now that the SID is updated.
     * clock() may update bufferpos.
     * NB: if chip2 exists, its bufferpos is identical to chip1's. */
    const int sampleCount = m_chips.front()->bufferpos();
    const unsigned int channels = m_stereo ? 2 : 1;

    short *buf = m_sampleBuffer + m_sampleIndex;

    int i = 0;
    while (i < sampleCount && m_sampleIndex < m_sampleCount)
    {
        const int dither = triangularDithering();

        for (size_t k = 0; k < m_buffers.size(); k++)
        {
            m_iSamples[k] = (int_least32_t(m_buffers[k][i]) * m_volume[k] + dither) / VOLUME_MAX;
        }

        for (unsigned int k = 0; k < channels; k++)
        {
            *buf++ = (m_mix[k])(&m_iSamples.front());
            m_sampleIndex++;
        }
        ++i;
    }

    /* move the unhandled data to start of buffer, if any. */
    const int samplesLeft = sampleCount - i;
    std::for_each(m_buffers.begin(), m_buffers.end(), bufferMove(i, samplesLeft));
    std::for_each(m_chips.begin(), m_chips.end(), bufferPos(samplesLeft));
}

void Mixer::renderSilence()
{
    std::for_each(m_chips.begin(), m_chips.end(), clockChipSilent);

    const int sampleCount = m_chips.front()->bufferpos();
    const unsigned int channels = m_stereo ? 2 : 1;
    const int i = std::min<int>(sampleCount, (m_sampleCount - m_sampleIndex) / channels);
    m_sampleIndex += channels * i;

    const int samplesLeft = sampleCount - i;
    std::for_each(m_chips.begin(), m_chips.end(), bufferPos(samplesLeft));
}

void Mixer::doMix()
{
    if (m_sampleBuffer)
    {
        renderSamples();
    }
    else
    {
        renderSilence();
    }
}

void Mixer::begin(short *buffer, uint_least32_t count)
{
    m_sampleIndex  = 0;
    m_sampleCount  = count;
    m_sampleBuffer = buffer;
}

void Mixer::updateParams()
{
    m_mix[0] = (!m_stereo && m_buffers.size() > 1) ? &channel1MonoMix : &channel1StereoMix;
    if (m_stereo)
        m_mix[1] = (m_buffers.size() > 1) ? &channel2FromStereoMix : &channel2FromMonoMix;
}

void Mixer::clearSids()
{
    m_chips.clear();
    m_buffers.clear();
}

void Mixer::addSid(sidemu *chip)
{
    if (chip != 0)
    {
        m_chips.push_back(chip);
        m_buffers.push_back(chip->buffer());

        m_iSamples.resize(m_buffers.size());

        if (m_mix.size() > 0)
            updateParams();
    }
}

void Mixer::setStereo(bool stereo)
{
    if (m_stereo != stereo)
    {
        m_stereo = stereo;

        m_mix.resize(m_stereo ? 2 : 1);

        updateParams();
    }
}

void Mixer::setVolume(int_least32_t left, int_least32_t right)
{
    m_volume.clear();
    m_volume.push_back(left);
    m_volume.push_back(right);
}
