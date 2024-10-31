/**
 *
 * @file
 *
 * @brief  AYM-based player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/ay/aym_plugin.h"

#include "core/plugins/players/ay/aym_conversion.h"
#include "core/plugins/players/plugin.h"
#include "module/players/aym/aym_base.h"
#include "module/players/factory.h"

#include "core/plugin_attrs.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <memory>
#include <utility>

namespace Module
{
  class AYMFactory : public Factory
  {
  public:
    explicit AYMFactory(AYM::Factory::Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data,
                             Parameters::Container::Ptr properties) const override
    {
      if (auto chiptune = Delegate->CreateChiptune(data, std::move(properties)))
      {
        return AYM::CreateHolder(std::move(chiptune));
      }
      else
      {
        return {};
      }
    }

  private:
    const AYM::Factory::Ptr Delegate;
  };
}  // namespace Module

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::AYM::Factory::Ptr factory)
  {
    auto modFactory = MakePtr<Module::AYMFactory>(std::move(factory));
    const uint_t ayCaps = Capabilities::Module::Device::AY38910 | Module::AYM::GetSupportedFormatConvertors();
    return CreatePlayerPlugin(id, caps | ayCaps, std::move(decoder), std::move(modFactory));
  }

  PlayerPlugin::Ptr CreateTrackPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                            Module::AYM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::TRACK, std::move(decoder), std::move(factory));
  }

  PlayerPlugin::Ptr CreateStreamPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                             Module::AYM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::STREAM, std::move(decoder), std::move(factory));
  }
}  // namespace ZXTune
