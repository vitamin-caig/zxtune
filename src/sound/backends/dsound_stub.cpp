/*
Abstract:
  DirectSound backend stub implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#include "dsound.h"
#include "storage.h"
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include "text/backends.h"

namespace Sound
{
  void RegisterDirectSoundBackend(BackendsStorage& storage)
  {
    storage.Register(Text::DSOUND_BACKEND_ID, L10n::translate("DirectSound support backend."), CAP_TYPE_SYSTEM);
  }

  namespace DirectSound
  {
    Device::Iterator::Ptr EnumerateDevices()
    {
      return Device::Iterator::CreateStub();
    }
  }
}

