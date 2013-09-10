/*
Abstract:
  Ogg file backend stub implementation

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
  void RegisterOggBackend(BackendsStorage& storage)
  {
    storage.Register(Text::OGG_BACKEND_ID, L10n::translate("OGG support backend"), CAP_TYPE_FILE); 
  }
}
