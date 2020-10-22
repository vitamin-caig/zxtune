/**
* 
* @file
*
* @brief  Container plugin implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/archives/archived.h"
#include "core/src/callback.h"
#include "core/src/detect.h"
#include "core/plugins/archives/l10n.h"
#include "core/plugins/plugins_types.h"
#include "core/plugins/utils.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <strings/format.h>
//text includes
#include <core/text/core.h>

namespace ZXTune
{
  const Debug::Stream ArchivedDbg("Core::ArchivesSupp");

  class LoggerHelper
  {
  public:
    LoggerHelper(uint_t total, Module::DetectCallback& delegate, String plugin, String path)
      : Total(total)
      , Delegate(delegate)
      , Progress(CreateProgressCallback(delegate, Total))
      , Id(std::move(plugin))
      , Path(std::move(path))
      , Current()
    {
    }

    void operator()(const Formats::Archived::File& cur)
    {
      if (Progress.get())
      {
        const String text = ProgressMessage(Id, Path, cur.GetName());
        Progress->OnProgress(Current, text);
      }
    }

    std::unique_ptr<Module::DetectCallback> CreateNestedCallback() const
    {
      Log::ProgressCallback* const parentProgress = Delegate.GetProgress();
      if (parentProgress && Total < 50)
      {
        Log::ProgressCallback::Ptr nestedProgress = CreateNestedPercentProgressCallback(Total, Current, *parentProgress);
        return std::unique_ptr<Module::DetectCallback>(new Module::CustomProgressDetectCallbackAdapter(Delegate, std::move(nestedProgress)));
      }
      else
      {
        return std::unique_ptr<Module::DetectCallback>(new Module::CustomProgressDetectCallbackAdapter(Delegate));
      }
    }

    void Next()
    {
      ++Current;
    }
  private:
    const uint_t Total;
    Module::DetectCallback& Delegate;
    const Log::ProgressCallback::Ptr Progress;
    const String Id;
    const String Path;
    uint_t Current;
  };

  class ContainerDetectCallback : public Formats::Archived::Container::Walker
  {
  public:
    ContainerDetectCallback(const Parameters::Accessor& params, std::size_t maxSize, String plugin, DataLocation::Ptr location, uint_t count, Module::DetectCallback& callback)
      : Params(params)
      , MaxSize(maxSize)
      , BaseLocation(std::move(location))
      , SubPlugin(std::move(plugin))
      , Logger(count, callback, SubPlugin, BaseLocation->GetPath()->AsString())
    {
    }

    void OnFile(const Formats::Archived::File& file) const override
    {
      const String& name = file.GetName();
      const std::size_t size = file.GetSize();
      if (size <= MaxSize)
      {
        ProcessFile(file);
      }
      else
      {
        ArchivedDbg("'%1%' is too big (%1%). Skipping.", name, size);
      }
    }
  private:
    void ProcessFile(const Formats::Archived::File& file) const
    {
      Logger(file);
      if (const Binary::Container::Ptr subData = file.GetData())
      {
        const String subPath = file.GetName();
        const ZXTune::DataLocation::Ptr subLocation = CreateNestedLocation(BaseLocation, subData, SubPlugin, subPath);
        const std::unique_ptr<Module::DetectCallback> nestedProgressCallback = Logger.CreateNestedCallback();
        Module::Detect(Params, subLocation, *nestedProgressCallback);
      }
      Logger.Next();
    }
  private:
    const Parameters::Accessor& Params;
    const std::size_t MaxSize;
    const DataLocation::Ptr BaseLocation;
    const String SubPlugin;
    mutable LoggerHelper Logger;
  };

  class ArchivedContainerPlugin : public ArchivePlugin
  {
  public:
    ArchivedContainerPlugin(Plugin::Ptr descr, Formats::Archived::Decoder::Ptr decoder)
      : Description(std::move(descr))
      , Decoder(std::move(decoder))
      , SupportDirectories(0 != (Description->Capabilities() & Capabilities::Container::Traits::DIRECTORIES))
    {
    }

    Plugin::Ptr GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr input, Module::DetectCallback& callback) const override
    {
      const Binary::Container::Ptr rawData = input->GetData();
      if (const Formats::Archived::Container::Ptr archive = Decoder->Decode(*rawData))
      {
        if (const uint_t count = archive->CountFiles())
        {
          ContainerDetectCallback detect(params, ~std::size_t(0), Description->Id(), input, count, callback);
          archive->ExploreFiles(detect);
        }
        return Analysis::CreateMatchedResult(archive->Size());
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), rawData);
    }

    DataLocation::Ptr Open(const Parameters::Accessor& /*params*/, DataLocation::Ptr location, const Analysis::Path& inPath) const override
    {
      const Binary::Container::Ptr rawData = location->GetData();
      if (const Formats::Archived::Container::Ptr archive = Decoder->Decode(*rawData))
      {
        if (const Formats::Archived::File::Ptr fileToOpen = FindFile(*archive, inPath))
        {
          if (const Binary::Container::Ptr subData = fileToOpen->GetData())
          {
            return CreateNestedLocation(location, subData, Description->Id(), fileToOpen->GetName());
          }
        }
      }
      return DataLocation::Ptr();
    }
  private:
    Formats::Archived::File::Ptr FindFile(const Formats::Archived::Container& container, const Analysis::Path& path) const
    {
      Analysis::Path::Ptr resolved = Analysis::ParsePath(String(), Text::MODULE_SUBPATH_DELIMITER[0]);
      for (const Analysis::Path::Iterator::Ptr components = path.GetIterator();
           components->IsValid(); components->Next())
      {
        resolved = resolved->Append(components->Get());
        const String filename = resolved->AsString();
        ArchivedDbg("Trying '%1%'", filename);
        if (Formats::Archived::File::Ptr file = container.FindFile(filename))
        {
          ArchivedDbg("Found");
          return file;
        }
        if (!SupportDirectories)
        {
          break;
        }
      }
      return Formats::Archived::File::Ptr();
    }
  private:
    const Plugin::Ptr Description;
    const Formats::Archived::Decoder::Ptr Decoder;
    const bool SupportDirectories;
  };

  class OnceAppliedContainerPluginAdapter : public ArchivePlugin
  {
  public:
    explicit OnceAppliedContainerPluginAdapter(ArchivePlugin::Ptr delegate)
      : Delegate(std::move(delegate))
      , Id(Delegate->GetDescription()->Id())
    {
    }

    Plugin::Ptr GetDescription() const override
    {
      return Delegate->GetDescription();
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Delegate->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData, Module::DetectCallback& callback) const override
    {
      if (SelfIsVisited(*inputData->GetPluginsChain()))
      {
        return Analysis::CreateUnmatchedResult(inputData->GetData()->Size());
      }
      else
      {
        return Delegate->Detect(params, inputData, callback);
      }
    }

    DataLocation::Ptr Open(const Parameters::Accessor& params, DataLocation::Ptr inputData, const Analysis::Path& pathToOpen) const override
    {
      if (SelfIsVisited(*inputData->GetPluginsChain()))
      {
        return DataLocation::Ptr();
      }
      else
      {
        return Delegate->Open(params, inputData, pathToOpen);
      }
    }
  private:
    bool SelfIsVisited(const Analysis::Path& path) const
    {
      for (Analysis::Path::Iterator::Ptr it = path.GetIterator(); it->IsValid(); it->Next())
      {
        if (it->Get() == Id)
        {
          return true;
        }
      }
      return false;
    }
  private:
    const ArchivePlugin::Ptr Delegate;
    const String Id;
  };
}

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(const String& id, uint_t caps, Formats::Archived::Decoder::Ptr decoder)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, decoder->GetDescription(), caps | Capabilities::Category::CONTAINER);
    const ArchivePlugin::Ptr result = MakePtr<ArchivedContainerPlugin>(description, decoder);
    return 0 != (caps & Capabilities::Container::Traits::ONCEAPPLIED)
      ? MakePtr<OnceAppliedContainerPluginAdapter>(result)
      : result;
  }

  String ProgressMessage(const String& id, const String& path)
  {
    return path.empty()
      ? Strings::Format(translate("%1% processing"), id)
      : Strings::Format(translate("%1% processing at %2%"), id, path);
  }

  String ProgressMessage(const String& id, const String& path, const String& element)
  {
    return path.empty()
      ? Strings::Format(translate("%1% processing for %2%"), id, element)
      : Strings::Format(translate("%1% processing for %2% at %3%"), id, element, path);
  }
}
