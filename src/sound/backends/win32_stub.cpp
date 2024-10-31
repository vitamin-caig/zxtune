/**
 *
 * @file
 *
 * @brief  Win32 backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/storage.h"
#include "sound/backends/win32.h"

#include "sound/backend_attrs.h"

namespace Sound
{
  void RegisterWin32Backend(BackendsStorage& storage)
  {
    storage.Register(Win32::BACKEND_ID, Win32::BACKEND_DESCRIPTION, CAP_TYPE_SYSTEM);
  }

  namespace Win32
  {
    Device::Iterator::Ptr EnumerateDevices()
    {
      return Device::Iterator::CreateStub();
    }
  }  // namespace Win32
}  // namespace Sound
