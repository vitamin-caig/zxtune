/**
 *
 * @file
 *
 * @brief  OSS backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "sound/backends/oss.h"
#include "sound/backends/storage.h"
// library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>

namespace Sound
{
  void RegisterOssBackend(BackendsStorage& storage)
  {
    storage.Register(Oss::BACKEND_ID, L10n::translate("OSS sound system backend"), CAP_TYPE_SYSTEM);
  }
}  // namespace Sound
