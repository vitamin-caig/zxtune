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

// library includes
#include <binary/view.h>
#include <devices/dac/sample.h>

namespace Devices::DAC
{
  Sample::Ptr CreateU8Sample(Binary::View content, std::size_t loop);
  Sample::Ptr CreateU4Sample(Binary::View content, std::size_t loop);
  Sample::Ptr CreateU4PackedSample(Binary::View content, std::size_t loop);
}  // namespace Devices::DAC
