/*
Abstract:
  Convert operation factory

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_CONVERT_OPERATION_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_CONVERT_OPERATION_H_DEFINED

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

#endif //ZXTUNE_QT_PLAYLIST_SUPP_CONVERT_OPERATION_H_DEFINED
