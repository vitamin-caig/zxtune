/**
 *
 * @file
 *
 * @brief  Container plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/archives/archived.h"
#include "core/plugins/archives/l10n.h"
#include "core/plugins/plugins_types.h"
#include "core/plugins/utils.h"
#include "core/src/callback.h"
// common includes
#include <make_ptr.h>
// library includes
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <module/attributes.h>
#include <strings/format.h>

namespace ZXTune
{
  const Debug::Stream ArchivedDbg("Core::ArchivesSupp");

  class LoggerHelper
  {
  public:
    LoggerHelper(uint_t total, Log::ProgressCallback* delegate, String plugin, String path)
      : Delegate(path.empty() ? delegate : nullptr)  // track only toplevel container
      , ToPercent(total, 100)
      , Id(std::move(plugin))
      , Path(std::move(path))
    {}

    void Report(const String& name)
    {
      if (Delegate)
      {
        const String text = ProgressMessage(Id, Path, name);
        Delegate->OnProgress(ToPercent(++Current), text);
      }
    }

  private:
    Log::ProgressCallback* const Delegate;
    const Math::ScaleFunctor<uint_t> ToPercent;
    const String Id;
    const String Path;
    uint_t Current = 0;
  };

  class ContainerDetectCallback : public Formats::Archived::Container::Walker
  {
  public:
    ContainerDetectCallback(std::size_t maxSize, String plugin, DataLocation::Ptr location, uint_t count,
                            ArchiveCallback& callback)
      : MaxSize(maxSize)
      , BaseLocation(std::move(location))
      , SubPlugin(std::move(plugin))
      , Logger(count, callback.GetProgress(), SubPlugin, BaseLocation->GetPath()->AsString())
      , Callback(callback)
    {}

    void OnFile(const Formats::Archived::File& file) const override
    {
      const auto& name = file.GetName();
      const std::size_t size = file.GetSize();
      if (size <= MaxSize)
      {
        ProcessFile(file);
      }
      else
      {
        ArchivedDbg("'%1%' is too big (%1%). Skipping.", name, size);
      }
      Logger.Report(name);
    }

  private:
    void ProcessFile(const Formats::Archived::File& file) const
    {
      if (auto subData = file.GetData())
      {
        const auto subPath = file.GetName();
        auto subLocation = CreateNestedLocation(BaseLocation, std::move(subData), SubPlugin, subPath);
        Callback.ProcessData(std::move(subLocation));
      }
    }

  private:
    const std::size_t MaxSize;
    const DataLocation::Ptr BaseLocation;
    const String SubPlugin;
    mutable LoggerHelper Logger;
    ArchiveCallback& Callback;
  };

  class ArchivedContainerPlugin : public ArchivePlugin
  {
  public:
    ArchivedContainerPlugin(Plugin::Ptr descr, Formats::Archived::Decoder::Ptr decoder)
      : Description(std::move(descr))
      , Decoder(std::move(decoder))
      , SupportDirectories(0 != (Description->Capabilities() & Capabilities::Container::Traits::DIRECTORIES))
    {}

    Plugin::Ptr GetDescription() const override
    {
      return Description;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& /*params*/, DataLocation::Ptr input,
                                 ArchiveCallback& callback) const override
    {
      const auto rawData = input->GetData();
      if (const auto archive = Decoder->Decode(*rawData))
      {
        if (const auto count = archive->CountFiles())
        {
          ContainerDetectCallback detect(~std::size_t(0), Description->Id(), input, count, callback);
          archive->ExploreFiles(detect);
        }
        return Analysis::CreateMatchedResult(archive->Size());
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), rawData);
    }

    DataLocation::Ptr TryOpen(const Parameters::Accessor& /*params*/, DataLocation::Ptr location,
                              const Analysis::Path& inPath) const override
    {
      const auto rawData = location->GetData();
      if (const auto archive = Decoder->Decode(*rawData))
      {
        if (const auto fileToOpen = FindFile(*archive, inPath))
        {
          if (auto subData = fileToOpen->GetData())
          {
            return CreateNestedLocation(std::move(location), std::move(subData), Description->Id(),
                                        fileToOpen->GetName());
          }
        }
      }
      return {};
    }

  private:
    Formats::Archived::File::Ptr FindFile(const Formats::Archived::Container& container,
                                          const Analysis::Path& path) const
    {
      auto resolved = Analysis::ParsePath(String(), Module::SUBPATH_DELIMITER);
      for (const auto components = path.GetIterator(); components->IsValid(); components->Next())
      {
        resolved = resolved->Append(components->Get());
        const String filename = resolved->AsString();
        ArchivedDbg("Trying '%1%'", filename);
        if (auto file = container.FindFile(filename))
        {
          ArchivedDbg("Found");
          return file;
        }
        if (!SupportDirectories)
        {
          break;
        }
      }
      return {};
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
    {}

    Plugin::Ptr GetDescription() const override
    {
      return Delegate->GetDescription();
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Delegate->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 ArchiveCallback& callback) const override
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

    DataLocation::Ptr TryOpen(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                              const Analysis::Path& pathToOpen) const override
    {
      if (SelfIsVisited(*inputData->GetPluginsChain()))
      {
        return {};
      }
      else
      {
        return Delegate->TryOpen(params, inputData, pathToOpen);
      }
    }

  private:
    bool SelfIsVisited(const Analysis::Path& path) const
    {
      for (auto it = path.GetIterator(); it->IsValid(); it->Next())
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
}  // namespace ZXTune

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(const String& id, uint_t caps, Formats::Archived::Decoder::Ptr decoder)
  {
    auto description = CreatePluginDescription(id, decoder->GetDescription(), caps | Capabilities::Category::CONTAINER);
    auto result = MakePtr<ArchivedContainerPlugin>(description, decoder);
    return 0 != (caps & Capabilities::Container::Traits::ONCEAPPLIED)
               ? MakePtr<OnceAppliedContainerPluginAdapter>(std::move(result))
               : result;
  }

  String ProgressMessage(const String& id, const String& path)
  {
    return path.empty() ? Strings::Format(translate("%1% processing"), id)
                        : Strings::Format(translate("%1% processing at %2%"), id, path);
  }

  String ProgressMessage(const String& id, const String& path, const String& element)
  {
    return path.empty() ? Strings::Format(translate("%1% processing for %2%"), id, element)
                        : Strings::Format(translate("%1% processing for %2% at %3%"), id, element, path);
  }
}  // namespace ZXTune
