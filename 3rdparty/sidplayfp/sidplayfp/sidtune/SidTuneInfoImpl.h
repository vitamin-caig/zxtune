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

#ifndef SIDTUNEINFOIMPL_H
#define SIDTUNEINFOIMPL_H

#include <stdint.h>
#include <vector>
#include <string>

#include "sidplayfp/SidTuneInfo.h"

/**
* The implementation of the SidTuneInfo interface.
*/
class SidTuneInfoImpl : public SidTuneInfo
{
public:
    const char* m_formatString;

    unsigned int m_songs;
    unsigned int m_startSong;
    unsigned int m_currentSong;

    int m_songSpeed;

    clock_t m_clockSpeed;
    
    model_t m_sidModel1;
    model_t m_sidModel2;
    
    compatibility_t m_compatibility;

    uint_least32_t m_dataFileLen;

    uint_least32_t m_c64dataLen;

    uint_least16_t m_loadAddr;
    uint_least16_t m_initAddr;
    uint_least16_t m_playAddr;

    uint_least16_t m_sidChipBase1;
    uint_least16_t m_sidChipBase2;

    uint_least8_t m_relocStartPage;

    uint_least8_t m_relocPages;

    std::string m_path;

    std::string m_dataFileName;

    std::string m_infoFileName;

    std::vector<std::string> m_infoString;

    std::vector<std::string> m_commentString;

    bool m_fixLoad;

private:    // prevent copying
    SidTuneInfoImpl(const SidTuneInfoImpl&);
    SidTuneInfoImpl& operator=(SidTuneInfoImpl&);

public:
    SidTuneInfoImpl() :
        m_formatString("N/A"),
        m_songs(0),
        m_startSong(0),
        m_currentSong(0),
        m_songSpeed(SPEED_VBI),
        m_clockSpeed(CLOCK_UNKNOWN),
        m_sidModel1(SIDMODEL_UNKNOWN),
        m_sidModel2(SIDMODEL_UNKNOWN),
        m_compatibility(COMPATIBILITY_C64),
        m_dataFileLen(0),
        m_c64dataLen(0),
        m_loadAddr(0),
        m_initAddr(0),
        m_playAddr(0),
        m_sidChipBase1(0xd400),
        m_sidChipBase2(0),
        m_relocStartPage(0),
        m_relocPages(0),
        m_fixLoad(false) {}

    uint_least16_t loadAddr() const { return m_loadAddr; }

    uint_least16_t initAddr() const { return m_initAddr; }

    uint_least16_t playAddr() const { return m_playAddr; }

    unsigned int songs() const { return m_songs; }

    unsigned int startSong() const { return m_startSong; }

    unsigned int currentSong() const { return m_currentSong; }

    uint_least16_t sidChipBase1() const { return m_sidChipBase1; }
    uint_least16_t sidChipBase2() const { return m_sidChipBase2; }

    bool isStereo() const { return (m_sidChipBase1!=0 && m_sidChipBase2!=0); }

    int songSpeed() const { return m_songSpeed; }

    uint_least8_t relocStartPage() const { return m_relocStartPage; }

    uint_least8_t relocPages() const { return m_relocPages; }

    model_t sidModel1() const { return m_sidModel1; }
    model_t sidModel2() const { return m_sidModel2; }

    compatibility_t compatibility() const { return m_compatibility; }

    unsigned int numberOfInfoStrings() const { return m_infoString.size(); }
    const char* infoString(unsigned int i) const { return i<numberOfInfoStrings()?m_infoString[i].c_str():""; }

    unsigned int numberOfCommentStrings() const { return m_commentString.size(); }
    const char* commentString(unsigned int i) const { return i<numberOfCommentStrings()?m_commentString[i].c_str():""; }

    uint_least32_t dataFileLen() const { return m_dataFileLen; }

    uint_least32_t c64dataLen() const { return m_c64dataLen; }

    clock_t clockSpeed() const { return m_clockSpeed; }

    const char* formatString() const { return m_formatString; }

    bool fixLoad() const { return m_fixLoad; }

    const char* path() const { return m_path.c_str(); }

    const char* dataFileName() const { return m_dataFileName.c_str(); }

    const char* infoFileName() const { return !m_infoFileName.empty()?m_infoFileName.c_str():0; }
};

#endif  /* SIDTUNEINFOIMPL_H */
