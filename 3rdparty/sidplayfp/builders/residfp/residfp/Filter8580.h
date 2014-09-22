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

#ifndef FILTER8580_H
#define FILTER8580_H

#include <cmath>

#include "siddefs-fp.h"

#include "Filter.h"

namespace reSIDfp
{

/**
 * Filter for 8580 chip based on simple linear approximation
 * of the FC control.
 *
 * This is like the original reSID filter except the phase
 * of BP output has been inverted. I saw samplings on the internet
 * that indicated it would genuinely happen like this.
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class Filter8580 : public Filter
{
private:
    double highFreq;
    float Vlp, Vbp, Vhp;
    float w0, _1_div_Q;
    int ve;

public:
    Filter8580() :
        highFreq(12500.),
        Vlp(0.f),
        Vbp(0.f),
        Vhp(0.f),
        w0(0.f),
        _1_div_Q(0.f),
        ve(0) {}

    int clock(int voice1, int voice2, int voice3);

    void updatedCenterFrequency() { w0 = (float)(2. * M_PI * highFreq * fc / 2047. / 1e6); }

    void updatedResonance() { _1_div_Q = 1.f / (0.707f + res / 15.f); }

    void input(int input) { ve = input << 4; }

    void updatedMixing() {}

    void setFilterCurve(double curvePosition) { highFreq = curvePosition; }
};

} // namespace reSIDfp

#if RESID_INLINING || defined(FILTER8580_CPP)

#include <stdlib.h>
#include <math.h>

namespace reSIDfp
{

RESID_INLINE
int Filter8580::clock(int voice1, int voice2, int voice3)
{
    voice1 >>= 7;
    voice2 >>= 7;
    voice3 >>= 7;

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

    const float dVbp = w0 * Vhp;
    const float dVlp = w0 * Vbp;
    Vbp -= dVbp;
    Vlp -= dVlp;
    Vhp = (Vbp * _1_div_Q) - Vlp - Vi + float(rand()) / float(RAND_MAX);

    float Vof = (float)Vo;

    if (lp)
    {
        Vof += Vlp;
    }

    if (bp)
    {
        Vof += Vbp;
    }

    if (hp)
    {
        Vof += Vhp;
    }

    return (int) Vof * vol >> 4;
}

} // namespace reSIDfp

#endif

#endif
