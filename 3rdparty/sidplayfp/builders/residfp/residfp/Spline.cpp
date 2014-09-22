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

#include "Spline.h"

#include <limits>

namespace reSIDfp
{

Spline::Spline(const double input[][2], int inputLength) :
    paramsLength(inputLength - 1),
    params(new double[paramsLength][6])
{

    for (int i = 0; i < paramsLength; i ++)
    {
        const double* p0 = i != 0 ? input[i - 1] : 0;
        const double* p1 = input[i];
        const double* p2 = input[i + 1];
        const double* p3 = i != inputLength - 2 ? input[i + 2] : 0;

        double k1, k2;

        if (p0 == 0)
        {
            k2 = (p3[1] - p1[1]) / (p3[0] - p1[0]);
            k1 = (3. * (p2[1] - p1[1]) / (p2[0] - p1[0]) - k2) / 2.;
        }
        else if (p3 == 0)
        {
            k1 = (p2[1] - p0[1]) / (p2[0] - p0[0]);
            k2 = (3. * (p2[1] - p1[1]) / (p2[0] - p1[0]) - k1) / 2.;
        }
        else
        {
            k1 = (p2[1] - p0[1]) / (p2[0] - p0[0]);
            k2 = (p3[1] - p1[1]) / (p3[0] - p1[0]);
        }

        const double x1 = p1[0];
        const double y1 = p1[1];
        const double x2 = p2[0];
        const double y2 = p2[1];

        const double dx = x2 - x1;
        const double dy = y2 - y1;

        const double a = ((k1 + k2) - 2. * dy / dx) / (dx * dx);
        const double b = ((k2 - k1) / dx - 3. * (x1 + x2) * a) / 2.;
        const double c = k1 - (3. * x1 * a + 2.*b) * x1;
        const double d = y1 - ((x1 * a + b) * x1 + c) * x1;

        params[i][0] = x1;
        params[i][1] = x2;
        params[i][2] = a;
        params[i][3] = b;
        params[i][4] = c;
        params[i][5] = d;
    }

    /* Fix the value ranges, because we interpolate outside original bounds if necessary. */
    params[0][0] = std::numeric_limits<double>::min();
    params[paramsLength - 1][1] = std::numeric_limits<double>::max();

    c = params[0];
}

void Spline::evaluate(double x, double* out)
{
    if (x < c[0] || x > c[1])
    {
        for (int i = 0; i < paramsLength; i ++)
        {
            if (params[i][1] < x)
            {
                continue;
            }

            c = params[i];
            break;
        }
    }

    const double y = ((c[2] * x + c[3]) * x + c[4]) * x + c[5];
    const double yd = (3. * c[2] * x + 2. * c[3]) * x + c[4];
    out[0] = y;
    out[1] = yd;
}

} // namespace reSIDfp
