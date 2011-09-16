/**
*
* @file      binary/container.h
* @brief     Binary data container interface and related
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __BINARY_CONTAINER_H_DEFINED__
#define __BINARY_CONTAINER_H_DEFINED__

//common includes
#include <types.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Binary
{
  //! @brief Data container abstraction interface
  class Container
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<const Container> Ptr;

    virtual ~Container() {}

    //! @brief Getting data size in bytes
    virtual std::size_t Size() const = 0;
    //! @brief Getting raw data accessible for at least Size() bytes
    virtual const void* Data() const = 0;
    //! @brief Provides isolated access to nested subcontainers should be able even after parent container destruction
    virtual Ptr GetSubcontainer(std::size_t offset, std::size_t size) const = 0;
  };

  //! @brief Creating data container based on raw data
  Container::Ptr CreateContainer(const void* data, std::size_t size);
  Container::Ptr CreateContainer(std::auto_ptr<Dump> data);
}

#endif //__BINARY_CONTAINER_H_DEFINED__
