/*
Abstract:
  AYM player plugin factory implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

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

    virtual Holder::Ptr CreateModule(PropertiesBuilder& properties, const Binary::Container& data) const
    {
      if (const AYM::Chiptune::Ptr chiptune = Delegate->CreateChiptune(properties, data))
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
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::AYM::Factory::Ptr factory)
  {
    const Module::Factory::Ptr modFactory = boost::make_shared<Module::AYMFactory>(factory);
    const uint_t caps = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | Module::AYM::SupportedFormatConvertors;
    return CreatePlayerPlugin(id, caps, decoder, modFactory);
  }
}
