/**
 *
 * @file
 *
 * @brief  Parameters visitor interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <parameters/types.h>
// std includes
#include <memory>

namespace Parameters
{
  //! @brief Interface to add/modify properties and parameters
  class Visitor
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<Visitor> Ptr;

    virtual ~Visitor() = default;

    //! Add/modify integer parameter
    virtual void SetValue(StringView name, IntType val) = 0;
    //! Add/modify string parameter
    virtual void SetValue(StringView name, StringView val) = 0;
    //! Add/modify data parameter
    virtual void SetValue(StringView name, Binary::View val) = 0;
  };
}  // namespace Parameters
