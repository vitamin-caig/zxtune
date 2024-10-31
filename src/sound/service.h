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

#include "module/holder.h"
#include "parameters/accessor.h"
#include "sound/backend.h"

#include <memory>
#include <span>
#include <vector>

namespace Sound
{
  class BackendId;

  class Service
  {
  public:
    //! Pointer type
    using Ptr = std::shared_ptr<const Service>;
    virtual ~Service() = default;

    //! Enumerate all the registered backends
    virtual std::span<const BackendInformation::Ptr> EnumerateBackends() const = 0;

    //! Return list of available backends ordered by preferences
    virtual std::vector<BackendId> GetAvailableBackends() const = 0;

    //! @brief Create backend using specified parameters
    //! @param params %Backend-related parameters
    //! @return Result backend
    //! @throw Error in case of error
    virtual Backend::Ptr CreateBackend(BackendId id, Module::Holder::Ptr module,
                                       BackendCallback::Ptr callback) const = 0;
  };

  Service::Ptr CreateSystemService(Parameters::Accessor::Ptr options);
  Service::Ptr CreateFileService(Parameters::Accessor::Ptr options);
  Service::Ptr CreateGlobalService(Parameters::Accessor::Ptr options);
}  // namespace Sound
