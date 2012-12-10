/*
Abstract:
  DAC sample factories

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef DEVICES_DAC_SAMPLE_FACTORIES_H_DEFINED
#define DEVICES_DAC_SAMPLE_FACTORIES_H_DEFINED

//library includes
#include <binary/data.h>
#include <devices/dac_sample.h>

namespace Devices
{
  namespace DAC
  {
    Sample::Ptr CreateU8Sample(Binary::Data::Ptr content, std::size_t loop);
    Sample::Ptr CreateU4Sample(Binary::Data::Ptr content, std::size_t loop);
    Sample::Ptr CreateU4PackedSample(Binary::Data::Ptr content, std::size_t loop);
  }
}

#endif //DEVICES_DAC_SAMPLE_FACTORIES_H_DEFINED
