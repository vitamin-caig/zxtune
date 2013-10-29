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
#include <core/module_holder.h>

namespace Module
{
  //! @param location Source data location
  //! @throw Error if no object detected
  Holder::Ptr Open(ZXTune::DataLocation::Ptr location);

  Holder::Ptr Open(const Binary::Container& data);
}
