/**
* 
* @file
*
* @brief Playlist controller implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "controller.h"
#include "model.h"
#include "scanner.h"
#include "ui/utils.h"
//common includes
#include <contract.h>
#include <error.h>
//library includes
#include <debug/log.h>
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtGui/QMessageBox>

namespace
{
  const Debug::Stream Dbg("Playlist::Controller");

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
      , State(Playlist::Item::STOPPED)
    {
      Require(connect(Model, SIGNAL(IndicesChanged(Playlist::Model::OldToNewIndexMap::Ptr)),
        SLOT(UpdateIndices(Playlist::Model::OldToNewIndexMap::Ptr))));
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

    virtual void Select(unsigned idx)
    {
      Activate(idx);
    }
    
    virtual void Reset(unsigned idx)
    {
      SelectItem(idx);
    }

    virtual void UpdateIndices(Playlist::Model::OldToNewIndexMap::Ptr remapping)
    {
      Dbg("Iterator: index changed.");
      if (NO_INDEX == Index)
      {
        return Activate(0);
      }
      if (const Playlist::Model::IndexType* moved = remapping->FindNewIndex(Index))
      {
        Dbg("Iterator: index updated %1% -> %2%", Index, *moved);
        Index = *moved;
        return;
      }
      const uint_t oldIndex = Index;
      Deactivate();
      if (const Playlist::Model::IndexType* newOne = remapping->FindNewSuitableIndex(oldIndex))
      {
        Activate(*newOne);
      }
    }
  private:
    bool SelectItem(unsigned idx)
    {
      if (Playlist::Item::Data::Ptr item = Model->GetItem(idx))
      {
        Dbg("Iterator: selected %1%", idx);
        Index = idx;
        if (item->GetState())
        {
          State = Playlist::Item::ERROR;
        }
        else
        {
          State = Playlist::Item::STOPPED;
          emit ItemActivated(item);
          emit ItemActivated(Index);
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

    void Activate(unsigned idx)
    {
      if (const Playlist::Item::Data::Ptr item = Model->GetItem(idx))
      {
        Index = idx;
        Dbg("Iterator: activated at %1%.", idx);
        emit Activated(item);
      }
    }

    void Deactivate()
    {
      Index = NO_INDEX;
      Dbg("Iterator: invalidated after removing.");
      emit Deactivated();
    }
  private:
    const Playlist::Model::Ptr Model;
    unsigned Index;
    Playlist::Item::State State;
  };

  class ControllerImpl : public Playlist::Controller
  {
  public:
    ControllerImpl(const QString& name, Playlist::Item::DataProvider::Ptr provider)
      : Name(name)
      , Scanner(Playlist::Scanner::Create(*this, provider))
      , Model(Playlist::Model::Create(*this))
      , Iterator(new ItemIteratorImpl(*this, Model))
    {
      //setup connections
      //use direct connection due to possible model locking
      Require(Model->connect(Scanner, SIGNAL(ItemFound(Playlist::Item::Data::Ptr)), SLOT(AddItem(Playlist::Item::Data::Ptr)), Qt::DirectConnection));
      Require(Model->connect(Scanner, SIGNAL(ItemsFound(Playlist::Item::Collection::Ptr)), SLOT(AddItems(Playlist::Item::Collection::Ptr)), Qt::DirectConnection));

      Dbg("Created at %1%", this);
    }

    virtual ~ControllerImpl()
    {
      Dbg("Destroyed at %1%", this);

      Scanner->Stop();
    }

    virtual QString GetName() const
    {
      return Name;
    }

    virtual void SetName(const QString& name)
    {
      if (name != Name)
      {
        Name = name;
        emit Renamed(Name);
      }
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

    virtual void Shutdown()
    {
      Dbg("Shutdown at %1%", this);
      Scanner->Stop();
      Model->CancelLongOperation();
    }

    virtual void ShowNotification(Playlist::TextNotification::Ptr notification)
    {
      QMessageBox msgBox(QMessageBox::Information,
        notification->Category(), notification->Text(),
        QMessageBox::Ok);
      msgBox.setDetailedText(notification->Details());
      msgBox.exec();
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

  Controller::Ptr Controller::Create(const QString& name, Playlist::Item::DataProvider::Ptr provider)
  {
    REGISTER_METATYPE(Playlist::TextNotification::Ptr);
    return boost::make_shared<ControllerImpl>(name, provider);
  }
}
