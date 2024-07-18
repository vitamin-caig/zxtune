/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright 2011-2020 Leandro Nini <drfiemost@users.sourceforge.net>
 * Copyright 2009-2014 VICE Project
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

void SerialPort::reset()
{
    count = 0;
    cntHistory = 0;
    cnt = 1;
    loaded = false;
    pending = false;
    forceFinish = false;

    lastSync = eventScheduler.getTime(EVENT_CLOCK_PHI1);
}

void SerialPort::event()
{
    parent.spInterrupt();
}

void SerialPort::syncCntHistory()
{
    const event_clock_t time = eventScheduler.getTime(EVENT_CLOCK_PHI1);
    const event_clock_t clocks = time - lastSync;
    lastSync = time;
    for (int i=0; i<clocks; i++)
    {
        cntHistory = (cntHistory << 1) | cnt;
    }
}

void SerialPort::startSdr()
{
    eventScheduler.schedule(startSdrEvent, 1);
}

void SerialPort::doStartSdr()
{
    if (!loaded)
        loaded = true;
    else
        pending = true;
}

void SerialPort::switchSerialDirection(bool input)
{
    syncCntHistory();

    if (input)
    {
        const uint8_t cntVal = model4485 ? 0x7 : 0x6;
        forceFinish = (cntHistory & cntVal) != cntVal;

        if (!forceFinish)
        {
            if ((count != 2) && (eventScheduler.remaining(flipCntEvent) == 1))
            {
                forceFinish = true;
            }
        }
    }
    else
    {
        if (forceFinish)
        {
            eventScheduler.cancel(*this);
            eventScheduler.schedule(*this, 2);
            forceFinish = false;
        }
    }

    cnt = 1;
    cntHistory |= 1;

    eventScheduler.cancel(flipCntEvent);
    eventScheduler.cancel(flipFakeEvent);

    count = 0;
    loaded = false;
    pending = false;
}

void SerialPort::handle()
{
    if (loaded && (count == 0))
    {
        // Output rate 8 bits at ta / 2
        count = 16;
    }

    if (count == 0)
        return;

    if (eventScheduler.isPending(flipFakeEvent) || eventScheduler.isPending(flipCntEvent))
    {
        eventScheduler.cancel(flipFakeEvent);
        eventScheduler.schedule(flipFakeEvent, 2);
    }
    else
    {
        eventScheduler.schedule(flipCntEvent, 2);
    }

}

void SerialPort::flipFake() {}

void SerialPort::flipCnt()
{
    if (count == 0)
        return;

    syncCntHistory();

    cnt ^= 1;

    if (--count == 1)
    {
        eventScheduler.cancel(*this);
        eventScheduler.schedule(*this, 2);

        loaded = pending;
        pending = false;
    }
}

}
