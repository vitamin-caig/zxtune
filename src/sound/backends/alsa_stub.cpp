/*
Abstract:
  ALSA backend stub implementation

Last changed:
  $Id: alsa_backend.cpp 1837 2012-07-01 13:42:01Z vitamin.caig $

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "alsa.h"

namespace ZXTune
{
  namespace Sound
  {
    void RegisterAlsaBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }

    namespace ALSA
    {
      Device::Iterator::Ptr EnumerateDevices()
      {
        return Device::Iterator::CreateStub();
      }
    }
  }
}
