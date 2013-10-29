/**
* 
* @file
*
* @brief Playlist item capabilities helper
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "data.h"
//library includes
#include <core/plugin.h>
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
