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

namespace Sound::DirectSound
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

    // clang-format off

    HRESULT DirectSoundEnumerateW(LPDSENUMCALLBACKW cb, LPVOID param) override
    {
      using FunctionType = decltype(&::DirectSoundEnumerateW);
      const auto func = Lib.GetSymbol<FunctionType>("DirectSoundEnumerateW");
      return func(cb, param);
    }

    HRESULT DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter) override
    {
      using FunctionType = decltype(&::DirectSoundCreate);
      const auto func = Lib.GetSymbol<FunctionType>("DirectSoundCreate");
      return func(pcGuidDevice, ppDS, pUnkOuter);
    }

    // clang-format on
  private:
    const Platform::SharedLibraryAdapter Lib;
  };

  Api::Ptr LoadDynamicApi()
  {
    auto lib = Platform::SharedLibrary::Load("dsound"_sv);
    return MakePtr<DynamicApi>(std::move(lib));
  }
}  // namespace Sound::DirectSound
