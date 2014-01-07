/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004 Dag Lem <resid@nimrod.no>
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

#ifndef VOICE_H
#define VOICE_H

#include "siddefs-fp.h"
#include "WaveformGenerator.h"
#include "EnvelopeGenerator.h"

namespace reSIDfp
{

/**
 * Representation of SID voice block.
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class Voice
{
private:
    WaveformGenerator* waveformGenerator;

    EnvelopeGenerator* envelopeGenerator;

public:
    /**
     * Amplitude modulated waveform output.
     *
     * The waveform DAC generates a voltage between 5 and 12 V corresponding
     * to oscillator state 0 .. 4095.
     *
     * The envelope DAC generates a voltage between waveform gen output and
     * the 5V level, corresponding to envelope state 0 .. 255.
     *
     * Ideal range [-2048*255, 2047*255].
     *
     * @param ringModulator Ring-modulator for waveform
     * @return waveformgenerator output
     */
    RESID_INLINE
    int output(const WaveformGenerator* ringModulator) const
    {
        return waveformGenerator->output(ringModulator) * envelopeGenerator->output();
    }

    /**
     * Constructor.
     */
    Voice() :
        waveformGenerator(new WaveformGenerator()),
        envelopeGenerator(new EnvelopeGenerator()) {}

    ~Voice()
    {
        delete waveformGenerator;
        delete envelopeGenerator;
    }

    WaveformGenerator* wave() const { return waveformGenerator; }

    EnvelopeGenerator* envelope() const { return envelopeGenerator; }

    // ----------------------------------------------------------------------------
    // Register functions.
    // ----------------------------------------------------------------------------

    /**
     * Register functions.
     *
     * @param control Control register value.
     */
    void writeCONTROL_REG(unsigned char control)
    {
        waveformGenerator->writeCONTROL_REG(control);
        envelopeGenerator->writeCONTROL_REG(control);
    }

    /**
     * SID reset.
     */
    void reset()
    {
        waveformGenerator->reset();
        envelopeGenerator->reset();
    }
};

} // namespace reSIDfp

#endif
