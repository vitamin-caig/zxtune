/**
 *
 * @file
 *
 * @brief  TFM-based player plugin factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "core/plugins/players/tfm/tfm_plugin.h"

#include "core/plugins/players/plugin.h"
#include "module/players/factory.h"
#include "module/players/tfm/tfm_base.h"
#include "module/players/tfm/tfm_chiptune.h"
#include "module/players/tfm/tfm_parameters.h"

#include "core/plugin_attrs.h"
#include "devices/tfm.h"
#include "module/holder.h"
#include "module/information.h"
#include "module/renderer.h"
#include "parameters/container.h"

#include "make_ptr.h"

#include <memory>
#include <utility>

namespace Module
{
  class TFMHolder : public Holder
  {
  public:
    explicit TFMHolder(TFM::Chiptune::Ptr chiptune)
      : Tune(std::move(chiptune))
    {}

    Information::Ptr GetModuleInformation() const override
    {
      return Tune->GetInformation();
    }

    Parameters::Accessor::Ptr GetModuleProperties() const override
    {
      return Tune->GetProperties();
    }

    Renderer::Ptr CreateRenderer(uint_t samplerate, Parameters::Accessor::Ptr params) const override
    {
      auto chipParams = TFM::CreateChipParameters(samplerate, std::move(params));
      auto chip = Devices::TFM::CreateChip(std::move(chipParams));
      auto iterator = Tune->CreateDataIterator();
      return TFM::CreateRenderer(Tune->GetFrameDuration() /*TODO: speed variation*/, std::move(iterator),
                                 std::move(chip));
    }

  private:
    const TFM::Chiptune::Ptr Tune;
  };

  class TFMFactory : public Factory
  {
  public:
    explicit TFMFactory(TFM::Factory::Ptr delegate)
      : Delegate(std::move(delegate))
    {}

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data,
                             Parameters::Container::Ptr properties) const override
    {
      if (auto chiptune = Delegate->CreateChiptune(data, std::move(properties)))
      {
        return MakePtr<TFMHolder>(std::move(chiptune));
      }
      else
      {
        return {};
      }
    }

  private:
    const TFM::Factory::Ptr Delegate;
  };
}  // namespace Module

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(PluginId id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder,
                                       Module::TFM::Factory::Ptr factory)
  {
    auto modFactory = MakePtr<Module::TFMFactory>(std::move(factory));
    const uint_t tfmCaps = Capabilities::Module::Device::TURBOFM;
    return CreatePlayerPlugin(id, caps | tfmCaps, std::move(decoder), std::move(modFactory));
  }

  PlayerPlugin::Ptr CreateTrackPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                            Module::TFM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::TRACK, std::move(decoder), std::move(factory));
  }

  PlayerPlugin::Ptr CreateStreamPlayerPlugin(PluginId id, Formats::Chiptune::Decoder::Ptr decoder,
                                             Module::TFM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::STREAM, std::move(decoder), std::move(factory));
  }
}  // namespace ZXTune
