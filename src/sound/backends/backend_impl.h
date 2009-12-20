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

#include <error.h>

#include <core/player.h>
#include <sound/mixer.h>
#include <sound/backend.h>
#include <sound/render_params.h>

#include <boost/thread.hpp>

namespace ZXTune
{
  namespace Sound
  {
    class BackendImpl : public Backend
    {
    public:
      BackendImpl();
      virtual ~BackendImpl();

      virtual Error SetPlayer(Module::Player::Ptr player);
      virtual boost::weak_ptr<const Module::Player> GetPlayer() const;
      
      // playback control functions
      virtual Error Play();
      virtual Error Pause();
      virtual Error Stop();
      virtual Error SetPosition(unsigned frame);
      
      virtual Error GetCurrentState(State& state) const;

      virtual Error SetMixer(const std::vector<MultiGain>& data);
      virtual Error SetFilter(Converter::Ptr converter);
      
      virtual Error SetParameters(const ParametersMap& params);
      virtual Error GetParameters(ParametersMap& params) const;
    protected:
      //internal usage functions. Should not call external interface funcs due to sync
      virtual void OnStartup() = 0;
      virtual void OnShutdown() = 0;
      virtual void OnPause() = 0;
      virtual void OnResume() = 0;
      virtual void OnParametersChanged(const ParametersMap& updates) = 0;
      virtual void OnBufferReady(std::vector<MultiSample>& buffer) = 0;
    private:
      void CheckState() const;
      void StopPlayback();
      bool SafeRenderFrame();
      void RenderFunc();
    protected:
      ParametersMap CommonParameters;
      RenderParameters RenderingParameters;
    protected:
      //sync
      typedef boost::lock_guard<boost::mutex> Locker;
      mutable boost::mutex BackendMutex;
    private:
      mutable boost::mutex PlayerMutex;
      boost::thread RenderThread;
      boost::barrier SyncBarrier;
      //state
      volatile State CurrentState;
      volatile bool InProcess;//STOP => STOPPING, STARTED => STARTING
      Error RenderError;
      //context
      unsigned Channels;
      boost::shared_ptr<Module::Player> Player;
      Converter::Ptr FilterObject;
      std::vector<Mixer::Ptr> MixersSet;
      Receiver::Ptr Renderer;
      std::vector<MultiSample> Buffer;
    };
  }
}


#endif //__SOUND_BACKEND_IMPL_H_DEFINED__
