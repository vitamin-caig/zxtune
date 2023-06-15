/**
 *
 * @file
 *
 * @brief  Multitrack player plugin implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "core/plugins/players/multitrack_plugin.h"
#include "core/plugins/archives/archived.h"
// common includes
#include <make_ptr.h>
#include <xrange.h>
// library includes
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <debug/log.h>
#include <formats/archived/multitrack/filename.h>
#include <module/players/properties_helper.h>
// std includes
#include <utility>

namespace ZXTune
{
  const Debug::Stream MultistreamDbg("Core::Multistream");

  struct Filename
  {
    static std::optional<std::size_t> FindIndex(StringView str)
    {
      auto res = Formats::Archived::MultitrackArchives::ParseFilename(str);
      if (res && *res > 0)
      {
        return *res - 1;
      }
      else
      {
        return {};
      }
    }

    static String FromIndex(uint_t idx)
    {
      return Formats::Archived::MultitrackArchives::CreateFilename(idx + 1);
    }
  };

  std::optional<std::size_t> FindTrackIndexIn(const Analysis::Path& path)
  {
    String lastPathSegment;
    // TODO: add method to Path
    for (auto it = path.GetIterator(); it->IsValid(); it->Next())
    {
      lastPathSegment = it->Get();
    }
    return Filename::FindIndex(lastPathSegment);
  }

  template<class BasePluginType>
  class MultitrackBasePlugin : public BasePluginType
  {
  public:
    MultitrackBasePlugin(PluginId id, uint_t caps, Formats::Multitrack::Decoder::Ptr decoder,
                         Module::MultitrackFactory::Ptr factory)
      : Identifier(id)
      , Caps(caps)
      , Decoder(std::move(decoder))
      , Factory(std::move(factory))
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

  protected:
    Analysis::Result::Ptr DetectModules(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                        Module::DetectCallback& callback) const
    {
      auto data = inputData->GetData();
      if (auto container = Decoder->Decode(*data))
      {
        if (DetectModules(params, std::move(inputData), *container, callback))
        {
          return Analysis::CreateMatchedResult(container->Size());
        }
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), std::move(data));
    }

    DataLocation::Ptr CreateSubtrackLocation(DataLocation::Ptr inputData, uint_t idx) const
    {
      auto blob = inputData->GetData();
      return CreateNestedLocation(std::move(inputData), std::move(blob), Identifier, Filename::FromIndex(idx));
    }

  private:
    bool DetectModules(const Parameters::Accessor& params, const DataLocation::Ptr& inputData,
                       const Formats::Multitrack::Container& container, Module::DetectCallback& callback) const
    {
      const auto tracksCount = container.TracksCount();
      if (tracksCount == 0)
      {
        return false;
      }
      if (tracksCount == 1)
      {
        MultistreamDbg("{}: detect in single track container", Identifier);
        return ProcessSubtrack(params, *inputData, container, callback);
      }
      else if (const auto index = FindTrackIndexIn(*inputData->GetPath()))
      {
        MultistreamDbg("{}: detect in specified track {} out of {}", Identifier, *index, tracksCount);
        return *index < tracksCount
               && ProcessSubtrack(params, *inputData, ChangedTrackIndexAdapter(container, *index), callback);
      }
      else
      {
        MultistreamDbg("{}: detect in all {} tracks", Identifier, tracksCount);
        return ProcessAllSubtracks(params, inputData, container, callback);
      }
    }

    bool ProcessSubtrack(const Parameters::Accessor& params, const DataLocation& inputData,
                         const Formats::Multitrack::Container& container, Module::DetectCallback& callback) const
    {
      auto properties = callback.CreateInitialProperties(inputData.GetPath()->AsString());
      Module::PropertiesHelper props(*properties);
      props.SetContainer(inputData.GetPluginsChain()->AsString());
      if (auto holder = Factory->CreateModule(params, container, properties))
      {
        props.SetSource(container);
        props.SetType(Identifier);
        callback.ProcessModule(inputData, *this, std::move(holder));
        return true;
      }
      return false;
    }

    bool ProcessAllSubtracks(const Parameters::Accessor& params, const DataLocation::Ptr& inputData,
                             const Formats::Multitrack::Container& container, Module::DetectCallback& callback) const
    {
      bool result = false;
      for (auto index : xrange(container.TracksCount()))
      {
        const auto subData = CreateSubtrackLocation(inputData, index);
        if (ProcessSubtrack(params, *subData, ChangedTrackIndexAdapter(container, index), callback))
        {
          result = true;
        }
      }
      return result;
    }

    class ChangedTrackIndexAdapter : public Formats::Multitrack::Container
    {
    public:
      ChangedTrackIndexAdapter(const Formats::Multitrack::Container& delegate, uint_t index)
        : Delegate(delegate)
        , Index(index)
      {}

      const void* Start() const override
      {
        return Delegate.Start();
      }

      std::size_t Size() const override
      {
        return Delegate.Size();
      }

      Binary::Container::Ptr GetSubcontainer(std::size_t offset, std::size_t size) const override
      {
        return Delegate.GetSubcontainer(offset, size);
      }

      uint_t FixedChecksum() const override
      {
        return Delegate.FixedChecksum();
      }

      uint_t TracksCount() const override
      {
        return Delegate.TracksCount();
      }

      uint_t StartTrackIndex() const override
      {
        return Index;
      }

    private:
      const Formats::Multitrack::Container& Delegate;
      const uint_t Index;
    };

  protected:
    const PluginId Identifier;
    const uint_t Caps;
    const Formats::Multitrack::Decoder::Ptr Decoder;
    const Module::MultitrackFactory::Ptr Factory;
  };

  class MultitrackPlayerPlugin : public MultitrackBasePlugin<PlayerPlugin>
  {
  public:
    MultitrackPlayerPlugin(PluginId id, uint_t caps, Formats::Multitrack::Decoder::Ptr decoder,
                           Module::MultitrackFactory::Ptr factory)
      : MultitrackBasePlugin(id, caps, std::move(decoder), std::move(factory))
    {}

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 Module::DetectCallback& callback) const override
    {
      return DetectModules(params, inputData, callback);
    }

    Module::Holder::Ptr TryOpen(const Parameters::Accessor& params, const Binary::Container& data,
                                Parameters::Container::Ptr properties) const override
    {
      if (auto container = Decoder->Decode(data))
      {
        if (container->TracksCount() != 1)
        {
          return {};
        }
        if (auto result = Factory->CreateModule(params, *container, properties))
        {
          Module::PropertiesHelper props(*properties);
          props.SetSource(*container);
          props.SetType(Identifier);
          return result;
        }
      }
      return {};
    }
  };

  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Multitrack::Decoder::Ptr decoder,
                                       Module::MultitrackFactory::Ptr factory)
  {
    const auto outCaps = caps | Capabilities::Category::MODULE;
    return MakePtr<MultitrackPlayerPlugin>(id, outCaps, std::move(decoder), std::move(factory));
  }

  class MultitrackArchivePlugin : public MultitrackBasePlugin<ArchivePlugin>
  {
  public:
    MultitrackArchivePlugin(PluginId id, Formats::Multitrack::Decoder::Ptr decoder,
                            Module::MultitrackFactory::Ptr factory)
      : MultitrackBasePlugin(id, Capabilities::Container::Type::MULTITRACK | Capabilities::Category::CONTAINER,
                             std::move(decoder), std::move(factory))
    {}

    String Description() const override
    {
      return "Multitrack " + Decoder->GetDescription();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 ArchiveCallback& callback) const override
    {
      return DetectModules(params, std::move(inputData), callback);
    }

    DataLocation::Ptr TryOpen(const Parameters::Accessor& /*params*/, DataLocation::Ptr inputData,
                              const Analysis::Path& pathToOpen) const override
    {
      if (const auto index = Filename::FindIndex(pathToOpen.AsString()))
      {
        return CreateSubtrackLocation(std::move(inputData), *index);
      }
      return {};
    }
  };

  ArchivePlugin::Ptr CreateArchivePlugin(PluginId id, Formats::Multitrack::Decoder::Ptr decoder,
                                         Module::MultitrackFactory::Ptr factory)
  {
    return MakePtr<MultitrackArchivePlugin>(id, std::move(decoder), std::move(factory));
  }
}  // namespace ZXTune
