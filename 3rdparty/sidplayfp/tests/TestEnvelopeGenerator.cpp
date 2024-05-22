/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2014-2020 Leandro Nini
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

#include "../src/builders/residfp-builder/residfp/Dac.cpp"

#define private public
#define protected public
#define class struct

#include "../src/builders/residfp-builder/residfp/EnvelopeGenerator.h"
#include "../src/builders/residfp-builder/residfp/EnvelopeGenerator.cpp"

using namespace UnitTest;

SUITE(EnvelopeGenerator)
{

struct TestFixture
{
    // Test setup
    TestFixture()
    {
        generator.reset();
        generator.envelope_counter = 0;
    }

    reSIDfp::EnvelopeGenerator generator;
};

TEST_FIXTURE(TestFixture, TestADSRDelayBug)
{
    // If the rate counter comparison value is set below the current value of the
    // rate counter, the counter will continue counting up until it wraps around
    // to zero at 2^15 = 0x8000, and then count rate_period - 1 before the
    // envelope can constly be stepped.

    generator.writeATTACK_DECAY(0x70);

    generator.writeCONTROL_REG(0x01);

    // wait 200 cycles
    for (int i=0; i<200; i++)
    {
        generator.clock();
    }

    CHECK_EQUAL(1, (int)generator.readENV());

    // set lower Attack time
    // should theoretically clock after 63 cycles
    generator.writeATTACK_DECAY(0x20);

    // wait another 200 cycles
    for (int i=0; i<200; i++)
    {
        generator.clock();
    }

    CHECK_EQUAL(1, (int)generator.readENV());
}

TEST_FIXTURE(TestFixture, TestFlipFFto00)
{
    // The envelope counter can flip from 0xff to 0x00 by changing state to
    // release, then to attack. The envelope counter is then frozen at
    // zero; to unlock this situation the state must be changed to release,
    // then to attack.

    generator.writeATTACK_DECAY(0x77);
    generator.writeSUSTAIN_RELEASE(0x77);

    generator.writeCONTROL_REG(0x01);

    do
    {
        generator.clock();
    } while ((int)generator.readENV() != 0xff);

    generator.writeCONTROL_REG(0x00);
    // run for three clocks, accounting for state pipeline
    generator.clock();
    generator.clock();
    generator.clock();
    generator.writeCONTROL_REG(0x01);

    // wait at least 313 cycles
    // so the counter is clocked once
    for (int i=0; i<315; i++)
    {
        generator.clock();
    }

    CHECK_EQUAL(0, (int)generator.readENV());
}

TEST_FIXTURE(TestFixture, TestFlip00toFF)
{
    // The envelope counter can flip from 0x00 to 0xff by changing state to
    // attack, then to release. The envelope counter will then continue
    // counting down in the release state.

    generator.counter_enabled = false;

    generator.writeATTACK_DECAY(0x77);
    generator.writeSUSTAIN_RELEASE(0x77);
    generator.clock();
    CHECK_EQUAL(0, (int)generator.readENV());

    generator.writeCONTROL_REG(0x01);
    // run for three clocks, accounting for state pipeline
    generator.clock();
    generator.clock();
    generator.clock();
    generator.writeCONTROL_REG(0x00);

    // wait at least 313 cycles
    // so the counter is clocked once
    for (int i=0; i<315; i++)
    {
        generator.clock();
    }

    CHECK_EQUAL(0xff, (int)generator.readENV());
}

}
