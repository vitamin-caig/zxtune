/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2012-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000-2001 Simon White
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

#include "PSID.h"

#include <string>
#include <memory>

#include "sidplayfp/SidTuneInfo.h"
#include "sidplayfp/sidendian.h"
#include "sidplayfp/sidmd5.h"

#define PSID_ID 0x50534944
#define RSID_ID 0x52534944

// Header has been extended for 'RSID' format
// The following changes are present:
//     id = 'RSID'
//     version = 2 only
//     play, load and speed reserved 0
//     psidspecific flag reserved 0
//     init cannot be under ROMS/IO
//     load cannot be less than 0x0801 (start of basic)

struct psidHeader           // all values big-endian
{
    char id[4];             // 'PSID' (ASCII)
    uint8_t version[2];     // 0x0001 or 0x0002
    uint8_t data[2];        // 16-bit offset to binary data in file
    uint8_t load[2];        // 16-bit C64 address to load file to
    uint8_t init[2];        // 16-bit C64 address of init subroutine
    uint8_t play[2];        // 16-bit C64 address of play subroutine
    uint8_t songs[2];       // number of songs
    uint8_t start[2];       // start song out of [1..256]
    uint8_t speed[4];       // 32-bit speed info
                            // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
    char name[32];          // ASCII strings, 31 characters long and
    char author[32];        // terminated by a trailing zero
    char released[32];      //
    uint8_t flags[2];       // only version >= 0x0002
    uint8_t relocStartPage; // only version >= 0x0002B
    uint8_t relocPages;     // only version >= 0x0002B
    char sidChipBase2;      // only version >= 0x0003
    char reserved;          // only version >= 0x0002
};

enum
{
    PSID_MUS       = 1 << 0,
    PSID_SPECIFIC  = 1 << 1, // These two are mutally exclusive
    PSID_BASIC     = 1 << 1,
    PSID_CLOCK     = 3 << 2,
    PSID_SIDMODEL  = 3 << 4
};

enum
{
    PSID_CLOCK_UNKNOWN = 0,
    PSID_CLOCK_PAL     = 1 << 2,
    PSID_CLOCK_NTSC    = 1 << 3,
    PSID_CLOCK_ANY     = PSID_CLOCK_PAL | PSID_CLOCK_NTSC
};

enum
{
    PSID_SIDMODEL1_UNKNOWN = 0,
    PSID_SIDMODEL1_6581    = 1 << 4,
    PSID_SIDMODEL1_8580    = 1 << 5,
    PSID_SIDMODEL1_ANY     = PSID_SIDMODEL1_6581 | PSID_SIDMODEL1_8580
};

enum
{
    PSID_SIDMODEL2_UNKNOWN = 0,
    PSID_SIDMODEL2_6581    = 1 << 6,
    PSID_SIDMODEL2_8580    = 1 << 7,
    PSID_SIDMODEL2_ANY     = PSID_SIDMODEL2_6581 | PSID_SIDMODEL2_8580
};

const char TXT_FORMAT_PSID[]  = "PlaySID one-file format (PSID)";
const char TXT_FORMAT_RSID[]  = "Real C64 one-file format (RSID)";
const char TXT_UNKNOWN_PSID[] = "Unsupported PSID version";
const char TXT_UNKNOWN_RSID[] = "Unsupported RSID version";

const char ERR_TRUNCATED[]    = "ERROR: File is most likely truncated";
const char ERR_INVALID[]      = "ERROR: File contains invalid data";

static const int psid_maxStrLen = 32;


SidTuneBase* PSID::load(buffer_t& dataBuf)
{
    // File format check
    if (dataBuf.size()<6)
        return 0;

    const psidHeader* pHeader = reinterpret_cast<const psidHeader*>(&dataBuf[0]);
    if ((endian_big32((const uint_least8_t*)pHeader->id)!=PSID_ID)
        && (endian_big32((const uint_least8_t*)pHeader->id)!=RSID_ID))
         return 0;

    std::auto_ptr<PSID> tune(new PSID());
    tune->tryLoad(dataBuf);

    return tune.release();
}

void PSID::tryLoad(buffer_t& dataBuf)
{
    SidTuneInfo::clock_t clock = SidTuneInfo::CLOCK_UNKNOWN;
    SidTuneInfo::compatibility_t compatibility = SidTuneInfo::COMPATIBILITY_C64;

    // Require minimum size to allow access to the first few bytes.
    // Require a valid ID and version number.
    const psidHeader* pHeader = reinterpret_cast<const psidHeader*>(&dataBuf[0]);

    if (endian_big32((const uint_least8_t*)pHeader->id)==PSID_ID)
    {
       switch (endian_big16(pHeader->version))
       {
       case 1:
           compatibility = SidTuneInfo::COMPATIBILITY_PSID;
           // Deliberate run on
       case 2:
       case 3:
           break;
       default:
           throw loadError(TXT_UNKNOWN_PSID);
       }
       info->m_formatString = TXT_FORMAT_PSID;
    }
    else if (endian_big32((const uint_least8_t*)pHeader->id)==RSID_ID)
    {
       switch (endian_big16(pHeader->version))
       {
       case 2:
       case 3:
           break;
       default:
           throw loadError(TXT_UNKNOWN_RSID);
       }
       info->m_formatString = TXT_FORMAT_RSID;
       compatibility = SidTuneInfo::COMPATIBILITY_R64;
    }

    // Due to security concerns, input must be at least as long as version 1
    // header plus 16-bit C64 load address. That is the area which will be
    // accessed.
    const buffer_t::size_type bufLen = dataBuf.size();
    if (bufLen < (sizeof(psidHeader)+2))
    {
        throw loadError(ERR_TRUNCATED);
    }

    fileOffset            = endian_big16(pHeader->data);
    info->m_loadAddr      = endian_big16(pHeader->load);
    info->m_initAddr      = endian_big16(pHeader->init);
    info->m_playAddr      = endian_big16(pHeader->play);
    info->m_songs         = endian_big16(pHeader->songs);
    info->m_startSong     = endian_big16(pHeader->start);
    info->m_sidChipBase1  = 0xd400;
    info->m_sidChipBase2  = 0;
    info->m_compatibility = compatibility;

    uint_least32_t speed = endian_big32(pHeader->speed);

    if (info->m_songs > MAX_SONGS)
    {
        info->m_songs = MAX_SONGS;
    }

    bool musPlayer = false;

    info->m_sidModel1      = SidTuneInfo::SIDMODEL_UNKNOWN;
    info->m_sidModel2      = SidTuneInfo::SIDMODEL_UNKNOWN;
    info->m_relocPages     = 0;
    info->m_relocStartPage = 0;
    if (endian_big16(pHeader->version) >= 2)
    {
        const uint_least16_t flags = endian_big16(pHeader->flags);
        if (flags & PSID_MUS)
        {   // MUS tunes run at any speed
            clock = SidTuneInfo::CLOCK_ANY;
            musPlayer = true;
        }

        // This flags is only available for the appropriate
        // file formats
        switch (compatibility)
        {
        case SidTuneInfo::COMPATIBILITY_C64:
            if (flags & PSID_SPECIFIC)
                info->m_compatibility = SidTuneInfo::COMPATIBILITY_PSID;
            break;
        case SidTuneInfo::COMPATIBILITY_R64:
            if (flags & PSID_BASIC)
                info->m_compatibility = SidTuneInfo::COMPATIBILITY_BASIC;
            break;
        }

        if ((flags & PSID_CLOCK_ANY) == PSID_CLOCK_ANY)
            clock = SidTuneInfo::CLOCK_ANY;
        else if (flags & PSID_CLOCK_PAL)
            clock = SidTuneInfo::CLOCK_PAL;
        else if (flags & PSID_CLOCK_NTSC)
            clock = SidTuneInfo::CLOCK_NTSC;
        info->m_clockSpeed = clock;

        if ((flags & PSID_SIDMODEL1_ANY) == PSID_SIDMODEL1_ANY)
            info->m_sidModel1 = SidTuneInfo::SIDMODEL_ANY;
        else if (flags & PSID_SIDMODEL1_6581)
            info->m_sidModel1 = SidTuneInfo::SIDMODEL_6581;
        else if (flags & PSID_SIDMODEL1_8580)
            info->m_sidModel1 = SidTuneInfo::SIDMODEL_8580;

        info->m_relocStartPage = pHeader->relocStartPage;
        info->m_relocPages     = pHeader->relocPages;

        if (endian_big16(pHeader->version) >= 3)
        {
            info->m_sidChipBase2 = 0xd000 | (pHeader->sidChipBase2<<4);

            if ((flags & PSID_SIDMODEL2_ANY) == PSID_SIDMODEL2_ANY)
                info->m_sidModel2 = SidTuneInfo::SIDMODEL_ANY;
            else if (flags & PSID_SIDMODEL2_6581)
                info->m_sidModel2 = SidTuneInfo::SIDMODEL_6581;
            else if (flags & PSID_SIDMODEL2_8580)
                info->m_sidModel2 = SidTuneInfo::SIDMODEL_8580;
        }
    }

    // Check reserved fields to force real c64 compliance
    // as required by the RSID specification
    if (compatibility == SidTuneInfo::COMPATIBILITY_R64)
    {
        if ((info->m_loadAddr != 0)
            || (info->m_playAddr != 0)
            || (speed != 0))
        {
            throw loadError(ERR_INVALID);
        }
        // Real C64 tunes appear as CIA
        speed = ~0;
    }
    // Create the speed/clock setting table.
    convertOldStyleSpeedToTables(speed, clock);

    // Copy info strings.
    info->m_infoString.push_back(std::string(pHeader->name, psid_maxStrLen));
    info->m_infoString.push_back(std::string(pHeader->author, psid_maxStrLen));
    info->m_infoString.push_back(std::string(pHeader->released, psid_maxStrLen));

    if (musPlayer)
        throw loadError("Compute!'s Sidplayer MUS data is not supported yet"); // TODO
}

const char *PSID::createMD5(char *md5)
{
    if (!md5)
        md5 = m_md5;
    *md5 = '\0';

    // Include C64 data.
    sidmd5 myMD5;
    uint8_t tmp[2];
    myMD5.append(&cache[fileOffset], info->m_c64dataLen);

    // Include INIT and PLAY address.
    endian_little16(tmp,info->m_initAddr);
    myMD5.append(tmp,sizeof(tmp));
    endian_little16(tmp,info->m_playAddr);
    myMD5.append(tmp,sizeof(tmp));

    // Include number of songs.
    endian_little16(tmp,info->m_songs);
    myMD5.append(tmp,sizeof(tmp));

    {   // Include song speed for each song.
        const unsigned int currentSong = info->m_currentSong;
        for (unsigned int s = 1; s <= info->m_songs; s++)
        {
            selectSong (s);
            const uint_least8_t songSpeed = (uint_least8_t)info->m_songSpeed;
            myMD5.append (&songSpeed,sizeof(songSpeed));
        }
        // Restore old song
        selectSong (currentSong);
    }

    // Deal with PSID v2NG clock speed flags: Let only NTSC
    // clock speed change the MD5 fingerprint. That way the
    // fingerprint of a PAL-speed sidtune in PSID v1, v2, and
    // PSID v2NG format is the same.
    if (info->m_clockSpeed == SidTuneInfo::CLOCK_NTSC)
    {
        const uint_least8_t ntsc_val = 2;
        myMD5.append (&ntsc_val,sizeof(ntsc_val));
    }

    // NB! If the fingerprint is used as an index into a
    // song-lengths database or cache, modify above code to
    // allow for PSID v2NG files which have clock speed set to
    // SIDTUNE_CLOCK_ANY. If the SID player program fully
    // supports the SIDTUNE_CLOCK_ANY setting, a sidtune could
    // either create two different fingerprints depending on
    // the clock speed chosen by the player, or there could be
    // two different values stored in the database/cache.

    myMD5.finish();

    // Get fingerprint.
    myMD5.getDigest().copy(md5, SidTune::MD5_LENGTH);
    md5[SidTune::MD5_LENGTH] ='\0';

    return md5;
}
