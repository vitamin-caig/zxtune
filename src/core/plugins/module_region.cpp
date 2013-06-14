/*
Abstract:
  Module region helper functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "enumerator.h"
//common includes
#include <crc.h>


namespace ZXTune
{
  uint_t ModuleRegion::Checksum(const Binary::Data& container) const
  {
    const uint8_t* const data = static_cast<const uint8_t*>(container.Start());
    assert(Offset + Size <= container.Size());
    return Crc32(data + Offset, Size);
  }

  Binary::Container::Ptr ModuleRegion::Extract(const Binary::Container& container) const
  {
    return container.GetSubcontainer(Offset, Size);
  }
}
