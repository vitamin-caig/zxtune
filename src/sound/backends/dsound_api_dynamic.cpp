/*
Abstract:
  DirectSound subsystem api dynamic gate implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "dsound_api.h"
//common includes
#include <logging.h>
#include <shared_library_adapter.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const std::string THIS_MODULE("Sound::Backend::DirectSound");

  using namespace ZXTune::Sound::DirectSound;

  class DynamicApi : public Api
  {
  public:
    explicit DynamicApi(SharedLibrary::Ptr lib)
      : Lib(lib)
    {
      Log::Debug(THIS_MODULE, "Library loaded");
    }

    virtual ~DynamicApi()
    {
      Log::Debug(THIS_MODULE, "Library unloaded");
    }

    
    virtual HRESULT DirectSoundEnumerateA(LPDSENUMCALLBACKA cb, LPVOID param)
    {
      static const char* NAME = "DirectSoundEnumerateA";
      typedef HRESULT (*FunctionType)(LPDSENUMCALLBACKA, LPVOID);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(cb, param);
    }
    
    virtual HRESULT DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
    {
      static const char* NAME = "DirectSoundCreate";
      typedef HRESULT (*FunctionType)(LPCGUID, LPDIRECTSOUND*, LPUNKNOWN);
      const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
      return func(pcGuidDevice, ppDS, pUnkOuter);
    }
    
  private:
    const SharedLibraryAdapter Lib;
  };

}

namespace ZXTune
{
  namespace Sound
  {
    namespace DirectSound
    {
      Api::Ptr LoadDynamicApi()
      {
        const SharedLibrary::Ptr lib = SharedLibrary::Load("dsound");
        return boost::make_shared<DynamicApi>(lib);
      }
    }
  }
}
