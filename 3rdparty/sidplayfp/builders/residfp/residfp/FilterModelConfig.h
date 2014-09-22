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

#ifndef FILTERMODELCONFIG_H
#define FILTERMODELCONFIG_H

#include <memory>

namespace reSIDfp
{

class Integrator;

/**
*/
class FilterModelConfig
{
private:
    static const unsigned int OPAMP_SIZE = 22;
    static const unsigned int DAC_BITS = 11;

private:
    static std::auto_ptr<FilterModelConfig> instance;
    // This allows access to the private constructor
    friend class std::auto_ptr<FilterModelConfig>;

    static const double opamp_voltage[OPAMP_SIZE][2];

    const double voice_voltage_range;
    const double voice_DC_voltage;

    // Capacitor value.
    const double C;

    // Transistor parameters.
    const double Vdd;
    const double Vth;           // Threshold voltage
    const double uCox_vcr;      // 1/2*u*Cox
    const double WL_vcr;        // W/L for VCR
    const double uCox_snake;    // 1/2*u*Cox
    const double WL_snake;      // W/L for "snake"

    // DAC parameters.
    const double dac_zero;
    const double dac_scale;

    /* Derived stuff */
    const double vmin, norm;
    double opamp_working_point;
    unsigned short* mixer[8];
    unsigned short* summer[7];
    unsigned short* gain[16];
    double dac[DAC_BITS];
    unsigned short vcr_Vg[1 << 16];
    unsigned short vcr_n_Ids_term[1 << 16];
    int opamp_rev[1 << 16];

private:
    double evaluateTransistor(double Vw, double vi, double vx);

    double getDacZero(double adjustment) const { return dac_zero - (adjustment - 0.5) * 2.; }

    FilterModelConfig();
    ~FilterModelConfig();

public:
    static FilterModelConfig* getInstance();

    int getVO_T16() const { return (int)(norm * ((1L << 16) - 1) * vmin); }

    int getVoiceScaleS14() const { return (int)((norm * ((1L << 14) - 1)) * voice_voltage_range); }

    int getVoiceDC() const { return (int)((norm * ((1L << 16) - 1)) * (voice_DC_voltage - vmin)); }

    unsigned short** getGain() { return gain; }

    unsigned short** getSummer() { return summer; }

    unsigned short** getMixer() { return mixer; }

    /**
     * Construct a DAC table.
     * Ownership is transferred to the requester which becomes responsible
     * of freeing the object when done.
     *
     * @param adjustment
     * @return the DAC table
     */
    unsigned int* getDAC(double adjustment) const;

    Integrator* buildIntegrator();

    /**
     * Estimate the center frequency corresponding to some FC setting.
     *
     * FIXME: this function is extremely sensitive to prevailing voltage offsets.
     * They got to be right within about 0.1V, or the results will be simply wrong.
     * This casts doubt on the feasibility of this approach. Perhaps the offsets
     * at the integrators would need to be statically solved first for 1-voice null
     * input.
     *
     * @param dac_zero
     * @param fc
     * @return frequency in Hz
     */
    double estimateFrequency(double dac_zero, int fc);
};

} // namespace reSIDfp

#endif
