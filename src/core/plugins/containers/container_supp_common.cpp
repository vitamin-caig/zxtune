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
//library includes
#include <core/plugin_attrs.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/plugins.h>

namespace
{
  using namespace ZXTune;

  const std::string THIS_MODULE("Core::ArchivesSupp");

  class LoggerHelper
  {
  public:
    LoggerHelper(uint_t total, const Module::DetectCallback& delegate, const Plugin& plugin, const String& path)
      : Total(total)
      , Delegate(delegate)
      , Progress(CreateProgressCallback(delegate, Total))
      , Id(plugin.Id())
      , Path(path)
      , Current()
    {
    }

    template<class T>
    void operator()(const T& cur)
    {
      if (Progress.get())
      {
        const String text = Path.empty()
          ? Strings::Format(Text::CONTAINER_PLUGIN_PROGRESS_NOPATH, Id, cur.GetName())
          : Strings::Format(Text::CONTAINER_PLUGIN_PROGRESS, Id, cur.GetName(), Path);
        Progress->OnProgress(Current, text);
      }
    }

    std::auto_ptr<Module::DetectCallback> CreateNestedCallback() const
    {
      Log::ProgressCallback* const parentProgress = Delegate.GetProgress();
      if (parentProgress && Total < 50)
      {
        Log::ProgressCallback::Ptr nestedProgress = CreateNestedPercentProgressCallback(Total, Current, *parentProgress);
        return std::auto_ptr<Module::DetectCallback>(new Module::CustomProgressDetectCallbackAdapter(Delegate, nestedProgress));
      }
      else
      {
        return std::auto_ptr<Module::DetectCallback>(new Module::CustomProgressDetectCallbackAdapter(Delegate));
      }
    }

    void Next()
    {
      ++Current;
    }
  private:
    const uint_t Total;
    const Module::DetectCallback& Delegate;
    const Log::ProgressCallback::Ptr Progress;
    const String Id;
    const String Path;
    uint_t Current;
  };

  class ContainerDetectCallback : public Container::Catalogue::Callback
                                , public Formats::Archived::Container::Walker
  {
  public:
    ContainerDetectCallback(std::size_t maxSize, Plugin::Ptr descr, DataLocation::Ptr location, uint_t count, const Module::DetectCallback& callback)
      : MaxSize(maxSize)
      , BaseLocation(location)
      , Description(descr)
      , Logger(count, callback, *Description, BaseLocation->GetPath()->AsString())
    {
    }

    void OnFile(const Container::File& file) const
    {
      ProcessFile(file);
    }

    void OnFile(const Formats::Archived::File& file) const
    {
      const String& name = file.GetName();
      const std::size_t size = file.GetSize();
      if (size <= MaxSize)
      {
        ProcessFile(file);
      }
      else
      {
        Log::Debug(THIS_MODULE, "'%1%' is too big (%1%). Skipping.", name, size);
      }
    }
  private:
    template<class Type>
    void ProcessFile(const Type& file) const
    {
      Logger(file);
      if (const Binary::Container::Ptr subData = file.GetData())
      {
        const String subPath = file.GetName();
        const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(BaseLocation, subData, Description, subPath);
        const std::auto_ptr<Module::DetectCallback> nestedProgressCallback = Logger.CreateNestedCallback();
        ZXTune::Module::Detect(subLocation, *nestedProgressCallback);
      }
    }
  private:
    const std::size_t MaxSize;
    const DataLocation::Ptr BaseLocation;
    const Plugin::Ptr Description;
    mutable LoggerHelper Logger;
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
      const Binary::Container::Ptr rawData = input->GetData();
      if (const Container::Catalogue::Ptr files = Factory->CreateContainer(*callback.GetPluginsParameters(), rawData))
      {
        if (const uint_t count = files->GetFilesCount())
        {
          ContainerDetectCallback detect(~std::size_t(0), Description, input, count, callback);
          files->ForEachFile(detect);
        }
        return DetectionResult::CreateMatched(files->GetSize());
      }
      return DetectionResult::CreateUnmatched(Factory->GetFormat(), rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& commonParams, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const Binary::Container::Ptr inData = location->GetData();
      if (const Container::Catalogue::Ptr files = Factory->CreateContainer(commonParams, inData))
      {
        if (const Container::File::Ptr fileToOpen = files->FindFile(inPath))
        {
          if (const Binary::Container::Ptr subData = fileToOpen->GetData())
          {
            return CreateNestedLocation(location, subData, Description, fileToOpen->GetName());
          }
        }
      }
      return DataLocation::Ptr();
    }
  private:
    const Plugin::Ptr Description;
    const ContainerFactory::Ptr Factory;
  };

  class ArchivedContainerPlugin : public ArchivePlugin
  {
  public:
    ArchivedContainerPlugin(Plugin::Ptr descr, Formats::Archived::Decoder::Ptr decoder)
      : Description(descr)
      , Decoder(decoder)
      , SupportDirectories(0 != (Description->Capabilities() & CAP_STOR_DIRS))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual DetectionResult::Ptr Detect(DataLocation::Ptr input, const Module::DetectCallback& callback) const
    {
      const Binary::Container::Ptr rawData = input->GetData();
      if (const Formats::Archived::Container::Ptr archive = Decoder->Decode(*rawData))
      {
        if (const uint_t count = archive->CountFiles())
        {
          ContainerDetectCallback detect(~std::size_t(0), Description, input, count, callback);
          archive->ExploreFiles(detect);
        }
        return DetectionResult::CreateMatched(archive->Size());
      }
      return DetectionResult::CreateUnmatched(Decoder->GetFormat(), rawData);
    }

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*commonParams*/, DataLocation::Ptr location, const DataPath& inPath) const
    {
      const Binary::Container::Ptr rawData = location->GetData();
      if (const Formats::Archived::Container::Ptr archive = Decoder->Decode(*rawData))
      {
        if (const Formats::Archived::File::Ptr fileToOpen = FindFile(*archive, inPath))
        {
          if (const Binary::Container::Ptr subData = fileToOpen->GetData())
          {
            return CreateNestedLocation(location, subData, Description, fileToOpen->GetName());
          }
        }
      }
      return DataLocation::Ptr();
    }
  private:
    Formats::Archived::File::Ptr FindFile(const Formats::Archived::Container& container, const DataPath& path) const
    {
      const String inPath = path.AsString();
      const String firstComponent = path.GetFirstComponent();
      if (inPath == firstComponent || !SupportDirectories)
      {
        return container.FindFile(firstComponent);
      }
      Log::Debug(THIS_MODULE, "Resolving '%1%'", inPath);
      DataPath::Ptr resolved = CreateDataPath(firstComponent);
      for (;;)
      {
        const String filename = resolved->AsString();
        Log::Debug(THIS_MODULE, "Trying '%1%'", filename);
        if (Formats::Archived::File::Ptr file = container.FindFile(filename))
        {
          Log::Debug(THIS_MODULE, "Found");
          return file;
        }
        if (filename == inPath)
        {
          return Formats::Archived::File::Ptr();
        }
        const DataPath::Ptr unresolved = SubstractDataPath(path, *resolved);
        resolved = CreateMergedDataPath(resolved, unresolved->GetFirstComponent());
      }
    }
  private:
    const Plugin::Ptr Description;
    const Formats::Archived::Decoder::Ptr Decoder;
    const bool SupportDirectories;
  };
}

namespace ZXTune
{
  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, const String& info, uint_t caps,
    ContainerFactory::Ptr factory)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, info, caps);
    return ArchivePlugin::Ptr(new CommonContainerPlugin(description, factory));
  }

  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, const String& info, uint_t caps, Formats::Archived::Decoder::Ptr decoder)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, info, caps);
    return ArchivePlugin::Ptr(new ArchivedContainerPlugin(description, decoder));
  }
}
