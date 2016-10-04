/**
*
* @file
*
* @brief  Parameters accessor interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/types.h>
//std includes
#include <memory>

namespace Parameters
{
  //! @brief Interface to give access to properties and parameters
  //! @invariant If value is not changed, parameter is not affected
  class Accessor
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<const Accessor> Ptr;

    virtual ~Accessor() {}

    //! Content version
    virtual uint_t Version() const = 0;

    //! Accessing integer parameters
    virtual bool FindValue(const NameType& name, IntType& val) const = 0;
    //! Accessing string parameters
    virtual bool FindValue(const NameType& name, StringType& val) const = 0;
    //! Accessing data parameters
    virtual bool FindValue(const NameType& name, DataType& val) const = 0;

    //! Valk along the stored values
    virtual void Process(class Visitor& visitor) const = 0;
  };
}
