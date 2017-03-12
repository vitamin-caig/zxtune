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

//std includes
#include <memory>

namespace ZXTune
{
  template<class PluginType>
  class PluginsEnumerator
  {
  public:
    typedef std::shared_ptr<const PluginsEnumerator> Ptr;
    virtual ~PluginsEnumerator() = default;

    virtual typename PluginType::Iterator::Ptr Enumerate() const = 0;

    //! Enumerate all supported plugins
    static Ptr Create();
  };
}
