/**
 *
 * @file
 *
 * @brief  GSF related stuff. ROM
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "module/players/xsf/memory_region.h"
// library includes
#include <binary/view.h>
// std includes
#include <memory>

namespace Module::GSF
{
  struct GbaRom
  {
    using Ptr = std::unique_ptr<const GbaRom>;
    using RWPtr = std::unique_ptr<GbaRom>;

    GbaRom() = default;
    GbaRom(const GbaRom&) = delete;
    GbaRom& operator=(const GbaRom&) = delete;

    uint32_t EntryPoint = 0;
    MemoryRegion Content;

    static void Parse(Binary::View data, GbaRom& rom);
  };
}  // namespace Module::GSF
