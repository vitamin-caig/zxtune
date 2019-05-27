/**
* 
* @file
*
* @brief  Memory region helper implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "module/players/xsf/memory_region.h"
//std includes
#include <cstring>

namespace Module
{
  void MemoryRegion::Update(uint_t addr, const void* data, std::size_t size)
  {
    if (const auto oldSize = Data.size())
    {
      const auto newAddr = std::min(addr, Start);
      const auto newEnd = std::max(addr + size, Start + oldSize);
      Data.resize(newEnd - newAddr);
      if (newAddr < Start)
      {
        std::memmove(Data.data() + (Start - newAddr), Data.data(), oldSize);
        Start = newAddr;
      }
      std::memcpy(Data.data() + (addr - newAddr), data, size);
    }
    else
    {
      const auto src = static_cast<const uint8_t*>(data);
      Data.assign(src, src + size);
      Start = addr;
    }
  }
}
