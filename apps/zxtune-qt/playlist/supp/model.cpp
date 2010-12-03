/*
Abstract:
  Playlist model implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "model.h"
#include "ui/utils.h"
//common includes
#include <logging.h>
#include <template_parameters.h>
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
//qt includes
#include <QtCore/QMimeData>
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtGui/QIcon>
//text includes
#include "text/text.h"

namespace
{
  const std::string THIS_MODULE("Playlist::Model");

  const QModelIndex EMPTY_INDEX = QModelIndex();

  class RowDataProvider
  {
  public:
    virtual ~RowDataProvider() {}

    virtual QVariant GetHeader(unsigned column) const = 0;
    virtual QVariant GetData(const Playlist::Item::Data& item, unsigned column) const = 0;
  };

  class DummyDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(unsigned /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const Playlist::Item::Data& /*item*/, unsigned /*column*/) const
    {
      return QVariant();
    }
  };

  class DecorationDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(unsigned /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const Playlist::Item::Data& item, unsigned column) const
    {
      switch (column)
      {
      case Playlist::Model::COLUMN_TYPEICON:
        {
          const QString iconPath = ToQString(
            Text::TYPEICONS_RESOURCE_PREFIX + item.GetType());
          return QIcon(iconPath);
        }
      default:
        return QVariant();
      };
    }
  };

  class DisplayDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(unsigned column) const
    {
      switch (column)
      {
      case Playlist::Model::COLUMN_TITLE:
        return Playlist::Model::tr("Author - Title");
      case Playlist::Model::COLUMN_DURATION:
        return Playlist::Model::tr("Duration");
      default:
        return QVariant();
      };
    }

    virtual QVariant GetData(const Playlist::Item::Data& item, unsigned column) const
    {
      switch (column)
      {
      case Playlist::Model::COLUMN_TITLE:
        return ToQString(item.GetTitle());
      case Playlist::Model::COLUMN_DURATION:
        return ToQString(item.GetDurationString());
      default:
        return QVariant();
      };
    }
  };

  class TooltipDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(unsigned /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const Playlist::Item::Data& item, unsigned /*column*/) const
    {
      return ToQString(item.GetTooltip());
    }
  };

  class DataProvidersSet
  {
  public:
    DataProvidersSet()
      : Decoration()
      , Display()
      , Tooltip()
      , Dummy()
    {
    }

    const RowDataProvider& GetProvider(int role) const
    {
      switch (role)
      {
      case Qt::DecorationRole:
        return Decoration;
      case Qt::DisplayRole:
        return Display;
      case Qt::ToolTipRole:
        return Tooltip;
      default:
        return Dummy;
      }
    }
  private:
    const DecorationDataProvider Decoration;
    const DisplayDataProvider Display;
    const TooltipDataProvider Tooltip;
    const DummyDataProvider Dummy;
  };

  typedef std::list<Playlist::Item::Data::Ptr> ItemsContainer;
  typedef std::vector<ItemsContainer::iterator> IteratorsArray;

  class PlayitemsContainer
  {
    class PlayitemIteratorImpl : public Playlist::Item::Data::Iterator
    {
    public:
      explicit PlayitemIteratorImpl(const IteratorsArray& iters)
      {
        Items.reserve(iters.size());
        for (IteratorsArray::const_iterator it = iters.begin(), lim = iters.end();
          it != lim; ++it)
        {
          Items.push_back(**it);
        }
        Current = Items.begin();
      }

      virtual bool IsValid() const
      {
        return Current != Items.end();
      }

      virtual Playlist::Item::Data::Ptr Get() const
      {
        if (Current != Items.end())
        {
          return *Current;
        }
        assert(!"Playitems iterator is out of range");
        return Playlist::Item::Data::Ptr();
      }

      virtual void Next()
      {
        if (Current != Items.end())
        {
          ++Current;
        }
        else
        {
          assert(!"Playitems iterator is out of range");
        }
      }
    private:
      std::vector<Playlist::Item::Data::Ptr> Items;
      std::vector<Playlist::Item::Data::Ptr>::const_iterator Current;
    };
  public:
    PlayitemsContainer()
    {
    }

    void AddItem(Playlist::Item::Data::Ptr item)
    {
      Items.push_back(item);
      IteratorsArray::value_type it = Items.end();
      Iterators.push_back(--it);
    }

    std::size_t CountItems() const
    {
      return Items.size();
    }

    Playlist::Item::Data::Ptr GetItemByIndex(int idx) const
    {
      if (idx >= int(Iterators.size()))
      {
        return Playlist::Item::Data::Ptr();
      }
      const IteratorsArray::value_type it = Iterators[idx];
      return *it;
    }

    Playlist::Item::Data::Iterator::Ptr GetAllItems() const
    {
      return Playlist::Item::Data::Iterator::Ptr(new PlayitemIteratorImpl(Iterators));
    }

    Playlist::Item::Data::Iterator::Ptr GetItems(const QSet<unsigned>& items) const
    {
      IteratorsArray choosenItems;
      GetChoosenItems(items, choosenItems);
      return Playlist::Item::Data::Iterator::Ptr(new PlayitemIteratorImpl(choosenItems));
    }

    void RemoveItems(const QSet<unsigned>& indexes)
    {
      IteratorsArray newIters;
      newIters.reserve(Iterators.size() - indexes.count());
      //TODO: optimize iteration- cycle only indexes range
      for (std::size_t idx = 0, lim = Iterators.size(); idx != lim; ++idx)
      {
        const IteratorsArray::value_type iter = Iterators[idx];
        if (indexes.contains(idx))
        {
          //remove
          Items.erase(iter);
        }
        else
        {
          //keep
          newIters.push_back(iter);
        }
      }
      Iterators.swap(newIters);
    }

    void MoveItems(const QSet<unsigned>& indexes, unsigned destination)
    {
      IteratorsArray beforeItems, movedItems, afterItems;
      for (std::size_t idx = 0, lim = Iterators.size(); idx != lim; ++idx)
      {
        const IteratorsArray::value_type iter = Iterators[idx];
        if (indexes.contains(idx))
        {
          movedItems.push_back(iter);
        }
        else
        {
          IteratorsArray& targetSubset = idx < destination
            ? beforeItems : afterItems;
          targetSubset.push_back(iter);
        }
      }
      const IteratorsArray::iterator movedTo = std::copy(beforeItems.begin(), beforeItems.end(), Iterators.begin());
      const IteratorsArray::iterator restAfter = std::copy(movedItems.begin(), movedItems.end(), movedTo);
      std::copy(afterItems.begin(), afterItems.end(), restAfter);
    }

    class Comparer
    {
    public:
      virtual ~Comparer() {}

      virtual bool CompareItems(IteratorsArray::value_type lh, IteratorsArray::value_type rh) const = 0;
    };

    void Sort(const Comparer& cmp)
    {
      std::stable_sort(Iterators.begin(), Iterators.end(),
        boost::bind(&Comparer::CompareItems, &cmp, _1, _2));
    }
  private:
    void GetChoosenItems(const QSet<unsigned>& items, IteratorsArray& result) const
    {
      IteratorsArray choosenItems;
      choosenItems.reserve(items.count());
      for (QSet<unsigned>::const_iterator it = items.begin(), lim = items.end();
        it != lim; ++it)
      {
        choosenItems.push_back(Iterators[*it]);
      }
      choosenItems.swap(result);
    }
  private:
    ItemsContainer Items;
    IteratorsArray Iterators;
  };

  template<class T>
  class TypedPlayitemsComparer : public PlayitemsContainer::Comparer
  {
  public:
    typedef T (Playlist::Item::Data::*Functor)() const;
    TypedPlayitemsComparer(Functor fn, bool ascending)
      : Getter(fn)
      , Ascending(ascending)
    {
    }

    bool CompareItems(IteratorsArray::value_type lh, IteratorsArray::value_type rh) const
    {
      const T val1 = ((**lh).*Getter)();
      const T val2 = ((**rh).*Getter)();
      return Ascending
        ? val1 < val2
        : val1 > val2;
    }
  private:
    const Functor Getter;
    const bool Ascending;
  };

  const char ITEMS_MIMETYPE[] = "application/playlist.indexes";

  class ModelImpl : public Playlist::Model
  {
  public:
    explicit ModelImpl(QObject& parent)
      : Playlist::Model(parent)
      , Providers()
      , Synchronizer(QMutex::Recursive)
      , FetchedItemsCount()
      , Container(new PlayitemsContainer())
    {
      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~ModelImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);
    }

    //new virtuals
    virtual unsigned CountItems() const
    {
      return Container->CountItems();
    }
    
    virtual Playlist::Item::Data::Ptr GetItem(unsigned index) const
    {
      QMutexLocker locker(&Synchronizer);
      return Container->GetItemByIndex(index);
    }

    virtual Playlist::Item::Data::Iterator::Ptr GetItems() const
    {
      QMutexLocker locker(&Synchronizer);
      return Container->GetAllItems();
    }

    virtual Playlist::Item::Data::Iterator::Ptr GetItems(const QSet<unsigned>& items) const
    {
      QMutexLocker locker(&Synchronizer);
      return Container->GetItems(items);
    }

    virtual void AddItems(Playlist::Item::Data::Iterator::Ptr iter)
    {
      QMutexLocker locker(&Synchronizer);
      for (; iter->IsValid(); iter->Next())
      {
        const Playlist::Item::Data::Ptr item = iter->Get();
        Container->AddItem(item);
      }
    }

    virtual void Clear()
    {
      QMutexLocker locker(&Synchronizer);
      Container.reset(new PlayitemsContainer());
      FetchedItemsCount = 0;
      reset();
    }

    virtual void RemoveItems(const QSet<unsigned>& items)
    {
      QMutexLocker locker(&Synchronizer);
      Container->RemoveItems(items);
      FetchedItemsCount = Container->CountItems();
      reset();
    }

    virtual void AddItem(Playlist::Item::Data::Ptr item)
    {
      QMutexLocker locker(&Synchronizer);
      Container->AddItem(item);
    }

    //base model virtuals

    /* Drag'n'Drop support*/
    virtual Qt::DropActions supportedDropActions() const
    {
      return Qt::MoveAction;
    }

    virtual Qt::ItemFlags flags(const QModelIndex& index) const
    {
      const Qt::ItemFlags defaultFlags = Playlist::Model::flags(index);

      const Playlist::Item::Data* const indexItem = index.isValid()
        ? static_cast<const Playlist::Item::Data*>(index.internalPointer())
        : 0;
      if (indexItem && indexItem->IsValid())
      {
         return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
      }
      else
      {
        return Qt::ItemIsDropEnabled | defaultFlags;
      }
    }

    virtual QStringList mimeTypes() const
    {
      QStringList types;
      types << ITEMS_MIMETYPE;
      return types;
    }

    virtual QMimeData* mimeData(const QModelIndexList& indexes) const
    {
      QMimeData* const mimeData = new QMimeData();
      QByteArray encodedData;
      QDataStream stream(&encodedData, QIODevice::WriteOnly);

      foreach (const QModelIndex& index, indexes)
      {
        if (index.isValid())
        {
          stream << index.row();
        }
      }

      mimeData->setData(ITEMS_MIMETYPE, encodedData);
      return mimeData;
    }

    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex& parent)
    {
      if (action == Qt::IgnoreAction)
      {
        return true;
      }

      if (!data->hasFormat(ITEMS_MIMETYPE))
      {
        return false;
      }

      int beginRow = 0;

      if (parent.isValid())
      {
        beginRow = parent.row();
      }

      QByteArray encodedData = data->data(ITEMS_MIMETYPE);
      QDataStream stream(&encodedData, QIODevice::ReadOnly);
      QSet<unsigned> movedItems;
      while (!stream.atEnd()) 
      {
        unsigned idx;
        stream >> idx;
        movedItems.insert(idx);
      }    
      if (movedItems.empty())
      {
        return false;
      }
      Log::Debug(THIS_MODULE, "Moving %1% items to row %2%", movedItems.size(), beginRow);
      QMutexLocker locker(&Synchronizer);
      Container->MoveItems(movedItems, beginRow);
      reset();
      return true;
    }

    /* end of Drag'n'Drop support */

    virtual bool canFetchMore(const QModelIndex& /*index*/) const
    {
      QMutexLocker locker(&Synchronizer);
      return FetchedItemsCount < Container->CountItems();
    }

    virtual void fetchMore(const QModelIndex& /*index*/)
    {
      QMutexLocker locker(&Synchronizer);
      const std::size_t nextCount = Container->CountItems();
      beginInsertRows(EMPTY_INDEX, FetchedItemsCount, nextCount - 1);
      FetchedItemsCount = nextCount;
      endInsertRows();
    }

    virtual QModelIndex index(int row, int column, const QModelIndex& parent) const
    {
      if (parent.isValid())
      {
        return EMPTY_INDEX;
      }
      QMutexLocker locker(&Synchronizer);
      if (const Playlist::Item::Data::Ptr item = Container->GetItemByIndex(row))
      {
        const void* const data = static_cast<const void*>(item.get());
        return createIndex(row, column, const_cast<void*>(data));
      }
      return EMPTY_INDEX;
    }

    virtual QModelIndex parent(const QModelIndex& /*index*/) const
    {
      return EMPTY_INDEX;
    }

    virtual int rowCount(const QModelIndex& index) const
    {
      QMutexLocker locker(&Synchronizer);
      return index.isValid()
        ? 0
        : FetchedItemsCount;
    }

    virtual int columnCount(const QModelIndex& index) const
    {
      return index.isValid()
        ? 0
        : COLUMNS_COUNT;
    }

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
      if (Qt::Horizontal == orientation)
      {
        const RowDataProvider& provider = Providers.GetProvider(role);
        return provider.GetHeader(section);
      }
      else if (Qt::Vertical == orientation && Qt::DisplayRole == role)
      {
        //item number is 1-based
        return QVariant(section + 1);
      }
      return QVariant();
    }

    virtual QVariant data(const QModelIndex& index, int role) const
    {
      if (!index.isValid())
      {
        return QVariant();
      }
      const int_t fieldNum = index.column();
      const int_t itemNum = index.row();
      QMutexLocker locker(&Synchronizer);
      if (const Playlist::Item::Data::Ptr item = Container->GetItemByIndex(itemNum))
      {
        const RowDataProvider& provider = Providers.GetProvider(role);
        assert(static_cast<const void*>(item.get()) == index.internalPointer());
        return provider.GetData(*item, fieldNum);
      }
      return QVariant();
    }

    virtual void sort(int column, Qt::SortOrder order)
    {
      Log::Debug(THIS_MODULE, "Sort data in column=%1% by order=%2%",
        column, order);
      const bool ascending = order == Qt::AscendingOrder;
      boost::scoped_ptr<PlayitemsContainer::Comparer> comparer;
      switch (column)
      {
      case COLUMN_TYPEICON:
        comparer.reset(new TypedPlayitemsComparer<String>(&Playlist::Item::Data::GetType, ascending));
        break;
      case COLUMN_TITLE:
        comparer.reset(new TypedPlayitemsComparer<String>(&Playlist::Item::Data::GetTitle, ascending));
        break;
      case COLUMN_DURATION:
        comparer.reset(new TypedPlayitemsComparer<unsigned>(&Playlist::Item::Data::GetDurationValue, ascending));
        break;
      default:
        break;
      }
      if (comparer)
      {
        QMutexLocker locker(&Synchronizer);
        Container->Sort(*comparer);
        reset();
      }
    }
  private:
    const DataProvidersSet Providers;
    mutable QMutex Synchronizer;
    std::size_t FetchedItemsCount;
    boost::scoped_ptr<PlayitemsContainer> Container;
  };
}

namespace Playlist
{
  Model::Model(QObject& parent) : QAbstractItemModel(&parent)
  {
  }

  Model::Ptr Model::Create(QObject& parent)
  {
    return new ModelImpl(parent);
  }
}
