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

#ifndef WAVEFORMCALCULATOR_h
#define WAVEFORMCALCULATOR_h

#include <map>

#include "siddefs-fp.h"
#include "array.h"


namespace reSIDfp
{

typedef struct
{
    float bias;
    float pulsestrength;
    float topbit;
    float distance;
    float stmix;
} CombinedWaveformConfig;

/**
 * Combined waveform calculator for WaveformGenerator.
 *
 * @author Antti Lankila
 */
class WaveformCalculator
{
private:
    typedef std::map<const CombinedWaveformConfig*, matrix_t> cw_cache_t;

private:
    cw_cache_t CACHE;

    static const CombinedWaveformConfig config[2][4];

    /**
     * Generate bitstate based on emulation of combined waves.
     *
     * @param config
     * @param waveform the waveform to emulate, 1 .. 7
     * @param accumulator the accumulator value
     */
    short calculateCombinedWaveform(CombinedWaveformConfig config, int waveform, int accumulator) const;

    WaveformCalculator() {}

public:
    static WaveformCalculator* getInstance();

    /**
     * Build waveform tables for use by WaveformGenerator.
     *
     * @param model Chip model to use
     * @return Waveform table
     */
    matrix_t* buildTable(ChipModel model);
};

} // namespace reSIDfp

#endif
