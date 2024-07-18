/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 *  Copyright (C) 2024 Leandro Nini
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

#define private public

#include "../src/builders/residfp-builder/residfp/resample/Resampler.h"

#include <limits>

using namespace UnitTest;
using namespace reSIDfp;

SUITE(Resampler)
{

TEST(TestSoftClip)
{
    CHECK(Resampler::softClipImpl(0) == 0);
    CHECK(Resampler::softClipImpl(28000) == 28000);
    CHECK(Resampler::softClipImpl(std::numeric_limits<int>::max()) <= 32767);
    CHECK(Resampler::softClipImpl(-28000) == -28000);
    CHECK(Resampler::softClipImpl(std::numeric_limits<int>::min()+1) >= -32768);
}

}
