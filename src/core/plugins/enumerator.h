/*
Abstract:
  Plugins enumerator interface and related

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_ENUMERATOR_H_DEFINED__
#define __CORE_PLUGINS_ENUMERATOR_H_DEFINED__

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
    virtual typename PluginType::Ptr Find(const String& id) const = 0;

    //! Enumerate all supported plugins
    static Ptr Create();
  };

  typedef PluginsEnumerator<ArchivePlugin> ArchivePluginsEnumerator;
  typedef PluginsEnumerator<PlayerPlugin> PlayerPluginsEnumerator;
}

#endif //__CORE_PLUGINS_ENUMERATOR_H_DEFINED__
