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
    typedef boost::shared_ptr<PluginsChain> Ptr;
    typedef boost::shared_ptr<const PluginsChain> ConstPtr;

    virtual ~PluginsChain() {}

    virtual void Add(Plugin::Ptr plugin) = 0;
    virtual Plugin::Ptr GetLast() const = 0;

    virtual uint_t Count() const = 0;
    virtual String AsString() const = 0;

    static Ptr Create();
    static Ptr CreateMerged(PluginsChain::ConstPtr parent, Plugin::Ptr newOne);
  };
}

#endif //__CORE_PLUGINS_CHAIN_H_DEFINED__
