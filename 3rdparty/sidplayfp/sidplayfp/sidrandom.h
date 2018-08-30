/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef SIDRANDOM_H
#define SIDRANDOM_H

/**
 * Simple thread-safe PRNG.
 */
class sidrandom
{
private:
    unsigned long m_seed;

public:
    sidrandom(unsigned seed = 1)
        : m_seed(seed)
    {
    }

    /**
     * Generate new pseudo-random number.
     */
    unsigned next()
    {
        //standard K&R implementation
        m_seed = m_seed * 1103515245 + 12345;
        return ((unsigned)(m_seed / 65536) % 32768);
    }
};

#endif
