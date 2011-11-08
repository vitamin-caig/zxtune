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
#include "storage.h"
#include "ui/utils.h"
//common includes
#include <logging.h>
#include <template_parameters.h>
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
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

  template<class T>
  class TypedPlayitemsComparer : public Playlist::Item::Comparer
  {
  public:
    typedef T (Playlist::Item::Data::*Functor)() const;
    TypedPlayitemsComparer(Functor fn, bool ascending)
      : Getter(fn)
      , Ascending(ascending)
    {
    }

    bool CompareItems(const Playlist::Item::Data& lh, const Playlist::Item::Data& rh) const
    {
      const T val1 = (lh.*Getter)();
      const T val2 = (rh.*Getter)();
      return Ascending
        ? val1 < val2
        : val2 < val1;
    }
  private:
    const Functor Getter;
    const bool Ascending;
  };

  Playlist::Item::Comparer::Ptr CreateComparerByColumn(int column, bool ascending)
  {
    switch (column)
    {
    case Playlist::Model::COLUMN_TYPEICON:
      return Playlist::Item::Comparer::Ptr(new TypedPlayitemsComparer<String>(&Playlist::Item::Data::GetType, ascending));
    case Playlist::Model::COLUMN_TITLE:
      return Playlist::Item::Comparer::Ptr(new TypedPlayitemsComparer<String>(&Playlist::Item::Data::GetTitle, ascending));
    case Playlist::Model::COLUMN_DURATION:
      return Playlist::Item::Comparer::Ptr(new TypedPlayitemsComparer<Time::Milliseconds>(&Playlist::Item::Data::GetDuration, ascending));
    default:
      return Playlist::Item::Comparer::Ptr();
    }
  }

  class ComparisonsCounter : public Playlist::Item::Comparer
  {
  public:
    ComparisonsCounter(const Playlist::Item::Comparer& delegate, Log::ProgressCallback& cb)
      : Delegate(delegate)
      , Callback(cb)
      , Done(0)
    {
    }

    bool CompareItems(const Playlist::Item::Data& lh, const Playlist::Item::Data& rh) const
    {
      Callback.OnProgress(++Done);
      return Delegate.CompareItems(lh, rh);
    }
  private:
    const Playlist::Item::Comparer& Delegate;
    Log::ProgressCallback& Callback;
    mutable uint_t Done;
  };

  class SortOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    explicit SortOperation(Playlist::Item::Comparer::Ptr comparer)
      : Comparer(comparer)
    {
    }

    virtual void Execute(Playlist::Item::Storage& storage, Log::ProgressCallback& cb)
    {
      const uint_t totalItems = storage.CountItems();
      //according to STL spec: The number of comparisons is approximately N log N, where N is the list's size. Assume that log is binary.
      const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(totalItems * Log2(totalItems), cb);
      const ComparisonsCounter countingComparer(*Comparer, *progress);
      storage.Sort(countingComparer);
    }
  private:
    const Playlist::Item::Comparer::Ptr Comparer;
  };

  const char ITEMS_MIMETYPE[] = "application/playlist.indices";

  class ModelImpl : public Playlist::Model
                  , private Log::ProgressCallback
  {
  public:
    explicit ModelImpl(QObject& parent)
      : Playlist::Model(parent)
      , Providers()
      , SyncAccess(QMutex::Recursive)
      , SyncModification(QMutex::Recursive)
      , FetchedItemsCount()
      , Container(Playlist::Item::Storage::Create())
    {
      Log::Debug(THIS_MODULE, "Created at %1%", this);
    }

    virtual ~ModelImpl()
    {
      Log::Debug(THIS_MODULE, "Destroyed at %1%", this);
    }

    virtual void PerformOperation(Playlist::Item::StorageAccessOperation::Ptr operation)
    {
      AsyncExecution = boost::thread(&ModelImpl::PerformAccessOperation, this, operation);
    }

    virtual void PerformOperation(Playlist::Item::StorageModifyOperation::Ptr operation)
    {
      AsyncExecution = boost::thread(&ModelImpl::PerformModifyOperation, this, operation);
    }

    //new virtuals
    virtual unsigned CountItems() const
    {
      return static_cast<unsigned>(Container->CountItems());
    }

    virtual Playlist::Item::Data::Ptr GetItem(IndexType index) const
    {
      QMutexLocker locker(&SyncAccess);
      return Container->GetItem(index);
    }

    virtual unsigned GetVersion() const
    {
      return Container->GetVersion();
    }

    virtual void ForAllItems(Visitor& visitor) const
    {
      QMutexLocker locker(&SyncAccess);
      return Container->ForAllItems(visitor);
    }

    virtual void ForSpecifiedItems(const IndexSet& items, Visitor& visitor) const
    {
      QMutexLocker locker(&SyncAccess);
      return Container->ForSpecifiedItems(items, visitor);
    }

    virtual void Clear()
    {
      QMutexLocker locker(&SyncModification);
      Container = Playlist::Item::Storage::Create();
      NotifyAboutIndexChanged();
    }

    virtual void RemoveItems(const Playlist::Model::IndexSet& items)
    {
      QMutexLocker locker(&SyncModification);
      Container->RemoveItems(items);
      NotifyAboutIndexChanged();
    }

    virtual void MoveItems(const IndexSet& items, IndexType target)
    {
      Log::Debug(THIS_MODULE, "Moving %1% items to row %2%", items.size(), target);
      QMutexLocker locker(&SyncModification);
      Container->MoveItems(items, target);
      NotifyAboutIndexChanged();
    }

    virtual void AddItem(Playlist::Item::Data::Ptr item)
    {
      QMutexLocker locker(&SyncModification);
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
      const Qt::ItemFlags validFlags = Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
      const Qt::ItemFlags invalidFlags = Qt::ItemIsDropEnabled | defaultFlags;
      if (index.internalId() != Container->GetVersion())
      {
        return invalidFlags;
      }
      //TODO: do not access item
      const Playlist::Item::Data::Ptr item = GetItem(index.row());
      return item && item->IsValid()
        ? validFlags
        : invalidFlags;
    }

    virtual QStringList mimeTypes() const
    {
      QStringList types;
      types << ITEMS_MIMETYPE;
      return types;
    }

    virtual QMimeData* mimeData(const QModelIndexList& indices) const
    {
      QMimeData* const mimeData = new QMimeData();
      QByteArray encodedData;
      QDataStream stream(&encodedData, QIODevice::WriteOnly);

      foreach (const QModelIndex& index, indices)
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

      const unsigned beginRow = parent.isValid()
        ? parent.row()
        : rowCount(EMPTY_INDEX);

      QByteArray encodedData = data->data(ITEMS_MIMETYPE);
      QDataStream stream(&encodedData, QIODevice::ReadOnly);
      Playlist::Model::IndexSet movedItems;
      while (!stream.atEnd())
      {
        IndexType idx;
        stream >> idx;
        movedItems.insert(idx);
      }
      if (movedItems.empty())
      {
        return false;
      }
      MoveItems(movedItems, beginRow);
      return true;
    }

    /* end of Drag'n'Drop support */

    virtual bool canFetchMore(const QModelIndex& /*index*/) const
    {
      QMutexLocker locker(&SyncAccess);
      return FetchedItemsCount < Container->CountItems();
    }

    virtual void fetchMore(const QModelIndex& /*index*/)
    {
      QMutexLocker locker(&SyncAccess);
      const std::size_t nextCount = Container->CountItems();
      beginInsertRows(EMPTY_INDEX, static_cast<int>(FetchedItemsCount), nextCount - 1);
      FetchedItemsCount = nextCount;
      endInsertRows();
    }

    virtual QModelIndex index(int row, int column, const QModelIndex& parent) const
    {
      if (parent.isValid())
      {
        return EMPTY_INDEX;
      }
      QMutexLocker locker(&SyncAccess);
      if (row < static_cast<int>(Container->CountItems()))
      {
        return createIndex(row, column, Container->GetVersion());
      }
      return EMPTY_INDEX;
    }

    virtual QModelIndex parent(const QModelIndex& /*index*/) const
    {
      return EMPTY_INDEX;
    }

    virtual int rowCount(const QModelIndex& index) const
    {
      QMutexLocker locker(&SyncAccess);
      return index.isValid()
        ? 0
        : static_cast<int>(FetchedItemsCount);
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
      QMutexLocker locker(&SyncAccess);
      if (const Playlist::Item::Data::Ptr item = Container->GetItem(itemNum))
      {
        const RowDataProvider& provider = Providers.GetProvider(role);
        return provider.GetData(*item, fieldNum);
      }
      return QVariant();
    }

    virtual void sort(int column, Qt::SortOrder order)
    {
      Log::Debug(THIS_MODULE, "Sort data in column=%1% by order=%2%",
        column, order);
      const bool ascending = order == Qt::AscendingOrder;
      if (Playlist::Item::Comparer::Ptr comparer = CreateComparerByColumn(column, ascending))
      {
        const Playlist::Item::StorageModifyOperation::Ptr op = boost::make_shared<SortOperation>(comparer);
        PerformOperation(op);
      }
    }
  private:
    void PerformAccessOperation(Playlist::Item::StorageAccessOperation::Ptr operation)
    {
      QMutexLocker rwLock(&SyncModification);
      OnLongOperationStart();
      operation->Execute(*Container, *this);
      OnLongOperationStop();
    }

    void PerformModifyOperation(Playlist::Item::StorageModifyOperation::Ptr operation)
    {
      QMutexLocker rwLock(&SyncModification);
      OnLongOperationStart();
      Playlist::Item::Storage::Ptr tmpStorage;
      {
        QMutexLocker roLock(&SyncAccess);
        tmpStorage = Container->Clone();
      }
      operation->Execute(*tmpStorage, *this);
      {
        QMutexLocker roLock(&SyncAccess);
        Container = tmpStorage;
      }
      NotifyAboutIndexChanged();
      OnLongOperationStop();
    }

    void NotifyAboutIndexChanged()
    {
      FetchedItemsCount = Container->CountItems();
      Playlist::Model::OldToNewIndexMap remapping;
      Container->GetIndexRemapping(remapping);
      Container->ResetIndices();
      OnIndexesChanged(remapping);
      reset();
    }

    virtual void OnProgress(uint_t current)
    {
      OnLongOperationProgress(current);
    }

    virtual void OnProgress(uint_t current, const String& message)
    {
      OnLongOperationProgress(current);
    }
  private:
    const DataProvidersSet Providers;
    mutable QMutex SyncAccess;
    mutable QMutex SyncModification;
    std::size_t FetchedItemsCount;
    Playlist::Item::Storage::Ptr Container;
    mutable boost::thread AsyncExecution;
  };
}

namespace Playlist
{
  Model::Model(QObject& parent) : QAbstractItemModel(&parent)
  {
  }

  Model::Ptr Model::Create(QObject& parent)
  {
    REGISTER_METATYPE(Playlist::Model::OldToNewIndexMap);
    return new ModelImpl(parent);
  }
}
