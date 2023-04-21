/**
 *
 * @file
 *
 * @brief  FLAC backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/flac.h"
#include "sound/backends/storage.h"

namespace Sound
{
  void RegisterFlacBackend(BackendsStorage& storage)
  {
    storage.Register(Flac::BACKEND_ID, Flac::BACKEND_DESCRIPTION, CAP_TYPE_FILE);
  }
}  // namespace Sound
