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

//library includes
#include <binary/data.h>
#include <devices/dac/sample.h>

namespace Devices
{
  namespace DAC
  {
    Sample::Ptr CreateU8Sample(const Binary::Data& content, std::size_t loop);
    Sample::Ptr CreateU4Sample(const Binary::Data& content, std::size_t loop);
    Sample::Ptr CreateU4PackedSample(const Binary::Data& content, std::size_t loop);
  }
}
