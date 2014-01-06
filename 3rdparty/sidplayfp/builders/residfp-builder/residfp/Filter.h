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

#ifndef FILTER_H
#define FILTER_H

namespace reSIDfp
{

/**
 * SID filter base class
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class Filter
{
private:
    /**
     * Filter enabled.
     */
    bool enabled;

    /**
     * Selects which inputs to route through filter.
     */
    char filt;

protected:

    /**
     * Current clock frequency.
     */
    double clockFrequency;

    /**
     * Filter cutoff frequency.
     */
    int fc;

    /**
     * Filter resonance.
     */
    int res;

    /**
     * Current volume.
     */
    int vol;

    /**
     * Routing to filter or outside filter
     */
    bool filt1, filt2, filt3, filtE;

    /**
     * Switch voice 3 off.
     */
    bool voice3off;

    /**
     * Highpass, bandpass, and lowpass filter modes.
     */
    bool hp, bp, lp;

protected:
    /**
     * Set filter cutoff frequency.
     */
    virtual void updatedCenterFrequency() = 0;

    /**
     * Set filter resonance.
     */
    virtual void updatedResonance() = 0;

    /**
     * Mixing configuration modified (offsets change)
     */
    virtual void updatedMixing() = 0;

public:
    Filter() :
        enabled(true),
        filt(0),
        clockFrequency(0.),
        fc(0),
        res(0),
        vol(0),
        filt1(false),
        filt2(false),
        filt3(false),
        filtE(false),
        voice3off(false),
        hp(false),
        bp(false),
        lp(false) {}

    virtual ~Filter() {}

    /**
     * SID clocking - 1 cycle
     *
     * @param v1 voice 1 in
     * @param v2 voice 2 in
     * @param v3 voice 3 in
     * @return filtered output
     */
    virtual int clock(int v1, int v2, int v3) = 0;

    /**
     * Enable filter.
     *
     * @param enable
     */
    void enable(bool enable);

    void setClockFrequency(double clock);

    /**
     * SID reset.
     */
    void reset();

    /**
     * Register function.
     *
     * @param fc_lo Frequency Cutoff Low-Byte
     */
    void writeFC_LO(unsigned char fc_lo);

    /**
     * Register function.
     *
     * @param fc_hi Frequency Cutoff High-Byte
     */
    void writeFC_HI(unsigned char fc_hi);

    /**
     * Register function.
     *
     * @param res_filt Resonance/Filter
     */
    void writeRES_FILT(unsigned char res_filt);

    /**
     * Register function.
     *
     * @param mode_vol Filter Mode/Volume
     */
    void writeMODE_VOL(unsigned char mode_vol);

    virtual void input(int input) = 0;
};

} // namespace reSIDfp

#endif
