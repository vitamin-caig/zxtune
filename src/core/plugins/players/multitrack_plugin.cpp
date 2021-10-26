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
// library includes
#include <binary/crc.h>
#include <core/module_detect.h>
#include <core/plugin_attrs.h>
#include <formats/archived/multitrack/multitrack.h>
#include <module/players/properties_helper.h>
// std includes
#include <utility>

namespace ZXTune
{
  class MultitrackPlayerPlugin : public PlayerPlugin
  {
  public:
    MultitrackPlayerPlugin(StringView id, StringView descr, uint_t caps, Formats::Multitrack::Decoder::Ptr decoder,
                           Module::MultitrackFactory::Ptr factory)
      : Identifier(id.to_string())
      , Desc(descr.to_string())
      , Caps(caps)
      , Decoder(std::move(decoder))
      , Factory(std::move(factory))
    {}

    String Id() const override
    {
      return Identifier;
    }

    String Description() const override
    {
      return Desc;
    }

    uint_t Capabilities() const override
    {
      return Caps;
    }

    Binary::Format::Ptr GetFormat() const override
    {
      return Decoder->GetFormat();
    }

    Analysis::Result::Ptr Detect(const Parameters::Accessor& params, DataLocation::Ptr inputData,
                                 Module::DetectCallback& callback) const override
    {
      auto data = inputData->GetData();
      if (auto container = Decoder->Decode(*data))
      {
        // TODO
        if (container->TracksCount() == 1 || SelfIsVisited(*inputData->GetPluginsChain()))
        {
          auto properties = callback.CreateInitialProperties(inputData->GetPath()->AsString());
          Module::PropertiesHelper props(*properties);
          props.SetContainer(inputData->GetPluginsChain()->AsString());
          if (auto holder = Factory->CreateModule(params, *container, properties))
          {
            props.SetSource(ChiptuneContainerAdapter(*container));
            props.SetType(Identifier);
            callback.ProcessModule(*inputData, *this, std::move(holder));
            return Analysis::CreateMatchedResult(container->Size());
          }
        }
      }
      return Analysis::CreateUnmatchedResult(Decoder->GetFormat(), std::move(data));
    }

    Module::Holder::Ptr TryOpen(const Parameters::Accessor& params, const Binary::Container& data,
                                Parameters::Container::Ptr properties) const override
    {
      if (auto container = Decoder->Decode(data))
      {
        // TODO
        if (container->TracksCount() != 1)
        {
          return {};
        }
        if (auto result = Factory->CreateModule(params, *container, properties))
        {
          Module::PropertiesHelper props(*properties);
          props.SetSource(ChiptuneContainerAdapter(*container));
          props.SetType(Identifier);
          return result;
        }
      }
      return {};
    }

  private:
    bool SelfIsVisited(const Analysis::Path& path) const
    {
      for (auto it = path.GetIterator(); it->IsValid(); it->Next())
      {
        if (it->Get() == Identifier)
        {
          return true;
        }
      }
      return false;
    }

    class ChiptuneContainerAdapter : public Formats::Chiptune::Container
    {
    public:
      ChiptuneContainerAdapter(const Formats::Multitrack::Container& delegate)
        : Delegate(delegate)
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

      uint_t Checksum() const override
      {
        return Binary::Crc32(Delegate);
      }

      uint_t FixedChecksum() const override
      {
        return Delegate.FixedChecksum();
      }

    private:
      const Formats::Multitrack::Container& Delegate;
    };

  private:
    const String Identifier;
    const String Desc;
    const uint_t Caps;
    const Formats::Multitrack::Decoder::Ptr Decoder;
    const Module::MultitrackFactory::Ptr Factory;
  };

  PlayerPlugin::Ptr CreatePlayerPlugin(StringView id, StringView description, uint_t caps,
                                       Formats::Multitrack::Decoder::Ptr decoder,
                                       Module::MultitrackFactory::Ptr factory)
  {
    const auto outCaps = caps | Capabilities::Category::MODULE;
    return MakePtr<MultitrackPlayerPlugin>(id, description, outCaps, std::move(decoder), std::move(factory));
  }

  ArchivePlugin::Ptr CreateArchivePlugin(StringView id, StringView description,
                                         Formats::Multitrack::Decoder::Ptr decoder)
  {
    const uint_t CAPS = Capabilities::Container::Type::MULTITRACK | Capabilities::Container::Traits::ONCEAPPLIED;
    auto containerDecoder =
        Formats::Archived::CreateMultitrackArchiveDecoder("Multitrack " + description.to_string(), decoder);
    return CreateArchivePlugin(id, CAPS, std::move(containerDecoder));
  }
}  // namespace ZXTune
