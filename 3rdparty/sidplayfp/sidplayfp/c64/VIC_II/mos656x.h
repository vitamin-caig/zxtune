/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2013 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2001 Simon White
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

#ifndef MOS656X_H
#define MOS656X_H

#include <stdint.h> 

#include "sidplayfp/event.h" 
#include "sidplayfp/component.h"
#include "sidplayfp/EventScheduler.h"


class MOS656X: public component, private Event
{
public:
    typedef enum
    {
        MOS6567R56A = 0  /* OLD NTSC CHIP */
        ,MOS6567R8       /* NTSC-M */
        ,MOS6569         /* PAL-B */
        ,MOS6572         /* PAL-N */
    } model_t;

private:
    typedef struct
    {
        unsigned int rasterLines;
        unsigned int cyclesPerLine;
        event_clock_t (MOS656X::*clock)();
    } model_data_t;

private:
    static const char *credit;

    static const model_data_t modelData[];

private:
    /** raster IRQ flag */
    static const int IRQ_RASTER = 1 << 0;

    /** Light-Pen IRQ flag */
    static const int IRQ_LIGHTPEN = 1 << 3;

protected:
    /** First line when we check for bad lines */
    static const unsigned int FIRST_DMA_LINE = 0x30;

    /** Last line when we check for bad lines */
    static const unsigned int LAST_DMA_LINE = 0xf7;

protected:
    event_clock_t (MOS656X::*clock)();

    event_clock_t rasterClk;

    /** CPU's event context. */
    EventContext &event_context;

    /** Number of cycles per line. */
    unsigned int cyclesPerLine;

    unsigned int maxRasters;

    /** Current visible line */
    unsigned int lineCycle;

    /** current raster line */
    unsigned int rasterY;

    /** vertical scrolling value */
    unsigned int yscroll;

    uint16_t raster_irq;

    /** are bad lines enabled for this frame? */
    bool areBadLinesEnabled;

    /** is the current line a bad line */
    bool isBadLine;

    /** Set when new frame starts. */
    bool vblanking;

    /** Has light pen IRQ been triggered in this frame already? */
    bool lp_triggered;

    /** internal IRQ flags */
    uint8_t irqFlags;

    /** masks for the IRQ flags */
    uint8_t irqMask;

    /** Light pen coordinates */
    uint8_t lpx, lpy;

    /** the 8 sprites data*/
    //@{
    uint8_t &sprite_enable, &sprite_y_expansion;
    uint8_t sprite_dma;
    uint8_t sprite_mc_base[8];
    uint8_t sprite_mc[8];
    //@}

    /** memory for chip registers */
    uint8_t regs[0x40];

private:
    event_clock_t clockPAL();
    event_clock_t clockNTSC();
    event_clock_t clockOldNTSC();

    /** Signal CPU interrupt if requested by VIC. */
    void handleIrqState();

    EventCallback<MOS656X> badLineStateChangeEvent;

    /** AEC state was updated. */
    void badLineStateChange() { setBA(!isBadLine); }

    /**
    * Set an IRQ flag and trigger an IRQ if the corresponding IRQ mask is set.
    * The IRQ only gets activated, i.e. flag 0x80 gets set, if it was not active before.
    */
    void activateIRQFlag(int flag)
    {
        irqFlags |= flag;
        handleIrqState();
    }

    /**
    * Read the DEN flag which tells whether the display is enabled
    *
    * @return true if DEN is set, otherwise false
    */
    bool readDEN() const { return (regs[0x11] & 0x10) != 0; }

    bool evaluateIsBadLine() const
    {
        return areBadLinesEnabled
            && rasterY >= FIRST_DMA_LINE
            && rasterY <= LAST_DMA_LINE
            && (rasterY & 7) == yscroll;
    }

    inline void sync()
    {
        event_context.cancel(*this);
        event();
    }

    inline void checkVblank()
    {
        // IRQ occurred (xraster != 0)
        if (rasterY == (maxRasters - 1))
            vblanking = true;
        else
        {
            rasterY++;
            // Trigger raster IRQ if IRQ line reached
            if (rasterY == raster_irq)
                activateIRQFlag(IRQ_RASTER);
        }

        // In line $30, the DEN bit controls if Bad Lines can occur
        if (rasterY == FIRST_DMA_LINE)
            areBadLinesEnabled = readDEN();

        // Test for bad line condition
        isBadLine = evaluateIsBadLine();
    }

    inline void vblank()
    {
        // Vertical blank (line 0)
        if (vblanking)
        {
            vblanking = lp_triggered = false;
            rasterY = 0;
            // Trigger raster IRQ if IRQ in line 0
            if (raster_irq == 0)
                activateIRQFlag(IRQ_RASTER);
        }
    }

    inline void updateMc()
    {
        // Update mc values in one pass
        // after the dma has been processed
        uint8_t mask = 1;
        for (unsigned int i=0; i<8; i++, mask<<=1)
        {
            if (sprite_enable & mask)
                sprite_mc[i] = (sprite_mc[i] + 3) & 0x3f;
        }
    }

    inline void updateMcBase()
    {
        uint8_t mask = 1;
        for (unsigned int i=0; i<8; i++, mask<<=1)
        {
            if (sprite_y_expansion & mask)
                sprite_mc_base[i] = sprite_mc[i];
            if (sprite_mc_base[i] == 0x3f)
                sprite_dma &= ~mask;
        }
    }

    /// Calculate sprite DMA and sprite expansion
    inline void checkSpriteDmaExp()
    {
        const uint8_t y = rasterY & 0xff;
        uint8_t mask = 1;
        for (unsigned int i=0; i<8; i++, mask<<=1)
        {
            if ((sprite_enable & mask) && (y == regs[i << 1]))
            {
                sprite_dma |= mask;
                sprite_mc_base[i] = 0;
                sprite_y_expansion |= mask;
            }
        }
    }

    /// Calculate sprite DMA
    inline void checkSpriteDma()
    {
        const uint8_t y = rasterY & 0xff;
        uint8_t mask = 1;
        for (unsigned int i=0; i<8; i++, mask<<=1)
        {
            if ((sprite_enable & mask) && (y == regs[i << 1]))
            {
                sprite_dma |= mask;
                sprite_mc_base[i] = 0;
            }
        }
    }

    inline void checkSpriteDisplay()
    {
        for (unsigned int i=0; i<8; i++)
        {
            sprite_mc[i] = sprite_mc_base[i];
        }
    }

    /// Start DMA for sprite n
    template<int n>
    inline void startDma()
    {
        if (sprite_dma & (0x01 << n))
            setBA(false);
    }

    /// End DMA for sprite n
    template<int n>
    inline void endDma()
    {
        if (!(sprite_dma & (0x06 << n)))
            setBA(true);
    }

    /// Start bad line
    inline void startBadline()
    {
        if (isBadLine)
            setBA(false);
    }

protected:
    MOS656X(EventContext *context);
    ~MOS656X() {}

    // Environment Interface
    virtual void interrupt (bool state) = 0;
    virtual void setBA     (bool state) = 0;

    /**
    * Read VIC register.
    *
    * @param addr
    *            Register to read.
    */
    uint8_t read(uint_least8_t addr);

    /**
    * Write to VIC register.
    *
    * @param addr
    *            Register to write to.
    * @param data
    *            Data byte to write.
    */
    void write(uint_least8_t addr, uint8_t data);

public:
    void event();

    void chip(model_t model);
    void lightpen();

    // Component Standard Calls
    void reset();

    const char *credits() const { return credit; }

    int getCyclesPerLine() const { return cyclesPerLine; }

    int getRasterLines() const { return maxRasters; }
};

// Template specializations

/// Start DMA for sprite 0
template<>
inline void MOS656X::startDma<0>()
{
    setBA(!(sprite_dma & 0x01));
}

/// End DMA for sprite 7
template<>
inline void MOS656X::endDma<7>()
{
    setBA(true);
}

#endif // MOS656X_H
