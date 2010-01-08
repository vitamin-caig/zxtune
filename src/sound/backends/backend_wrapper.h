/*
Abstract:
  Safe backend wrapper. Used for safe playback stopping if object is destroyed

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include <sound/backend.h>

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
      
      virtual void GetInfo(BackendInfo& info) const
      {
        return Delegate->GetInfo(info);
      }

      virtual Error SetModule(Module::Holder::Ptr holder)
      {
        return Delegate->SetModule(holder);
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

      virtual Event WaitForEvent(Event evt, unsigned timeoutMs) const
      {
        return Delegate->WaitForEvent(evt, timeoutMs);
      }

      virtual Error SetMixer(const std::vector<MultiGain>& data)
      {
        return Delegate->SetMixer(data);
      }

      virtual Error SetFilter(Converter::Ptr converter)
      {
        return Delegate->SetFilter(converter);
      }

      virtual Error GetVolume(MultiGain& volume) const
      {
        return Delegate->GetVolume(volume);
      }
      
      virtual Error SetVolume(const MultiGain& volume)
      {
        return Delegate->SetVolume(volume);
      }

      virtual Error SetParameters(const Parameters::Map& params)
      {
        return Delegate->SetParameters(params);
      }
      
      virtual Error GetParameters(Parameters::Map& params) const
      {
        return Delegate->GetParameters(params);
      }
      
    private:
      std::auto_ptr<Impl> Delegate;
    };
  }
}
