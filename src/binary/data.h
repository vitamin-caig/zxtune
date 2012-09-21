/**
*
* @file      binary/data.h
* @brief     Simple binary data abstraction
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef BINARY_DATA_H_DEFINED
#define BINARY_DATA_H_DEFINED

//boost includes
#include <boost/shared_ptr.hpp>

namespace Binary
{
  //! @brief Data container abstraction interface
  class Data
  {
  public:
    //! @brief Pointer type
    typedef boost::shared_ptr<const Data> Ptr;

    virtual ~Data() {}

    //! @brief Raw data accessible for at least Size() bytes
    //! @invariant Should not change after first call
    //! @invariant Always != null
    virtual const void* Start() const = 0;
    //! @brief Getting data size in bytes
    //! @invariant Always >= 0
    virtual std::size_t Size() const = 0;
  };
}

#endif //BINARY_DATA_H_DEFINED
