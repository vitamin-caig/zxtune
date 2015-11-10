/**
* 
* @file
*
* @brief  Module factory interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

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

    virtual Holder::Ptr CreateModule(const Parameters::Accessor& params, const Binary::Container& data, PropertiesBuilder& properties) const = 0;
  };
}
