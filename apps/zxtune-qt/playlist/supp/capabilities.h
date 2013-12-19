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

//common includes
#include <types.h>
//library includes
#include <core/plugin_attrs.h>

namespace Playlist
{
  namespace Item
  {
    class Capabilities
    {
    public:
      explicit Capabilities(uint_t pluginCaps)
        : PluginCaps(pluginCaps)
      {
      }

      bool IsAYM() const
      {
        return 0 != (PluginCaps & (ZXTune::CAP_DEV_AY38910 | ZXTune::CAP_DEV_TURBOSOUND));
      }

      bool IsDAC() const
      {
        return 0 != (PluginCaps & ZXTune::CAP_DEV_DAC);
      }
    private:
      const uint_t PluginCaps;
    };
  }
}
