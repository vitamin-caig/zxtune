/**
* 
* @file
*
* @brief Convert operation factory
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "operations.h"
//library includes
#include <sound/service.h>

namespace Playlist
{
  namespace Item
  {
    TextResultOperation::Ptr CreateConvertOperation(Playlist::Model::IndexSetPtr items,
      const String& type, Sound::Service::Ptr service, ConversionResultNotification::Ptr result);
  }
}
