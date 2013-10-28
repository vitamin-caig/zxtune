/*
Abstract:
  Backend helper interface definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef SOUND_BACKEND_IMPL_H_DEFINED
#define SOUND_BACKEND_IMPL_H_DEFINED

//library includes
#include <sound/backend.h>
#include <sound/chunk.h>

namespace Sound
{
  class BackendWorker
  {
  public:
    typedef boost::shared_ptr<BackendWorker> Ptr;
    virtual ~BackendWorker() {}

    virtual void Startup() = 0;
    virtual void Shutdown() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void FrameStart(const Module::TrackState& state) = 0;
    virtual void FrameFinish(Chunk::Ptr buffer) = 0;
    virtual VolumeControl::Ptr GetVolumeControl() const = 0;
  };

  class BackendWorkerFactory
  {
  public:
    typedef boost::shared_ptr<const BackendWorkerFactory> Ptr;
    virtual ~BackendWorkerFactory() {}

    virtual BackendWorker::Ptr CreateWorker(Parameters::Accessor::Ptr params) const = 0;
  };

  Backend::Ptr CreateBackend(Parameters::Accessor::Ptr params, Module::Holder::Ptr holder, BackendCallback::Ptr callback, BackendWorker::Ptr worker);
}

#endif //SOUND_BACKEND_IMPL_H_DEFINED
