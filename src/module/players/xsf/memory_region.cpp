/**
 *
 * @file
 *
 * @brief  Memory region helper implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "module/players/xsf/memory_region.h"
// std includes
#include <cstring>

namespace Module
{
  void MemoryRegion::Update(uint_t addr, Binary::View data)
  {
    if (const auto oldSize = Data.size())
    {
      const auto newAddr = std::min(addr, Start);
      const auto newEnd = std::max(addr + data.Size(), Start + oldSize);
      Data.resize(newEnd - newAddr);
      if (newAddr < Start)
      {
        std::memmove(Data.data() + (Start - newAddr), Data.data(), oldSize);
        Start = newAddr;
      }
      std::memcpy(Data.data() + (addr - newAddr), data.Start(), data.Size());
    }
    else
    {
      const auto* const src = static_cast<const uint8_t*>(data.Start());
      Data.assign(src, src + data.Size());
      Start = addr;
    }
  }
}  // namespace Module
