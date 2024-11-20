/**
 *
 * @file
 *
 * @brief  Metainfo operating interfaces
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "binary/container.h"
#include "binary/view.h"

#include "types.h"

#include <memory>

namespace Formats::Chiptune
{
  class PatchedDataBuilder
  {
  public:
    using Ptr = std::unique_ptr<PatchedDataBuilder>;
    virtual ~PatchedDataBuilder() = default;

    virtual void InsertData(std::size_t offset, Binary::View data) = 0;
    virtual void OverwriteData(std::size_t offset, Binary::View data) = 0;
    // offset in original, non-patched data
    virtual void FixLEWord(std::size_t offset, int_t delta) = 0;
    virtual Binary::Container::Ptr GetResult() const = 0;

    static Ptr Create(Binary::View data);
  };
}  // namespace Formats::Chiptune
