/**
*
* @file
*
* @brief  Conversion API interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <core/module_holder.h>
#include <core/conversion.h>

namespace Module
{
  //! @return Binary::Data::Ptr if cannot convert
  //! @throw Error in case of internal problems
  Binary::Data::Ptr Convert(const Holder& holder, const Conversion::Parameter& spec, Parameters::Accessor::Ptr params);
}
