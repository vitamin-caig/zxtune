/*
Abstract:
  Backend helper interface definition

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

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
    // Internal implementation for backend
    class BackendImpl
    {
    public:
      BackendImpl();
      virtual ~BackendImpl();

      Error SetModule(Module::Holder::Ptr holder);
      Module::Player::ConstPtr GetPlayer() const;

      // playback control functions
      Error Play();
      Error Pause();
      Error Stop();
      Error SetPosition(uint_t frame);

      Backend::State GetCurrentState(Error* error) const;

      SignalsCollector::Ptr CreateSignalsCollector(uint_t signalsMask) const;

      Error SetMixer(const std::vector<MultiGain>& data);
      Error SetFilter(Converter::Ptr converter);
      //volume control should be implemented explicitly
      Error SetParameters(const Parameters::Accessor& params);
    protected:
      //internal usage functions. Should not call external interface funcs due to sync
      virtual void OnStartup() = 0;
      virtual void OnShutdown() = 0;
      virtual void OnPause() = 0;
      virtual void OnResume() = 0;
      virtual void OnParametersChanged(const Parameters::Accessor& newParams) = 0;
      virtual void OnBufferReady(std::vector<MultiSample>& buffer) = 0;
      virtual bool OnRenderFrame();
    private:
      void DoStartup();
      void DoShutdown();
      void DoPause();
      void DoResume();
      void DoBufferReady(std::vector<MultiSample>& buffer);
    private:
      void CheckState() const;
      void StopPlayback();
      bool SafeRenderFrame();
      void RenderFunc();
      void SendSignal(uint_t sig);
    protected:
      //inheritances' context
      Parameters::Container::Ptr CommonParameters;
      RenderParameters RenderingParameters;
      Module::Player::Ptr Player;
    protected:
      //sync
      typedef boost::lock_guard<boost::mutex> Locker;
      mutable boost::mutex BackendMutex;
      mutable boost::mutex PlayerMutex;
    private:
      //events-related
      const SignalsDispatcher::Ptr Signaller;
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
      uint_t Channels;
      Converter::Ptr FilterObject;
      std::vector<Mixer::Ptr> MixersSet;
      Receiver::Ptr Renderer;
      std::vector<MultiSample> Buffer;
    };
  }
}


#endif //__SOUND_BACKEND_IMPL_H_DEFINED__
