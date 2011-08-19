/*
Abstract:
  Common container plugins support implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "container_supp_common.h"
#include "core/src/callback.h"
#include "core/src/core.h"
#include "core/plugins/utils.h"
//common includes
#include <format.h>
#include <logging.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  class LoggerHelper
  {
  public:
    LoggerHelper(Log::ProgressCallback* callback, const Plugin& plugin, const String& path)
      : Callback(callback)
      , Id(plugin.Id())
      , Path(path)
      , Current()
    {
    }

    void operator()(const Container::File& cur)
    {
      if (Callback)
      {
        const String text = Path.empty()
          ? Strings::Format(Text::CONTAINER_PLUGIN_PROGRESS_NOPATH, Id, cur.GetName())
          : Strings::Format(Text::CONTAINER_PLUGIN_PROGRESS, Id, cur.GetName(), Path);
        Callback->OnProgress(Current++, text);
      }
    }
  private:
    Log::ProgressCallback* const Callback;
    const String Id;
    const String Path;
    uint_t Current;
  };

  class CommonContainerPlugin : public ArchivePlugin
  {
  public:
    CommonContainerPlugin(Plugin::Ptr descr, ContainerFactory::Ptr factory)
      : Description(descr)
      , Factory(factory)
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const IO::DataContainer::Ptr rawData = input->GetData();
      if (const Container::Catalogue::Ptr files = Factory->CreateContainer(*callback.GetPluginsParameters(), rawData))
      {
        if (const uint_t count = files->GetFilesCount())
        {
          const Log::ProgressCallback::Ptr progress = CreateProgressCallback(callback, count);
          LoggerHelper logger(progress.get(), *Description, input->GetPath()->AsString());
          const ZXTune::Module::NoProgressDetectCallbackAdapter noProgressCallback(callback);
          for (Container::Catalogue::Iterator::Ptr it = files->GetFiles(); it->IsValid(); it->Next())
          {
            const Container::File::Ptr file = it->Get();
            logger(*file);
            if (const IO::DataContainer::Ptr subData = file->GetData())
            {
              const String subPath = file->GetName();
              const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(input, subData, Description, subPath);
              ZXTune::Module::Detect(subLocation, noProgressCallback);
            }
          }
        }
        return DetectionResult::CreateMatched(files->GetUsedSize());
      }
      return DetectionResult::CreateUnmatched(Factory->GetFormat(), rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& commonParams, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const String& pathComp = inPath.GetFirstComponent();
      if (pathComp.empty())
      {
        return DataLocation::Ptr();
      }
      const IO::DataContainer::Ptr inData = location->GetData();
      if (const Container::Catalogue::Ptr files = Factory->CreateContainer(commonParams, inData))
      {
        if (const Container::File::Ptr fileToOpen = files->FindFile(pathComp))
        {
          const IO::DataContainer::Ptr subData = fileToOpen->GetData();
          return CreateNestedLocation(location, subData, Description, pathComp); 
        }
      }
      return DataLocation::Ptr();
    }
  private:
    const Plugin::Ptr Description;
    const ContainerFactory::Ptr Factory;
  };
}

namespace ZXTune
{
  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, const String& info, const String& version, uint_t caps,
    ContainerFactory::Ptr factory)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, info, version, caps);
    return ArchivePlugin::Ptr(new CommonContainerPlugin(description, factory));
  }
}
