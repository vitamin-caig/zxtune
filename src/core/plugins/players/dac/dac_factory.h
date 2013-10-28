/*
Abstract:
  Abstract module factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_DAC_FACTORY_H_DEFINED
#define CORE_PLUGINS_PLAYERS_DAC_FACTORY_H_DEFINED

//local includes
#include "dac_chiptune.h"
#include "core/plugins/players/module_properties.h"
//library includes
#include <binary/container.h>

namespace Module
{
  namespace DAC
  {
    class Factory
    {
    public:
      typedef boost::shared_ptr<const Factory> Ptr;
      virtual ~Factory() {}

      virtual Chiptune::Ptr CreateChiptune(PropertiesBuilder& properties, const Binary::Container& data) const = 0;
    };
  }
}

#endif //CORE_PLUGINS_PLAYERS_DAC_FACTORY_H_DEFINED
