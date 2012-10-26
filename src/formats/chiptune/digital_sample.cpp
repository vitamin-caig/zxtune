/*
Abstract:
  Digital samples helper interfaces

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "digital_sample.h"
//library includes
#include <binary/container_factories.h>
//std includes
#include <algorithm>

namespace
{
  inline uint8_t Convert(uint8_t val)
  {
    //AY table
    static const uint8_t TABLE[16] =
    {
      0x00, 0x03, 0x04, 0x06, 0x0a, 0x0f, 0x15, 0x22, 0x28, 0x41, 0x5b, 0x72, 0x90, 0xb5, 0xd7, 0xff
    };
    return TABLE[val];
  }

  inline uint8_t MaskAndConvert(uint8_t in)
  {
    return Convert(in & 15);
  }
}

namespace Formats
{
  namespace Chiptune
  {
    Binary::Data::Ptr Convert4BitSample(const Binary::Data& sample)
    {
      std::auto_ptr<Dump> res(new Dump(sample.Size()));
      const uint8_t* const start = static_cast<const uint8_t*>(sample.Start());
      std::transform(start, start + sample.Size(), &res->front(), &MaskAndConvert);
      return Binary::CreateContainer(res);
    }
  }
}
