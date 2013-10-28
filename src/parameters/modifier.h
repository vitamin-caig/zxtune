/**
*
* @file     parameters/modifier.h
* @brief    Parameters modifier interface
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PARAMETERS_MODIFIER_H_DEFINED
#define PARAMETERS_MODIFIER_H_DEFINED

//library includes
#include <parameters/visitor.h>

namespace Parameters
{
  class Modifier : public Visitor
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Modifier> Ptr;

    //! Remove parameter
    virtual void RemoveValue(const NameType& name) = 0;
  };
}

#endif //PARAMETERS_MODIFIER_H_DEFINED
