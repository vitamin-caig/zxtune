/*
Abstract:
  Flac file stub backend implementation

Last changed:
  $Id$

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
  void RegisterFlacBackend(BackendsStorage& storage)
  {
    storage.Register(Text::FLAC_BACKEND_ID, L10n::translate("FLAC support backend."), CAP_TYPE_FILE); 
  }
}
