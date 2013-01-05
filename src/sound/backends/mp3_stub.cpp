/*
Abstract:
  Mp3 file backend stub implementation

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
    void RegisterMp3Backend(BackendsEnumerator& enumerator)
    {
      enumerator.RegisterCreator(CreateDisabledBackendStub(Text::MP3_BACKEND_ID, L10n::translate("MP3 support backend"), CAP_TYPE_FILE)); 
    }
  }
}
