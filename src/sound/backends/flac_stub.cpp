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
#include "sound/backends/storage.h"
// library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
// text includes
#include <sound/backends/text/backends.h>

namespace Sound
{
  void RegisterFlacBackend(BackendsStorage& storage)
  {
    storage.Register(Text::FLAC_BACKEND_ID, L10n::translate("FLAC support backend."), CAP_TYPE_FILE);
  }
}  // namespace Sound
