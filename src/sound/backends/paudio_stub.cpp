/**
 *
 * @file
 *
 * @brief  PulseAudio backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/paudio.h"
#include "sound/backends/storage.h"

#include "sound/backend_attrs.h"

namespace Sound
{
  void RegisterPulseAudioBackend(BackendsStorage& storage)
  {
    storage.Register(PulseAudio::BACKEND_ID, PulseAudio::BACKEND_DESCRIPTION, CAP_TYPE_SYSTEM);
  }
}  // namespace Sound
