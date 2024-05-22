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

#ifndef SERIALPORT_H
#define SERIALPORT_H

#include "interrupt.h"

#include "Event.h"

namespace libsidplayfp
{

class MOS652X;

class SerialPort : private Event
{
private:
    /// Pointer to the MOS6526 which this Serial Port belongs to.
    MOS652X &parent;

    /// Event context.
    EventScheduler &eventScheduler;

    EventCallback<SerialPort> flipCntEvent;
    EventCallback<SerialPort> flipFakeEvent;
    EventCallback<SerialPort> startSdrEvent;

    event_clock_t lastSync;

    int count;

    uint8_t cnt;
    uint8_t cntHistory;

    bool loaded;
    bool pending;

    bool forceFinish;

    bool model4485;

private:
    void event() override;

    void flipCnt();
    void flipFake();

    void doStartSdr();

    void syncCntHistory();

public:
    explicit SerialPort(EventScheduler &scheduler, MOS652X &parent) :
        Event("Serial Port interrupt"),
        parent(parent),
        eventScheduler(scheduler),
        flipCntEvent("flip CNT", *this, &SerialPort::flipCnt),
        flipFakeEvent("flip fake", *this, &SerialPort::flipFake),
        startSdrEvent("start SDR", *this, &SerialPort::doStartSdr),
        model4485(false)
    {}

    void reset();

    void setModel4485(bool is4485) { model4485 = is4485; }

    void startSdr();

    void switchSerialDirection(bool input);

    void handle();
};

}

#endif // SERIALPORT_H
