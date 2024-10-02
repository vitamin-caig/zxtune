/**
 *
 * @file
 *
 * @brief  Detect callback helpers
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// common includes
#include <string_view.h>
// core includes
#include <core/module_detect.h>

namespace Module
{
  // Helper classes
  class DetectCallbackDelegate : public DetectCallback
  {
  public:
    explicit DetectCallbackDelegate(DetectCallback& delegate)
      : Delegate(delegate)
    {}

    Parameters::Container::Ptr CreateInitialProperties(StringView subpath) const override
    {
      return Delegate.CreateInitialProperties(subpath);
    }

    void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& decoder,
                       Module::Holder::Ptr holder) override
    {
      return Delegate.ProcessModule(location, decoder, std::move(holder));
    }

    void ProcessUnknownData(const ZXTune::DataLocation& location) override
    {
      return Delegate.ProcessUnknownData(location);
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return Delegate.GetProgress();
    }

  protected:
    DetectCallback& Delegate;
  };
}  // namespace Module
