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

//platform-dependent includes
#define NOMINMAX
#include <dsound.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Sound
{
  namespace DirectSound
  {
    class Api
    {
    public:
      typedef boost::shared_ptr<Api> Ptr;
      virtual ~Api() {}

      
      virtual HRESULT DirectSoundEnumerateA(LPDSENUMCALLBACKA cb, LPVOID param) = 0;
      virtual HRESULT DirectSoundCreate(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter) = 0;
    };

    //throw exception in case of error
    Api::Ptr LoadDynamicApi();

  }
}
