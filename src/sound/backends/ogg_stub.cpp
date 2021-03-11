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
#include "sound/backends/storage.h"
// library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
// text includes
#include <sound/backends/text/backends.h>

namespace Sound
{
  void RegisterOggBackend(BackendsStorage& storage)
  {
    storage.Register(Text::OGG_BACKEND_ID, L10n::translate("OGG support backend"), CAP_TYPE_FILE);
  }
}  // namespace Sound
