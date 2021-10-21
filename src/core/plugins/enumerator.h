/**
 *
 * @file
 *
 * @brief  Plugins enumerator interface
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#pragma once

// std includes
#include <memory>
#include <vector>

namespace ZXTune
{
  template<class PluginType>
  class PluginsEnumerator
  {
  public:
    static const std::vector<typename PluginType::Ptr>& GetPlugins();
  };
}  // namespace ZXTune
