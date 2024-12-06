/**
 *
 * @file
 *
 * @brief  TurboSound containers support
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/player_plugins_registrator.h"
#include "core/plugins/players/plugin.h"
#include "formats/chiptune/aym/turbosound.h"
#include "module/players/aym/aym_base.h"
#include "module/players/aym/turbosound.h"
#include "module/players/properties_helper.h"
#include "module/players/tracking.h"

#include "core/plugin_attrs.h"
#include "debug/log.h"

#include "make_ptr.h"

namespace Module::TS
{
  const Debug::Stream Dbg("Core::TSSupp");

  class DataBuilder : public Formats::Chiptune::TurboSound::Builder
  {
  public:
    DataBuilder(const Parameters::Accessor& params, const Binary::Container& data)
      : Params(params)
      , Data(data)
    {}

    void SetFirstSubmoduleLocation(std::size_t offset, std::size_t size) override
    {
      if (!(First = LoadChiptune(offset, size)))
      {
        Dbg("Failed to create first module");
      }
    }

    void SetSecondSubmoduleLocation(std::size_t offset, std::size_t size) override
    {
      if (!(Second = LoadChiptune(offset, size)))
      {
        Dbg("Failed to create second module");
      }
    }

    bool HasResult() const
    {
      return First && Second;
    }

    AYM::Chiptune::Ptr GetFirst() const
    {
      return First;
    }

    AYM::Chiptune::Ptr GetSecond() const
    {
      return Second;
    }

  private:
    AYM::Chiptune::Ptr LoadChiptune(std::size_t offset, std::size_t size) const
    {
      const auto content = Data.GetSubcontainer(offset, size);
      if (const auto holder = std::dynamic_pointer_cast<const AYM::Holder>(TryOpenAYModule(*content)))
      {
        return holder->GetChiptune();
      }
      else
      {
        return {};
      }
    }

    Module::Holder::Ptr TryOpenAYModule(const Binary::Container& data) const
    {
      const auto initialProperties = Parameters::Container::Create();
      for (const auto& plugin : ZXTune::PlayerPlugin::Enumerate())
      {
        if (!IsAYPlugin(*plugin))
        {
          continue;
        }
        if (auto result = plugin->TryOpen(Params, data, initialProperties))
        {
          return result;
        }
      }
      return {};
    }

    static bool IsAYPlugin(const ZXTune::PlayerPlugin& plugin)
    {
      return 0 != (plugin.Capabilities() & ZXTune::Capabilities::Module::Device::AY38910);
    }

  private:
    const Parameters::Accessor& Params;
    const Binary::Container& Data;
    AYM::Chiptune::Ptr First;
    AYM::Chiptune::Ptr Second;
  };

  class Factory : public Module::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::TurboSound::Decoder::Ptr decoder)
      : Decoder(std::move(decoder))
    {}

    Module::Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& data,
                                     Parameters::Container::Ptr properties) const override
    {
      DataBuilder dataBuilder(params, data);
      if (const auto container = Decoder->Parse(data, dataBuilder))
      {
        if (dataBuilder.HasResult())
        {
          PropertiesHelper props(*properties);
          props.SetSource(*container);
          props.SetChannels(TurboSound::MakeChannelsNames());
          auto chiptune =
              TurboSound::CreateChiptune(std::move(properties), dataBuilder.GetFirst(), dataBuilder.GetSecond());
          return TurboSound::CreateHolder(std::move(chiptune));
        }
      }
      return {};
    }

  private:
    const Formats::Chiptune::TurboSound::Decoder::Ptr Decoder;
  };
}  // namespace Module::TS

namespace ZXTune
{
  void RegisterTSSupport(PlayerPluginsRegistrator& registrator)
  {
    // plugin attributes
    const auto ID = "TS"_id;
    const uint_t CAPS = Capabilities::Module::Type::MULTI | Capabilities::Module::Device::TURBOSOUND;

    auto decoder = Formats::Chiptune::TurboSound::CreateDecoder();
    auto factory = MakePtr<Module::TS::Factory>(decoder);
    auto plugin = CreatePlayerPlugin(ID, CAPS, std::move(decoder), std::move(factory));
    registrator.RegisterPlugin(std::move(plugin));
  }
}  // namespace ZXTune
