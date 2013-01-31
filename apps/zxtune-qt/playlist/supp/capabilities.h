/*
Abstract:
  Playlist item capabilities helper

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_CAPABILITIES_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_CAPABILITIES_H_DEFINED

//local includes
#include "data.h"
//library includes
#include <core/plugin_attrs.h>

namespace Playlist
{
  namespace Item
  {
    class Capabilities
    {
    public:
      explicit Capabilities(Data::Ptr data)
        : PluginCaps(ZXTune::FindPlugin(data->GetType())->Capabilities())
      {
      }

      bool IsAYM() const
      {
        return 0 != (PluginCaps & ZXTune::CAP_DEV_AYM_MASK);
      }

      bool IsDAC() const
      {
        return 0 != (PluginCaps & ZXTune::CAP_DEV_DAC_MASK);
      }
    private:
      const uint_t PluginCaps;
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_CAPABILITIES_H_DEFINED
