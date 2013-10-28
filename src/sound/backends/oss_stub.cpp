/*
Abstract:
  Oss backend stub implementation

Last changed:
  $Id: oss_backend.cpp 1799 2012-06-11 15:04:38Z vitamin.caig $

Author:
  (C) Vitamin/CAIG/2001
*/

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
