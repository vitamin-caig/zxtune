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
  class PluginsEnumerator
  {
  public:
    typedef boost::shared_ptr<const PluginsEnumerator> Ptr;
    virtual ~PluginsEnumerator() {}

    virtual Plugin::Iterator::Ptr Enumerate() const = 0;
    virtual PlayerPlugin::Iterator::Ptr EnumeratePlayers() const = 0;
    virtual ArchivePlugin::Iterator::Ptr EnumerateArchives() const = 0;

    //! Enumerate all supported plugins
    static Ptr Create();
  };

  //TODO: remove
  struct ModuleRegion
  {
    ModuleRegion() : Offset(), Size()
    {
    }
    ModuleRegion(std::size_t off, std::size_t sz)
      : Offset(off), Size(sz)
    {
    }
    std::size_t Offset;
    std::size_t Size;

    uint_t Checksum(const Binary::Container& container) const;
    Binary::Container::Ptr Extract(const Binary::Container& container) const;
  };
}

#endif //__CORE_PLUGINS_ENUMERATOR_H_DEFINED__
