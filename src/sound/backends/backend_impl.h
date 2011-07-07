/*
Abstract:
  Backend helper interface definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __SOUND_BACKEND_IMPL_H_DEFINED__
#define __SOUND_BACKEND_IMPL_H_DEFINED__

//library includes
#include <sound/backend.h>

namespace ZXTune
{
  namespace Sound
  {
    class BackendWorker
    {
    public:
      typedef boost::shared_ptr<BackendWorker> Ptr;
      virtual ~BackendWorker() {}

      virtual void Test() = 0;
      virtual void OnStartup(const Module::Holder& module) = 0;
      virtual void OnShutdown() = 0;
      virtual void OnPause() = 0;
      virtual void OnResume() = 0;
      virtual void OnFrame(const Module::TrackState& state) = 0;
      virtual void OnBufferReady(std::vector<MultiSample>& buffer) = 0;
      virtual VolumeControl::Ptr GetVolumeControl() const = 0;
    };

    Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker);
  }
}


#endif //__SOUND_BACKEND_IMPL_H_DEFINED__
