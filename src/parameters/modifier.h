/**
 *
 * @file
 *
 * @brief  Parameters modifier interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "parameters/visitor.h"  // IWYU pragma: export

namespace Parameters
{
  class Modifier : public Visitor
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<Modifier>;

    //! Remove parameter
    virtual void RemoveValue(Identifier name) = 0;
  };
}  // namespace Parameters
