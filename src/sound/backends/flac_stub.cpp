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
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include "text/backends.h"

namespace ZXTune
{
  namespace Sound
  {
    void RegisterFlacBackend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterCreator(CreateDisabledBackendStub(Text::FLAC_BACKEND_ID, L10n::translate("FLAC support backend."), CAP_TYPE_FILE)); 
    }
  }
}
