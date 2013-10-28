/**
*
* @file      binary/container.h
* @brief     Binary data container interface
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_CONTAINER_H_DEFINED
#define BINARY_CONTAINER_H_DEFINED

//library includes
#include <binary/data.h>

namespace Binary
{
  class Container : public Data
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<const Container> Ptr;

    //! @brief Provides isolated access to nested subcontainers should be able even after parent container destruction
    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const = 0;
  };
}

#endif //BINARY_CONTAINER_H_DEFINED
