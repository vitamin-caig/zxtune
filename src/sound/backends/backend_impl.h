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

//common includes
#include <error.h>
//library includes
#include <sound/backend.h>
#include <sound/mixer.h>
#include <sound/render_params.h>
//boost includes
#include <boost/thread.hpp>

namespace ZXTune
{
  namespace Sound
  {
    class BackendWorker
    {
    public:
      typedef boost::shared_ptr<BackendWorker> Ptr;
      virtual ~BackendWorker() {}

      virtual void OnStartup() = 0;
      virtual void OnShutdown() = 0;
      virtual void OnPause() = 0;
      virtual void OnResume() = 0;
      virtual void OnFrame() = 0;
      virtual void OnBufferReady(std::vector<MultiSample>& buffer) = 0;
      virtual VolumeControl::Ptr GetVolumeControl() const = 0;
    };

    // Internal implementation for backend
    class BackendImpl : public BackendWorker
    {
    public:
      explicit BackendImpl(CreateBackendParameters::Ptr params);
      virtual ~BackendImpl();

      Module::Holder::Ptr GetModule() const;
      Module::Player::ConstPtr GetPlayer() const;

      // playback control functions
      Error Play();
      Error Pause();
      Error Stop();
      Error SetPosition(uint_t frame);

      Backend::State GetCurrentState() const;

      Async::Signals::Collector::Ptr CreateSignalsCollector(uint_t signalsMask) const;
    private:
      void DoStartup();
      void DoShutdown();
      void DoPause();
      void DoResume();
    private:
      void StopPlayback();
      bool SafeRenderFrame();
      void RenderFunc();
      void SendSignal(uint_t sig);
      bool RenderFrame();
    protected:
      //inheritances' context
      const Mixer::Ptr CurrentMixer;
      const Module::Holder::Ptr Holder;
      const Module::Player::Ptr Player;
      const Parameters::Accessor::Ptr SoundParameters;
      RenderParameters::Ptr RenderingParameters;
    private:
      //sync
      mutable boost::mutex BackendMutex;
      mutable boost::mutex PlayerMutex;
      //events-related
      const Async::Signals::Dispatcher::Ptr Signaller;
      //sync-related
      boost::thread RenderThread;
      boost::barrier SyncBarrier;
      boost::mutex PauseMutex;
      boost::condition_variable PauseEvent;
      //state
      volatile Backend::State CurrentState;
      volatile bool InProcess;//STOP => STOPPING, STARTED => STARTING
      Error RenderError;
      //context
      Receiver::Ptr Renderer;
      std::vector<MultiSample> Buffer;
    };

    Backend::Ptr CreateBackend(CreateBackendParameters::Ptr params, BackendWorker::Ptr worker);
  }
}


#endif //__SOUND_BACKEND_IMPL_H_DEFINED__
