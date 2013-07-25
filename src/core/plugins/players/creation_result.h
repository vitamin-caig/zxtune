/*
Abstract:
  Module creation result interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_PLAYER_CREATION_RESULT_H_DEFINED__
#define __CORE_PLUGINS_PLAYER_CREATION_RESULT_H_DEFINED__

//local includes
#include "module_properties.h"
#include "core/plugins/plugins_types.h"
//library includes
#include <binary/container.h>
#include <core/module_holder.h>
#include <formats/chiptune.h>

namespace Module
{
  class Factory
  {
  public:
    typedef boost::shared_ptr<const Factory> Ptr;
    virtual ~Factory() {}

    virtual Holder::Ptr CreateModule(PropertiesBuilder& properties, const Binary::Container& data) const = 0;
  };
}

namespace ZXTune
{
  PlayerPlugin::Ptr CreatePlayerPlugin(const String& id, uint_t caps, Formats::Chiptune::Decoder::Ptr decoder, Module::Factory::Ptr factory);
}

#endif //__CORE_PLUGINS_PLAYER_CREATION_RESULT_H_DEFINED__
