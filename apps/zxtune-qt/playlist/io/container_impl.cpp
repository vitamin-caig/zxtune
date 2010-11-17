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

  class CollectorStub : public PlayitemDetectParameters
  {
  public:
    virtual bool ProcessPlayitem(Playitem::Ptr item)
    {
      assert(!Item);
      Item = item;
      return false;
    }

    virtual void ShowProgress(const Log::MessageData& /*msg*/)
    {
    }

    Playitem::Ptr GetItem() const
    {
      return Item;
    }
  private:
    Playitem::Ptr Item;
  };

  Playitem::Ptr OpenItem(const PlayitemsProvider& provider, const PlaylistContainerItem& item)
  {
    CollectorStub collector;
    //error is ignored, just take from collector
    provider.DetectModules(item.Path, collector);
    if (const Playitem::Ptr realItem = collector.GetItem())
    {
      Log::Debug(THIS_MODULE, "Opened '%1%'", item.Path);
      const Parameters::Container::Ptr newParams = realItem->GetAdjustedParameters();
      item.AdjustedParameters->Process(*newParams);
      return realItem;
    }
    Log::Debug(THIS_MODULE, "Failed to open '%1%'", item.Path);
    return Playitem::Ptr();
  }

  class PlayitemIteratorImpl : public Playitem::Iterator
  {
  public:
    PlayitemIteratorImpl(PlayitemsProvider::Ptr provider, PlaylistContainerItemsPtr container)
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
  
    virtual Playitem::Ptr Get() const
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
    const PlayitemsProvider::Ptr Provider;
    const PlaylistContainerItemsPtr Container;
    PlaylistContainerItems::const_iterator Current;
    Playitem::Ptr Item;
  };

  class PlaylistIOContainerImpl : public PlaylistIOContainer
  {
  public:
    PlaylistIOContainerImpl(PlayitemsProvider::Ptr provider,
      Parameters::Accessor::Ptr properties,
      PlaylistContainerItemsPtr items)
      : Provider(provider)
      , Properties(properties)
      , Items(items)
    {
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual Playitem::Iterator::Ptr GetItems() const
    {
      return Playitem::Iterator::Ptr(new PlayitemIteratorImpl(Provider, Items));
    }
  private:
    const PlayitemsProvider::Ptr Provider;
    const Parameters::Accessor::Ptr Properties;
    const PlaylistContainerItemsPtr Items;
  };
}

PlaylistIOContainer::Ptr CreatePlaylistIOContainer(PlayitemsProvider::Ptr provider,
  Parameters::Accessor::Ptr properties,
  PlaylistContainerItemsPtr items)
{
  return boost::make_shared<PlaylistIOContainerImpl>(provider, properties, items);
}
