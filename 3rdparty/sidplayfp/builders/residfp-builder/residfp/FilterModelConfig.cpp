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

#include "FilterModelConfig.h"

#include <cmath>

#include "Dac.h"
#include "Integrator.h"
#include "OpAmp.h"
#include "Spline.h"

namespace reSIDfp
{

const double FilterModelConfig::opamp_voltage[OPAMP_SIZE][2] =
{
    { 0.75, 10.02 }, // Approximate start of actual range
    { 2.50, 10.13 },
    { 2.75, 10.12 },
    { 2.90, 10.04 },
    { 3.00, 9.92 },
    { 3.10, 9.74 },
    { 3.25, 9.40 },
    { 3.50, 8.68 },
    { 4.00, 6.90 },
    { 4.25, 5.88 },
    { 4.53, 4.53 }, // Working point (vi = vo)
    { 4.75, 3.20 },
    { 4.90, 2.30 }, // Change of curvature
    { 4.95, 2.05 },
    { 5.00, 1.90 },
    { 5.10, 1.71 },
    { 5.25, 1.57 },
    { 5.50, 1.41 },
    { 6.00, 1.23 },
    { 7.50, 1.02 },
    { 9.00, 0.93 },
    { 10.25, 0.91 }, // Approximate end of actual range
};

std::auto_ptr<FilterModelConfig> FilterModelConfig::instance(0);

FilterModelConfig* FilterModelConfig::getInstance()
{
    if (!instance.get())
    {
        instance.reset(new FilterModelConfig());
    }

    return instance.get();
}

FilterModelConfig::FilterModelConfig() :
    voice_voltage_range(1.5),
    voice_DC_voltage(5.0),
    C(470e-12),
    Vdd(12.18),
    Vth(1.31),
    uCox_vcr(20e-6),
    WL_vcr(9.0 / 1.0),
    uCox_snake(20e-6),
    WL_snake(1.0 / 115.0),
    dac_zero(6.65),
    dac_scale(2.63),
    vmin(opamp_voltage[0][0]),
    norm(1.0 / ((Vdd - Vth) - vmin))
{
    // Convert op-amp voltage transfer to 16 bit values.

    for (unsigned int i = 0; i < OPAMP_SIZE; i ++)
    {
        if (opamp_voltage[i][0] == opamp_voltage[i][1])
        {
            opamp_working_point = opamp_voltage[i][0];
            break;
        }
    }

    Dac::kinkedDac(dac, DAC_BITS, 2.2, false);

    const double N16 = norm * ((1 << 16) - 1);

    // The "zero" output level of the voices.
    // The digital range of one voice is 20 bits; create a scaling term
    // for multiplication which fits in 11 bits.
    const double N15 = norm * ((1L << 15) - 1);
    // Create lookup table mapping capacitor voltage to op-amp input voltage:
    // vc -> vx
    double scaled_voltage[OPAMP_SIZE][2];

    for (unsigned int i = 0; i < OPAMP_SIZE; i++)
    {
        scaled_voltage[i][0] = (N16 * (opamp_voltage[i][0] - opamp_voltage[i][1]) + (1 << 16)) / 2.;
        scaled_voltage[i][1] = N16 * opamp_voltage[i][0];
    }

    Spline s(scaled_voltage, OPAMP_SIZE);
    double out[2];

    for (int x = 0; x < (1 << 16); x ++)
    {
        s.evaluate(x, out);
        opamp_rev[x] = (int)(out[0] + 0.5);
    }

    // The filter summer operates at n ~ 1, and has 5 fundamentally different
    // input configurations (2 - 6 input "resistors").
    //
    // Note that all "on" transistors are modeled as one. This is not
    // entirely accurate, since the input for each transistor is different,
    // and transistors are not linear components. However modeling all
    // transistors separately would be extremely costly.
    OpAmp opampModel(opamp_voltage, OPAMP_SIZE, Vdd - Vth);

    for (int i = 0; i < 7; i++)
    {
        const int idiv = 2 + i;        // 2 - 6 input "resistors".
        const int size = idiv << 16;
        opampModel.reset();
        summer[i] = new unsigned short[size];

        for (int vi = 0; vi < size; vi++)
        {
            const double vin = vmin + vi / N16 / idiv; /* vmin .. vmax */
            summer[i][vi] = (unsigned short)((opampModel.solve(idiv, vin) - vmin) * N16 + 0.5);
        }
    }

    // The audio mixer operates at n ~ 8/6, and has 8 fundamentally different
    // input configurations (0 - 7 input "resistors").
    //
    // All "on", transistors are modeled as one - see comments above for
    // the filter summer.
    for (int i = 0; i < 8; i++)
    {
        const int size = (i == 0) ? 1 : i << 16;
        opampModel.reset();
        mixer[i] = new unsigned short[size];

        for (int vi = 0; vi < size; vi++)
        {
            const double vin = vmin + vi / N16 / (i == 0 ? 1 : i); /* vmin .. vmax */
            mixer[i][vi] = (unsigned short)((opampModel.solve(i * 8.0 / 6.0, vin) - vmin) * N16 + 0.5);
        }
    }

    // 4 bit "resistor" ladders in the bandpass resonance gain and the audio
    // output gain necessitate 16 gain tables.
    // From die photographs of the bandpass and volume "resistor" ladders
    // it follows that gain ~ vol/8 and 1/Q ~ ~res/8 (assuming ideal
    // op-amps and ideal "resistors").
    for (int n8 = 0; n8 < 16; n8++)
    {
        opampModel.reset();
        gain[n8] = new unsigned short[1 << 16];

        for (int vi = 0; vi < (1 << 16); vi++)
        {
            const double vin = vmin + vi / N16; /* vmin .. vmax */
            gain[n8][vi] = (unsigned short)((opampModel.solve(n8 / 8.0, vin) - vmin) * N16 + 0.5);
        }
    }

    const int Vddt = (int)(N16 * (Vdd - Vth) + 0.5);

    for (int i = 0; i < (1 << 16); i++)
    {
        // The table index is right-shifted 16 times in order to fit in
        // 16 bits; the argument to sqrt is thus multiplied by (1 << 16).
        int Vg = Vddt - (int)(sqrt((double) i * (1 << 16)) + 0.5);

        if (Vg >= (1 << 16))
        {
            // Clamp to 16 bits.
            // FIXME: If the DAC output voltage exceeds the max op-amp output
            // voltage while the input voltage is at the max op-amp output
            // voltage, Vg will not fit in 16 bits.
            // Check whether this can happen, and if so, change the lookup table
            // to a plain sqrt.
            Vg = (1 << 16) - 1;
        }

        vcr_Vg[i] = (unsigned short) Vg;
    }

    const double Ut = 26.0e-3;  // Thermal voltage.
    const double k = 1.0;
    const double Is = 2. * uCox_vcr * Ut * Ut / k * WL_vcr;
    // Normalized current factor for 1 cycle at 1MHz.
    const double n_Is = N15 * 1.0e-6 / C * Is;

    /* 1st term is used for clamping and must therefore be fixed to 0. */
    vcr_n_Ids_term[0] = 0;

    for (int Vgx = 1; Vgx < (1 << 16); Vgx++)
    {
        const double log_term = log(1. + exp((Vgx / N16 - k * Vth) / (2. * Ut)));
        // Scaled by m*2^15
        vcr_n_Ids_term[Vgx] = (unsigned short)(n_Is * log_term * log_term + 0.5);
    }
}

FilterModelConfig::~FilterModelConfig()
{
    for (int i = 0; i < 7; i++)
    {
        delete [] summer[i];
    }

    for (int i = 0; i < 8; i++)
    {
        delete [] mixer[i];
    }

    for (int i = 0; i < 16; i++)
    {
        delete [] gain[i];
    }
}

double FilterModelConfig::evaluateTransistor(double Vw, double vi, double vx)
{
    const double Vgst = Vdd - Vth - vx;
    const double Vgdt = Vdd - Vth - vi;
    const double n_snake = uCox_snake / 2. * WL_snake;
    const double n_I_snake = n_snake * (Vgst * Vgst - Vgdt * Vgdt);

    const double Vg = Vdd - Vth - sqrt(pow(Vdd - Vth - Vw, 2.) / 2. + pow(Vgdt, 2.) / 2.);
    const double Vgs = Vg - vx;
    const double Vgd = Vg - vi;

    const double Ut = 26.0e-3;  // Thermal voltage.
    const double k = 1.0;
    const double Is = 2. * uCox_vcr * Ut * Ut / k * WL_vcr;

    const double log_term_f = log(1. + exp((Vgs - k * Vth) / (2. * Ut)));
    const double n_I_vcr_f = Is * log_term_f * log_term_f;

    const double log_term_r = log(1. + exp((Vgd - k * Vth) / (2. * Ut)));
    const double n_I_vcr_r = Is * log_term_r * log_term_r;

    const double n_I_vcr = n_I_vcr_f - n_I_vcr_r;
    return n_I_snake + n_I_vcr;
}

unsigned int* FilterModelConfig::getDAC(double adjustment) const
{
    const double dac_zero = getDacZero(adjustment);

    const double N16 = norm * ((1L << 16) - 1);
    unsigned int* f0_dac = new unsigned int[1 << DAC_BITS];

    for (int i = 0; i < (1 << DAC_BITS); i++)
    {
        double fcd = 0.;

        for (unsigned int j = 0; j < DAC_BITS; j ++)
        {
            if ((i & (1 << j)) != 0)
            {
                fcd += dac[j];
            }
        }

        f0_dac[i] = (unsigned int)(N16 * (dac_zero + fcd * dac_scale / (1 << DAC_BITS)) + 0.5);
    }

    return f0_dac;
}

Integrator* FilterModelConfig::buildIntegrator()
{
    const double N16 = norm * ((1 << 16) - 1);
    const int Vddt = (int)(N16 * (Vdd - Vth) + 0.5);
    const int n_snake = (int)((1 << 13) / norm * (uCox_snake / 2. * WL_snake * 1.0e-6 / C) + 0.5);
    return new Integrator(vcr_Vg, vcr_n_Ids_term, opamp_rev, Vddt, n_snake);
}

double FilterModelConfig::estimateFrequency(double dac_zero, int fc)
{
    /* Calculate input from DAC */
    double Vw = 0.;

    for (unsigned int j = 0; j < DAC_BITS; j ++)
    {
        if ((fc & (1 << j)) != 0)
        {
            Vw += dac[j];
        }
    }

    Vw = dac_zero + dac_scale * Vw / (1 << DAC_BITS);

    /* Estimate the behavior for small signals around the op-amp working point. */
    const double vx = opamp_working_point;
    const double diff = 0.2;
    double n_I = 0.;
    n_I -= evaluateTransistor(Vw, vx - diff, vx);
    n_I += evaluateTransistor(Vw, vx + diff, vx);
    n_I /= 2.;

    /* Convert the current to frequency based on the calculated current and the potential. */
    return n_I / (2. * M_PI * C * diff);
}

} // namespace reSIDfp
