/*
Abstract:
  DirectSound backend stub implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "dsound.h"

namespace ZXTune
{
  namespace Sound
  {
    void RegisterDirectSoundBackend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }

    namespace DirectSound
    {
      Device::Iterator::Ptr EnumerateDevices()
      {
        return Device::Iterator::CreateStub();
      }
    }
  }
}
