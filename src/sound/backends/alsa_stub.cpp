/**
 *
 * @file
 *
 * @brief  ALSA backend stub implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/alsa.h"
#include "sound/backends/storage.h"

namespace Sound
{
  void RegisterAlsaBackend(BackendsStorage& storage)
  {
    storage.Register(Alsa::BACKEND_ID, Alsa::BACKEND_DESCRIPTION, CAP_TYPE_SYSTEM);
  }

  namespace Alsa
  {
    Device::Iterator::Ptr EnumerateDevices()
    {
      return Device::Iterator::CreateStub();
    }
  }  // namespace Alsa
}  // namespace Sound
