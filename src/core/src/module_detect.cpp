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
#include <error.h>
#include <logging.h>
//library includes
#include <io/fs_tools.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <src/core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::Detection");

  const String ARCHIVE_PLUGIN_PREFIX(Text::ARCHIVE_PLUGIN_PREFIX);

  String EncodeArchivePluginToPath(const String& pluginId)
  {
    return ARCHIVE_PLUGIN_PREFIX + pluginId;
  }

  class OpenModuleCallback : public Module::DetectCallback
  {
  public:
    OpenModuleCallback(PluginsEnumerator::Ptr usedPlugins, Parameters::Accessor::Ptr moduleParams)
      : Plugins(usedPlugins)
      , ModuleParams(moduleParams)
    {
    }

    virtual PluginsEnumerator::Ptr GetUsedPlugins() const
    {
      return Plugins;
    }

    virtual Parameters::Accessor::Ptr GetPluginsParameters() const
    {
      return Parameters::Container::Create();
    }

    virtual Parameters::Accessor::Ptr CreateModuleParameters(const DataLocation& /*location*/) const
    {
      return ModuleParams;
    }

    virtual Error ProcessModule(const DataLocation& /*location*/, Module::Holder::Ptr holder) const
    {
      Result = holder;
      return Error();
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
    const PluginsEnumerator::Ptr Plugins;
    const Parameters::Accessor::Ptr ModuleParams;
    mutable Module::Holder::Ptr Result;
  };
  
  template<class Type>
  std::size_t DetectByPlugins(typename Type::Iterator::Ptr plugins, DataLocation::Ptr location, const Module::DetectCallback& callback)
  {
    for (; plugins->IsValid(); plugins->Next())
    {
      const typename Type::Ptr plugin = plugins->Get();
      const DetectionResult::Ptr result = plugin->Detect(location, callback);
      if (std::size_t usedSize = result->GetMatchedDataSize())
      {
        Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", plugin->Id(), usedSize, location->GetPath());
        return usedSize;
      }
    }
    return 0;
  }
}

namespace ZXTune
{
  namespace Module
  {
    Holder::Ptr Open(DataLocation::Ptr location, PluginsEnumerator::Ptr usedPlugins, Parameters::Accessor::Ptr moduleParams)
    {
      const OpenModuleCallback callback(usedPlugins, moduleParams);
      Detect(location, callback);
      return callback.GetResult();
    }

    std::size_t Detect(DataLocation::Ptr location, const DetectCallback& callback)
    {
      const PluginsEnumerator::Ptr usedPlugins = callback.GetUsedPlugins();
      if (std::size_t usedSize = DetectByPlugins<ContainerPlugin>(usedPlugins->EnumerateContainers(), location, callback))
      {
        return usedSize;
      }
      if (std::size_t usedSize = DetectByPlugins<ArchivePlugin>(usedPlugins->EnumerateArchives(), location, callback))
      {
        return usedSize;
      }
      return DetectByPlugins<PlayerPlugin>(usedPlugins->EnumeratePlayers(), location, callback);
    }
  }
}
