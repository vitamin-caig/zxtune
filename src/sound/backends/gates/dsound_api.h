/**
 *
 * @file
 *
 * @brief  DirectSound subsystem API gate interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <memory>
// platform-dependent includes
#define NOMINMAX
#include <dsound.h>

namespace Sound
{
  namespace DirectSound
  {
    class Api
    {
    public:
      typedef std::shared_ptr<Api> Ptr;
      virtual ~Api() = default;

// clang-format off

      virtual HRESULT DirectSoundEnumerateW(LPDSENUMCALLBACKW cb, LPVOID param) = 0;
      virtual HRESULT DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter) = 0;
// clang-format on
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();
  }
}  // namespace Sound
