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

#include "core/plugin_attrs.h"

#include "types.h"

namespace Playlist::Item
{
  class Capabilities
  {
  public:
    explicit Capabilities(uint_t pluginCaps)
      : PluginCaps(pluginCaps)
    {}

    bool IsAYM() const
    {
      using namespace ZXTune::Capabilities::Module::Device;
      return 0 != (PluginCaps & (AY38910 | TURBOSOUND));
    }

    bool IsDAC() const
    {
      using namespace ZXTune::Capabilities::Module::Device;
      return 0 != (PluginCaps & DAC);
    }

    // DAC is a weak trait (same over-match as in the control panel): it is shared by frame-based
    // in-tree players and sample/stream-based external ones
    bool IsFramed() const
    {
      using namespace ZXTune::Capabilities::Module::Device;
      return 0 != (PluginCaps & (AY38910 | TURBOSOUND | TURBOFM | DAC | SAA1099));
    }

  private:
    const uint_t PluginCaps;
  };
}  // namespace Playlist::Item
