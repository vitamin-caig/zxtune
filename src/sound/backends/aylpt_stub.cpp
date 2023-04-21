/**
 *
 * @file
 *
 * @brief  AYLPT backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/aylpt.h"
#include "sound/backends/storage.h"

namespace Sound
{
  void RegisterAyLptBackend(BackendsStorage& storage)
  {
    storage.Register(AyLpt::BACKEND_ID, AyLpt::BACKEND_DESCRIPTION, CAP_TYPE_HARDWARE);
  }
}  // namespace Sound
