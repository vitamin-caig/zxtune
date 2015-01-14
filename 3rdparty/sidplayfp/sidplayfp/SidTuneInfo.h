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

#ifndef SIDTUNEINFO_H
#define SIDTUNEINFO_H

#include <stdint.h>

/**
 * This interface is used to get values from SidTune objects.
 *
 * You must read (i.e. activate) sub-song specific information
 * via:
 *        const SidTuneInfo* tuneInfo = SidTune.getInfo();
 *        const SidTuneInfo* tuneInfo = SidTune.getInfo(songNumber);
 */
class SidTuneInfo
{
public:
    typedef enum {
        CLOCK_UNKNOWN,
        CLOCK_PAL,
        CLOCK_NTSC,
        CLOCK_ANY
    } clock_t;

    typedef enum {
        SIDMODEL_UNKNOWN,
        SIDMODEL_6581,
        SIDMODEL_8580,
        SIDMODEL_ANY
    } model_t;

    typedef enum {
        COMPATIBILITY_C64,   ///< File is C64 compatible
        COMPATIBILITY_PSID,  ///< File is PSID specific
        COMPATIBILITY_R64,   ///< File is Real C64 only
        COMPATIBILITY_BASIC  ///< File requires C64 Basic
    } compatibility_t;

public:
    /// Vertical-Blanking-Interrupt
    static const int SPEED_VBI = 0;

    /// CIA 1 Timer A
    static const int SPEED_CIA_1A = 60;

public:
    /**
     * Load Address.
     */
    virtual uint_least16_t loadAddr() const =0;

    /**
     * Init Address.
     */
    virtual uint_least16_t initAddr() const =0;

    /**
     * Play Address.
     */
    virtual uint_least16_t playAddr() const =0;

    /**
     * The number of songs.
     */
    virtual unsigned int songs() const =0;

    /**
     * The default starting song.
     */
    virtual unsigned int startSong() const =0;

    /**
     * The tune that has been initialized.
     */
    virtual unsigned int currentSong() const =0;

    /**
     * @name Base addresses
     * The SID chip base address(es) used by the sidtune.
     */
    //@{
    virtual uint_least16_t sidChipBase1() const =0;    ///< 0xD400 (normal, 1st SID)
    virtual uint_least16_t sidChipBase2() const =0;    ///< 0xD?00 (2nd SID) or 0 (no 2nd SID)
    //@}

    /**
     * Whether sidtune uses two SID chips.
     */
    virtual bool isStereo() const=0;

    /**
     * Intended speed.
     */
    virtual int songSpeed() const =0;

    /**
     * First available page for relocation.
     */
    virtual uint_least8_t relocStartPage() const =0;

    /**
     * Number of pages available for relocation.
     */
    virtual uint_least8_t relocPages() const =0;

    /**
     * @name SID model
     * The SID chip model(s) requested by the sidtune.
     */
    //@{
    virtual model_t sidModel1() const =0;    ///< first SID
    virtual model_t sidModel2() const =0;    ///< second SID
    //@}

    /**
     * Compatibility requirements.
     */
    virtual compatibility_t compatibility() const =0;

    /**
     * @name Tune infos
     * Song title, credits, ...
     * - 0 = Title
     * - 1 = Author
     * - 2 = Released
     */
    //@{
    virtual unsigned int numberOfInfoStrings() const =0;     ///< the number of available text info lines
    virtual const char* infoString(unsigned int i) const =0; ///< text info from the format headers etc.
    //@}

    /**
     * @name Tune comments
     * MUS comments.
     */
    //@{
    virtual unsigned int numberOfCommentStrings() const =0;     ///< Number of comments
    virtual const char* commentString(unsigned int i) const =0; ///<  Used to stash the MUS comment somewhere.
    //@}

    /**
     * Length of single-file sidtune file.
     */
    virtual uint_least32_t dataFileLen() const =0;

    /**
     * Length of raw C64 data without load address.
     */
    virtual uint_least32_t c64dataLen() const =0;

    /**
     * The tune clock speed.
     */
    virtual clock_t clockSpeed() const =0;

    /**
     * The name of the identified file format.
     */
    virtual const char* formatString() const =0;

    /**
     * Whether load address might be duplicate.
     */
    virtual bool fixLoad() const =0;

    /**
     * Path to sidtune files.
     */
    virtual const char* path() const =0;

    /**
     * A first file: e.g. "foo.sid" or "foo.mus".
     */
    virtual const char* dataFileName() const =0;

    /**
     * A second file: e.g. "foo.str".
     * Returns 0 if none.
     */
    virtual const char* infoFileName() const =0;

protected:
    ~SidTuneInfo() {}
};

#endif  /* SIDTUNEINFO_H */
