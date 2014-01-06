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

#include "OpAmp.h"

#include <cmath>

namespace reSIDfp
{

const double OpAmp::EPSILON = 1e-8;

double OpAmp::solve(double n, double vi)
{
    // Start off with an estimate of x and a root bracket [ak, bk].
    // f is decreasing, so that f(ak) > 0 and f(bk) < 0.
    double ak = vmin;
    double bk = vmax;

    const double a = n + 1.;
    const double b = Vddt;
    const double b_vi = (b - vi);
    const double c = n * (b_vi * b_vi);

    for (;;)
    {
        const double xk = x;

        // Calculate f and df.
        opamp->evaluate(x, out);
        const double vo = out[0];
        const double dvo = out[1];

        const double b_vx = b - x;
        const double b_vo = b - vo;

        const double f = a * (b_vx * b_vx) - c - (b_vo * b_vo);
        const double df = 2. * (b_vo * dvo - a * b_vx);

        x -= f / df;

        if (fabs(x - xk) < EPSILON)
        {
            opamp->evaluate(x, out);
            return out[0];
        }

        // Narrow down root bracket.
        (f < 0. ? bk : ak) = xk;

        if (x <= ak || x >= bk)
        {
            // Bisection step (ala Dekker's method).
            x = (ak + bk) * 0.5;
        }
    }
}

} // namespace reSIDfp
