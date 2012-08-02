/*
Abstract:
  Flac file stub backend implementation

Last changed:
  $Id$

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
    void RegisterFlacBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterCreator(CreateDisabledBackendStub(Text::FLAC_BACKEND_ID, Text::FLAC_BACKEND_DESCRIPTION, CAP_TYPE_FILE)); 
    }
  }
}
