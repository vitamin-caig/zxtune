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

//library includes
#include <module/state.h>
#include <module/holder.h>
#include <sound/backend.h>
#include <sound/chunk.h>

namespace Sound
{
  class BackendWorker
  {
  public:
    typedef std::shared_ptr<BackendWorker> Ptr;
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
    typedef std::shared_ptr<const BackendWorkerFactory> Ptr;
    virtual ~BackendWorkerFactory() = default;

    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder) const = 0;
  };

  Backend::Ptr CreateBackend(Parameters::Accessor::Ptr globalParams, Module::Holder::Ptr holder, BackendCallback::Ptr callback, BackendWorker::Ptr worker);
}
