/**
 *
 * @file
 *
 * @brief  DirectSound backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/dsound.h"
#include "sound/backends/storage.h"
// library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>

namespace Sound
{
  void RegisterDirectSoundBackend(BackendsStorage& storage)
  {
    storage.Register(DirectSound::BACKEND_ID, L10n::translate("DirectSound support backend."), CAP_TYPE_SYSTEM);
  }

  namespace DirectSound
  {
    Device::Iterator::Ptr EnumerateDevices()
    {
      return Device::Iterator::CreateStub();
    }
  }  // namespace DirectSound
}  // namespace Sound
