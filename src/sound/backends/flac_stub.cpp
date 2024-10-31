/**
 *
 * @file
 *
 * @brief  FLAC backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/flac.h"
#include "sound/backends/storage.h"

#include "sound/backend_attrs.h"

namespace Sound
{
  void RegisterFlacBackend(BackendsStorage& storage)
  {
    storage.Register(Flac::BACKEND_ID, Flac::BACKEND_DESCRIPTION, CAP_TYPE_FILE);
  }
}  // namespace Sound
