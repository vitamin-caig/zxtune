/**
 *
 * @file
 *
 * @brief  DirectSound subsystem API gate implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/gates/dsound_api.h"
// common includes
#include <make_ptr.h>
// library includes
#include <debug/log.h>
#include <platform/shared_library_adapter.h>

namespace Sound
{
  namespace DirectSound
  {

    class DynamicApi : public Api
    {
    public:
      explicit DynamicApi(Platform::SharedLibrary::Ptr lib)
        : Lib(std::move(lib))
      {
        Debug::Log("Sound::Backend::DirectSound", "Library loaded");
      }

      ~DynamicApi() override
      {
        Debug::Log("Sound::Backend::DirectSound", "Library unloaded");
      }

      
      HRESULT DirectSoundEnumerateW(LPDSENUMCALLBACKW cb, LPVOID param) override
      {
        static const char NAME[] = "DirectSoundEnumerateW";
        typedef HRESULT (WINAPI *FunctionType)(LPDSENUMCALLBACKW, LPVOID);
        const FunctionType func = Lib.GetSymbol<FunctionType>(NAME);
        return func(cb, param);
      }
      
      HRESULT DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter) override
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
      auto lib = Platform::SharedLibrary::Load("dsound"_sv);
      return MakePtr<DynamicApi>(std::move(lib));
    }
  }  // namespace DirectSound
}  // namespace Sound
