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
    explicit DetectCallbackDelegate(const DetectCallback& delegate)
      : Delegate(delegate)
    {
    }

    void ProcessModule(ZXTune::DataLocation::Ptr location, ZXTune::Plugin::Ptr decoder, Module::Holder::Ptr holder) const override
    {
      return Delegate.ProcessModule(location, decoder, holder);
    }

    Log::ProgressCallback* GetProgress() const override
    {
      return Delegate.GetProgress();
    }
  protected:
    const DetectCallback& Delegate;
  };

  class CustomProgressDetectCallbackAdapter : public DetectCallbackDelegate
  {
  public:
    CustomProgressDetectCallbackAdapter(const DetectCallback& delegate, Log::ProgressCallback::Ptr progress)
      : DetectCallbackDelegate(delegate)
      , Progress(std::move(progress))
    {
    }

    CustomProgressDetectCallbackAdapter(const DetectCallback& delegate)
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
