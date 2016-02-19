/**
*
* @file
*
* @brief  OpenAL backend stub
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "storage.h"
//library includes
#include <l10n/api.h>
#include <sound/backend_attrs.h>
//text includes
#include "text/backends.h"

namespace Sound
{
  void RegisterOpenAlBackend(BackendsStorage& storage)
  {
    storage.Register(Text::OPENAL_BACKEND_ID, L10n::translate("OpenAL backend"), CAP_TYPE_SYSTEM); 
  }
}
