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

//core includes
#include <core/module_detect.h>
//common includes
#include <progress_callback.h>

namespace Module
{
  // Helper classes
  class DetectCallbackDelegate : public DetectCallback
  {
  public:
    explicit DetectCallbackDelegate(DetectCallback& delegate)
      : Delegate(delegate)
    {
    }

    Parameters::Container::Ptr CreateInitialProperties(const String& subpath) const
    {
      return Delegate.CreateInitialProperties(subpath);
    }

    void ProcessModule(const ZXTune::DataLocation& location, const ZXTune::Plugin& decoder, Module::Holder::Ptr holder) override
    {
      return Delegate.ProcessModule(location, decoder, std::move(holder));
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return Delegate.GetProgress();
    }
  protected:
    DetectCallback& Delegate;
  };

  class CustomProgressDetectCallbackAdapter : public DetectCallbackDelegate
  {
  public:
    CustomProgressDetectCallbackAdapter(DetectCallback& delegate, Log::ProgressCallback::Ptr progress)
      : DetectCallbackDelegate(delegate)
      , Progress(std::move(progress))
    {
    }

    CustomProgressDetectCallbackAdapter(DetectCallback& delegate)
      : DetectCallbackDelegate(delegate)
    {
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return Progress.get();
    }
  private:
    const Log::ProgressCallback::Ptr Progress;
  };
}
