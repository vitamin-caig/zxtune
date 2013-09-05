/**
*
* @file     parameters/container.h
* @brief    Parameters container interface and factory
* @version  $Id$
* @author   (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef PARAMETERS_CONTAINER_H_DEFINED
#define PARAMETERS_CONTAINER_H_DEFINED

//library includes
#include <parameters/accessor.h>
#include <parameters/modifier.h>

namespace Parameters
{
  //! @brief Service type to simply properties keep and give access
  //! @invariant Only last value is kept for multiple assignment
  class Container : public Accessor
                  , public Modifier
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<Container> Ptr;

    static Ptr Create();
  };
}

#endif //PARAMETERS_CONTAINER_H_DEFINED
