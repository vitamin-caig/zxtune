/**
 *
 * @file
 *
 * @brief  Modules detecting functionality
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
// library includes
#include <binary/container.h>
#include <module/holder.h>
#include <parameters/container.h>

// forward declarations
namespace Log
{
  class ProgressCallback;
}

namespace ZXTune
{
  class DataLocation;
  class Plugin;
}  // namespace ZXTune

//! @brief Global library namespace
namespace Module
{
  class DetectCallback
  {
  public:
    virtual ~DetectCallback() = default;

    virtual Parameters::Container::Ptr CreateInitialProperties(StringView subpath) const = 0;
    //! @brief Process module
    virtual void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& decoder,
                               Module::Holder::Ptr holder) = 0;
    virtual void ProcessUnknownData(const ZXTune::DataLocation& /*location*/){};
    //! @brief Logging callback
    virtual Log::ProgressCallback* GetProgress() const = 0;
  };
}  // namespace Module
