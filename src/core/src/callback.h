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

//local includes
#include "core/plugins/enumerator.h"
//core includes
#include <core/module_holder.h>
//common includes
#include <error.h>
#include <logging.h>

namespace ZXTune
{
  namespace Module
  {
    class DetectCallback
    {
    public:
      virtual ~DetectCallback() {}

      //! @brief Returns plugins parameters
      virtual Parameters::Accessor::Ptr GetPluginsParameters() const = 0;
      //! @brief Returns parameters for future module
      virtual Parameters::Accessor::Ptr CreateModuleParameters(DataLocation::Ptr location) const = 0;
      //! @brief Process module
      virtual void ProcessModule(DataLocation::Ptr location, Module::Holder::Ptr holder) const = 0;
      //! @brief Logging callback
      virtual Log::ProgressCallback* GetProgress() const = 0;
    };

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

      virtual Parameters::Accessor::Ptr CreateModuleParameters(DataLocation::Ptr location) const
      {
        return Delegate.CreateModuleParameters(location);
      }

      virtual void ProcessModule(DataLocation::Ptr location, Module::Holder::Ptr holder) const
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
}

#endif //__CORE_PLUGINS_CALLBACK_H_DEFINED__
