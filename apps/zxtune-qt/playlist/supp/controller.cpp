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
#include "controller.h"
#include "model.h"
#include "scanner.h"
#include "ui/utils.h"
//common includes
#include <logging.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMessageBox>

namespace
{
  const std::string THIS_MODULE("Playlist::Controller");

  unsigned Randomized(unsigned idx, unsigned total)
  {
    ::srand(::time(0) * idx);
    return ::rand() % total;
  }

  const unsigned NO_INDEX = ~0;

  class ItemIteratorImpl : public Playlist::Item::Iterator
  {
  public:
    ItemIteratorImpl(QObject& parent, Playlist::Model::Ptr model)
      : Playlist::Item::Iterator(parent)
      , Model(model)
      , Index(NO_INDEX)
      , Item()
      , State(Playlist::Item::STOPPED)
    {
    }

    virtual const Playlist::Item::Data* GetData() const
    {
      return Item.get();
    }

    virtual unsigned GetIndex() const
    {
      return Index;
    }

    virtual Playlist::Item::State GetState() const
    {
      return State;
    }

    virtual bool Next(unsigned playorderMode)
    {
      return
        Index != NO_INDEX &&
        Navigate(Index + 1, playorderMode);
    }

    virtual bool Prev(unsigned playorderMode)
    {
      return
        Index != NO_INDEX &&
        Navigate(int(Index) - 1, playorderMode);
    }

    virtual void SetState(Playlist::Item::State state)
    {
      State = state;
    }

    virtual void Reset(unsigned idx)
    {
      SelectItem(idx);
    }

    virtual void IndexesChanged(Playlist::Model::OldToNewIndexMap::Ptr remapping)
    {
      Log::Debug(THIS_MODULE, "Iterator: index changed.");
      if (NO_INDEX == Index)
      {
        Log::Debug(THIS_MODULE, "Iterator: nothing to update");
        return;
      }
      if (const Playlist::Model::IndexType* moved = remapping->FindNewIndex(Index))
      {
        Log::Debug(THIS_MODULE, "Iterator: index updated %1% -> %2%", Index, *moved);
        Index = *moved;
        return;
      }
      if (const Playlist::Model::IndexType* newOne = remapping->FindNewSuitableIndex(Index))
      {
        if (SelectItem(*newOne))
        {
          return;
        }
      }
      //invalidated
      Index = NO_INDEX;
      Log::Debug(THIS_MODULE, "Iterator: invalidated after removing.");
    }
  private:
    bool SelectItem(unsigned idx)
    {
      if (Playlist::Item::Data::Ptr item = Model->GetItem(idx))
      {
        Log::Debug(THIS_MODULE, "Iterator: selected %1%", idx);
        Item = item;
        Index = idx;
        if (Item->IsValid())
        {
          State = Playlist::Item::STOPPED;
          OnListItemActivated(Index, item);
        }
        else
        {
          State = Playlist::Item::ERROR;
        }
        return true;
      }
      return false;
    }

    bool Navigate(int newIndex, unsigned playorderMode)
    {
      const unsigned itemsCount = Model->CountItems();
      if (!itemsCount)
      {
        return false;
      }
      const bool isEnd = newIndex >= int(itemsCount) || newIndex < 0;
      if (isEnd)
      {
        if (Playlist::Item::LOOPED == (playorderMode & Playlist::Item::LOOPED))
        {
          newIndex = (newIndex + itemsCount) % itemsCount;
        }
        else
        {
          return false;
        }
      }
      const bool isRandom = Playlist::Item::RANDOMIZED == (playorderMode & Playlist::Item::RANDOMIZED);
      const unsigned mappedIndex = isRandom
        ? Randomized(newIndex, itemsCount)
        : newIndex;
      return SelectItem(mappedIndex);
    }
  private:
    const Playlist::Model::Ptr Model;
    unsigned Index;
    Playlist::Item::Data::Ptr Item;
    Playlist::Item::State State;
  };

  class ControllerImpl : public Playlist::Controller
  {
  public:
    ControllerImpl(QObject& parent, const QString& name, Playlist::Item::DataProvider::Ptr provider)
      : Playlist::Controller(parent)
      , Name(name)
      , Scanner(Playlist::Scanner::Create(*this, provider))
      , Model(Playlist::Model::Create(*this))
      , Iterator(new ItemIteratorImpl(*this, Model))
    {
      //setup connections
      //use direct connection due to possible model locking
      Model->connect(Scanner, SIGNAL(OnGetItem(Playlist::Item::Data::Ptr)), SLOT(AddItem(Playlist::Item::Data::Ptr)), Qt::DirectConnection);
      Iterator->connect(Model, SIGNAL(OnIndexesChanged(Playlist::Model::OldToNewIndexMap::Ptr)), 
        SLOT(IndexesChanged(Playlist::Model::OldToNewIndexMap::Ptr)));

      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~ControllerImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);

      Scanner->Cancel();
      Scanner->wait();
    }

    virtual QString GetName() const
    {
      return Name;
    }

    virtual void SetName(const QString& name)
    {
      Name = name;
    }

    virtual Playlist::Scanner::Ptr GetScanner() const
    {
      return Scanner;
    }

    virtual Playlist::Model::Ptr GetModel() const
    {
      return Model;
    }

    virtual Playlist::Item::Iterator::Ptr GetIterator() const
    {
      return Iterator;
    }

    virtual void ShowNotification(Playlist::TextNotification::Ptr notification)
    {
      QMessageBox msgBox(QMessageBox::Information,
        ToQString(notification->Category()), ToQString(notification->Text()),
        QMessageBox::Ok);
      msgBox.setDetailedText(ToQString(notification->Details()));
      msgBox.exec();
    }

    virtual void CopyDetailToClipboard(Playlist::TextNotification::Ptr notification)
    {
      QApplication::clipboard()->setText(ToQString(notification->Details()));
    }
  private:
    QString Name;
    Playlist::Item::DataProvider::Ptr Provider;
    const Playlist::Scanner::Ptr Scanner;
    const Playlist::Model::Ptr Model;
    const Playlist::Item::Iterator::Ptr Iterator;
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

  Controller::Controller(QObject& parent) : QObject(&parent)
  {
  }

  Controller::Ptr Controller::Create(QObject& parent, const QString& name, Playlist::Item::DataProvider::Ptr provider)
  {
    REGISTER_METATYPE(Playlist::TextNotification::Ptr);
    return boost::make_shared<ControllerImpl>(boost::ref(parent), name, provider);
  }
}
