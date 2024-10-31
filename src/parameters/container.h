/**
 *
 * @file
 *
 * @brief  Parameters container interface and factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include <parameters/accessor.h>
#include <parameters/modifier.h>

namespace Parameters
{
  //! @brief Service type to simply properties keep and give access
  //! @invariant Only last value is kept for multiple assignment
  class Container
    : public Accessor
    , public Modifier
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<Container>;

    static Ptr Create();
    static Ptr Clone(const Parameters::Accessor& source);
    static Ptr CreateAdapter(Accessor::Ptr accessor, Modifier::Ptr modifier);
  };
}  // namespace Parameters
