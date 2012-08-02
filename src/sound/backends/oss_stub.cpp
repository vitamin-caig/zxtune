/*
Abstract:
  Oss backend stub implementation

Last changed:
  $Id: oss_backend.cpp 1799 2012-06-11 15:04:38Z vitamin.caig $

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "enumerator.h"
//library includes
#include <sound/backend_attrs.h>
//text includes
#include <sound/text/backends.h>
 
namespace ZXTune
{
  namespace Sound
  {
    void RegisterOssBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterCreator(CreateDisabledBackendStub(Text::OSS_BACKEND_ID, Text::OSS_BACKEND_DESCRIPTION, CAP_TYPE_SYSTEM)); 
    }
  }
}
