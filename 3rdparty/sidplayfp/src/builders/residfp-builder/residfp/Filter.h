/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2024 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef FILTER_H
#define FILTER_H

#include "FilterModelConfig.h"

#include "siddefs-fp.h"

namespace reSIDfp
{

class Integrator;

/**
 * SID filter base class
 */
class Filter
{
private:
    FilterModelConfig* fmc;

    unsigned short** mixer;
    unsigned short** summer;
    unsigned short** resonance;
    unsigned short** volume;

protected:
    /// VCR + associated capacitor connected to highpass output.
    Integrator* const hpIntegrator;

    /// VCR + associated capacitor connected to bandpass output.
    Integrator* const bpIntegrator;

private:
    /// Current filter/voice mixer setting.
    unsigned short* currentMixer;

    /// Filter input summer setting.
    unsigned short* currentSummer;

    /// Filter resonance value.
    unsigned short* currentResonance;

    /// Current volume amplifier setting.
    unsigned short* currentVolume;

    /// Filter highpass state.
    int Vhp;

    /// Filter bandpass state.
    int Vbp;

    /// Filter lowpass state.
    int Vlp;

    /// Filter external input.
    int Ve;

    /// Filter cutoff frequency.
    unsigned int fc;

    /// Routing to filter or outside filter
    bool filt1, filt2, filt3, filtE;

    /// Switch voice 3 off.
    bool voice3off;

    /// Highpass, bandpass, and lowpass filter modes.
    bool hp, bp, lp;

    /// Current volume.
    unsigned char vol;

    /// Filter enabled.
    bool enabled;

    /// Selects which inputs to route through filter.
    unsigned char filt;

protected:
    /**
     * Update filter cutoff frequency.
     */
    virtual void updateCenterFrequency() = 0;

    /**
     * Update filter resonance.
     *
     * @param res the new resonance value
     */
    void updateResonance(unsigned char res) { currentResonance = resonance[res]; }

    /**
     * Mixing configuration modified (offsets change)
     */
    void updateMixing();

    /**
     * Get the filter cutoff register value
     */
    unsigned int getFC() const { return fc; }

public:
    Filter(FilterModelConfig* fmc);

    virtual ~Filter();

    /**
     * SID clocking - 1 cycle
     *
     * @param v1 voice 1 in
     * @param v2 voice 2 in
     * @param v3 voice 3 in
     * @return filtered output
     */
    unsigned short clock(float v1, float v2, float v3);

    /**
     * Enable filter.
     *
     * @param enable
     */
    void enable(bool enable);

    /**
     * SID reset.
     */
    void reset();

    /**
     * Write Frequency Cutoff Low register.
     *
     * @param fc_lo Frequency Cutoff Low-Byte
     */
    void writeFC_LO(unsigned char fc_lo);

    /**
     * Write Frequency Cutoff High register.
     *
     * @param fc_hi Frequency Cutoff High-Byte
     */
    void writeFC_HI(unsigned char fc_hi);

    /**
     * Write Resonance/Filter register.
     *
     * @param res_filt Resonance/Filter
     */
    void writeRES_FILT(unsigned char res_filt);

    /**
     * Write filter Mode/Volume register.
     *
     * @param mode_vol Filter Mode/Volume
     */
    void writeMODE_VOL(unsigned char mode_vol);

    /**
     * Apply a signal to EXT-IN
     *
     * @param input a signed 16 bit sample
     */
    void input(int input) { Ve = fmc->getNormalizedVoice(input/65536.f); }
};

} // namespace reSIDfp

#if RESID_INLINING || defined(FILTER_CPP)

#include "Integrator.h"

namespace reSIDfp
{

RESID_INLINE
unsigned short Filter::clock(float voice1, float voice2, float voice3)
{
    const int V1 = fmc->getNormalizedVoice(voice1);
    const int V2 = fmc->getNormalizedVoice(voice2);
    // Voice 3 is silenced by voice3off if it is not routed through the filter.
    const int V3 = (filt3 || !voice3off) ? fmc->getNormalizedVoice(voice3) : 0;

    int Vsum = 0;
    int Vmix = 0;

    (filt1 ? Vsum : Vmix) += V1;
    (filt2 ? Vsum : Vmix) += V2;
    (filt3 ? Vsum : Vmix) += V3;
    (filtE ? Vsum : Vmix) += Ve;

    Vhp = currentSummer[currentResonance[Vbp] + Vlp + Vsum];
    Vbp = hpIntegrator->solve(Vhp);
    Vlp = bpIntegrator->solve(Vbp);

    if (lp) Vmix += Vlp;
    if (bp) Vmix += Vbp;
    if (hp) Vmix += Vhp;

    return currentVolume[currentMixer[Vmix]];
}

} // namespace reSIDfp

#endif

#endif
