/*
Abstract:
  Alsa backend stub implementation

Last changed:
  $Id: alsa_backend.cpp 1837 2012-07-01 13:42:01Z vitamin.caig $

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "alsa.h"
#include "storage.h"
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include "text/backends.h"

namespace Sound
{
  void RegisterAlsaBackend(BackendsStorage& storage)
  {
    storage.Register(Text::ALSA_BACKEND_ID, L10n::translate("ALSA sound system backend"), CAP_TYPE_SYSTEM);
  }

  namespace Alsa
  {
    Device::Iterator::Ptr EnumerateDevices()
    {
      return Device::Iterator::CreateStub();
    }
  }
}
