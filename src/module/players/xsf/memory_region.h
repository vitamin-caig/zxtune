/**
* 
* @file
*
* @brief  Memory region helper
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//common includes
#include <types.h>

namespace Module
{
  struct MemoryRegion
  {
    MemoryRegion() = default;
    MemoryRegion(const MemoryRegion&) = delete;
    MemoryRegion(MemoryRegion&& rh)//= default
    {
      *this = std::move(rh);
    }
    MemoryRegion& operator = (const MemoryRegion&) = delete;
    
    MemoryRegion& operator = (MemoryRegion&& rh)//= default
    {
      Start = rh.Start;
      Data = std::move(rh.Data);
      return *this;
    }
    
    void Update(uint_t addr, const void* data, std::size_t size);

    uint_t Start = 0;
    Dump Data;
  };
}
