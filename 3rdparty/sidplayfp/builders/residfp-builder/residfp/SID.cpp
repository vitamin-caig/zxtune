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

#define SID_CPP

#include "SID.h"

#include <limits>

#include "array.h"
#include "Filter6581.h"
#include "Filter8580.h"
#include "Potentiometer.h"
#include "WaveformCalculator.h"
#include "resample/TwoPassSincResampler.h"
#include "resample/ZeroOrderResampler.h"

namespace reSIDfp
{
// This value has been adjusted empirically from the original reSID value (0x9000)
// according to this discussion on CSDb:
// http://noname.c64.org/csdb/forums/?roomid=11&topicid=29025&showallposts=1
const int SID::BUS_TTL = 0x1000;

SID::SID() :
    filter6581(new Filter6581()),
    filter8580(new Filter8580()),
    externalFilter(new ExternalFilter()),
    resampler(0),
    potX(new Potentiometer()),
    potY(new Potentiometer())
{
    voice[0] = new Voice();
    voice[1] = new Voice();
    voice[2] = new Voice();

    muted[0] = muted[1] = muted[2] = false;

    reset();
    setChipModel(MOS8580);
}

SID::~SID()
{
    delete filter6581;
    delete filter8580;
    delete externalFilter;
    delete potX;
    delete potY;
    delete voice[0];
    delete voice[1];
    delete voice[2];
    delete resampler;
}

void SID::writeImmediate(int offset, unsigned char value)
{
    switch (offset)
    {
    case 0x00:
        voice[0]->wave()->writeFREQ_LO(value);
        break;

    case 0x01:
        voice[0]->wave()->writeFREQ_HI(value);
        break;

    case 0x02:
        voice[0]->wave()->writePW_LO(value);
        break;

    case 0x03:
        voice[0]->wave()->writePW_HI(value);
        break;

    case 0x04:
        voice[0]->writeCONTROL_REG(muted[0] ? 0 : value);
        break;

    case 0x05:
        voice[0]->envelope()->writeATTACK_DECAY(value);
        break;

    case 0x06:
        voice[0]->envelope()->writeSUSTAIN_RELEASE(value);
        break;

    case 0x07:
        voice[1]->wave()->writeFREQ_LO(value);
        break;

    case 0x08:
        voice[1]->wave()->writeFREQ_HI(value);
        break;

    case 0x09:
        voice[1]->wave()->writePW_LO(value);
        break;

    case 0x0a:
        voice[1]->wave()->writePW_HI(value);
        break;

    case 0x0b:
        voice[1]->writeCONTROL_REG(muted[1] ? 0 : value);
        break;

    case 0x0c:
        voice[1]->envelope()->writeATTACK_DECAY(value);
        break;

    case 0x0d:
        voice[1]->envelope()->writeSUSTAIN_RELEASE(value);
        break;

    case 0x0e:
        voice[2]->wave()->writeFREQ_LO(value);
        break;

    case 0x0f:
        voice[2]->wave()->writeFREQ_HI(value);
        break;

    case 0x10:
        voice[2]->wave()->writePW_LO(value);
        break;

    case 0x11:
        voice[2]->wave()->writePW_HI(value);
        break;

    case 0x12:
        voice[2]->writeCONTROL_REG(muted[2] ? 0 : value);
        break;

    case 0x13:
        voice[2]->envelope()->writeATTACK_DECAY(value);
        break;

    case 0x14:
        voice[2]->envelope()->writeSUSTAIN_RELEASE(value);
        break;

    case 0x15:
        filter6581->writeFC_LO(value);
        filter8580->writeFC_LO(value);
        break;

    case 0x16:
        filter6581->writeFC_HI(value);
        filter8580->writeFC_HI(value);
        break;

    case 0x17:
        filter6581->writeRES_FILT(value);
        filter8580->writeRES_FILT(value);
        break;

    case 0x18:
        filter6581->writeMODE_VOL(value);
        filter8580->writeMODE_VOL(value);
        break;

    default:
        break;
    }

    /* Update voicesync just in case. */
    voiceSync(false);
}

void SID::ageBusValue(int n)
{
    if (busValueTtl != 0)
    {
        busValueTtl -= n;

        if (busValueTtl <= 0)
        {
            busValue = 0;
            busValueTtl = 0;
        }
    }
}

void SID::voiceSync(bool sync)
{
    if (sync)
    {
        /* Synchronize the 3 waveform generators. */
        for (int i = 0; i < 3; i++)
        {
            voice[i]->wave()->synchronize(voice[(i + 1) % 3]->wave(), voice[(i + 2) % 3]->wave());
        }
    }

    /* Calculate the time to next voice sync */
    nextVoiceSync = std::numeric_limits<int>::max();

    for (int i = 0; i < 3; i++)
    {
        const int accumulator = voice[i]->wave()->readAccumulator();
        const int freq = voice[i]->wave()->readFreq();

        if (voice[i]->wave()->readTest() || freq == 0 || !voice[(i + 1) % 3]->wave()->readSync())
        {
            continue;
        }

        const int thisVoiceSync = ((0x7fffff - accumulator) & 0xffffff) / freq + 1;

        if (thisVoiceSync < nextVoiceSync)
        {
            nextVoiceSync = thisVoiceSync;
        }
    }
}

void SID::setChipModel(ChipModel model)
{
    switch (model)
    {
    case MOS6581:
        filter = filter6581;
        break;

    case MOS8580:
        filter = filter8580;
        break;

    default:
        throw SIDError("Unknown chip type\n");
    }

    this->model = model;

    /* calculate waveform-related tables, feed them to the generator */
    matrix_t* tables = WaveformCalculator::getInstance()->buildTable(model);

    /* update voice offsets */
    for (int i = 0; i < 3; i++)
    {
        voice[i]->envelope()->setChipModel(model);
        voice[i]->wave()->setChipModel(model);
        voice[i]->wave()->setWaveformModels(tables);
    }
}

void SID::reset()
{
    for (int i = 0; i < 3; i++)
    {
        voice[i]->reset();
    }

    filter6581->reset();
    filter8580->reset();
    externalFilter->reset();

    if (resampler)
    {
        resampler->reset();
    }

    busValue = 0;
    busValueTtl = 0;
    delayedOffset = -1;
    voiceSync(false);
}

void SID::input(int value)
{
    filter6581->input(value);
    filter8580->input(value);
}

unsigned char SID::read(int offset)
{
    unsigned char value;

    switch (offset)
    {
    case 0x19:
        value = potX->readPOT();
        busValueTtl = BUS_TTL;
        break;

    case 0x1a:
        value = potY->readPOT();
        busValueTtl = BUS_TTL;
        break;

    case 0x1b:
        value = voice[2]->wave()->readOSC();
        break;

    case 0x1c:
        value = voice[2]->envelope()->readENV();
        busValueTtl = BUS_TTL;
        break;

    default:
        value = busValue;
        busValueTtl /= 2;
        break;
    }

    busValue = value;
    return value;
}

void SID::write(int offset, unsigned char value)
{
    busValue = value;
    busValueTtl = BUS_TTL;

    if (model == MOS8580)
    {
        delayedOffset = offset;
        delayedValue = value;
    }
    else
    {
        writeImmediate(offset, value);
    }
}

void SID::setSamplingParameters(double clockFrequency, SamplingMethod method, double samplingFrequency, double highestAccurateFrequency)
{
    filter6581->setClockFrequency(clockFrequency);
    filter8580->setClockFrequency(clockFrequency);
    externalFilter->setClockFrequency(clockFrequency);

    delete resampler;

    switch (method)
    {
    case DECIMATE:
        resampler = new ZeroOrderResampler(clockFrequency, samplingFrequency);
        break;

    case RESAMPLE:
        resampler = new TwoPassSincResampler(clockFrequency, samplingFrequency, highestAccurateFrequency);
        break;

    default:
        throw SIDError("Unknown sampling method\n");
    }
}

void SID::clockSilent(int cycles)
{
    ageBusValue(cycles);

    while (cycles != 0)
    {
        int delta_t = std::min(nextVoiceSync, cycles);

        if (delta_t > 0)
        {
            if (delayedOffset != -1)
            {
                delta_t = 1;
            }

            for (int i = 0; i < delta_t; i++)
            {
                /* clock waveform generators (can affect OSC3) */
                voice[0]->wave()->clock();
                voice[1]->wave()->clock();
                voice[2]->wave()->clock();

                /* clock ENV3 only */
                voice[2]->envelope()->clock();
            }

            if (delayedOffset != -1)
            {
                writeImmediate(delayedOffset, delayedValue);
                delayedOffset = -1;
            }

            cycles -= delta_t;
            nextVoiceSync -= delta_t;
        }

        if (nextVoiceSync == 0)
        {
            voiceSync(true);
        }
    }
}

} // namespace reSIDfp
