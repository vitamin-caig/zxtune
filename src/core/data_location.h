/**
 *
 * @file
 *
 * @brief  Data location interace and factories
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "analysis/path.h"
#include "binary/container.h"

namespace ZXTune
{
  // Describes piece of data and defenitely location inside
  class DataLocation
  {
  public:
    using Ptr = std::shared_ptr<const DataLocation>;
    virtual ~DataLocation() = default;

    virtual Binary::Container::Ptr GetData() const = 0;
    virtual Analysis::Path::Ptr GetPath() const = 0;
    virtual Analysis::Path::Ptr GetPluginsChain() const = 0;
  };
}  // namespace ZXTune
