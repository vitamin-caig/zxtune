/*
Abstract:
  Win32 waveout backend stub implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "win32.h"

namespace ZXTune
{
  namespace Sound
  {
    void RegisterWin32Backend(class BackendsEnumerator& /*enumerator*/)
    {
      //do nothing
    }

    namespace Win32
    {
      Device::Iterator::Ptr EnumerateDevices()
      {
        return Device::Iterator::CreateStub();
      }
    }
  }
}
