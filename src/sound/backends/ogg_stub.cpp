/**
 *
 * @file
 *
 * @brief  OGG backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/ogg.h"
#include "sound/backends/storage.h"

namespace Sound
{
  void RegisterOggBackend(BackendsStorage& storage)
  {
    storage.Register(Ogg::BACKEND_ID, Ogg::BACKEND_DESCRIPTION, CAP_TYPE_FILE);
  }
}  // namespace Sound
