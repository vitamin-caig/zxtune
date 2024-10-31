/**
 *
 * @file
 *
 * @brief  Backend internal interfaces and factory
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

#include "module/holder.h"
#include "module/state.h"
#include "sound/backend.h"
#include "sound/chunk.h"

namespace Sound
{
  class BackendWorker
  {
  public:
    using Ptr = std::shared_ptr<BackendWorker>;
    virtual ~BackendWorker() = default;

    virtual void Startup() = 0;
    virtual void Shutdown() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void FrameStart(const Module::State& state) = 0;
    virtual void FrameFinish(Chunk buffer) = 0;
    virtual VolumeControl::Ptr GetVolumeControl() const = 0;
  };

  class BackendWorkerFactory
  {
  public:
    using Ptr = std::shared_ptr<const BackendWorkerFactory>;
    virtual ~BackendWorkerFactory() = default;

    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const = 0;
  };

  Backend::Ptr CreateBackend(Parameters::Accessor::Ptr globalParams, const Module::Holder::Ptr& holder,
                             BackendCallback::Ptr callback, BackendWorker::Ptr worker);
}  // namespace Sound
