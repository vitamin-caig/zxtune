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
#include "container_supp_common.h"
#include "core/src/callback.h"
#include "core/plugins/plugins_types.h"
#include "core/plugins/utils.h"
//library includes
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <l10n/api.h>
#include <strings/format.h>
//boost includes
#include <boost/make_shared.hpp>
//text includes
#include <core/text/core.h>

namespace
{
  const Debug::Stream Dbg("Core::ArchivesSupp");
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("core");
}

namespace ZXTune
{
  class LoggerHelper
  {
  public:
    LoggerHelper(uint_t total, const Module::DetectCallback& delegate, const String& plugin, const String& path)
      : Total(total)
      , Delegate(delegate)
      , Progress(CreateProgressCallback(delegate, Total))
      , Id(plugin)
      , Path(path)
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

  class ContainerDetectCallback : public Formats::Archived::Container::Walker
  {
  public:
    ContainerDetectCallback(const Parameters::Accessor& params, std::size_t maxSize, const String& plugin, DataLocation::Ptr location, uint_t count, const Module::DetectCallback& callback)
      : Params(params)
      , MaxSize(maxSize)
      , BaseLocation(location)
      , SubPlugin(plugin)
      , Logger(count, callback, SubPlugin, BaseLocation->GetPath()->AsString())
    {
    }

    virtual void OnFile(const Formats::Archived::File& file) const
    {
      const String& name = file.GetName();
      const std::size_t size = file.GetSize();
      if (size <= MaxSize)
      {
        ProcessFile(file);
      }
      else
      {
        Dbg("'%1%' is too big (%1%). Skipping.", name, size);
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
        const std::auto_ptr<Module::DetectCallback> nestedProgressCallback = Logger.CreateNestedCallback();
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
      : Description(descr)
      , Decoder(decoder)
      , SupportDirectories(0 != (Description->Capabilities() & Capabilities::Container::Traits::DIRECTORIES))
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Description;
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Decoder->GetFormat();
    }

    virtual Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr input, const Module::DetectCallback& callback) const
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

    virtual DataLocation::Ptr Open(const Parameters::Accessor& /*params*/, DataLocation::Ptr location, const Analysis::Path& inPath) const
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
        Dbg("Trying '%1%'", filename);
        if (Formats::Archived::File::Ptr file = container.FindFile(filename))
        {
          Dbg("Found");
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
      : Delegate(delegate)
      , Id(Delegate->GetDescription()->Id())
    {
    }

    virtual Plugin::Ptr GetDescription() const
    {
      return Delegate->GetDescription();
    }

    virtual Binary::Format::Ptr GetFormat() const
    {
      return Delegate->GetFormat();
    }

    virtual Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData, const Module::DetectCallback& callback) const
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

    virtual DataLocation::Ptr Open(const Parameters::Accessor& params, DataLocation::Ptr inputData, const Analysis::Path& pathToOpen) const
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
  ArchivePlugin::Ptr CreateContainerPlugin(const String& id, uint_t caps, Formats::Archived::Decoder::Ptr decoder)
  {
    const Plugin::Ptr description = CreatePluginDescription(id, decoder->GetDescription(), caps | Capabilities::Category::CONTAINER);
    const ArchivePlugin::Ptr result = boost::make_shared<ArchivedContainerPlugin>(description, decoder);
    return 0 != (caps & Capabilities::Container::Traits::ONCEAPPLIED)
      ? boost::make_shared<OnceAppliedContainerPluginAdapter>(result)
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
