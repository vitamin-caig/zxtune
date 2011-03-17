/*
Abstract:
  Plugins chain implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001
*/

//local includes
#include "plugins_chain.h"
//common includes
#include <string_helpers.h>
//std includes
#include <list>
//boost includes
#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/join.hpp>
//text includes
#include <core/text/core.h>

namespace
{
  using namespace ZXTune;

  class PluginsChainImpl : public PluginsChain
  {
    typedef std::list<Plugin::Ptr> PluginsList;
  public:
    virtual void Add(Plugin::Ptr plugin)
    {
      Container.push_back(plugin);
    }

    virtual Plugin::Ptr GetLast() const
    {
      if (!Container.empty())
      {
        return Container.back();
      }
      return Plugin::Ptr();
    }

    virtual uint_t Count() const
    {
      return Container.size();
    }

    virtual String AsString() const
    {
      StringArray ids(Container.size());
      std::transform(Container.begin(), Container.end(),
        ids.begin(), boost::mem_fn(&Plugin::Id));
      return boost::algorithm::join(ids, String(Text::MODULE_CONTAINERS_DELIMITER));
    }
  private:
    PluginsList Container;
  };

  class NestedPluginsChain : public PluginsChain
  {
  public:
    NestedPluginsChain(PluginsChain::ConstPtr parent, Plugin::Ptr newOne)
      : Parent(parent)
      , NewPlug(newOne)
    {
    }

    virtual void Add(Plugin::Ptr /*plugin*/)
    {
      assert(!"Should not be called");
    }

    virtual Plugin::Ptr GetLast() const
    {
      return NewPlug;
    }

    virtual uint_t Count() const
    {
      return Parent->Count() + 1;
    }

    virtual String AsString() const
    {
      const String thisId = NewPlug->Id();
      if (Parent->Count())
      {
        return Parent->AsString() + Text::MODULE_CONTAINERS_DELIMITER + thisId;
      }
      else
      {
        return thisId;
      }
    }
  private:
    const PluginsChain::ConstPtr Parent;
    const Plugin::Ptr NewPlug;
  };
}

namespace ZXTune
{
  PluginsChain::Ptr PluginsChain::Create()
  {
    return boost::make_shared<PluginsChainImpl>();
  }

  PluginsChain::Ptr PluginsChain::CreateMerged(PluginsChain::ConstPtr parent, Plugin::Ptr newOne)
  {
    return boost::make_shared<NestedPluginsChain>(parent, newOne);
  }

}
