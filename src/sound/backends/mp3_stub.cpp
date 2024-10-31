/**
 *
 * @file
 *
 * @brief  MP3 backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/mp3.h"
#include "sound/backends/storage.h"

#include "sound/backend_attrs.h"

namespace Sound
{
  void RegisterMp3Backend(BackendsStorage& storage)
  {
    storage.Register(Mp3::BACKEND_ID, Mp3::BACKEND_DESCRIPTION, CAP_TYPE_FILE);
  }
}  // namespace Sound
