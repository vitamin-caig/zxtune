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

    virtual void Test() = 0;
    virtual void Startup() = 0;
    virtual void Shutdown() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void BufferReady(Chunk& buffer) = 0;
    virtual VolumeControl::Ptr GetVolumeControl() const = 0;
  };

  //worker can implement BackendCallback interface
  Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker);
}

#endif //SOUND_BACKEND_IMPL_H_DEFINED
