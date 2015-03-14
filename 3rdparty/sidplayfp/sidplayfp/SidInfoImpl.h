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

#ifndef SIDINFOIMPL_H
#define SIDINFOIMPL_H

#include <stdint.h>
#include <vector>
#include <string>

#include "sidplayfp/SidInfo.h"

#include "mixer.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifndef PACKAGE_NAME
#   define PACKAGE_NAME PACKAGE
#endif

#ifndef PACKAGE_VERSION
#   define PACKAGE_VERSION VERSION
#endif

/**
 * The implementation of the SidInfo interface.
 */
class SidInfoImpl : public SidInfo
{
public:
    const std::string m_name;
    const std::string m_version;
    std::vector<std::string> m_credits;

    std::string m_speedString;

    std::string m_kernalDesc;
    std::string m_basicDesc;
    std::string m_chargenDesc;

    const unsigned int m_maxsids;

    unsigned int m_channels;

    uint_least16_t m_driverAddr;
    uint_least16_t m_driverLength;

    uint_least16_t m_powerOnDelay;

private:
    // prevent copying
    SidInfoImpl(const SidInfoImpl&);
    SidInfoImpl& operator=(SidInfoImpl&);

public:
    SidInfoImpl() :
        m_name(PACKAGE_NAME),
        m_version(PACKAGE_VERSION),
        m_maxsids(Mixer::MAX_SIDS),
        m_channels(1),
        m_driverAddr(0),
        m_driverLength(0),
        m_powerOnDelay(0)
    {
        m_credits.push_back(PACKAGE_NAME " V" PACKAGE_VERSION " Engine:\n"
            "\tCopyright (C) 2000 Simon White\n"
            "\tCopyright (C) 2007-2010 Antti Lankila\n"
            "\tCopyright (C) 2010-2014 Leandro Nini\n"
            "\t" PACKAGE_URL "\n");
    }

    const char *name() const { return m_name.c_str(); }
    const char *version() const { return m_version.c_str(); }

    unsigned int numberOfCredits() const { return m_credits.size(); }
    const char *credits(unsigned int i) const { return i<m_credits.size()?m_credits[i].c_str():""; }

    unsigned int maxsids() const { return m_maxsids; }

    unsigned int channels() const { return m_channels; }

    uint_least16_t driverAddr() const { return m_driverAddr; }
    uint_least16_t driverLength() const { return m_driverLength; }

    uint_least16_t powerOnDelay() const { return m_powerOnDelay; }

    const char *speedString() const { return m_speedString.c_str(); }

    const char *kernalDesc() const { return m_kernalDesc.c_str(); }
    const char *basicDesc() const { return m_basicDesc.c_str(); }
    const char *chargenDesc() const { return m_chargenDesc.c_str(); }
};

#endif  /* SIDTUNEINFOIMPL_H */
