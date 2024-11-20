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

#include "binary/data.h"
#include "module/holder.h"

namespace Module
{
  namespace Conversion
  {
    struct Parameter;
  }

  //! @return Binary::Data::Ptr if cannot convert
  //! @throw Error in case of internal problems
  Binary::Data::Ptr Convert(const Holder& holder, const Conversion::Parameter& spec,
                            const Parameters::Accessor::Ptr& params);
}  // namespace Module
