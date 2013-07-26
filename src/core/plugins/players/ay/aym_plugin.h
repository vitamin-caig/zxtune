/*
Abstract:
  AYM player plugin factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_AYM_PLUGIN_H_DEFINED
#define CORE_PLUGINS_PLAYERS_AYM_PLUGIN_H_DEFINED

//local includes
#include "aym_factory.h"
#include "core/plugins/plugins_types.h"
//library includes
#include <formats/chiptune.h>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, Formats::Chiptune::Decoder::Ptr decoder, Module::AYM::Factory::Ptr factory);
}

#endif //CORE_PLUGINS_PLAYERS_AYM_PLUGIN_H_DEFINED
