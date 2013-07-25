/*
Abstract:
  Module detection callback functions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CALLBACK_H_DEFINED__
#define __CORE_PLUGINS_CALLBACK_H_DEFINED__

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

    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      return Delegate.GetPluginsParameters();
    }

    virtual void ProcessModule(ZXTune::DataLocation::Ptr location, Module::Holder::Ptr holder) const
    {
      return Delegate.ProcessModule(location, holder);
    }

    virtual Log::ProgressCallback* GetProgress() const
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
      , Progress(progress)
    {
    }

    CustomProgressDetectCallbackAdapter(const DetectCallback& delegate)
      : DetectCallbackDelegate(delegate)
    {
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return Progress.get();
    }
  private:
    const Log::ProgressCallback::Ptr Progress;
  };
}

#endif //__CORE_PLUGINS_CALLBACK_H_DEFINED__
