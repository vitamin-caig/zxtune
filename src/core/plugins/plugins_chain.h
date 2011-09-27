/*
Abstract:
  Plugins chain definitions

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

#pragma once
#ifndef __CORE_PLUGINS_CHAIN_H_DEFINED__
#define __CORE_PLUGINS_CHAIN_H_DEFINED__

//library includes
#include <core/plugin.h>

namespace ZXTune
{
  class PluginsChain
  {
  public:
    typedef boost::shared_ptr<const PluginsChain> Ptr;

    virtual ~PluginsChain() {}

    virtual String AsString() const = 0;

    static Ptr CreateMerged(PluginsChain::Ptr parent, Plugin::Ptr newOne);
  };
}

#endif //__CORE_PLUGINS_CHAIN_H_DEFINED__
