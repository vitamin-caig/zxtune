/**
 *
 * @file
 *
 * @brief  SDL backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/sdl.h"
#include "sound/backends/storage.h"
// library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>

namespace Sound
{
  void RegisterSdlBackend(BackendsStorage& storage)
  {
    storage.Register(Sdl::BACKEND_ID, L10n::translate("SDL support backend"), CAP_TYPE_SYSTEM);
  }
}  // namespace Sound
