/**
 *
 * @file
 *
 * @brief  Sound service interface and factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// library includes
#include <module/holder.h>
#include <sound/backend.h>
#include <strings/array.h>

namespace Sound
{
  class Service
  {
  public:
    //! Pointer type
    typedef std::shared_ptr<const Service> Ptr;
    virtual ~Service() = default;

    //! Enumerate all the registered backends
    virtual BackendInformation::Iterator::Ptr EnumerateBackends() const = 0;

    //! Return list of available backends ordered by preferences
    virtual Strings::Array GetAvailableBackends() const = 0;

    //! @brief Create backend using specified parameters
    //! @param params %Backend-related parameters
    //! @return Result backend
    //! @throw Error in case of error
    virtual Backend::Ptr CreateBackend(StringView backendId, Module::Holder::Ptr module,
                                       BackendCallback::Ptr callback) const = 0;
  };

  Service::Ptr CreateSystemService(Parameters::Accessor::Ptr options);
  Service::Ptr CreateFileService(Parameters::Accessor::Ptr options);
  Service::Ptr CreateGlobalService(Parameters::Accessor::Ptr options);
}  // namespace Sound
