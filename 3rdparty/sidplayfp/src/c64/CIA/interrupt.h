/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2020 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2007-2010 Antti Lankila
 * Copyright 2000 Simon White
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

#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "Event.h"
#include "EventScheduler.h"
#include "EventCallback.h"

#include <stdint.h>

#include "sidcxx11.h"

namespace libsidplayfp
{

class MOS652X;

/**
 * This is the base class for the MOS6526 interrupt sources.
 * Based on Denise emu code.
 */
class InterruptSource
{
public:
    enum
    {
        INTERRUPT_NONE         = 0,      ///< no interrupt
        INTERRUPT_UNDERFLOW_A  = 1 << 0, ///< underflow Timer A
        INTERRUPT_UNDERFLOW_B  = 1 << 1, ///< underflow Timer B
        INTERRUPT_ALARM        = 1 << 2, ///< alarm clock
        INTERRUPT_SP           = 1 << 3, ///< serial port
        INTERRUPT_FLAG         = 1 << 4, ///< external flag
        INTERRUPT_REQUEST      = 1 << 7  ///< control bit
    };

private:
    /// Pointer to the MOS6526 which this Interrupt belongs to.
    MOS652X &parent;

protected:
    /// Event scheduler.
    EventScheduler &eventScheduler;

    /// Clock when clear was called last
    event_clock_t last_clear;
    event_clock_t last_set;

    /// Interrupt control register
    uint8_t icr;

    /// Interrupt data register
    uint8_t idr;

    uint8_t idrTemp;

    /// Have we already scheduled CIA->CPU interrupt transition?
    bool scheduled;

    /// is the irq pin asserted?
    bool asserted;

private:
    EventCallback<InterruptSource> interruptEvent;

    EventCallback<InterruptSource> updateIdrEvent;

    EventCallback<InterruptSource> setIrqEvent;

    EventCallback<InterruptSource> clearIrqEvent;

protected:
    inline bool interruptTriggered() const { return idr & INTERRUPT_REQUEST; }

    inline bool interruptMasked(uint8_t interruptMask) const
    {
        return ((interruptMask != INTERRUPT_NONE) ? interruptMask : idr) & icr;
    }

    virtual void triggerInterrupt() =0;

    /**
     * Check if interrupts were ackowledged during previous cycle.
     */
    inline bool ack0() const { return eventScheduler.getTime(EVENT_CLOCK_PHI2) == (last_clear+1); }
    inline bool write0() const { return eventScheduler.getTime(EVENT_CLOCK_PHI2) == (last_set+1); }

    /**
     * Signal interrupt to CPU.
     */
    void interrupt();

    void updateIdr();

    void setIrq();

    void clearIrq();

protected:
    /**
     * Create a new InterruptSource.
     *
     * @param scheduler event scheduler
     * @param parent the CIA chip which this Interrupt belongs to
     */
    InterruptSource(EventScheduler &scheduler, MOS652X &parent) :
        parent(parent),
        eventScheduler(scheduler),
        last_clear(0),
        last_set(0),
        icr(0),
        idr(0),
        scheduled(false),
        asserted(false),
        interruptEvent("CIA Interrupt", *this, &InterruptSource::interrupt),
        updateIdrEvent("CIA update ICR", *this, &InterruptSource::updateIdr),
        setIrqEvent("CIA set IRQ", *this, &InterruptSource::setIrq),
        clearIrqEvent("CIA clear IRQ", *this, &InterruptSource::clearIrq)
    {}

    /**
     * Schedules an IRQ asserting state transition for next cycle.
     */
    void schedule(int delay)
    {
        if (!scheduled)
        {
            eventScheduler.schedule(interruptEvent, delay, EVENT_CLOCK_PHI1);
            scheduled = true;
        }
    }

    void scheduleIrq()
    {
        eventScheduler.schedule(setIrqEvent, 1, EVENT_CLOCK_PHI1);
    }

    bool isTriggered(uint8_t interruptMask);

public:
    virtual ~InterruptSource() {}

    /**
     * Trigger an interrupt.
     * 
     * @param interruptMask Interrupt flag number
     */
    virtual void trigger(uint8_t interruptMask) =0;

    /**
     * Clear interrupt state.
     * 
     * @return old interrupt state
     */
    virtual uint8_t clear();

    /**
     * Clear pending interrupts, but do not signal to CPU we lost them.
     * It is assumed that all components get reset() calls in synchronous manner.
     */
    virtual void reset()
    {
        last_clear = 0;
        last_set = 0;

        icr = 0;
        idr = 0;

        eventScheduler.cancel(updateIdrEvent);
        eventScheduler.cancel(setIrqEvent);
        eventScheduler.cancel(clearIrqEvent);
        eventScheduler.cancel(interruptEvent);
        scheduled = false;

        asserted = false;
    }

    /**
     * Set interrupt control mask bits.
     *
     * @param interruptMask control mask bits
     */
    void set(uint8_t interruptMask);
};

}

#endif // INTERRUPT_H
