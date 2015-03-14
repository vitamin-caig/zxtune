/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright (C) 2000 Simon White
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef MIXER_H
#define MIXER_H

#include <stdint.h>
#include <cstdlib>

#include <vector>

class sidemu;

/**
 * This class implements the mixer.
 */
class Mixer
{
public:
    /// Maximum number of supported SIDs (mono and stereo)
    static const unsigned int MAX_SIDS = 2;

private:
    typedef short (*mixer_func_t)(const int_least32_t* s);

public:
    /**
     * Maximum allowed volume, must be a power of 2.
     */
    static const int_least32_t VOLUME_MAX = 1024;

private:
    std::vector<sidemu*> m_chips;
    std::vector<short*> m_buffers;

    std::vector<int_least32_t> m_iSamples;
    std::vector<int_least32_t> m_volume;

    std::vector<mixer_func_t> m_mix;

    int oldRandomValue;

    // Mixer settings
    short         *m_sampleBuffer;
    uint_least32_t m_sampleCount;
    uint_least32_t m_sampleIndex;

    bool m_stereo;

private:
    void updateParams();

    int triangularDithering()
    {
        const int prevValue = oldRandomValue;
        oldRandomValue = rand() & (VOLUME_MAX-1);
        return oldRandomValue - prevValue;
    }

    void renderSamples();
    void renderSilence();

    static short channel1MonoMix(const int_least32_t* s) { return static_cast<short>((s[0] + s[1]) / 2); }
    static short channel1StereoMix(const int_least32_t* s) { return static_cast<short>(s[0]); }

    static short channel2FromMonoMix(const int_least32_t* s) { return static_cast<short>(s[0]); }
    static short channel2FromStereoMix(const int_least32_t* s) { return static_cast<short>(s[1]); }

public:
    /**
     * Create a new mixer.
     *
     * @param context event context
     */
    Mixer() :
        oldRandomValue(0),
        m_sampleCount(0),
        m_stereo(false)
    {
        m_mix.push_back(&Mixer::channel1MonoMix);
    }

    /**
     * Do the mixing.
     */
    void doMix();

    /**
     * This clocks the SIDs to the present moment, if they aren't already.
     */
    void clockChips();

    /**
     * Reset sidemu buffer position discarding produced samples.
     */
    void resetBufs();

    /**
     * Prepare for mixing cycle.
     *
     * @param buffer output. 0 for optimized silence rendering
     * @param count size of the buffer in samples
     */
    void begin(short *buffer, uint_least32_t count);

    /**
     * Remove all SIDs from the mixer.
     */
    void clearSids();

    /**
     * Add a SID to the mixer.
     *
     * @param chip the sid emu to add
     */
    void addSid(sidemu *chip);

    /**
     * Get a SID to the mixer.
     *
     * @param i the number of the SID to get
     * @return a pointer to the requested sid emu or 0 if not found
     */
    sidemu* getSid(unsigned int i) const { return (i < m_chips.size()) ? m_chips[i] : 0; }

    /**
     * Set mixing volumes, from 0 to #VOLUME_MAX.
     *
     * @param left volume for left or mono channel
     * @param right volume for right channel in stereo mode
     */
    void setVolume(int_least32_t left, int_least32_t right);

    /**
     * Set mixing mode.
     *
     * @param stereo true for stereo mode, false for mono
     */
    void setStereo(bool stereo);

    /**
     * Check if the buffer have been filled.
     */
    bool notFinished() const { return m_sampleIndex != m_sampleCount; }

    /**
     * Get the number of samples generated up to now.
     */
    uint_least32_t samplesGenerated() const { return m_sampleIndex; }

    /**
     * Get the current buffer
     */
    const short* samplesBuffer() const { return m_sampleBuffer; }
};

#endif // MIXER_H
