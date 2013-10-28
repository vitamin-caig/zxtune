/*
Abstract:
  Player plugin factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_PLUGIN_H_DEFINED
#define CORE_PLUGINS_PLAYERS_PLUGIN_H_DEFINED

//local includes
#include "factory.h"
#include "core/plugins/plugins_types.h"
//library includes
#include <formats/chiptune.h>

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder, Module::Factory::Ptr factory);
}

#endif //CORE_PLUGINS_PLAYERS_PLUGIN_H_DEFINED
