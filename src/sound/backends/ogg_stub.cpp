/*
Abstract:
  Ogg file backend stub implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "enumerator.h"
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include <sound/text/backends.h> 

namespace ZXTune
{
  namespace Sound
  {
    void RegisterOggBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterCreator(CreateDisabledBackendStub(Text::OGG_BACKEND_ID, L10n::translate("OGG support backend"), CAP_TYPE_FILE)); 
    }
  }
}
