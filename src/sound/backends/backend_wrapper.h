/*
Abstract:
  Safe backend wrapper. Used for safe playback stopping if object is destroyed

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//library includes
#include <sound/backend.h>
//std includes
#include <memory>

namespace ZXTune
{
  namespace Sound
  {
    template<class Impl>
    class SafeBackendWrapper : public Backend
    {
    public:
      explicit SafeBackendWrapper(const Parameters::Map& params) : Delegate(new Impl())
      {
        //apply parameters to delegate
        ThrowIfError(Delegate->SetParameters(params));
        //perform fast test to detect if parameters are correct
        Delegate->OnStartup();
        Delegate->OnShutdown();
      }
      
      virtual ~SafeBackendWrapper()
      {
        Delegate->Stop();//TODO: warn if error
      }
      
      virtual void GetInformation(BackendInformation& info) const
      {
        return Delegate->GetInformation(info);
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
      
      virtual Error SetPosition(uint_t frame)
      {
        return Delegate->SetPosition(frame);
      }
      
      virtual Error GetCurrentState(State& state) const
      {
        return Delegate->GetCurrentState(state);
      }

      virtual Event WaitForEvent(Event evt, uint_t timeoutMs) const
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

      virtual VolumeControl::Ptr GetVolumeControl() const
      {
        return Delegate->GetVolumeControl();
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
