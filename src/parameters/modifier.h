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

//library includes
#include <parameters/visitor.h>

namespace Parameters
{
  class Modifier : public Visitor
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<Modifier> Ptr;

    //! Remove parameter
    virtual void RemoveValue(const NameType& name) = 0;
  };
}
