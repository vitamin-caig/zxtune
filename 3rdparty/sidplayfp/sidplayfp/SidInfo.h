/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright 2011-2012 Leandro Nini
 *  Copyright 2007-2010 Antti Lankila
 *  Copyright 2000 Simon White
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

#ifndef SIDINFO_H
#define SIDINFO_H

#include <stdint.h>

/**
* This interface is used to get sid engine informations.
*/
class SidInfo
{
public:
    /// Library name
    virtual const char *name() const =0;

    /// Library version
    virtual const char *version() const =0;

    /// Library credits
    //@{
    virtual unsigned int numberOfCredits() const =0;
    virtual const char *credits(unsigned int i) const =0;
    //@}

    /// Number of SIDs supported by this library
    virtual unsigned int maxsids() const =0;

    /// Number of output channels (1-mono, 2-stereo)
    virtual unsigned int channels() const =0;

    /// Address of the driver
    virtual uint_least16_t driverAddr() const =0;

    /// Size of the driver in bytes
    virtual uint_least16_t driverLength() const =0;

    /// Power on delay
    virtual uint_least16_t powerOnDelay() const =0;

    /// Describes the speed current song is running at
    virtual const char *speedString() const =0;

    /// Description of the laoded ROM images
    //@{
    virtual const char *kernalDesc() const =0;
    virtual const char *basicDesc() const =0;
    virtual const char *chargenDesc() const =0;
    //@}

protected:
    ~SidInfo() {}
};

#endif  /* SIDINFO_H */
