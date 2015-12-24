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
#include "aym_plugin.h"
#include "aym_base.h"
#include "aym_parameters.h"
#include "core/plugins/players/plugin.h"
#include "core/conversion/aym.h"
//library includes
#include <core/plugin_attrs.h>

namespace Module
{
  class AYMFactory : public Factory
  {
  public:
    explicit AYMFactory(AYM::Factory::Ptr delegate)
      : Delegate(delegate)
    {
    }

    virtual Holder::Ptr CreateModule(const Parameters::Accessor& /*params*/, const Binary::Container& data, Parameters::Container::Ptr properties) const
    {
      if (const AYM::Chiptune::Ptr chiptune = Delegate->CreateChiptune(data, properties))
      {
        return AYM::CreateHolder(chiptune);
      }
      else
      {
        return Holder::Ptr();
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
    const Module::Factory::Ptr modFactory = boost::make_shared<Module::AYMFactory>(factory);
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
