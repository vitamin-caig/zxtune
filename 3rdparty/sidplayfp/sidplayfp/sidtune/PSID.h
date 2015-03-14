/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PSID_H
#define PSID_H

#include <stdint.h>

#include "SidTuneBase.h"

#include "sidplayfp/SidTune.h"

class PSID : public SidTuneBase
{
private:
    char m_md5[SidTune::MD5_LENGTH+1];

private:
    void tryLoad(buffer_t& dataBuf);

protected:
    PSID() {}

public:
    virtual ~PSID() {}

    static SidTuneBase* load(buffer_t& dataBuf);

    virtual const char *createMD5(char *md5);

private:
    // prevent copying
    PSID(const PSID&);
    PSID& operator=(PSID&);
};

#endif // PSID_H
