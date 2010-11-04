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
#include "playlist_moc.h"
#include "playlist_model.h"
#include "playlist_scanner.h"

namespace
{
  class PlayitemIteratorImpl : public PlayitemIterator
  {
  public:
    PlayitemIteratorImpl(QObject* parent, const PlaylistModel& model)
      : Model(model)
      , Index(0)
      , Item(Model.GetItem(Index))
      , State(STOPPED)
    {
      setParent(parent);
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

  class PlaylistSupportImpl : public PlaylistSupport
  {
  public:
    PlaylistSupportImpl(QObject* parent, const QString& name, PlayitemsProvider::Ptr provider)
      : Scanner(PlaylistScanner::Create(this, provider))
      , Model(PlaylistModel::Create(this))
      , Iterator(new PlayitemIteratorImpl(this, *Model))
    {
      //setup self
      setParent(parent);
      setObjectName(name);
      //setup connections
      Model->connect(Scanner, SIGNAL(OnGetItem(Playitem::Ptr)), SLOT(AddItem(Playitem::Ptr)));
    }

    virtual ~PlaylistSupportImpl()
    {
      Scanner->Cancel();
      Scanner->wait();
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

  private:
    PlayitemsProvider::Ptr Provider;
    PlaylistScanner* const Scanner;
    PlaylistModel* const Model;
    PlayitemIterator* const Iterator;
  };
}

PlaylistSupport* PlaylistSupport::Create(QObject* parent, const QString& name, PlayitemsProvider::Ptr provider)
{
  return new PlaylistSupportImpl(parent, name, provider);
}
