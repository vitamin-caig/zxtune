/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
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

#ifndef OPAMP_H
#define OPAMP_H

#include "Spline.h"

namespace reSIDfp
{

/**
 * This class solves the opamp equation when loaded by different sets of resistors.
 * Equations and first implementation were written by Dag Lem.
 * This class is a rewrite without use of fixed point integer mathematics, and
 * uses the actual voltages instead of the normalized values.
 *
 * @author alankila
 */
class OpAmp
{
private:
    static const double EPSILON;

    /** Current root position (cached as guess to speed up next iteration) */
    double x;

    const double Vddt, vmin, vmax;

    Spline* opamp;

    double out[2];

public:
    /**
     * Opamp input -> output voltage conversion
     *
     * @param opamp opamp mapping table as pairs of points (in -> out)
     * @param opamplength length of the opamp array
     * @param Vddt transistor dt parameter (in volts)
     */
    OpAmp(const double opamp[][2], int opamplength, double Vddt) :
        x(0.),
        Vddt(Vddt),
        vmin(opamp[0][0]),
        vmax(opamp[opamplength - 1][0]),
        opamp(new Spline(opamp, opamplength)) {}

    ~OpAmp() { delete opamp; }

    void reset()
    {
        x = vmin;
    }

    /**
     * Solve the opamp equation for input vi in loading context n
     *
     * @param n the ratio of input/output loading
     * @param vi input
     * @return vo
     */
    double solve(double n, double vi);
};

} // namespace reSIDfp

#endif
