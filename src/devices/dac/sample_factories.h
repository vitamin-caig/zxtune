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
#include <binary/data_view.h>
#include <devices/dac/sample.h>

namespace Devices
{
  namespace DAC
  {
    Sample::Ptr CreateU8Sample(Binary::DataView content, std::size_t loop);
    Sample::Ptr CreateU4Sample(Binary::DataView content, std::size_t loop);
    Sample::Ptr CreateU4PackedSample(Binary::DataView content, std::size_t loop);
  }
}
