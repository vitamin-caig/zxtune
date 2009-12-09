/*
Abstract:
  Safe backend wrapper. Used for safe playback stopping if object is destroyed

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "../backend.h"

#include <memory>

namespace ZXTune
{
  namespace Sound
  {
    template<class Impl>
    class SafeBackendWrapper : public Backend
    {
    public:
      SafeBackendWrapper() : Delegate(new Impl())
      {
      }
      
      virtual ~SafeBackendWrapper()
      {
        Delegate->Stop();//TODO: warn if error
      }
      
      virtual void GetInfo(Info& info) const
      {
        return Delegate->GetInfo(info);
      }

      virtual Error SetPlayer(Module::Player::Ptr player)
      {
        return Delegate->SetPlayer(player);
      }
      
      virtual boost::weak_ptr<const Module::Player> GetPlayer() const
      {
        return Delegate->GetPlayer();
      }
      
      virtual Error Play()
      {
        return Delegate->Play();
      }
      
      virtual Error Pause()
      {
        return Delegate->Pause();
      }
      
      virtual Error Stop()
      {
        return Delegate->Stop();
      }
      
      virtual Error SetPosition(unsigned frame)
      {
        return Delegate->SetPosition(frame);
      }
      
      virtual Error GetCurrentState(State& state) const
      {
        return Delegate->GetCurrentState(state);
      }

      virtual Error SetMixer(const std::vector<MultiGain>& data)
      {
        return Delegate->SetMixer(data);
      }

      virtual Error SetFilter(Converter::Ptr converter)
      {
        return Delegate->SetFilter(converter);
      }

      virtual Error SetDriverParameters(const ParametersMap& params)
      {
        return Delegate->SetDriverParameters(params);
      }
      
      virtual Error GetDriverParameters(ParametersMap& params) const
      {
        return Delegate->GetDriverParameters(params);
      }
      
      virtual Error SetRenderParameters(const RenderParameters& params)
      {
        return Delegate->SetRenderParameters(params);
      }
      
      virtual Error GetRenderParameters(RenderParameters& params) const
      {
        return Delegate->GetRenderParameters(params);
      }
      
      virtual Error GetVolume(MultiGain& volume) const
      {
        return Delegate->GetVolume(volume);
      }
      
      virtual Error SetVolume(const MultiGain& volume)
      {
        return Delegate->SetVolume(volume);
      }
    private:
      std::auto_ptr<Impl> Delegate;
    };
  }
}
