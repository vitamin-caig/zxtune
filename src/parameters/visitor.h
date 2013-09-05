/**
*
* @file     parameters/visitor.h
* @brief    Parameters visitor interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PARAMETERS_VISITOR_H_DEFINED
#define PARAMETERS_VISITOR_H_DEFINED

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

#endif //PARAMETERS_VISITOR_H_DEFINED
