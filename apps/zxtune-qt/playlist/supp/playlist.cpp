/*
Abstract:
  Playlist entity and view implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist.h"
#include "model.h"
#include "scanner.h"
#include "ui/utils.h"
//common includes
#include <logging.h>
//boost includes
#include <boost/make_shared.hpp>

namespace
{
  const std::string THIS_MODULE("Playlist::Support");
  class PlayitemIteratorImpl : public PlayitemIterator
  {
  public:
    PlayitemIteratorImpl(QObject& parent, const PlaylistModel& model)
      : PlayitemIterator(parent)
      , Model(model)
      , Index(0)
      , Item(Model.GetItem(Index))
      , State(STOPPED)
    {
    }

    virtual const Playitem* Get() const
    {
      return Item.get();
    }

    virtual PlayitemState GetState() const
    {
      return State;
    }

    virtual bool Reset(unsigned idx)
    {
      if (Playitem::Ptr item = Model.GetItem(idx))
      {
        Index = idx;
        Item = item;
        State = STOPPED;
        OnItem(*Item);
        return true;
      }
      return false;
    }

    virtual bool Next()
    {
      return Reset(Index + 1);
    }

    virtual bool Prev()
    {
      return Index &&
             Reset(Index - 1);
    }

    virtual void SetState(PlayitemState state)
    {
      State = state;
    }
  private:
    const PlaylistModel& Model;
    unsigned Index;
    Playitem::Ptr Item;
    PlayitemState State;
  };

  class PlaylistIOContainerImpl : public PlaylistIOContainer
  {
  public:
    PlaylistIOContainerImpl(const QString& name, const PlaylistModel& model)
      : Properties(Parameters::Container::Create())
      , Model(model)
    {
      Properties->SetStringValue(Playlist::ATTRIBUTE_NAME, FromQString(name));
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual Playitem::Iterator::Ptr GetItems() const
    {
      return Model.GetItems();
    }
  private:
    const Parameters::Container::Ptr Properties;
    const PlaylistModel& Model;
  };

  class PlaylistSupportImpl : public PlaylistSupport
  {
  public:
    PlaylistSupportImpl(QObject& parent, const QString& name, PlayitemsProvider::Ptr provider)
      : PlaylistSupport(parent)
      , Name(name)
      , Scanner(PlaylistScanner::Create(*this, provider))
      , Model(PlaylistModel::Create(*this))
      , Iterator(new PlayitemIteratorImpl(*this, *Model))
    {
      //setup connections
      Model->connect(Scanner, SIGNAL(OnGetItem(Playitem::Ptr)), SLOT(AddItem(Playitem::Ptr)));

      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~PlaylistSupportImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);

      Scanner->Cancel();
      Scanner->wait();
    }

    virtual QString GetName() const
    {
      return Name;
    }

    virtual class PlaylistScanner& GetScanner() const
    {
      return *Scanner;
    }

    virtual class PlaylistModel& GetModel() const
    {
      return *Model;
    }

    virtual PlayitemIterator& GetIterator() const
    {
      return *Iterator;
    }

    virtual PlaylistIOContainer::Ptr GetContainer() const
    {
      return boost::make_shared<PlaylistIOContainerImpl>(Name, *Model);
    }
  private:
    const QString Name;
    PlayitemsProvider::Ptr Provider;
    PlaylistScanner* const Scanner;
    PlaylistModel* const Model;
    PlayitemIterator* const Iterator;
  };
}

PlayitemIterator::PlayitemIterator(QObject& parent) : QObject(&parent)
{
}

PlaylistSupport::PlaylistSupport(QObject& parent) : QObject(&parent)
{
}

PlaylistSupport* PlaylistSupport::Create(QObject& parent, const QString& name, PlayitemsProvider::Ptr provider)
{
  return new PlaylistSupportImpl(parent, name, provider);
}
