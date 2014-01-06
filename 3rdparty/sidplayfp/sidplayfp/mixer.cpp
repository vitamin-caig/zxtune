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

#include "sidplayfp/sidemu.h"

/**
* Scheduling time for next sample mixing event.
*/
const int MIXER_EVENT_RATE = OUTPUTBUFFERSIZE;

void clockChip(sidemu *s) { s->clock(); }

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

void Mixer::event()
{
    /* this clocks the SIDs to the present moment, if they aren't already. */
    std::for_each(m_chips.begin(), m_chips.end(), clockChip);

    if (m_sampleBuffer && m_chips.size())
    {
        short *buf = m_sampleBuffer + m_sampleIndex;

        /* extract buffer info now that the SID is updated.
         * clock() may update bufferpos.
         * NB: if chip2 exists, its bufferpos is identical to chip1's. */
        const int sampleCount = m_chips[0]->bufferpos();

        int i = 0;
        while (i < sampleCount)
        {
            /* Handle whatever output the sid has generated so far */
            if (m_sampleIndex >= m_sampleCount)
            {
                break;
            }
            /* Are there enough samples to generate the next one? */
            if (i + m_fastForwardFactor >= sampleCount)
            {
                break;
            }

            const int dither = triangularDithering();

            /* This is a crude boxcar low-pass filter to
             * reduce aliasing during fast forward. */
            for (size_t k = 0; k < m_buffers.size(); k++)
            {
                int_least32_t sample = 0;
                const short *buffer = m_buffers[k] + i;
                for (int j = 0; j < m_fastForwardFactor; j++)
                {
                    sample += buffer[j];
                }

                m_iSamples[k] = (sample * m_volume[k] + dither) / VOLUME_MAX;
                m_iSamples[k] /= m_fastForwardFactor;
            }

            /* increment i to mark we ate some samples, finish the boxcar thing. */
            i += m_fastForwardFactor;

            const unsigned int channels = m_stereo ? 2 : 1;
            for (unsigned int k = 0; k < channels; k++)
            {
                *buf++ = (this->*(m_mix[k]))();
                m_sampleIndex++;
            }
        }

        /* move the unhandled data to start of buffer, if any. */
        const int samplesLeft = sampleCount - i;
        std::for_each(m_buffers.begin(), m_buffers.end(), bufferMove(i, samplesLeft));
        std::for_each(m_chips.begin(), m_chips.end(), bufferPos(samplesLeft - 1));
    }
    else
    {
        m_sampleIndex++; // FIXME this sucks
        std::for_each(m_chips.begin(), m_chips.end(), bufferPos(0));
    }

    /* Post a callback to ourselves. */
    event_context.schedule(*this, MIXER_EVENT_RATE);
}

void Mixer::reset()
{
    event_context.schedule(*this, MIXER_EVENT_RATE, EVENT_CLOCK_PHI1);
}

void Mixer::begin(short *buffer, uint_least32_t count)
{
    m_sampleIndex  = 0;
    m_sampleCount  = count;
    m_sampleBuffer = buffer;
}

void Mixer::updateParams()
{
    m_mix[0] = (!m_stereo && m_buffers.size() > 1) ? &Mixer::channel1MonoMix : &Mixer::channel1StereoMix;
    if (m_stereo)
        m_mix[1] = (m_buffers.size() > 1) ? &Mixer::channel2FromStereoMix : &Mixer::channel2FromMonoMix;
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

bool Mixer::setFastForward(int ff)
{
    if (ff < 1 || ff > 32)
        return false;

    m_fastForwardFactor = ff;
    return true;
}

void Mixer::setVolume(int_least32_t left, int_least32_t right)
{
    m_volume.clear();
    m_volume.push_back(left);
    m_volume.push_back(right);
}
