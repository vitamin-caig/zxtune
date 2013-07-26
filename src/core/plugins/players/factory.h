/*
Abstract:
  Abstract module factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef CORE_PLUGINS_PLAYERS_FACTORY_H_DEFINED
#define CORE_PLUGINS_PLAYERS_FACTORY_H_DEFINED

//local includes
#include "module_properties.h"
//library includes
#include <binary/container.h>
#include <core/module_holder.h>

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

#endif //CORE_PLUGINS_PLAYERS_FACTORY_H_DEFINED
