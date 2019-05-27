/**
*
* @file
*
* @brief  AYLPT backend stub
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sound/backends/storage.h"
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include <sound/backends/text/backends.h>

namespace Sound
{
  void RegisterAyLptBackend(BackendsStorage& storage)
  {
    storage.Register(Text::AYLPT_BACKEND_ID, L10n::translate("Real AY via LPT backend"), CAP_TYPE_HARDWARE); 
  }
}
