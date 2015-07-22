/**
*
* @file
*
* @brief  PulseAudio backend stub
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
  void RegisterPulseAudioBackend(BackendsStorage& storage)
  {
    storage.Register(Text::PAUDIO_BACKEND_ID, L10n::translate("PulseAudio support backend"), CAP_TYPE_SYSTEM); 
  }
}
