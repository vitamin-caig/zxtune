/**
 *
 * @file
 *
 * @brief  PSF related stuff. Exe
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/players/xsf/memory_region.h"

#include <binary/view.h>

#include <memory>

namespace Module::PSF
{
  struct PsxExe
  {
    using Ptr = std::unique_ptr<const PsxExe>;
    using RWPtr = std::unique_ptr<PsxExe>;

    PsxExe() = default;
    PsxExe(const PsxExe&) = delete;
    PsxExe& operator=(const PsxExe&) = delete;

    uint_t RefreshRate = 0;
    uint32_t PC = 0;
    uint32_t SP = 0;
    MemoryRegion RAM;

    static void Parse(Binary::View data, PsxExe& exe);
  };
}  // namespace Module::PSF
