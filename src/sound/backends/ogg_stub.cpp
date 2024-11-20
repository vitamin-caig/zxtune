/**
 *
 * @file
 *
 * @brief  OGG backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/ogg.h"
#include "sound/backends/storage.h"

#include "sound/backend_attrs.h"

namespace Sound
{
  void RegisterOggBackend(BackendsStorage& storage)
  {
    storage.Register(Ogg::BACKEND_ID, Ogg::BACKEND_DESCRIPTION, CAP_TYPE_FILE);
  }
}  // namespace Sound
