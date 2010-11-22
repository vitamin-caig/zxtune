/*
Abstract:
  Playlist container internal implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "container_impl.h"
//common includes
#include <error.h>
#include <logging.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const std::string THIS_MODULE("Playlist::IO::Base");

  class CollectorStub : public Playlist::Item::DetectParameters
  {
  public:
    virtual bool ProcessItem(Playlist::Item::Data::Ptr item)
    {
      assert(!Item);
      Item = item;
      return false;
    }

    virtual void ShowProgress(unsigned /*progress*/)
    {
    }

    virtual void ShowMessage(const String& /*message*/)
    {
    }

    Playlist::Item::Data::Ptr GetItem() const
    {
      return Item;
    }
  private:
    Playlist::Item::Data::Ptr Item;
  };

  Playlist::Item::Data::Ptr OpenItem(const Playlist::Item::DataProvider& provider, const Playlist::IO::ContainerItem& item)
  {
    CollectorStub collector;
    //error is ignored, just take from collector
    provider.DetectModules(item.Path, collector);
    if (const Playlist::Item::Data::Ptr realItem = collector.GetItem())
    {
      Log::Debug(THIS_MODULE, "Opened '%1%'", item.Path);
      const Parameters::Container::Ptr newParams = realItem->GetAdjustedParameters();
      item.AdjustedParameters->Process(*newParams);
      return realItem;
    }
    Log::Debug(THIS_MODULE, "Failed to open '%1%'", item.Path);
    return Playlist::Item::Data::Ptr();
  }

  class PlayitemIteratorImpl : public Playlist::Item::Data::Iterator
  {
  public:
    PlayitemIteratorImpl(Playlist::Item::DataProvider::Ptr provider, Playlist::IO::ContainerItemsPtr container)
      : Provider(provider)
      , Container(container)
      , Current(Container->begin())
    {
      FetchItem();
    }

    virtual bool IsValid() const
    {
      return Current != Container->end();
    }
  
    virtual Playlist::Item::Data::Ptr Get() const
    {
      return Item;
    }

    virtual void Next()
    {
      assert(IsValid() || !"Invalid playitems iterator");
      ++Current;
      FetchItem();
    }
  private:
    void FetchItem()
    {
      for (Item.reset(); Current != Container->end(); ++Current)
      {
        if (Item = OpenItem(*Provider, *Current))
        {
          break;
        }
      }
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const Playlist::IO::ContainerItemsPtr Container;
    Playlist::IO::ContainerItems::const_iterator Current;
    Playlist::Item::Data::Ptr Item;
  };

  class ContainerImpl : public Playlist::IO::Container
  {
  public:
    ContainerImpl(Playlist::Item::DataProvider::Ptr provider,
      Parameters::Accessor::Ptr properties,
      Playlist::IO::ContainerItemsPtr items)
      : Provider(provider)
      , Properties(properties)
      , Items(items)
    {
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual Playlist::Item::Data::Iterator::Ptr GetItems() const
    {
      return Playlist::Item::Data::Iterator::Ptr(new PlayitemIteratorImpl(Provider, Items));
    }
  private:
    const Playlist::Item::DataProvider::Ptr Provider;
    const Parameters::Accessor::Ptr Properties;
    const Playlist::IO::ContainerItemsPtr Items;
  };
}

namespace Playlist
{
  namespace IO
  {
    Container::Ptr CreateContainer(Item::DataProvider::Ptr provider,
      Parameters::Accessor::Ptr properties,
      ContainerItemsPtr items)
    {
      return boost::make_shared<ContainerImpl>(provider, properties, items);
    }
  }
}
