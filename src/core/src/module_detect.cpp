/*
Abstract:
  Module detection in container

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "callback.h"
#include "core.h"
#include "core/plugins/plugins_types.h"
//common includes
#include <debug_log.h>
#include <error.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <src/core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const Debug::Stream Dbg("Core::Detection");

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return ARCHIVE_PLUGIN_PREFIX + pluginId;
  }

  class OpenModuleCallback : public Module::DetectCallback
  {
  public:
    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      return Parameters::Container::Create();
    }

    virtual void ProcessModule(DataLocation::Ptr /*location*/, Module::Holder::Ptr holder) const
    {
      Result = holder;
    }

    virtual Log::ProgressCallback* GetProgress() const
    {
      return 0;
    }

    Module::Holder::Ptr GetResult() const
    {
      return Result;
    }
  private:
    mutable Module::Holder::Ptr Result;
  };
  
  template<class T>
  std::size_t DetectByPlugins(typename T::Iterator::Ptr plugins, DataLocation::Ptr location, const Module::DetectCallback& callback)
  {
    for (; plugins->IsValid(); plugins->Next())
    {
      const typename T::Ptr plugin = plugins->Get();
      const Analysis::Result::Ptr result = plugin->Detect(location, callback);
      if (std::size_t usedSize = result->GetMatchedDataSize())
      {
        Dbg("Detected %1% in %2% bytes at %3%.", plugin->GetDescription()->Id(), usedSize, location->GetPath()->AsString());
        return usedSize;
      }
    }
    return 0;
  }

  class MixedPropertiesHolder : public Module::Holder
  {
  public:
    MixedPropertiesHolder(Module::Holder::Ptr delegate, Parameters::Accessor::Ptr props)
      : Delegate(delegate)
      , Properties(props)
    {
    }

    virtual Plugin::Ptr GetPlugin() const
    {
      return Delegate->GetPlugin();
    }

    virtual Module::Information::Ptr GetModuleInformation() const
    {
      return Delegate->GetModuleInformation();
    }

    virtual Parameters::Accessor::Ptr GetModuleProperties() const
    {
      return Parameters::CreateMergedAccessor(Properties, Delegate->GetModuleProperties());
    }

    virtual Module::Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::MultichannelReceiver::Ptr target) const
    {
      return Delegate->CreateRenderer(Parameters::CreateMergedAccessor(params, Properties), target);
    }

    virtual Binary::Data::Ptr Convert(const Module::Conversion::Parameter& spec, Parameters::Accessor::Ptr params) const
    {
      return Delegate->Convert(spec, Parameters::CreateMergedAccessor(params, Properties));
    }
  private:
    const Module::Holder::Ptr Delegate;
    const Parameters::Accessor::Ptr Properties;
  };
}

namespace ZXTune
{
  namespace Module
  {
    Holder::Ptr Open(DataLocation::Ptr location)
    {
      const PlayerPluginsEnumerator::Ptr usedPlugins = PlayerPluginsEnumerator::Create();
      const OpenModuleCallback callback;
      DetectByPlugins<PlayerPlugin>(usedPlugins->Enumerate(), location, callback);
      return callback.GetResult();
    }

    std::size_t Detect(DataLocation::Ptr location, const DetectCallback& callback)
    {
      if (std::size_t usedSize = DetectByPlugins<ArchivePlugin>(ArchivePluginsEnumerator::Create()->Enumerate(), location, callback))
      {
        return usedSize;
      }
      return DetectByPlugins<PlayerPlugin>(PlayerPluginsEnumerator::Create()->Enumerate(), location, callback);
    }

    Holder::Ptr CreateMixedPropertiesHolder(Holder::Ptr delegate, Parameters::Accessor::Ptr props)
    {
      return boost::make_shared<MixedPropertiesHolder>(delegate, props);
    }
  }
}
