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

//library includes
#include <parameters/types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Parameters
{
  //! @brief Interface to add/modify properties and parameters
  class Visitor
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Visitor> Ptr;

    virtual ~Visitor() {}

    //! Add/modify integer parameter
    virtual void SetValue(const NameType& name, IntType val) = 0;
    //! Add/modify string parameter
    virtual void SetValue(const NameType& name, const StringType& val) = 0;
    //! Add/modify data parameter
    virtual void SetValue(const NameType& name, const DataType& val) = 0;
  };
}
