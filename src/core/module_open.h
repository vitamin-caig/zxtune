/**
*
* @file
*
* @brief  Modules opening functions
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <binary/container.h>
#include <module/holder.h>
#include <parameters/container.h>

namespace Module
{
  //! @param params Parameters for plugins
  //! @param data Source data location
  //! @throw Error if no object detected
  Holder::Ptr Open(const Parameters::Accessor& params, Binary::Container::Ptr data,
    const String& subpath, Parameters::Container::Ptr initialProperties);

  Holder::Ptr Open(const Parameters::Accessor& params, const Binary::Container& data,
    Parameters::Container::Ptr initialProperties);
}
