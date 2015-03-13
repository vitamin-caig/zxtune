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

//local includes
#include "plugins_types.h"

namespace ZXTune
{
  template<class PluginType>
  class PluginsEnumerator
  {
  public:
    typedef boost::shared_ptr<const PluginsEnumerator> Ptr;
    virtual ~PluginsEnumerator() {}

    virtual typename PluginType::Iterator::Ptr Enumerate() const = 0;

    //! Enumerate all supported plugins
    static Ptr Create();
  };

  typedef PluginsEnumerator<ArchivePlugin> ArchivePluginsEnumerator;
  typedef PluginsEnumerator<PlayerPlugin> PlayerPluginsEnumerator;
}
