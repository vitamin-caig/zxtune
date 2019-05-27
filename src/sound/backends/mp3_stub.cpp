/**
*
* @file
*
* @brief  MP3 backend stub
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
  void RegisterMp3Backend(BackendsStorage& storage)
  {
    storage.Register(Text::MP3_BACKEND_ID, L10n::translate("MP3 support backend"), CAP_TYPE_FILE); 
  }
}
