/**
*
* @file
*
* @brief  ALSA backend stub implementation
*
* @author vitamin.caig@gmail.com
*
**/

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
