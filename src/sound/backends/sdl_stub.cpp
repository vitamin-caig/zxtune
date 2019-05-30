/**
*
* @file
*
* @brief  SDL backend stub
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
  void RegisterSdlBackend(BackendsStorage& storage)
  {
    storage.Register(Text::SDL_BACKEND_ID, L10n::translate("SDL support backend"), CAP_TYPE_SYSTEM); 
  }
}
