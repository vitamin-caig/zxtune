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

  class ItemIteratorImpl : public Playlist::Item::Iterator
  {
  public:
    ItemIteratorImpl(QObject& parent, const Playlist::Model& model)
      : Playlist::Item::Iterator(parent)
      , Model(model)
      , Index(0)
      , Item(Model.GetItem(Index))
      , State(Playlist::Item::STOPPED)
    {
    }

    virtual const Playitem* Get() const
    {
      return Item.get();
    }

    virtual Playlist::Item::State GetState() const
    {
      return State;
    }

    virtual bool Reset(unsigned idx)
    {
      if (Playitem::Ptr item = Model.GetItem(idx))
      {
        Index = idx;
        Item = item;
        State = Playlist::Item::STOPPED;
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

    virtual void SetState(Playlist::Item::State state)
    {
      State = state;
    }
  private:
    const Playlist::Model& Model;
    unsigned Index;
    Playitem::Ptr Item;
    Playlist::Item::State State;
  };

  class ContainerImpl : public Playlist::IO::Container
  {
  public:
    ContainerImpl(const QString& name, const Playlist::Model& model)
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
    const Playlist::Model& Model;
  };

  class SupportImpl : public Playlist::Support
  {
  public:
    SupportImpl(QObject& parent, const QString& name, PlayitemsProvider::Ptr provider)
      : Playlist::Support(parent)
      , Name(name)
      , Scanner(Playlist::Scanner::Create(*this, provider))
      , Model(Playlist::Model::Create(*this))
      , Iterator(new ItemIteratorImpl(*this, *Model))
    {
      //setup connections
      Model->connect(Scanner, SIGNAL(OnGetItem(Playitem::Ptr)), SLOT(AddItem(Playitem::Ptr)));

      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~SupportImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);

      Scanner->Cancel();
      Scanner->wait();
    }

    virtual QString GetName() const
    {
      return Name;
    }

    virtual class Playlist::Scanner& GetScanner() const
    {
      return *Scanner;
    }

    virtual class Playlist::Model& GetModel() const
    {
      return *Model;
    }

    virtual Playlist::Item::Iterator& GetIterator() const
    {
      return *Iterator;
    }

    virtual Playlist::IO::Container::Ptr GetContainer() const
    {
      return boost::make_shared<ContainerImpl>(Name, *Model);
    }
  private:
    const QString Name;
    PlayitemsProvider::Ptr Provider;
    Playlist::Scanner* const Scanner;
    Playlist::Model* const Model;
    Playlist::Item::Iterator* const Iterator;
  };
}

namespace Playlist
{
  namespace Item
  {
    Iterator::Iterator(QObject& parent) : QObject(&parent)
    {
    }
  }

  Support::Support(QObject& parent) : QObject(&parent)
  {
  }

  Support* Support::Create(QObject& parent, const QString& name, PlayitemsProvider::Ptr provider)
  {
    return new SupportImpl(parent, name, provider);
  }
}
