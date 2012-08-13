/**
*
* @file      io/provider.h
* @brief     Data provider interface
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef __IO_PROVIDER_H_DEFINED__
#define __IO_PROVIDER_H_DEFINED__

//common includes
#include <iterator.h>
//library includes
#include <binary/container.h>
#include <io/identifier.h>
//std includes
#include <vector>

//forward declarations
class Error;
namespace Parameters
{
  class Accessor;
}

namespace Log
{
  class ProgressCallback;
}

namespace ZXTune
{
  namespace IO
  {
    //! @brief Resolve uri to identifier object
    //! @param uri Full data identifier
    //! @throw Error if failed to resolve
    Identifier::Ptr ResolveUri(const String& uri);

    //! @brief Performs opening specified uri
    //! @param path External data identifier
    //! @param params %Parameters accessor to setup providers work
    //! @param cb Callback for long-time controlable operations
    //! @throw Error if failed to open
    Binary::Container::Ptr OpenData(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb);

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
}

#endif //__IO_PROVIDER_H_DEFINED__
