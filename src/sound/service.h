/**
*
* @file      sound/service.h
* @brief     Sound service interface and factory
* @version   $Id$
* @author    (C) Vitamin/CAIG/2001
*
**/

#pragma once
#ifndef SOUND_SERVICE_H_DEFINED
#define SOUND_SERVICE_H_DEFINED

//library includes
#include <sound/backend.h>
#include <strings/array.h>

namespace Sound
{
  class Service
  {
  public:
    //! Pointer type
    typedef boost::shared_ptr<const Service> Ptr;
    virtual ~Service() {}

    //! Enumerate all the registered backends
    virtual BackendInformation::Iterator::Ptr EnumerateBackends() const = 0;

    //! Return list of available backends ordered by preferences
    virtual Strings::Array GetAvailableBackends() const = 0;

    //! @brief Create backend using specified parameters
    //! @param params %Backend-related parameters
    //! @return Result backend
    //! @throw Error in case of error
    virtual Backend::Ptr CreateBackend(const String& backendId, Module::Holder::Ptr module, BackendCallback::Ptr callback) const = 0;
  };

  Service::Ptr CreateSystemService(Parameters::Accessor::Ptr options);
  Service::Ptr CreateFileService(Parameters::Accessor::Ptr options);
  Service::Ptr CreateGlobalService(Parameters::Accessor::Ptr options);
}

#endif //SOUND_SERVICE_H_DEFINED
