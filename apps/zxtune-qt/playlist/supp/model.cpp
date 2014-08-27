/**
* 
* @file
*
* @brief Playlist model implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "model.h"
#include "storage.h"
#include "ui/utils.h"
//library includes
#include <async/activity.h>
#include <core/module_attrs.h>
#include <debug/log.h>
#include <math/bitops.h>
#include <parameters/template.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/ref.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
//qt includes
#include <QtCore/QMimeData>
#include <QtCore/QSet>
#include <QtCore/QStringList>
//text includes
#include "text/text.h"

namespace
{
  const Debug::Stream Dbg("Playlist::Model");

  const QModelIndex EMPTY_INDEX = QModelIndex();

  class RowDataProvider
  {
  public:
    virtual ~RowDataProvider() {}

    virtual QVariant GetData(const Playlist::Item::Data& item, unsigned column) const = 0;
  };

  class DummyDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetData(const Playlist::Item::Data& /*item*/, unsigned /*column*/) const
    {
      return QVariant();
    }
  };

  class DisplayDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetData(const Playlist::Item::Data& item, unsigned column) const
    {
      switch (column)
      {
      case Playlist::Model::COLUMN_TYPE:
        return ToQString(item.GetType());
      case Playlist::Model::COLUMN_DISPLAY_NAME:
        return ToQString(item.GetDisplayName());
      case Playlist::Model::COLUMN_DURATION:
        return ToQString(item.GetDuration().ToString());
      case Playlist::Model::COLUMN_AUTHOR:
        return ToQString(item.GetAuthor());
      case Playlist::Model::COLUMN_TITLE:
        return ToQString(item.GetTitle());
      case Playlist::Model::COLUMN_PATH:
        return ToQString(item.GetFullPath());
      case Playlist::Model::COLUMN_SIZE:
        return QString::number(item.GetSize());
      case Playlist::Model::COLUMN_CRC:
        return QString::number(item.GetChecksum(), 16);
      case Playlist::Model::COLUMN_FIXEDCRC:
        return QString::number(item.GetCoreChecksum(), 16);
      default:
        return QVariant();
      };
    }
  };

  class DataProvidersSet
  {
  public:
    DataProvidersSet()
      : Display()
      , Dummy()
    {
    }

    const RowDataProvider& GetProvider(int role) const
    {
      switch (role)
      {
      case Qt::DisplayRole:
        return Display;
      default:
        return Dummy;
      }
    }
  private:
    const DisplayDataProvider Display;
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

  template<class R>
  Playlist::Item::Comparer::Ptr CreateComparer(R (Playlist::Item::Data::*func)() const, bool ascending)
  {
    return boost::make_shared<TypedPlayitemsComparer<R> >(func, ascending);
  }

  Playlist::Item::Comparer::Ptr CreateComparerByColumn(int column, bool ascending)
  {
    switch (column)
    {
    case Playlist::Model::COLUMN_TYPE:
      return CreateComparer(&Playlist::Item::Data::GetType, ascending);
    case Playlist::Model::COLUMN_DISPLAY_NAME:
      return CreateComparer(&Playlist::Item::Data::GetDisplayName, ascending);
    case Playlist::Model::COLUMN_DURATION:
      return CreateComparer(&Playlist::Item::Data::GetDuration, ascending);
    case Playlist::Model::COLUMN_AUTHOR:
      return CreateComparer(&Playlist::Item::Data::GetAuthor, ascending);
    case Playlist::Model::COLUMN_TITLE:
      return CreateComparer(&Playlist::Item::Data::GetTitle, ascending);
    case Playlist::Model::COLUMN_PATH:
      return CreateComparer(&Playlist::Item::Data::GetFullPath, ascending);
    case Playlist::Model::COLUMN_SIZE:
      return CreateComparer(&Playlist::Item::Data::GetSize, ascending);
    case Playlist::Model::COLUMN_CRC:
      return CreateComparer(&Playlist::Item::Data::GetChecksum, ascending);
    case Playlist::Model::COLUMN_FIXEDCRC:
      return CreateComparer(&Playlist::Item::Data::GetCoreChecksum, ascending);
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
      const Log::ProgressCallback::Ptr progress = Log::CreatePercentProgressCallback(totalItems * Math::Log2(totalItems), cb);
      const ComparisonsCounter countingComparer(*Comparer, *progress);
      storage.Sort(countingComparer);
    }
  private:
    const Playlist::Item::Comparer::Ptr Comparer;
  };

  const QLatin1String INDICES_MIMETYPE("application/playlist.indices");

  template<class OpType>
  class OperationTarget
  {
  public:
    virtual ~OperationTarget() {}

    virtual void ExecuteOperation(typename OpType::Ptr op) = 0;
  };

  template<class OpType>
  class AsyncOperation : public Async::Operation
  {
  public:
    AsyncOperation(typename OpType::Ptr op, OperationTarget<OpType>& executor)
      : Op(op)
      , Delegate(executor)
    {
    }

    virtual void Prepare()
    {
    }

    virtual void Execute()
    {
      Delegate.ExecuteOperation(Op);
    }
  private:
    const typename OpType::Ptr Op;
    OperationTarget<OpType>& Delegate;
  };

  class PathsVisitor : public Playlist::Item::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      if (!data->GetState())
      {
        Paths.push_back(ToQString(data->GetFullPath()));
      }
    }

    QStringList GetResult()
    {
      return Paths;
    }
  private:
    QStringList Paths;
  };

  class ModelImpl : public Playlist::Model
                  , public OperationTarget<Playlist::Item::StorageAccessOperation>
                  , public OperationTarget<Playlist::Item::StorageModifyOperation>
                  , private Log::ProgressCallback
  {
  public:
    explicit ModelImpl(QObject& parent)
      : Playlist::Model(parent)
      , Providers()
      , FetchedItemsCount()
      , Container(Playlist::Item::Storage::Create())
      , AsyncExecution(Async::Activity::CreateStub())
      , Canceled(false)
    {
      Dbg("Created at %1%", this);
    }

    virtual ~ModelImpl()
    {
      Dbg("Destroyed at %1%", this);
    }

    virtual void PerformOperation(Playlist::Item::StorageAccessOperation::Ptr operation)
    {
      WaitOperationFinish();
      const Async::Operation::Ptr wrapper = boost::make_shared<AsyncOperation<Playlist::Item::StorageAccessOperation> >(operation, boost::ref(*this));
      AsyncExecution = Async::Activity::Create(wrapper);
    }

    virtual void PerformOperation(Playlist::Item::StorageModifyOperation::Ptr operation)
    {
      WaitOperationFinish();
      const Async::Operation::Ptr wrapper = boost::make_shared<AsyncOperation<Playlist::Item::StorageModifyOperation> >(operation, boost::ref(*this));
      AsyncExecution = Async::Activity::Create(wrapper);
    }

    virtual void WaitOperationFinish()
    {
      AsyncExecution->Wait();
      Canceled = false;
    }

    //new virtuals
    virtual unsigned CountItems() const
    {
      const boost::shared_lock<boost::shared_mutex> lock(SyncAccess);
      return static_cast<unsigned>(Container->CountItems());
    }

    virtual Playlist::Item::Data::Ptr GetItem(IndexType index) const
    {
      const boost::shared_lock<boost::shared_mutex> lock(SyncAccess);
      return Container->GetItem(index);
    }

    virtual QStringList GetItemsPaths(const IndexSet& items) const
    {
      PathsVisitor visitor;
      {
        const boost::shared_lock<boost::shared_mutex> lock(SyncAccess);
        Container->ForSpecifiedItems(items, visitor);
      }
      return visitor.GetResult();
    }

    virtual unsigned GetVersion() const
    {
      return Container->GetVersion();
    }

    virtual void Clear()
    {
      Playlist::Model::OldToNewIndexMap::Ptr remapping;
      {
        boost::upgrade_lock<boost::shared_mutex> prepare(SyncAccess);
        const boost::upgrade_to_unique_lock<boost::shared_mutex> lock(prepare);
        Container = Playlist::Item::Storage::Create();
        remapping = GetIndicesChanges();
      }
      NotifyAboutIndexChanged(remapping);
    }

    virtual void RemoveItems(IndexSetPtr items)
    {
      if (!items || items->empty())
      {
        return;
      }
      Playlist::Model::OldToNewIndexMap::Ptr remapping;
      {
        boost::upgrade_lock<boost::shared_mutex> prepare(SyncAccess);
        const boost::upgrade_to_unique_lock<boost::shared_mutex> lock(prepare);
        Container->RemoveItems(*items);
        remapping = GetIndicesChanges();
      }
      NotifyAboutIndexChanged(remapping);
    }

    virtual void MoveItems(const IndexSet& items, IndexType target)
    {
      Dbg("Moving %1% items to row %2%", items.size(), target);
      Playlist::Model::OldToNewIndexMap::Ptr remapping;
      {
        boost::upgrade_lock<boost::shared_mutex> prepare(SyncAccess);
        const boost::upgrade_to_unique_lock<boost::shared_mutex> lock(prepare);
        Container->MoveItems(items, target);
        remapping = GetIndicesChanges();
      }
      NotifyAboutIndexChanged(remapping);
    }

    virtual void AddItem(Playlist::Item::Data::Ptr item)
    {
      //Called for each item found during scan, so notify only first time
      if (0 == CountItems())
      {
        AddAndNotify(item);
      }
      else
      {
        Add(item);
      }
    }

    virtual void AddItems(Playlist::Item::Collection::Ptr items)
    {
      AddAndNotify(items);
    }

    virtual void CancelLongOperation()
    {
      Canceled = true;
      AsyncExecution->Wait();
    }

    //base model virtuals

    /* Drag'n'Drop support*/
    virtual Qt::DropActions supportedDropActions() const
    {
      return Qt::MoveAction;
    }

    virtual QStringList mimeTypes() const
    {
      QStringList types;
      types << INDICES_MIMETYPE;
      return types;
    }

    virtual QMimeData* mimeData(const QModelIndexList& indices) const
    {
      QMimeData* const mimeData = new QMimeData();
      QByteArray encodedData;
      {
        QDataStream stream(&encodedData, QIODevice::WriteOnly);
        foreach (const QModelIndex& index, indices)
        {
          if (index.isValid())
          {
            stream << index.row();
          }
        }
      }

      mimeData->setData(INDICES_MIMETYPE, encodedData);
      return mimeData;
    }

    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/, const QModelIndex& parent)
    {
      if (action == Qt::IgnoreAction)
      {
        return true;
      }

      if (!data->hasFormat(INDICES_MIMETYPE))
      {
        return false;
      }
      const unsigned beginRow = parent.isValid()
        ? parent.row()
        : rowCount(EMPTY_INDEX);

      const QByteArray& encodedData = data->data(INDICES_MIMETYPE);
      Playlist::Model::IndexSet movedItems;
      {
        QDataStream stream(encodedData);
        while (!stream.atEnd())
        {
          IndexType idx;
          stream >> idx;
          movedItems.insert(idx);
        }
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
      const boost::shared_lock<boost::shared_mutex> sharedReaders(SyncAccess);
      return FetchedItemsCount < Container->CountItems();
    }

    virtual void fetchMore(const QModelIndex& /*index*/)
    {
      const boost::shared_lock<boost::shared_mutex> sharedReaders(SyncAccess);
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
      const boost::shared_lock<boost::shared_mutex> lock(SyncAccess);
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
      const boost::shared_lock<boost::shared_mutex> lock(SyncAccess);
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
      if (Qt::Vertical == orientation && Qt::DisplayRole == role)
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
      const boost::shared_lock<boost::shared_mutex> lock(SyncAccess);
      if (const Playlist::Item::Data::Ptr item = Container->GetItem(itemNum))
      {
        const RowDataProvider& provider = Providers.GetProvider(role);
        return provider.GetData(*item, fieldNum);
      }
      return QVariant();
    }

    virtual void sort(int column, Qt::SortOrder order)
    {
      Dbg("Sort data in column=%1% by order=%2%", column, order);
      const bool ascending = order == Qt::AscendingOrder;
      if (Playlist::Item::Comparer::Ptr comparer = CreateComparerByColumn(column, ascending))
      {
        const Playlist::Item::StorageModifyOperation::Ptr op = boost::make_shared<SortOperation>(comparer);
        PerformOperation(op);
      }
    }
  private:
    virtual void ExecuteOperation(Playlist::Item::StorageAccessOperation::Ptr operation)
    {
      const boost::shared_lock<boost::shared_mutex> lock(SyncAccess);
      emit OperationStarted();
      try
      {
        operation->Execute(*Container, *this);
      }
      catch (const std::exception&)
      {
      }
      emit OperationStopped();
    }

    virtual void ExecuteOperation(Playlist::Item::StorageModifyOperation::Ptr operation)
    {
      Playlist::Model::OldToNewIndexMap::Ptr remapping;
      {
        boost::upgrade_lock<boost::shared_mutex> prepare(SyncAccess);
        emit OperationStarted();
        Playlist::Item::Storage::Ptr tmpStorage = Container->Clone();
        try
        {
          operation->Execute(*tmpStorage, *this);
          const boost::upgrade_to_unique_lock<boost::shared_mutex> lock(prepare);
          Container = tmpStorage;
          remapping = GetIndicesChanges();
        }
        catch (const std::exception&)
        {
        }
        emit OperationStopped();
      }
      NotifyAboutIndexChanged(remapping);
    }

    Playlist::Model::OldToNewIndexMap::Ptr GetIndicesChanges()
    {
      FetchedItemsCount = Container->CountItems();
      return Container->ResetIndices();
    }

    void NotifyAboutIndexChanged(Playlist::Model::OldToNewIndexMap::Ptr changes)
    {
      if (changes)
      {
        emit IndicesChanged(changes);
        reset();
      }
    }

    virtual void OnProgress(uint_t current)
    {
      emit OperationProgressChanged(current);
      if (Canceled)
      {
        throw std::exception();
      }
    }

    virtual void OnProgress(uint_t current, const String& /*message*/)
    {
      OnProgress(current);
    }

    template<class T>
    void AddAndNotify(const T& val)
    {
      Playlist::Model::OldToNewIndexMap::Ptr remapping;
      {
        boost::upgrade_lock<boost::shared_mutex> prepare(SyncAccess);
        const boost::upgrade_to_unique_lock<boost::shared_mutex> lock(prepare);
        Container->Add(val);
        remapping = GetIndicesChanges();
      }
      NotifyAboutIndexChanged(remapping);
    }

    template<class T>
    void Add(const T& val)
    {
      boost::upgrade_lock<boost::shared_mutex> prepare(SyncAccess);
      const boost::upgrade_to_unique_lock<boost::shared_mutex> lock(prepare);
      Container->Add(val);
    }
  private:
    const DataProvidersSet Providers;
    mutable boost::shared_mutex SyncAccess;
    std::size_t FetchedItemsCount;
    Playlist::Item::Storage::Ptr Container;
    Async::Activity::Ptr AsyncExecution;
    volatile bool Canceled;
  };
}

namespace Playlist
{
  Model::Model(QObject& parent) : QAbstractItemModel(&parent)
  {
  }

  Model::Ptr Model::Create(QObject& parent)
  {
    REGISTER_METATYPE(Playlist::Model::OldToNewIndexMap::Ptr);
    return new ModelImpl(parent);
  }

  const Model::IndexType* Model::OldToNewIndexMap::FindNewIndex(IndexType oldIdx) const
  {
    const const_iterator it = find(oldIdx);
    if (it != end())
    {
      return &it->second;
    }
    return 0;
  }

  const Model::IndexType* Model::OldToNewIndexMap::FindNewSuitableIndex(IndexType oldIdx) const
  {
    if (empty())
    {
      return 0;
    }
    if (const Model::IndexType* direct = FindNewIndex(oldIdx))
    {
      return direct;
    }
    //try to find next one
    for (unsigned nextIdx = oldIdx + 1, totalOldItems = rbegin()->first + 1; nextIdx < totalOldItems; ++nextIdx)
    {
      if (const Model::IndexType* afterRemoved = FindNewIndex(nextIdx))
      {
        return afterRemoved;
      }
    }
    //try to find previous one
    for (unsigned prevIdx = oldIdx; prevIdx; --prevIdx)
    {
      if (const Model::IndexType* beforeRemoved = FindNewIndex(prevIdx - 1))
      {
        return beforeRemoved;
      }
    }
    assert(!"Invalid case");
    return 0;
  }
}
