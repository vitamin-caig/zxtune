/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2004,2010 Dag Lem <resid@nimrod.no>
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

#ifndef FILTER6581_H
#define FILTER6581_H

#include "siddefs-fp.h"

#include "Filter.h"
#include "FilterModelConfig.h"

namespace reSIDfp
{

class Integrator;

/**
 * Filter based on Dag Lem's 6581 filter from reSID 1.0 prerelease. See original
 * source for discussion about theory of operation.
 *
 * Java port by Antti S. Lankila
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class Filter6581 : public Filter
{
private:
    /** Filter highpass state. */
    int Vhp;

    /** Filter bandpass state. */
    int Vbp;

    /** Filter lowpass state. */
    int Vlp;

    /** Filter external input. */
    int ve;

    const int voiceScaleS14, voiceDC, vo_T16;

    /** Current volume amplifier setting. */
    unsigned short* currentGain;

    /** Current filter/voice mixer setting. */
    unsigned short* currentMixer;

    /** Filter input summer setting. */
    unsigned short* currentSummer;

    /** Filter resonance value. */
    unsigned short* currentResonance;

    /** VCR + associated capacitor connected to highpass output. */
    Integrator* hpIntegrator;

    /** VCR + associated capacitor connected to lowpass output. */
    Integrator* bpIntegrator;

    const unsigned int* f0_dac;

    unsigned short** mixer;
    unsigned short** summer;
    unsigned short** gain;

public:
    Filter6581() :
        Vhp(0),
        Vbp(0),
        Vlp(0),
        ve(0),
        voiceScaleS14(FilterModelConfig::getInstance()->getVoiceScaleS14()),
        voiceDC(FilterModelConfig::getInstance()->getVoiceDC()),
        vo_T16(FilterModelConfig::getInstance()->getVO_T16()),
        currentGain(0),
        currentMixer(0),
        currentSummer(0),
        currentResonance(0),
        hpIntegrator(FilterModelConfig::getInstance()->buildIntegrator()),
        bpIntegrator(FilterModelConfig::getInstance()->buildIntegrator()),
        f0_dac(FilterModelConfig::getInstance()->getDAC(0.5)),
        mixer(FilterModelConfig::getInstance()->getMixer()),
        summer(FilterModelConfig::getInstance()->getSummer()),
        gain(FilterModelConfig::getInstance()->getGain())
    {
        input(0);
    }

    ~Filter6581();

    int clock(int voice1, int voice2, int voice3);

    void input(int sample) { ve = (sample * voiceScaleS14 * 3 >> 10) + mixer[0][0]; }

    /**
     * Switch to new distortion curve.
     */
    void updatedCenterFrequency();

    /**
     * Resonance tuned by ear, based on a few observations:
     *
     * - there's a small notch even in allpass mode - size of resonance hump is
     * about 8 dB
     */
    void updatedResonance() { currentResonance = gain[~res & 0xf]; }

    void updatedMixing();

public:
    /**
     * Set filter curve type based on single parameter.
     *
     * @param curvePosition 0 .. 1, where 0 sets center frequency high ("light") and 1 sets it low ("dark")
     */
    void setFilterCurve(double curvePosition);
};

} // namespace reSIDfp

#if RESID_INLINING || defined(FILTER6581_CPP)

#include "Integrator.h"

namespace reSIDfp
{

RESID_INLINE
int Filter6581::clock(int voice1, int voice2, int voice3)
{
    voice1 = (voice1 * voiceScaleS14 >> 18) + voiceDC;
    voice2 = (voice2 * voiceScaleS14 >> 18) + voiceDC;
    voice3 = (voice3 * voiceScaleS14 >> 18) + voiceDC;

    int Vi = 0;
    int Vo = 0;

    (filt1 ? Vi : Vo) += voice1;

    (filt2 ? Vi : Vo) += voice2;

    // NB! Voice 3 is not silenced by voice3off if it is routed
    // through the filter.
    if (filt3)
    {
        Vi += voice3;
    }
    else if (!voice3off)
    {
        Vo += voice3;
    }

    (filtE ? Vi : Vo) += ve;

    const int oldVhp = Vhp;
    Vhp = currentSummer[currentResonance[Vbp] + Vlp + Vi];
    Vlp = bpIntegrator->solve(Vbp + vo_T16) - vo_T16;
    Vbp = hpIntegrator->solve(oldVhp + vo_T16) - vo_T16;

    if (lp)
    {
        Vo += Vlp;
    }

    if (bp)
    {
        Vo += Vbp;
    }

    if (hp)
    {
        Vo += Vhp;
    }

    return currentGain[currentMixer[Vo]] - (1 << 15);
}

} // namespace reSIDfp

#endif

#endif
