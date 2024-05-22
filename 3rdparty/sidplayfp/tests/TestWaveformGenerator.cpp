/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2014-2023 Leandro Nini
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

#include "../src/builders/residfp-builder/residfp/WaveformCalculator.cpp"
#include "../src/builders/residfp-builder/residfp/Dac.cpp"

#include "../src/builders/residfp-builder/residfp/WaveformCalculator.h"

#define private public
#define protected public
#define class struct

#include "../src/builders/residfp-builder/residfp/WaveformGenerator.h"
#include "../src/builders/residfp-builder/residfp/WaveformGenerator.cpp"

using namespace UnitTest;

SUITE(WaveformGenerator)
{

TEST(TestShiftRegisterInitValue)
{
    reSIDfp::WaveformGenerator generator;
    generator.reset();

    CHECK_EQUAL(0x3fffff, generator.shift_register);
}

TEST(TestClockShiftRegister)
{
    reSIDfp::WaveformGenerator generator;
    generator.reset();

    generator.shift_register = 0x35555e;
    // shift phase 1
    generator.test_or_reset = false;
    generator.shift_latch = generator.shift_register;
    // shift phase 2
    generator.shift_phase2(0, 0);

    CHECK_EQUAL(0x9e0, generator.noise_output);
}

TEST(TestNoiseOutput)
{
    reSIDfp::WaveformGenerator generator;
    generator.reset();

    generator.shift_register = 0x35555f;
    generator.set_noise_output();

    CHECK_EQUAL(0xe20, generator.noise_output);
}

TEST(TestWriteShiftRegister)
{
    reSIDfp::WaveformGenerator generator;
    generator.reset();

    generator.waveform = 0xf;
    generator.write_shift_register();

    CHECK_EQUAL(0x2dd6eb, generator.shift_register);
}

TEST(TestSetTestBit)
{
    matrix_t* wavetables = reSIDfp::WaveformCalculator::getInstance()->getWaveTable();
    matrix_t* tables = reSIDfp::WaveformCalculator::getInstance()->buildPulldownTable(reSIDfp::MOS6581, reSIDfp::AVERAGE);

    reSIDfp::WaveformGenerator generator;
    generator.reset();
    generator.shift_register = 0x35555e;
    generator.setWaveformModels(wavetables);
    generator.setPulldownModels(tables);

    generator.writeCONTROL_REG(0x08); // set test bit
    generator.clock();
    generator.writeCONTROL_REG(0x00); // unset test bit
    generator.clock();

    CHECK_EQUAL(0x9f0, generator.noise_output);
}

TEST(TestNoiseWriteBack1)
{
    matrix_t* wavetables = reSIDfp::WaveformCalculator::getInstance()->getWaveTable();
    matrix_t* tables = reSIDfp::WaveformCalculator::getInstance()->buildPulldownTable(reSIDfp::MOS6581, reSIDfp::AVERAGE);

    reSIDfp::WaveformGenerator modulator;

    reSIDfp::WaveformGenerator generator;
    generator.setModel(true); //6581
    generator.setWaveformModels(wavetables);
    generator.setPulldownModels(tables);
    generator.reset();

    // switch from noise to noise+triangle
    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x90);
    generator.clock();
    generator.output(&modulator);


    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x80);
    generator.clock();
    generator.output(&modulator);
    CHECK_EQUAL(0xfc, (int)generator.readOSC());
    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x80);
    generator.clock();
    generator.output(&modulator);
    CHECK_EQUAL(0x6c, (int)generator.readOSC());
    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x80);
    generator.clock();
    generator.output(&modulator);
    CHECK_EQUAL(0xd8, (int)generator.readOSC());
    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x80);
    generator.clock();
    generator.output(&modulator);
    CHECK_EQUAL(0xb1, (int)generator.readOSC());
    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x80);
    generator.clock();
    generator.output(&modulator);
    CHECK_EQUAL(0xd8, (int)generator.readOSC());
    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x80);
    generator.clock();
    generator.output(&modulator);
    CHECK_EQUAL(0x6a, (int)generator.readOSC());
    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x80);
    generator.clock();
    generator.output(&modulator);
    CHECK_EQUAL(0xb1, (int)generator.readOSC());
    generator.writeCONTROL_REG(0x88);
    generator.clock();
    generator.output(&modulator);
    generator.writeCONTROL_REG(0x80);
    generator.clock();
    generator.output(&modulator);
    CHECK_EQUAL(0xf0, (int)generator.readOSC());
}

}
