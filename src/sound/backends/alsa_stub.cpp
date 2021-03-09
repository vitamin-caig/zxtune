/**
 *
 * @file
 *
 * @brief  ALSA backend stub implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/alsa.h"
#include "sound/backends/storage.h"
// library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
// text includes
#include <sound/backends/text/backends.h>

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
  }  // namespace Alsa
}  // namespace Sound
