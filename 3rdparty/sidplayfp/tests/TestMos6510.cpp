/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2020 Leandro Nini
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

#include "UnitTest++/UnitTest++.h"
#include "UnitTest++/TestReporter.h"


#include "../src/EventScheduler.h"
#include "../src/EventScheduler.cpp"

#define private protected

#include "../src/c64/CPU/mos6510.h"
#include "../src/c64/CPU/opcodes.h"
#include "../src/c64/CPU/mos6510.cpp"

#include <iostream>
#include <iomanip>

using namespace UnitTest;
using namespace libsidplayfp;

class testcpu final : public MOS6510
{
private:
    uint8_t mem[65536];

private:
    uint8_t getInstr() const { return cycleCount >> 3; }

protected:
    uint8_t cpuRead(uint_least16_t addr) override { return mem[addr]; }

    void cpuWrite(uint_least16_t addr, uint8_t data) override { mem[addr] = data; }

public:
    testcpu(EventScheduler &scheduler) :
        MOS6510(scheduler)
    {
        mem[0xFFFC] = 0x00;
        mem[0xFFFD] = 0x10;
    }

    void setMem(uint8_t offset, uint8_t opcode) { mem[0x1000+offset] = opcode; }

    void print() {
        std::cout << "-> " << std::hex << (int)getInstr() << std::endl;
    }

    bool check(uint8_t opcode) const { return getInstr() == opcode; }
};

SUITE(mos6510)
{

struct TestFixture
{
    // Test setup
    TestFixture() :
        cpu(scheduler)
    {
        scheduler.reset();
        cpu.reset();
    }

    EventScheduler scheduler;
    testcpu cpu;
};

/*
 * Interrupt is recognized at T0 and triggered on the following T1
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=20&a=0010&d=58eaeaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=4&irq1=100&logmore=rdy,irq
 */
TEST_FIXTURE(TestFixture, TestNop)
{
    cpu.setMem(0, CLIn);
    cpu.setMem(1, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

/*
 * Interrupt is not recognized at T0 as the I flag is still set
 * It is recognized during the following opcode's T0
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=20&a=0010&d=7858eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=4&irq1=100&logmore=rdy,irq
 */
TEST_FIXTURE(TestFixture, TestCli)
{
    cpu.setMem(0, SEIn);
    cpu.setMem(1, CLIn);
    cpu.setMem(2, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(NOPn));

    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

/*
 * Interrupt is recognized at T0 during CLI
 * as the I flag is cleared while the CPU is stalled by RDY line
 * It is triggered on the following T1
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=20&a=0010&d=7858eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=4&irq1=100&logmore=rdy,irq&rdy0=6&rdy1=8
 */
TEST_FIXTURE(TestFixture, TestCliRdy)
{
    cpu.setMem(0, SEIn);
    cpu.setMem(1, CLIn);
    cpu.setMem(2, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    cpu.setRDY(false);
    scheduler.clock(); // CPU stalled but the I flag is being cleared
    cpu.setRDY(true);
    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

/*
 * Interrupt is recognized at T0 as the I flag is still cleared
 * It is triggered on the following T1
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=20&a=0010&d=5878eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=4&irq1=100&logmore=rdy,irq
 */
TEST_FIXTURE(TestFixture, TestSei)
{
    cpu.setMem(0, CLIn);
    cpu.setMem(1, SEIn);
    cpu.setMem(2, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

/*
 * Interrupt is recognized at T0 during SEI
 * even if the I flag is set while the CPU is stalled by RDY line
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=20&a=0010&d=5878eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=4&irq1=100&logmore=rdy,irq&rdy0=6&rdy1=8
 */
TEST_FIXTURE(TestFixture, TestSeiRdy)
{
    cpu.setMem(0, CLIn);
    cpu.setMem(1, SEIn);
    cpu.setMem(2, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    cpu.setRDY(false);
    scheduler.clock(); // CPU stalled but the I flag is being set
    cpu.setRDY(true);
    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

/*
 * Interrupt is not recognized at T0 during SEI
 * even if the I flag is set while the CPU is stalled by RDY line
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=20&a=0010&d=5878eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=6&irq1=100&logmore=rdy,irq&rdy0=6&rdy1=8
 */
TEST_FIXTURE(TestFixture, TestSeiRdy2)
{
    cpu.setMem(0, CLIn);
    cpu.setMem(1, SEIn);
    cpu.setMem(2, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    scheduler.clock(); // T1
    cpu.triggerIRQ();
    cpu.setRDY(false);
    scheduler.clock(); // CPU stalled but the I flag is being set
    cpu.setRDY(true);
    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(NOPn));
}

/*
 * Interrupt is not recognized at T0 as the I flag is still set
 * It is recognized during the following opcode's T0
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=30&a=0010&d=58087828eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=14&irq1=100&logmore=rdy,irq
 */
TEST_FIXTURE(TestFixture, TestPlp1)
{
    cpu.setMem(0, CLIn);
    cpu.setMem(1, PHPn);
    cpu.setMem(2, SEIn);
    cpu.setMem(3, PLPn);
    cpu.setMem(4, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    scheduler.clock(); // T1
    scheduler.clock(); // T2
    scheduler.clock(); // T0

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    scheduler.clock(); // T2
    scheduler.clock(); // T3
    scheduler.clock(); // T0
    scheduler.clock(); // T1
    CHECK(cpu.check(NOPn));

    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

/*
 * Interrupt is not recognized at T0 as the I flag is still set
 * It is recognized during the following opcode's T0
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=30&a=0010&d=58087828eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=14&irq1=100&logmore=rdy,irq&rdy0=20&rdy1=22
 */
TEST_FIXTURE(TestFixture, TestPlp1Rdy)
{
    cpu.setMem(0, CLIn);
    cpu.setMem(1, PHPn);
    cpu.setMem(2, SEIn);
    cpu.setMem(3, PLPn);
    cpu.setMem(4, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    scheduler.clock(); // T1
    scheduler.clock(); // T2
    scheduler.clock(); // T0

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    scheduler.clock(); // T2
    scheduler.clock(); // T3
    cpu.setRDY(false);
    scheduler.clock(); // CPU stalled
    cpu.setRDY(true);
    scheduler.clock(); // T0
    scheduler.clock(); // T1
    CHECK(cpu.check(NOPn));

    scheduler.clock(); // T0+T2
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

/*
 * Interrupt is not recognized at T0 as the I flag is still set
 * It is recognized during the following opcode's T0
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=30&a=0010&d=78085828eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=14&irq1=100&logmore=rdy,irq
 */
TEST_FIXTURE(TestFixture, TestPlp2)
{
    cpu.setMem(0, SEIn);
    cpu.setMem(1, PHPn);
    cpu.setMem(2, CLIn);
    cpu.setMem(3, PLPn);
    cpu.setMem(4, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    scheduler.clock(); // T1
    scheduler.clock(); // T2
    scheduler.clock(); // T0

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    scheduler.clock(); // T2
    scheduler.clock(); // T3
    scheduler.clock(); // T0
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

/*
 * Interrupt is not recognized at T0 as the I flag is still set
 * It is recognized during the following opcode's T0
 *
 * http://visual6502.org/JSSim/expert.html?graphics=f&loglevel=2&steps=30&a=0010&d=78085828eaeaeaea&a=fffe&d=2000&a=0020&d=e840&r=0010&irq0=14&irq1=100&logmore=rdy,irq&rdy0=20&rdy1=22
 */
TEST_FIXTURE(TestFixture, TestPlp2Rdy)
{
    cpu.setMem(0, SEIn);
    cpu.setMem(1, PHPn);
    cpu.setMem(2, CLIn);
    cpu.setMem(3, PLPn);
    cpu.setMem(4, NOPn);

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    scheduler.clock(); // T1
    scheduler.clock(); // T2
    scheduler.clock(); // T0

    scheduler.clock(); // T1
    scheduler.clock(); // T0+T2

    cpu.triggerIRQ();
    scheduler.clock(); // T1
    scheduler.clock(); // T2
    scheduler.clock(); // T3
    cpu.setRDY(false);
    scheduler.clock(); // CPU stalled
    cpu.setRDY(true);
    scheduler.clock(); // T0
    scheduler.clock(); // T1
    CHECK(cpu.check(BRKn));
}

}
