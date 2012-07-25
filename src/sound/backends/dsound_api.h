/*
Abstract:
  DirectSound subsystem api gate interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#ifndef SOUND_BACKENDS_DSOUND_API_H_DEFINED
#define SOUND_BACKENDS_DSOUND_API_H_DEFINED

//platform-dependent includes
#define NOMINMAX
#include <dsound.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace ZXTune
{
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
}

#endif //SOUND_BACKENDS_DSOUND_API_H_DEFINED
