/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2021 Leandro Nini <drfiemost@users.sourceforge.net>
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

#include "interrupt.h"

#include "mos652x.h"

namespace libsidplayfp
{

void InterruptSource::interrupt()
{
    if (!interruptTriggered())
    {
        triggerInterrupt();
        setIrq();
    }

    scheduled = false;
}

void InterruptSource::updateIdr()
{
    idr = idrTemp;

    if (ack0())
    {
        eventScheduler.schedule(updateIdrEvent, 1, EVENT_CLOCK_PHI1);
        idrTemp = 0;
    }
}

void InterruptSource::setIrq()
{
    if (!ack0())
    {
        if (!asserted)
        {
            parent.interrupt(true);
            asserted = true;
        }
    }
}

void InterruptSource::clearIrq()
{
    if (asserted)
    {
        parent.interrupt(false);
        asserted = false;
    }
}

bool InterruptSource::isTriggered(uint8_t interruptMask)
{
    idr |= interruptMask;
    idrTemp |= interruptMask;

    if (interruptMasked(interruptMask))
        return true;

    if ((interruptMask == INTERRUPT_NONE) && write0())
    {
        // cancel pending interrupts
        if (scheduled)
        {
            eventScheduler.cancel(interruptEvent);
            scheduled = false;
        }
    }
    return false;
}

void InterruptSource::set(uint8_t interruptMask)
{
    if (interruptMask & INTERRUPT_REQUEST)
    {
        icr |= interruptMask & ~INTERRUPT_REQUEST;
    }
    else
    {
        icr &= ~interruptMask;
    }

    if (!ack0())
        trigger(INTERRUPT_NONE);

    last_set = eventScheduler.getTime(EVENT_CLOCK_PHI2);
}

uint8_t InterruptSource::clear()
{
    last_clear = eventScheduler.getTime(EVENT_CLOCK_PHI2);

    eventScheduler.schedule(clearIrqEvent, 0, EVENT_CLOCK_PHI1);

    if (!eventScheduler.isPending(updateIdrEvent))
    {
        eventScheduler.schedule(updateIdrEvent, 1, EVENT_CLOCK_PHI1);
        idrTemp = 0;
    }

    return idr;
}

}
