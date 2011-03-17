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
#include "core.h"
//common includes
#include <error.h>

namespace ZXTune
{
  namespace Module
  {
    class DetectCallbackDelegate : public DetectCallback
    {
    public:
      explicit DetectCallbackDelegate(const DetectCallback& delegate)
        : Delegate(delegate)
      {
      }

      virtual PluginsEnumerator::Ptr GetEnabledPlugins() const
      {
        return Delegate.GetEnabledPlugins();
      }
      
      virtual Parameters::Accessor::Ptr GetPluginsParameters() const
      {
        return Delegate.GetPluginsParameters();
      }

      virtual Parameters::Accessor::Ptr GetModuleParameters(const Container& container) const
      {
        return Delegate.GetModuleParameters(container);
      }

      virtual Error ProcessModule(const Container& container, Module::Holder::Ptr holder) const
      {
        return Delegate.ProcessModule(container, holder);
      }

      virtual Log::ProgressCallback* GetProgressCallback() const
      {
        return Delegate.GetProgressCallback();
      }
    protected:
      const DetectCallback& Delegate;
    };

    class NoProgressDetectCallback : public DetectCallbackDelegate
    {
    public:
      explicit NoProgressDetectCallback(const DetectCallback& delegate)
        : DetectCallbackDelegate(delegate)
      {
      }

      virtual Log::ProgressCallback* GetProgressCallback() const
      {
        return 0;
      }
    };
  }
}

#endif //__CORE_PLUGINS_CALLBACK_H_DEFINED__
