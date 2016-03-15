/**
*
* @file
*
* @brief  DirectSound subsystem API gate implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "dsound_api.h"
//common includes
#include <make_ptr.h>
//library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace DirectSound
  {
    const Debug::Stream Dbg("Sound::Backend::DirectSound");

    class DynamicApi : public Api
    {
    public:
      explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
        : Lib(lib)
      {
        Dbg("Library loaded");
      }

      virtual ~DynamicApi()
      {
        Dbg("Library unloaded");
      }

      
      virtual HRESULT DirectSoundEnumerateA(LPDSENUMCALLBACKA cb, LPVOID param)
      {
        static const char NAME[] = "DirectSoundEnumerateA";
        typedef HRESULT (WINAPI *FunctionType)(LPDSENUMCALLBACKA, LPVOID);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(cb, param);
      }
      
      virtual HRESULT DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
      {
        static const char NAME[] = "DirectSoundCreate";
        typedef HRESULT (WINAPI *FunctionType)(LPCGUID, LPDIRECTSOUND*, LPUNKNOWN);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(pcGuidDevice, ppDS, pUnkOuter);
      }
      
    private:
      const Platform::SharedLibraryAdapter Lib;
    };


    Api::Ptr LoadDynamicApi()
    {
      const Platform::SharedLibrary::Ptr lib = Platform::SharedLibrary::Load("dsound");
      return MakePtr<DynamicApi>(lib);
    }
  }
}
