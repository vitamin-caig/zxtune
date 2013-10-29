/**
*
* @file
*
* @brief  OSS backend stub
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "storage.h"
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include "text/backends.h"

namespace Sound
{
  void RegisterOssBackend(BackendsStorage& storage)
  {
    storage.Register(Text::OSS_BACKEND_ID, L10n::translate("OSS sound system backend"), CAP_TYPE_SYSTEM); 
  }
}
