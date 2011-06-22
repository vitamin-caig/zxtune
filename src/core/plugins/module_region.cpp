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
  uint_t ModuleRegion::Checksum(const IO::DataContainer& container) const
  {
    const uint8_t* const data = static_cast<const uint8_t*>(container.Data());
    assert(Offset + Size <= container.Size());
    return Crc32(data + Offset, Size);
  }

  IO::DataContainer::Ptr ModuleRegion::Extract(const IO::DataContainer& container) const
  {
    return container.GetSubcontainer(Offset, Size);
  }
}
