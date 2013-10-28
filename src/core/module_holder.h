/**
*
* @file     core/module_holder.h
* @brief    Modules holder interface definition
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __CORE_MODULE_HOLDER_H_DEFINED__
#define __CORE_MODULE_HOLDER_H_DEFINED__

//library includes
#include "plugins/players/renderer.h"
#include <binary/data.h>
#include <parameters/accessor.h>
#include <sound/receiver.h>

namespace Module
{
  //! @brief %Module holder interface
  class Holder
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<const Holder> Ptr;

    virtual ~Holder() {}

    //! @brief Retrieving info about loaded module
    virtual Information::Ptr GetModuleInformation() const = 0;

    //! @brief Retrieving properties of loaded module
    virtual Parameters::Accessor::Ptr GetModuleProperties() const = 0;

    //! @brief Creating new renderer instance
    //! @return New player
    virtual Renderer::Ptr CreateRenderer(Parameters::Accessor::Ptr params, Sound::Receiver::Ptr target) const = 0;
  };

  //TODO: remove
  Binary::Data::Ptr GetRawData(const Holder& holder);

  Holder::Ptr CreateMixedPropertiesHolder(Holder::Ptr delegate, Parameters::Accessor::Ptr props);
}

#endif //__CORE_MODULE_HOLDER_H_DEFINED__
