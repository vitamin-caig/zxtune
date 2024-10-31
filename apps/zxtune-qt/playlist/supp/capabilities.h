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

#include <core/plugin_attrs.h>

#include <types.h>

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

  private:
    const uint_t PluginCaps;
  };
}  // namespace Playlist::Item
