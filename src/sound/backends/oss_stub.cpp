/**
 *
 * @file
 *
 * @brief  OSS backend stub
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "sound/backends/oss.h"
#include "sound/backends/storage.h"

#include "sound/backend_attrs.h"

namespace Sound
{
  void RegisterOssBackend(BackendsStorage& storage)
  {
    storage.Register(Oss::BACKEND_ID, Oss::BACKEND_DESCRIPTION, CAP_TYPE_SYSTEM);
  }
}  // namespace Sound
