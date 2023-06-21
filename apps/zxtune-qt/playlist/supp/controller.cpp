/**
 *
 * @file
 *
 * @brief Playlist controller implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

// local includes
#include "controller.h"
#include "model.h"
#include "scanner.h"
#include "supp/ptr_utils.h"
#include "supp/thread_utils.h"
#include "ui/utils.h"
// common includes
#include <contract.h>
#include <error.h>
#include <make_ptr.h>
// library includes
#include <debug/log.h>
// std includes
#include <utility>
// qt includes
#include <QtWidgets/QMessageBox>

namespace
{
  const Debug::Stream Dbg("Playlist::Controller");

  unsigned Randomized(unsigned idx, unsigned total)
  {
    ::srand(::time(nullptr) * idx);
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
    {
      Require(connect(Model, &Playlist::Model::IndicesChanged, this, &ItemIteratorImpl::UpdateIndices));
    }

    unsigned GetIndex() const override
    {
      return Index;
    }

    Playlist::Item::State GetState() const override
    {
      return State;
    }

    bool Next(unsigned playorderMode) override
    {
      return Index != NO_INDEX && Navigate(Index + 1, playorderMode);
    }

    bool Prev(unsigned playorderMode) override
    {
      return Index != NO_INDEX && Navigate(int(Index) - 1, playorderMode);
    }

    void SetState(Playlist::Item::State state) override
    {
      State = state;
    }

    void Select(unsigned idx) override
    {
      Activate(idx);
    }

    void Reset() override
    {
      Reset(0);
    }

    void Reset(unsigned idx) override
    {
      SelectItem(idx);
    }

  private:
    void UpdateIndices(Playlist::Model::OldToNewIndexMap::Ptr remapping)
    {
      Dbg("Iterator: index changed.");
      if (NO_INDEX == Index)
      {
        return Activate(0);
      }
      if (const Playlist::Model::IndexType* moved = remapping->FindNewIndex(Index))
      {
        Dbg("Iterator: index updated {} -> {}", Index, *moved);
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

    bool SelectItem(unsigned idx)
    {
      if (auto item = Model->GetItem(idx))
      {
        if (!item->IsLoaded())
        {
          IOThread::Execute([weakItem = toWeak(item), idx, self = toWeak(this)]() {
            if (auto item = weakItem.lock())
            {
              item->GetModule();
              SelfThread::Execute(self, &ItemIteratorImpl::SetItem, idx, std::move(item));
            }
          });
          return true;
        }
        SetItem(idx, std::move(item));
        return true;
      }
      return false;
    }

    void SetItem(unsigned idx, Playlist::Item::Data::Ptr item)
    {
      Dbg("Iterator: selected {}", idx);
      Index = idx;
      if (item->GetState())
      {
        State = Playlist::Item::ERROR;
      }
      else
      {
        State = Playlist::Item::STOPPED;
        emit ItemActivated(std::move(item));
        emit ItemActivated(Index);
      }
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
      const unsigned mappedIndex = isRandom ? Randomized(newIndex, itemsCount) : newIndex;
      return SelectItem(mappedIndex);
    }

    void Activate(unsigned idx)
    {
      if (const Playlist::Item::Data::Ptr item = Model->GetItem(idx))
      {
        Index = idx;
        Dbg("Iterator: activated at {}.", idx);
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
    Playlist::Item::State State = Playlist::Item::STOPPED;
  };

  class ControllerImpl : public Playlist::Controller
  {
  public:
    ControllerImpl(QString name, Playlist::Item::DataProvider::Ptr provider)
      : Name(std::move(name))
      , Scanner(Playlist::Scanner::Create(*this, std::move(provider)))
      , Model(Playlist::Model::Create(*this))
      , Iterator(new ItemIteratorImpl(*this, Model))
    {
      // setup connections
      // use direct connection due to possible model locking
      Require(connect(Scanner, &Playlist::Scanner::ItemFound, Model, &Playlist::Model::AddItem, Qt::DirectConnection));
      Require(
          connect(Scanner, &Playlist::Scanner::ItemsFound, Model, &Playlist::Model::AddItems, Qt::DirectConnection));

      Dbg("Created at {}", static_cast<void*>(this));
    }

    ~ControllerImpl() override
    {
      Dbg("Destroyed at {}", static_cast<void*>(this));

      Scanner->Stop();
    }

    QString GetName() const override
    {
      return Name;
    }

    void SetName(const QString& name) override
    {
      if (name != Name)
      {
        Name = name;
        emit Renamed(Name);
      }
    }

    Playlist::Scanner::Ptr GetScanner() const override
    {
      return Scanner;
    }

    Playlist::Model::Ptr GetModel() const override
    {
      return Model;
    }

    Playlist::Item::Iterator::Ptr GetIterator() const override
    {
      return Iterator;
    }

    void Shutdown() override
    {
      Dbg("Shutdown at {}", static_cast<void*>(this));
      Scanner->Stop();
      Model->CancelLongOperation();
    }

    void ShowNotification(Playlist::TextNotification::Ptr notification) override
    {
      QMessageBox msgBox(QMessageBox::Information, notification->Category(), notification->Text(), QMessageBox::Ok);
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
}  // namespace

namespace Playlist
{
  namespace Item
  {
    Iterator::Iterator(QObject& parent)
      : QObject(&parent)
    {}
  }  // namespace Item

  Controller::Ptr Controller::Create(const QString& name, Playlist::Item::DataProvider::Ptr provider)
  {
    REGISTER_METATYPE(Playlist::TextNotification::Ptr);
    return MakePtr<ControllerImpl>(name, std::move(provider));
  }
}  // namespace Playlist
