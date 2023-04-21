/**
 *
 * @file
 *
 * @brief  Backends storage interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// local includes
#include "sound/backends/backend_impl.h"

namespace Sound
{
  class BackendsStorage
  {
  public:
    virtual ~BackendsStorage() = default;

    // Functional
    virtual void Register(BackendId id, const char* description, uint_t caps, BackendWorkerFactory::Ptr factory) = 0;
    // Disabled due to error
    virtual void Register(BackendId id, const char* description, uint_t caps, const Error& status) = 0;
    // Disabled due to configuration
    virtual void Register(BackendId id, const char* description, uint_t caps) = 0;
  };
}  // namespace Sound
