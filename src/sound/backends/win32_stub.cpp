/**
*
* @file
*
* @brief  Win32 backend stub
*
* @author vitamin.caig@gmail.com
*
**/

#include "win32.h"
#include "storage.h"
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include "text/backends.h"

namespace Sound
{
  void RegisterWin32Backend(BackendsStorage& storage)
  {
    storage.Register(Text::WIN32_BACKEND_ID, L10n::translate("Win32 sound system backend"), CAP_TYPE_SYSTEM);
  }

  namespace Win32
  {
    Device::Iterator::Ptr EnumerateDevices()
    {
      return Device::Iterator::CreateStub();
    }
  }
}

