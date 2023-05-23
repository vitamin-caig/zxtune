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
// common includes
#include <make_ptr.h>
#include <progress_callback.h>
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
    LoggerHelper(uint_t total, Log::ProgressCallback* delegate, PluginId plugin, StringView path)
      : Delegate(path.empty() ? delegate : nullptr)  // track only toplevel container
      , ToPercent(total, 100)
      , Id(plugin)
      , Path(path.to_string())
    {}

    void Report(StringView name)
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
    const PluginId Id;
    const String Path;
    uint_t Current = 0;
  };

  class ContainerDetectCallback : public Formats::Archived::Container::Walker
  {
  public:
    ContainerDetectCallback(std::size_t maxSize, PluginId plugin, DataLocation::Ptr location, uint_t count,
                            ArchiveCallback& callback)
      : MaxSize(maxSize)
      , BaseLocation(std::move(location))
      , SubPlugin(plugin)
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
        ArchivedDbg("'{}' is too big ({}). Skipping.", name, size);
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
    const PluginId SubPlugin;
    mutable LoggerHelper Logger;
    ArchiveCallback& Callback;
  };

  class ArchivedContainerPlugin : public ArchivePlugin
  {
  public:
    ArchivedContainerPlugin(PluginId id, uint_t caps, Formats::Archived::Decoder::Ptr decoder)
      : Identifier(id)
      , Caps(caps)
      , Decoder(std::move(decoder))
    {}

    PluginId Id() const override
    {
      return Identifier;
    }

    String Description() const override
    {
      return Decoder->GetDescription();
    }

    uint_t Capabilities() const override
    {
      return Caps;
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
          ContainerDetectCallback detect(~std::size_t(0), Identifier, input, count, callback);
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
            return CreateNestedLocation(std::move(location), std::move(subData), Identifier, fileToOpen->GetName());
          }
        }
      }
      return {};
    }

  private:
    bool SupportDirectories() const
    {
      return 0 != (Caps & Capabilities::Container::Traits::DIRECTORIES);
    }

    Formats::Archived::File::Ptr FindFile(const Formats::Archived::Container& container,
                                          const Analysis::Path& path) const
    {
      auto resolved = Analysis::ParsePath(String(), Module::SUBPATH_DELIMITER);
      for (const auto components = path.GetIterator(); components->IsValid(); components->Next())
      {
        resolved = resolved->Append(components->Get());
        const String filename = resolved->AsString();
        ArchivedDbg("Trying '{}'", filename);
        if (auto file = container.FindFile(filename))
        {
          ArchivedDbg("Found");
          return file;
        }
        if (!SupportDirectories())
        {
          break;
        }
      }
      return {};
    }

  private:
    const PluginId Identifier;
    const uint_t Caps;
    const Formats::Archived::Decoder::Ptr Decoder;
  };
}  // namespace ZXTune

namespace ZXTune
{
  ArchivePlugin::Ptr CreateArchivePlugin(PluginId id, uint_t caps, Formats::Archived::Decoder::Ptr decoder)
  {
    return MakePtr<ArchivedContainerPlugin>(id, caps | Capabilities::Category::CONTAINER, std::move(decoder));
  }

  String ProgressMessage(PluginId id, StringView path)
  {
    return path.empty() ? Strings::Format(translate("{} processing"), id)
                        : Strings::Format(translate("{0} processing at {1}"), id, path);
  }

  String ProgressMessage(PluginId id, StringView path, StringView element)
  {
    return path.empty() ? Strings::Format(translate("{0} processing for {1}"), id, element)
                        : Strings::Format(translate("{0} processing for {1} at {2}"), id, element, path);
  }
}  // namespace ZXTune
