/*
Abstract:
  ST3 modules playback support

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "soundtracker.h"
#include "core/plugins/registrator.h"
//library includes
#include <core/plugin_attrs.h>
#include <core/conversion/aym.h>

namespace ST3
{
  using namespace ZXTune;
  using namespace ZXTune::Module;

  //plugin attributes
  const Char ID[] = {'S', 'T', '3', 0};
  const uint_t CAPS = CAP_STOR_MODULE | CAP_DEV_AYM | CAP_CONV_RAW | SupportedAYMFormatConvertors;
}

namespace ZXTune
{
  void RegisterST3Support(PlayerPluginsRegistrator& registrator)
  {
    const Formats::Chiptune::SoundTracker::Decoder::Ptr decoder = Formats::Chiptune::SoundTracker::Ver3::CreateDecoder();
    const ModulesFactory::Ptr factory = SoundTracker::CreateModulesFactory(decoder);
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ST3::ID, decoder->GetDescription(), ST3::CAPS, factory);
    registrator.RegisterPlugin(plugin);
  }
}
