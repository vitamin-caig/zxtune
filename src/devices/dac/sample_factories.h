/**
 *
 * @file
 *
 * @brief  DAC sample factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <devices/dac/sample.h>

#include <binary/view.h>

namespace Devices::DAC
{
  Sample::Ptr CreateU8Sample(Binary::View content, std::size_t loop);
  Sample::Ptr CreateU4Sample(Binary::View content, std::size_t loop);
  Sample::Ptr CreateU4PackedSample(Binary::View content, std::size_t loop);
}  // namespace Devices::DAC
