/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
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

#ifndef SIDMD5_H
#define SIDMD5_H

#include <string>
#include <sstream>
#include <iomanip>

#include "utils/MD5/MD5.h"

/**
* A wrapper around the md5 implementation that provides
* an hex formatted digest
*/
class sidmd5
{
private:
    MD5 m_md5;

public:
    /// Append a string to the message.
    void append(const void* data, int nbytes) { m_md5.append(data, nbytes); }

    /// Finish the message.
    void finish() { m_md5.finish(); }

    /// Initialize the algorithm. Reset starting values.
    void reset() { m_md5.reset(); }

    /// Return pointer to 32-byte hex fingerprint.
    std::string getDigest()
    {
        // Construct fingerprint.
        std::ostringstream ss;
        ss.fill('0');
        ss.flags(std::ios::hex);

        for (int di = 0; di < 16; ++di)
        {
            ss << std::setw(2) << (int) m_md5.getDigest()[di];
        }

        return ss.str();
    }
};
    
#endif
