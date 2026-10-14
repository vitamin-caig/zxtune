/**
 *
 * @file
 *
 * @brief Playlist model implementation
 *
 * @author vitamin.caig@gmail.com
 *
 **/

#include "apps/zxtune-qt/playlist/supp/model.h"

#include "apps/zxtune-qt/playlist/supp/storage.h"
#include "apps/zxtune-qt/supp/thread_utils.h"
#include "apps/zxtune-qt/ui/utils.h"

#include "async/activity.h"
#include "debug/log.h"
#include "math/bitops.h"
#include "parameters/template.h"
#include "time/serialize.h"
#include "tools/progress_callback_helpers.h"

#include "contract.h"
#include "make_ptr.h"
#include "string_view.h"

#include <QtCore/QDataStream>
#include <QtCore/QMimeData>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QStringList>

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>

namespace
{
  const Debug::Stream Dbg("Playlist::Model");

  const QModelIndex EMPTY_INDEX = QModelIndex();

  class RowDataProvider
  {
  public:
    virtual ~RowDataProvider() = default;

    virtual bool IsLightweightField(unsigned column) const = 0;
    virtual QVariant GetData(const Playlist::Item::Data& item, unsigned column) const = 0;
  };

  class DummyDataProvider : public RowDataProvider
  {
  public:
    bool IsLightweightField(unsigned /*column*/) const override
    {
      return true;
    }

    QVariant GetData(const Playlist::Item::Data& /*item*/, unsigned /*column*/) const override
    {
      return {};
    }
  };

  template<class T>
  inline QString ToHex(T val)
  {
    return QString::number(val, 16).rightJustified(2 * sizeof(val), '0');
  }

  class DisplayDataProvider : public RowDataProvider
  {
  public:
    bool IsLightweightField(unsigned column) const override
    {
      return column == Playlist::Model::COLUMN_PATH;
    }

    QVariant GetData(const Playlist::Item::Data& item, unsigned column) const override
    {
      switch (column)
      {
      case Playlist::Model::COLUMN_TYPE:
        return ToQString(item.GetType());
      case Playlist::Model::COLUMN_DISPLAY_NAME:
        return ToQString(item.GetDisplayName());
      case Playlist::Model::COLUMN_DURATION:
        return ToQString(Time::ToString(item.GetDuration()));
      case Playlist::Model::COLUMN_AUTHOR:
        return ToQString(item.GetAuthor());
      case Playlist::Model::COLUMN_TITLE:
        return ToQString(item.GetTitle());
      case Playlist::Model::COLUMN_COMMENT:
        return ToQString(item.GetComment());
      case Playlist::Model::COLUMN_PATH:
        return ToQString(item.GetFullPath());
      case Playlist::Model::COLUMN_SIZE:
        return QString::number(item.GetSize());
      case Playlist::Model::COLUMN_CRC:
        return ToHex(item.GetChecksum());
      case Playlist::Model::COLUMN_FIXEDCRC:
        return ToHex(item.GetCoreChecksum());
      default:
        return {};
      };
    }
  };

  class DataProvidersSet
  {
  public:
    DataProvidersSet()
      : Display()
      , Dummy()
    {}

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
    using Functor = T (Playlist::Item::Data::*)() const;
    TypedPlayitemsComparer(Functor fn, bool ascending)
      : Getter(fn)
      , Ascending(ascending)
    {}

    bool CompareItems(const Playlist::Item::Data& lh, const Playlist::Item::Data& rh) const override
    {
      const T val1 = (lh.*Getter)();
      const T val2 = (rh.*Getter)();
      return Ascending ? val1 < val2 : val2 < val1;
    }

  private:
    const Functor Getter;
    const bool Ascending;
  };

  template<class R>
  Playlist::Item::Comparer::Ptr CreateComparer(R (Playlist::Item::Data::*func)() const, bool ascending)
  {
    return MakePtr<TypedPlayitemsComparer<R>>(func, ascending);
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
    case Playlist::Model::COLUMN_COMMENT:
      return CreateComparer(&Playlist::Item::Data::GetComment, ascending);
    case Playlist::Model::COLUMN_PATH:
      return CreateComparer(&Playlist::Item::Data::GetFullPath, ascending);
    case Playlist::Model::COLUMN_SIZE:
      return CreateComparer(&Playlist::Item::Data::GetSize, ascending);
    case Playlist::Model::COLUMN_CRC:
      return CreateComparer(&Playlist::Item::Data::GetChecksum, ascending);
    case Playlist::Model::COLUMN_FIXEDCRC:
      return CreateComparer(&Playlist::Item::Data::GetCoreChecksum, ascending);
    default:
      return {};
    }
  }

  class ComparisonsCounter : public Playlist::Item::Comparer
  {
  public:
    ComparisonsCounter(const Playlist::Item::Comparer& delegate, Log::ProgressCallback& cb)
      : Delegate(delegate)
      , Callback(cb)
    {}

    bool CompareItems(const Playlist::Item::Data& lh, const Playlist::Item::Data& rh) const override
    {
      Callback.OnProgress(++Done);
      return Delegate.CompareItems(lh, rh);
    }

  private:
    const Playlist::Item::Comparer& Delegate;
    Log::ProgressCallback& Callback;
    mutable uint_t Done = 0;
  };

  class SortOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    explicit SortOperation(Playlist::Item::Comparer::Ptr comparer)
      : Comparer(std::move(comparer))
    {}

    void Execute(Playlist::Item::Storage& storage, Log::ProgressCallback& cb) override
    {
      const uint_t totalItems = storage.CountItems();
      // according to STL spec: The number of comparisons is approximately N log N, where N is the list's size. Assume
      // that log is binary.
      Log::PercentProgressCallback progress(totalItems * Math::Log2(totalItems), cb);
      const ComparisonsCounter countingComparer(*Comparer, progress);
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
    virtual ~OperationTarget() = default;

    virtual void ExecuteOperation(typename OpType::Ptr op) = 0;
  };

  template<class OpType>
  class AsyncOperation : public Async::Operation
  {
  public:
    AsyncOperation(typename OpType::Ptr op, OperationTarget<OpType>& executor)
      : Op(std::move(op))
      , Delegate(executor)
    {}

    void Prepare() override {}

    void Execute() override
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
    void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data) override
    {
      const auto& state = data->GetState();
      if (state.IsReady() && !state.GetIfError())
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

  class ModelImpl
    : public Playlist::Model
    , public OperationTarget<Playlist::Item::StorageAccessOperation>
    , public OperationTarget<Playlist::Item::StorageModifyOperation>
    , private Log::ProgressCallback
  {
  public:
    explicit ModelImpl(QObject& parent)
      : Playlist::Model(parent)
      , Providers()
      , Container(Playlist::Item::Storage::Create())
      , AsyncExecution(Async::Activity::CreateStub())
      , Canceled(false)
    {
      Dbg("Created at {}", Self());
    }

    ~ModelImpl() override
    {
      WaitOperationFinish();
      Dbg("Destroyed at {}", Self());
    }

    void PerformOperation(Playlist::Item::StorageAccessOperation::Ptr operation) override
    {
      WaitOperationFinish();
      const Async::Operation::Ptr wrapper =
          MakePtr<AsyncOperation<Playlist::Item::StorageAccessOperation>>(operation, *this);
      AsyncExecution = Async::Activity::Create(wrapper);
    }

    void PerformOperation(Playlist::Item::StorageModifyOperation::Ptr operation) override
    {
      WaitOperationFinish();
      const Async::Operation::Ptr wrapper =
          MakePtr<AsyncOperation<Playlist::Item::StorageModifyOperation>>(operation, *this);
      AsyncExecution = Async::Activity::Create(wrapper);
    }

    void WaitOperationFinish() override
    {
      try
      {
        AsyncExecution->Wait();
      }
      catch (const Error& e)
      {
        Dbg("Operation failed: {}", e.ToString());
      }
      Canceled = false;
    }

    // new virtuals
    unsigned CountItems() const override
    {
      return static_cast<unsigned>(WithStorage([](const auto& storage) { return storage.CountItems(); }));
    }

    Playlist::Item::Data::Ptr GetItem(IndexType index) const override
    {
      return WithStorage([index](const auto& storage) { return storage.GetItem(index); });
    }

    QStringList GetItemsPaths(const IndexSet& items) const override
    {
      PathsVisitor visitor;
      WithStorage([&](const auto& storage) { storage.ForSpecifiedItems(items, visitor); });
      return visitor.GetResult();
    }

    unsigned GetVersion() const override
    {
      return WithStorage([](const auto& storage) { return storage.GetVersion(); });
    }

    void Clear() override
    {
      ChangeModel([this]() { Container = Playlist::Item::Storage::Create(); });
    }

    void RemoveItems(IndexSet::Ptr items) override
    {
      if (!items || items->empty())
      {
        return;
      }
      ChangeModel([this, &items]() { Container->RemoveItems(*items); });
    }

    void MoveItems(const IndexSet& items, IndexType target) override
    {
      Dbg("Moving {} items to row {}", items.size(), target);
      ChangeModel([this, &items, &target]() { Container->MoveItems(items, target); });
    }

    void AddItem(Playlist::Item::Data::Ptr item) override
    {
      // Called for each item found during scan, so notify only first time
      if (0 == CountItems())
      {
        AddAndNotify(item);
      }
      else
      {
        Add(item);
      }
    }

    void AddItems(Playlist::Item::Collection::Ptr items) override
    {
      AddAndNotify(items);
    }

    void CancelLongOperation() override
    {
      Canceled = true;
      AsyncExecution->Wait();
    }

    // base model virtuals

    /* Drag'n'Drop support*/
    Qt::DropActions supportedDropActions() const override
    {
      return Qt::MoveAction;
    }

    // required for drag/drop enabling
    // do not pay attention on item state
    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
      const Qt::ItemFlags defaultFlags = Playlist::Model::flags(index);
      const Qt::ItemFlags invalidFlags = Qt::ItemIsDropEnabled | defaultFlags;
      return WithStorage([&](const auto& storage) {
        if (index.internalId() != storage.GetVersion())
        {
          return invalidFlags;
        }
        const Qt::ItemFlags validFlags = Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
        return validFlags;
      });
    }

    QStringList mimeTypes() const override
    {
      QStringList types;
      types << INDICES_MIMETYPE;
      return types;
    }

    QMimeData* mimeData(const QModelIndexList& indices) const override
    {
      std::unique_ptr<QMimeData> mimeData(new QMimeData());
      QByteArray encodedData;
      {
        QDataStream stream(&encodedData, QIODevice::WriteOnly);
        for (const QModelIndex& index : indices)
        {
          if (index.isValid())
          {
            stream << index.row();
          }
        }
      }

      mimeData->setData(INDICES_MIMETYPE, encodedData);
      return mimeData.release();
    }

    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int /*row*/, int /*column*/,
                      const QModelIndex& parent) override
    {
      if (action == Qt::IgnoreAction)
      {
        return true;
      }

      if (!data->hasFormat(INDICES_MIMETYPE))
      {
        return false;
      }
      const unsigned beginRow = parent.isValid() ? parent.row() : rowCount(EMPTY_INDEX);

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

    bool canFetchMore(const QModelIndex& /*index*/) const override
    {
      return WithStorage([this](const auto& storage) { return FetchedItemsCount < storage.CountItems(); });
    }

    void fetchMore(const QModelIndex& /*index*/) override
    {
      // Runs in the model's thread only, so the shared lock is enough for the count update
      WithStorage([this](const auto& storage) {
        const std::size_t nextCount = storage.CountItems();
        beginInsertRows(EMPTY_INDEX, static_cast<int>(FetchedItemsCount), nextCount - 1);
        FetchedItemsCount = nextCount;
        endInsertRows();
      });
    }

    QModelIndex index(int row, int column, const QModelIndex& parent) const override
    {
      if (parent.isValid())
      {
        return EMPTY_INDEX;
      }
      return WithStorage([&](const auto& storage) {
        if (row < static_cast<int>(storage.CountItems()))
        {
          return createIndex(row, column, storage.GetVersion());
        }
        return EMPTY_INDEX;
      });
    }

    QModelIndex parent(const QModelIndex& /*index*/) const override
    {
      return EMPTY_INDEX;
    }

    int rowCount(const QModelIndex& index) const override
    {
      const std::shared_lock lock(SyncAccess);
      return index.isValid() ? 0 : static_cast<int>(FetchedItemsCount);
    }

    int columnCount(const QModelIndex& index) const override
    {
      return index.isValid() ? 0 : COLUMNS_COUNT;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
      if (Qt::Vertical == orientation && Qt::DisplayRole == role)
      {
        // item number is 1-based
        return {section + 1};
      }
      return {};
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
      if (!index.isValid())
      {
        return {};
      }
      const int_t fieldNum = index.column();
      const int_t itemNum = index.row();
      return WithStorage([&](const auto& storage) -> QVariant {
        if (auto item = storage.GetItem(itemNum))
        {
          const auto& provider = Providers.GetProvider(role);
          if (provider.IsLightweightField(fieldNum) || item->GetState().IsReady())
          {
            return provider.GetData(*item, fieldNum);
          }
          else
          {
            const_cast<ModelImpl*>(this)->AsyncLoad(std::move(item), index);
          }
        }
        return {};
      });
    }

    void sort(int column, Qt::SortOrder order) override
    {
      Dbg("Sort data in column={} by order={}", column, static_cast<int>(order));
      const bool ascending = order == Qt::AscendingOrder;
      if (auto comparer = CreateComparerByColumn(column, ascending))
      {
        auto op = MakePtr<SortOperation>(std::move(comparer));
        PerformOperation(std::move(op));
      }
    }

  private:
    const void* Self() const
    {
      return this;
    }

    void AsyncLoad(Playlist::Item::Data::Ptr item, const QModelIndex& index)
    {
      IOThread::Execute(this, &ModelImpl::LoadItem, item, index);
    }

    //! Runs on the IO thread
    static void LoadItem(QPointer<ModelImpl> self, std::weak_ptr<const Playlist::Item::Data> weakItem,
                         const QModelIndex& index)
    {
      if (auto item = weakItem.lock())
      {
        const auto& state = item->GetState();
        if (!state.IsLoading() && !state.IsReady())
        {
          item->GetModule();
          SelfThread::Execute(self, &ModelImpl::NotifyRowChanged, index);
        }
      }
    }

    void NotifyRowChanged(const QModelIndex& index)
    {
      dataChanged(index.siblingAtColumn(0), index.siblingAtColumn(Playlist::Model::COLUMNS_COUNT));
    }

    template<class F>
    auto WithStorage(F func) const -> decltype(func(std::declval<const Playlist::Item::Storage&>()))
    {
      const std::shared_lock lock(SyncAccess);
      return func(static_cast<const Playlist::Item::Storage&>(*Container));
    }

    //! May be called from any thread; the storage change is guarded, the model notification is marshalled
    template<class Function>
    void ChangeModel(Function cmd)
    {
      Playlist::Model::OldToNewIndexMap::Ptr remapping;
      {
        const std::lock_guard lock(SyncAccess);
        cmd();
        FetchedItemsCount = Container->CountItems();
        remapping = Container->ResetIndices();
      }
      SelfThread::Execute(this, &ModelImpl::NotifyModelReset, std::move(remapping));
    }

    void NotifyModelReset(Playlist::Model::OldToNewIndexMap::Ptr remapping)
    {
      beginResetModel();
      endResetModel();
      if (remapping)
      {
        emit IndicesChanged(std::move(remapping));
      }
    }

    void ExecuteOperation(Playlist::Item::StorageAccessOperation::Ptr operation) override
    {
      // Access operations read the live storage, so block model mutations for their duration
      const std::shared_lock lock(SyncAccess);
      emit OperationStarted();
      try
      {
        operation->Execute(*Container, *this);
      }
      catch (const std::exception&)
      {}
      emit OperationStopped();
    }

    void ExecuteOperation(Playlist::Item::StorageModifyOperation::Ptr operation) override
    {
      {
        emit OperationStarted();
        Playlist::Item::Storage::Ptr tmpStorage;
        {
          const std::shared_lock lock(SyncAccess);
          tmpStorage = Container->Clone();
        }
        try
        {
          operation->Execute(*tmpStorage, *this);
          ChangeModel([this, &tmpStorage]() { Container = std::move(tmpStorage); });
        }
        catch (const std::exception&)
        {}
        emit OperationStopped();
      }
    }

    void OnProgress(uint_t current) override
    {
      emit OperationProgressChanged(current);
      if (Canceled)
      {
        throw std::exception();
      }
    }

    void OnProgress(uint_t current, StringView /*message*/) override
    {
      OnProgress(current);
    }

    template<class T>
    void AddAndNotify(const T& val)
    {
      ChangeModel([this, &val]() { Container->Add(val); });
    }

    template<class T>
    void Add(const T& val)
    {
      const std::lock_guard lock(SyncAccess);
      Container->Add(val);
    }

  private:
    const DataProvidersSet Providers;
    mutable std::shared_mutex SyncAccess;
    std::size_t FetchedItemsCount = 0;
    Playlist::Item::Storage::Ptr Container;
    Async::Activity::Ptr AsyncExecution;
    std::atomic<bool> Canceled;
  };
}  // namespace

namespace Playlist
{
  Model::Model(QObject& parent)
    : QAbstractItemModel(&parent)
  {}

  Model::Ptr Model::Create(QObject& parent)
  {
    REGISTER_METATYPE(Playlist::Model::OldToNewIndexMap::Ptr);
    return new ModelImpl(parent);
  }

  const Model::IndexType* Model::OldToNewIndexMap::FindNewIndex(IndexType oldIdx) const
  {
    const auto it = find(oldIdx);
    if (it != end())
    {
      return &it->second;
    }
    return nullptr;
  }

  const Model::IndexType* Model::OldToNewIndexMap::FindNewSuitableIndex(IndexType oldIdx) const
  {
    if (empty())
    {
      return nullptr;
    }
    if (const Model::IndexType* direct = FindNewIndex(oldIdx))
    {
      return direct;
    }
    // try to find next one
    for (unsigned nextIdx = oldIdx + 1, totalOldItems = rbegin()->first + 1; nextIdx < totalOldItems; ++nextIdx)
    {
      if (const Model::IndexType* afterRemoved = FindNewIndex(nextIdx))
      {
        return afterRemoved;
      }
    }
    // try to find previous one
    for (unsigned prevIdx = oldIdx; prevIdx; --prevIdx)
    {
      if (const Model::IndexType* beforeRemoved = FindNewIndex(prevIdx - 1))
      {
        return beforeRemoved;
      }
    }
    assert(!"Invalid case");
    return nullptr;
  }
}  // namespace Playlist
