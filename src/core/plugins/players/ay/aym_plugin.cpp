/**
* 
* @file
*
* @brief  AYM-based player plugin factory 
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "core/plugins/players/ay/aym_plugin.h"
#include "core/plugins/players/ay/aym_conversion.h"
#include "core/plugins/players/plugin.h"
//common includes
#include <make_ptr.h>
//library includes
#include <core/plugin_attrs.h>
#include <module/players/aym/aym_base.h>
#include <module/players/aym/aym_parameters.h>
//std includes
#include <utility>

namespace Module
{
  class AYMFactory : public Factory
  {
  public:
    explicit AYMFactory(AYM::Factory::Ptr delegate)
      : Delegate(std::move(delegate))
    {
    }

    Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data, Parameters::Container::Ptr properties) const override
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
}

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder, Module::AYM::Factory::Ptr factory)
  {
    const Module::Factory::Ptr modFactory = MakePtr<Module::AYMFactory>(factory);
    const uint_t ayCaps = Capabilities::Module::Device::AY38910 | Module::AYM::GetSupportedFormatConvertors();
    return CreatePlayerPlugin(id, caps | ayCaps, decoder, modFactory);
  }

  PlayerPlugin::Ptr CreateTrackPlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::AYM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::TRACK, decoder, factory);
  }
  
  PlayerPlugin::Ptr CreateStreamPlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::AYM::Factory::Ptr factory)
  {
    return CreatePlayerPlugin(id, Capabilities::Module::Type::STREAM, decoder, factory);
  }
}
