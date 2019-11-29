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
//library includes
#include <binary/view.h>

namespace Module
{
  struct MemoryRegion
  {
    MemoryRegion() = default;
    MemoryRegion(const MemoryRegion&) = delete;
    MemoryRegion(MemoryRegion&& rh) noexcept//= default
    {
      *this = std::move(rh);
    }
    MemoryRegion& operator = (const MemoryRegion&) = delete;
    
    MemoryRegion& operator = (MemoryRegion&& rh) noexcept//= default
    {
      Start = rh.Start;
      Data = std::move(rh.Data);
      return *this;
    }
    
    void Update(uint_t addr, Binary::View data);

    uint_t Start = 0;
    Dump Data;
  };
}
