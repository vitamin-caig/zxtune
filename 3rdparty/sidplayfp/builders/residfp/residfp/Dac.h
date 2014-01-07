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

#ifndef DAC_H
#define DAC_H

namespace reSIDfp
{

/**
* Estimate DAC nonlinearity. The SID contains R-2R ladder, and some likely errors
* in the resistor lengths which result in errors depending on the bits chosen.
*
* This model was derived by Dag Lem, and is port of the upcoming reSID version.
* In average, it shows a value higher than the target by a value that depends
* on the _2R_div_R parameter. It differs from the version written by Antti Lankila
* chiefly in the emulation of the lacking termination of the 2R ladder, which
* destroys the output with respect to the low bits of the DAC.
*/
class Dac
{
public:
    /**
     * @param dac an array to be filled with the resulting analog values
     * @param dacLength the dac array length
     * @param _2R_div_R nonlinearity parameter, 1.0 for perfect linearity.
     * @param term is the dac terminated by a 2R resistor? (6581 DACs are not)
     */
    static void kinkedDac(double* dac, int dacLength, double _2R_div_R, bool term);
};

} // namespace reSIDfp

#endif
