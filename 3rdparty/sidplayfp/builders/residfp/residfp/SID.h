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

#ifndef SIDFP_H
#define SIDFP_H

#include "residfp-config.h"

namespace reSIDfp
{

class Filter;
class Filter6581;
class Filter8580;
class ExternalFilter;
class Potentiometer;
class Voice;
class Resampler;

/**
 * SID error exception.
 */
class SIDError
{
private:
    const char* message;

public:
    SIDError(const char* msg) :
        message(msg) {}
    const char* getMessage() { return message; }
};

/**
 * MOS6581/MOS8580 emulation.
 * Based on reSID 0.16 by Dag Lem,
 * and then hacked on by Antti S. Lankila.
 * Ported to Java by Ken Händel.
 *
 * @author Ken Händel
 * @author Dag Lem
 * @author Antti Lankila
 * @author Leandro Nini
 */
class SID
{
private:
    /**
     * Bus value stays alive for some time after each operation.
     */
    static const int BUS_TTL;

    /** Currently active filter */
    Filter* filter;

    /** Filter used, if model is set to 6581 */
    Filter6581* filter6581;

    /** Filter used, if model is set to 8580 */
    Filter8580* filter8580;

    /**
     * External filter that provides high-pass and low-pass filtering
     * to adjust sound tone slightly.
     */
    ExternalFilter* externalFilter;

    /**
     * Resampler used by audio generation code.
     */
    Resampler* resampler;

    /** Paddle X register support */
    Potentiometer* potX;

    /** Paddle Y register support */
    Potentiometer* potY;

    /** SID voices */
    Voice* voice[3];

    /** Time to live for the last written value */
    int busValueTtl;

    /**
     * Time until #voiceSync must be run.
     */
    int nextVoiceSync;

    /** Delayed MOS8580 write register */
    int delayedOffset;

    /**
     * Currently active chip model.
     */
    ChipModel model;

    /** Delayed MOS8580 write value */
    unsigned char delayedValue;

    /** Last written value */
    unsigned char busValue;

    bool muted[3];

private:
    /**
     * Write value to register during this clock cycle.
     *
     * @param offset chip register to write
     * @param value value to write
     */
    void writeImmediate(int offset, unsigned char value);

    /**
     * Age the bus value and zero it if it's TTL has expired
     *
     * @param n the number of cycles
     */
    void ageBusValue(int n);

    /**
     * Get output sample.
     *
     * @return the output sample
     */
    int output() const;

    /**
     * Calculate the numebr of cycles according to current parameters
     * that it takes to reach sync.
     *
     * @param sync whether to do the actual voice synchronization
     */
    void voiceSync(bool sync);

public:
    SID();
    ~SID();

    /**
     * Set chip model.
     *
     * @param model chip model to use
     */
    void setChipModel(ChipModel model);

    ChipModel getChipModel() const { return model; }

    /**
     * SID reset.
     */
    void reset();

    /**
     * 16-bit input (EXT IN). Write 16-bit sample to audio input. NB! The caller
     * is responsible for keeping the value within 16 bits. Note that to mix in
     * an external audio signal, the signal should be resampled to 1MHz first to
     * avoid sampling noise.
     *
     * @param value input level to set
     */
    void input(int value);

    /**
     * Read registers.
     * <P>
     * Reading a write only register returns the last char written to any SID register.
     * The individual bits in this value start to fade down towards zero after a few cycles.
     * All bits reach zero within approximately $2000 - $4000 cycles.
     * It has been claimed that this fading happens in an orderly fashion,
     * however sampling of write only registers reveals that this is not the case.
     * NB! This is not correctly modeled.
     * The actual use of write only registers has largely been made
     * in the belief that all SID registers are readable.
     * To support this belief the read would have to be done immediately
     * after a write to the same register (remember that an intermediate write
     * to another register would yield that value instead).
     * With this in mind we return the last value written to any SID register
     * for $2000 cycles without modeling the bit fading.
     *
     * @param offset SID register to read
     * @return value read from chip
     */
    unsigned char read(int offset);

    /**
     * Write registers.
     *
     * @param offset chip register to write
     * @param value value to write
     */
    void write(int offset, unsigned char value);

    /**
     * SID voice muting.
     *
     * @param channel channel to modify
     * @param enable is muted?
     */
    void mute(int channel, bool enable) { muted[channel] = enable; }

    /**
     * Setting of SID sampling parameters.
     * <P>
     * Use a clock freqency of 985248Hz for PAL C64, 1022730Hz for NTSC C64.
     * The default end of passband frequency is pass_freq = 0.9*sample_freq/2
     * for sample frequencies up to ~ 44.1kHz, and 20kHz for higher sample frequencies.
     * <P>
     * For resampling, the ratio between the clock frequency and the sample frequency
     * is limited as follows: 125*clock_freq/sample_freq < 16384
     * E.g. provided a clock frequency of ~ 1MHz, the sample frequency can not be set
     * lower than ~ 8kHz. A lower sample frequency would make the resampling code
     * overfill its 16k sample ring buffer.
     * <P>
     * The end of passband frequency is also limited: pass_freq <= 0.9*sample_freq/2
     * <P>
     * E.g. for a 44.1kHz sampling rate the end of passband frequency
     * is limited to slightly below 20kHz.
     * This constraint ensures that the FIR table is not overfilled.
     *
     * @param clockFrequency System clock frequency at Hz
     * @param method sampling method to use
     * @param samplingFrequency Desired output sampling rate
     * @param highestAccurateFrequency
     */
    void setSamplingParameters(double clockFrequency, SamplingMethod method, double samplingFrequency, double highestAccurateFrequency);

    /**
     * Clock SID forward using chosen output sampling algorithm.
     *
     * @param cycles c64 clocks to clock
     * @param buf audio output buffer
     * @return number of samples produced
     */
    int clock(int cycles, short* buf);

    /**
     * Clock SID forward with no audio production.
     * <p>
     * <b>Warning:</b>
     * You can't mix this method of clocking with the audio-producing
     * clock() because components that don't affect OSC3/ENV3 are not
     * emulated.
     *
     * @param cycles c64 clocks to clock.
     */
    void clockSilent(int cycles);

    /**
     * Get chip's 6581 filter.
     *
     * @return filter
     */
    Filter6581* getFilter6581() const { return filter6581; }

    /**
     * Get chip's 8580 filter.
     *
     * @return filter
     */
    Filter8580* getFilter8580() const { return filter8580; }
};

} // namespace reSIDfp

#if RESID_INLINING || defined(SID_CPP)

#include <algorithm>

#include "Filter.h"
#include "ExternalFilter.h"
#include "Voice.h"
#include "resample/Resampler.h"

namespace reSIDfp
{
RESID_INLINE
int SID::output() const
{
    return externalFilter->clock(
               filter->clock(
                   voice[0]->output(voice[2]->wave()),
                   voice[1]->output(voice[0]->wave()),
                   voice[2]->output(voice[1]->wave())
               )
           );
}


RESID_INLINE
int SID::clock(int cycles, short* buf)
{
    ageBusValue(cycles);
    int s = 0;

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
                if (resampler->input(output()))
                {
                    buf[s++] = resampler->getOutput();
                }

                /* clock waveform generators */
                voice[0]->wave()->clock();
                voice[1]->wave()->clock();
                voice[2]->wave()->clock();

                /* clock envelope generators */
                voice[0]->envelope()->clock();
                voice[1]->envelope()->clock();
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

    return s;
}

} // namespace reSIDfp

#endif

#endif
