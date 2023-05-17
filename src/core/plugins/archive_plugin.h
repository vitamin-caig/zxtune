/**
 *
 * @file
 *
 * @brief  ArchivePlugin interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "core/src/location.h"
// library includes
#include <analysis/result.h>
#include <core/module_detect.h>
#include <core/plugin.h>

namespace Parameters
{
  class Accessor;
}

namespace ZXTune
{
  class ArchiveCallback : public Module::DetectCallback
  {
  public:
    ~ArchiveCallback() override = default;

    virtual void ProcessData(DataLocation::Ptr data) = 0;
  };

  class ArchivePlugin : public Plugin
  {
  public:
    using Ptr = std::shared_ptr<const ArchivePlugin>;

    static const std::vector<Ptr>& Enumerate();

    virtual Binary::Format::Ptr GetFormat() const = 0;

    //! @brief Detect modules in data
    virtual Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                         ArchiveCallback& callback) const = 0;

    virtual DataLocation::Ptr TryOpen(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                      const Analysis::Path& pathToOpen) const = 0;
  };
}  // namespace ZXTune
