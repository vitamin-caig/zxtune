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
    //! @brief Performs opening specified uri
    //! @param path External data identifier
    //! @param params %Parameters accessor to setup providers work
    //! @param cb Callback for long-time controlable operations
    //! @param data Reference to result data container
    //! @return Error() in case of success
    Error OpenData(const String& path, const Parameters::Accessor& params, Log::ProgressCallback& cb, Binary::Container::Ptr& data);

    //! @brief Performs splitting specified uri to filesystem and internal parts
    //! @param uri Full path
    //! @param path Reference to external identifier result
    //! @param subpath Reference to internal identifier result
    //! @return Error() in case of success
    Error SplitUri(const String& uri, String& path, String& subpath);

    //! @brief Performs combining external and internal identifiers in scheme-dependent style
    //! @param path External data identifier
    //! @param subpath Internal data identifier
    //! @param uri Reference to merged uri result
    //! @return Error() in case of success
    Error CombineUri(const String& path, const String& subpath, String& uri);

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
