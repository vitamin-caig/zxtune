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

#include <memory>
#define NOMINMAX
#include <dsound.h>  // IWYU pragma: export

namespace Sound::DirectSound
{
  class Api
  {
  public:
    using Ptr = std::shared_ptr<Api>;
    virtual ~Api() = default;

    // clang-format off

    virtual HRESULT DirectSoundEnumerateW(LPDSENUMCALLBACKW cb, LPVOID param) = 0;
    virtual HRESULT DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter) = 0;
    // clang-format on
  };

  // throw exception in case of error
  Api::Ptr LoadDynamicApi();
}  // namespace Sound::DirectSound
