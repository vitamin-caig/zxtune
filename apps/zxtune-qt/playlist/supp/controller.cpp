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
#include "apps/base/app.h"
//common includes
#include <logging.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>

namespace
{
  const std::string THIS_MODULE("Playlist::Controller");

  unsigned Randomized(unsigned idx, unsigned total)
  {
    ::srand(::time(0) * idx);
    return ::rand() % total;
  }

  const unsigned NO_INDEX = ~0;

  const unsigned* FindNewIndex(const Playlist::Model::OldToNewIndexMap& remapping, unsigned oldIdx)
  {
    const Playlist::Model::OldToNewIndexMap::const_iterator it = remapping.find(oldIdx);
    if (it != remapping.end())
    {
      return &it->second;
    }
    return 0;
  }

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

    virtual void IndexesChanged(const Playlist::Model::OldToNewIndexMap& remapping)
    {
      Log::Debug(THIS_MODULE, "Iterator: index changed.");
      if (UpdateCurrentIndex(remapping))
      {
        return;
      }
      //removed
      if (!remapping.empty())
      {
        //try to find next one
        for (unsigned oldIdx = Index + 1, totalOldItems = remapping.rbegin()->first + 1; oldIdx < totalOldItems; ++oldIdx)
        {
          if (SetNewIndex(remapping, oldIdx))
          {
            return;
          }
        }
        //try to find previous one
        for (unsigned oldIdx = Index; oldIdx; --oldIdx)
        {
          if (SetNewIndex(remapping, oldIdx - 1))
          {
            return;
          }
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
          OnListItemActivated(Index, *item);
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

    bool UpdateCurrentIndex(const Playlist::Model::OldToNewIndexMap& remapping)
    {
      if (NO_INDEX == Index)
      {
        Log::Debug(THIS_MODULE, "Iterator: nothing to update");
        return true;
      }
      if (const unsigned* newIdx = FindNewIndex(remapping, Index))
      {
        //updated
        Log::Debug(THIS_MODULE, "Iterator: index updated %1% -> %2%", Index, *newIdx);
        Index = *newIdx;
        return true;
      }
      return false;
    }

    bool SetNewIndex(const Playlist::Model::OldToNewIndexMap& remapping, unsigned oldIdx)
    {
      if (const unsigned* newIdx = FindNewIndex(remapping, oldIdx))
      {
        //updated
        return SelectItem(*newIdx);
      }
      return false;
    }
  private:
    const Playlist::Model::Ptr Model;
    unsigned Index;
    Playlist::Item::Data::Ptr Item;
    Playlist::Item::State State;
  };

  class ContainerImpl : public Playlist::IO::Container
  {
  public:
    ContainerImpl(const QString& name, Playlist::Model::Ptr model)
      : Properties(Parameters::Container::Create())
      , Model(model)
    {
      Properties->SetStringValue(Playlist::ATTRIBUTE_NAME, FromQString(name));
      Properties->SetStringValue(Playlist::ATTRIBUTE_CREATOR, GetProgramVersionString());
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual unsigned GetItemsCount() const
    {
      return Model->CountItems();
    }

    virtual void ForAllItems(Playlist::Item::Callback& callback) const
    {
      ItemCallbackAdapter adapter(callback);
      Model->ForAllItems(adapter);
    }
  private:
    class ItemCallbackAdapter : public Playlist::Model::Visitor
    {
    public:
      explicit ItemCallbackAdapter(Playlist::Item::Callback& delegate)
        : Delegate(delegate)
      {
      }

      virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
      {
        return Delegate.OnItem(data);
      }
    private:
      Playlist::Item::Callback& Delegate;
    };
  private:
    const Parameters::Container::Ptr Properties;
    const Playlist::Model::Ptr Model;
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
      Model->connect(Scanner, SIGNAL(OnGetItem(Playlist::Item::Data::Ptr)), SLOT(AddItem(Playlist::Item::Data::Ptr)));
      Iterator->connect(Model, SIGNAL(OnIndexesChanged(const Playlist::Model::OldToNewIndexMap&)), 
        SLOT(IndexesChanged(const Playlist::Model::OldToNewIndexMap&)));

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

    virtual Playlist::IO::Container::Ptr GetContainer() const
    {
      return boost::make_shared<ContainerImpl>(Name, Model);
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
    return boost::make_shared<ControllerImpl>(boost::ref(parent), name, provider);
  }
}
