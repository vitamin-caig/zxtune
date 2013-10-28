/**
*
* @file      io/provider.h
* @brief     Data provider interface
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef IO_PROVIDER_H_DEFINED
#define IO_PROVIDER_H_DEFINED

//common includes
#include <iterator.h>
#include <types.h>

//forward declarations
class Error;

namespace IO
{
  //! @brief Provider information interface
  class Provider
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<const Provider> Ptr;
    //! Iterator type
    typedef ObjectIterator<Provider::Ptr> Iterator;

    //! Virtual destructor
    virtual ~Provider() {}

    //! Provider's identifier
    virtual String Id() const = 0;
    //! Description in any form
    virtual String Description() const = 0;
    //! Actuality status
    virtual Error Status() const = 0;
  };

  //! @brief Enumerating supported %IO providers
  Provider::Iterator::Ptr EnumerateProviders();
}

#endif //IO_PROVIDER_H_DEFINED
