/**
 *
 * @file
 *
 * @brief  PulseAudio backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/paudio.h"
#include "sound/backends/storage.h"

namespace Sound
{
  void RegisterPulseAudioBackend(BackendsStorage& storage)
  {
    storage.Register(PulseAudio::BACKEND_ID, PulseAudio::BACKEND_DESCRIPTION, CAP_TYPE_SYSTEM);
  }
}  // namespace Sound
