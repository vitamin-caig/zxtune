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
#include <core/data_location.h>
#include <module/holder.h>

namespace Module
{
  //! @param params Parameters for plugins
  //! @param location Source data location
  //! @throw Error if no object detected
  Holder::Ptr Open(const Parameters::Accessor& params, ZXTune::DataLocation::Ptr location);

  Holder::Ptr Open(const Parameters::Accessor& params, const Binary::Container& data);
}
