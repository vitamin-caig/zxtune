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
  
  class DataProcessorImpl : public DataProcessor
  {
  public:
    DataProcessorImpl(DataLocation::Ptr location, const Module::DetectCallback& callback)
      : Location(location)
      , Data(Location->GetData())
      , Callback(callback)
      , UsedPlugins(Callback.GetUsedPlugins())
      , Params(Callback.GetPluginsParameters())
    {
    }

    virtual std::size_t ProcessContainers() const
    {
      for (ContainerPlugin::Iterator::Ptr iter = UsedPlugins->EnumerateContainers(); iter->IsValid(); iter->Next())
      {
        const ContainerPlugin::Ptr plugin = iter->Get();
        const DetectionResult::Ptr result = plugin->Detect(Location, Callback);
        if (std::size_t usedSize = result->GetMatchedDataSize())
        {
          Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", plugin->Id(), usedSize, Location->GetPath());
          return usedSize;
        }
      }
      return 0;
    }

    virtual std::size_t ProcessArchives() const
    {
      for (ArchivePlugin::Iterator::Ptr iter = UsedPlugins->EnumerateArchives(); iter->IsValid(); iter->Next())
      {
        const ArchivePlugin::Ptr plugin = iter->Get();
        const DetectionResult::Ptr result = plugin->Detect(Location, Callback);
        if (std::size_t usedSize = result->GetMatchedDataSize())
        {
          Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", plugin->Id(), usedSize, Location->GetPath());
          return usedSize;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return 0;
    }

    virtual std::size_t ProcessModules() const
    {
      for (PlayerPlugin::Iterator::Ptr plugins = UsedPlugins->EnumeratePlayers(); plugins->IsValid(); plugins->Next())
      {
        const PlayerPlugin::Ptr plugin = plugins->Get();
        const DetectionResult::Ptr result = plugin->Detect(Location, Callback);
        if (std::size_t usedSize = result->GetMatchedDataSize())
        {
          Log::Debug(THIS_MODULE, "Detected %1% in %2% bytes at %3%.", plugin->Id(), usedSize, Location->GetPath());
          return usedSize;
        }
        //TODO: dispatch heavy checks- return false if not enabled
      }
      return 0;
    }
  private:
    const DataLocation::Ptr Location;
    const IO::DataContainer::Ptr Data;
    const Module::DetectCallback& Callback;
    const PluginsEnumerator::Ptr UsedPlugins;
    const Parameters::Accessor::Ptr Params;
  };
}

namespace ZXTune
{
  DataProcessor::Ptr DataProcessor::Create(DataLocation::Ptr location, const Module::DetectCallback& callback)
  {
    return boost::make_shared<DataProcessorImpl>(location, callback);
  }

  namespace Module
  {
    Holder::Ptr Open(DataLocation::Ptr location, PluginsEnumerator::Ptr usedPlugins, Parameters::Accessor::Ptr moduleParams)
    {
      const OpenModuleCallback callback(usedPlugins, moduleParams);
      const DataProcessorImpl detector(location, callback);
      detector.ProcessModules();
      return callback.GetResult();
    }

    std::size_t Detect(DataLocation::Ptr location, const DetectCallback& callback)
    {
      const DataProcessorImpl detector(location, callback);
      return Detect(detector);
    }

    std::size_t Detect(const DataProcessor& detector)
    {
      if (std::size_t usedSize = detector.ProcessContainers())
      {
        return usedSize;
      }
      if (std::size_t usedSize = detector.ProcessArchives())
      {
        return usedSize;
      }
      return detector.ProcessModules();
    }
  }
}
