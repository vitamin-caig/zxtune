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
#include "playlist_model.h"
#include "playlist_model_moc.h"
#include "ui/utils.h"
#include "ui/format.h"
//common includes
#include <formatter.h>
#include <logging.h>
#include <template_parameters.h>
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/bind.hpp>
//qt includes
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtGui/QApplication>
//text includes
#include "text/text.h"

namespace
{
  const std::string THIS_MODULE("UI::PlaylistModel");

  const QModelIndex EMPTY_INDEX = QModelIndex();

  class PlayitemWrapper
  {
  public:
    PlayitemWrapper()
    {
    }

    explicit PlayitemWrapper(Playitem::Ptr item)
      : Item(item)
      , TooltipTemplate(StringTemplate::Create(Text::TOOLTIP_TEMPLATE))
    {
    }

    PlayitemWrapper(const PlayitemWrapper& rh)
      : Item(rh.Item)
      , TooltipTemplate(StringTemplate::Create(Text::TOOLTIP_TEMPLATE))
    {
    }

    String GetTitle() const
    {
      const ZXTune::Module::Information::Ptr info = Item->GetModuleInfo();
      const Parameters::Accessor::Ptr props = info->Properties();
      return GetModuleTitle(Text::MODULE_PLAYLIST_FORMAT, *props);
    }

    uint_t GetDuration() const
    {
      const ZXTune::Module::Information::Ptr info = Item->GetModuleInfo();
      return info->FramesCount();
    }

    String GetTooltip() const
    {
      const ZXTune::Module::Information::Ptr info = Item->GetModuleInfo();
      const Parameters::Accessor::Ptr props = info->Properties();
      const Parameters::FieldsSourceAdapter<SkipFieldsSource> fields(*props);
      return TooltipTemplate->Instantiate(fields);
    }

    Playitem::Ptr GetPlayitem() const
    {
      return Item;
    }
  private:
    const Playitem::Ptr Item;
    const StringTemplate::Ptr TooltipTemplate;
  };

  class RowDataProvider
  {
  public:
    virtual ~RowDataProvider() {}

    virtual QVariant GetHeader(unsigned column) const = 0;
    virtual QVariant GetData(const PlayitemWrapper& item, unsigned column) const = 0;
  };

  class DummyDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(unsigned /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const PlayitemWrapper& /*item*/, unsigned /*column*/) const
    {
      return QVariant();
    }
  };

  class DisplayDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(unsigned column) const
    {
      switch (column)
      {
      case PlaylistModel::COLUMN_TITLE:
        return QApplication::translate("Playlist", "Author - Title", 0, QApplication::UnicodeUTF8);
      case PlaylistModel::COLUMN_DURATION:
        return QApplication::translate("Playlist", "Duration", 0, QApplication::UnicodeUTF8);
      default:
        return QVariant();
      };
    }

    virtual QVariant GetData(const PlayitemWrapper& item, unsigned column) const
    {
      switch (column)
      {
      case PlaylistModel::COLUMN_TITLE:
        {
          return ToQString(item.GetTitle());
        }
      case PlaylistModel::COLUMN_DURATION:
        {
          const String strFormat = FormatTime(item.GetDuration(), 20000);//TODO
          return ToQString(strFormat);
        }
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

    virtual QVariant GetData(const PlayitemWrapper& item, unsigned /*column*/) const
    {
      return ToQString(item.GetTooltip());
    }
  };

  class DataProvidersSet
  {
  public:
    DataProvidersSet()
      : Display()
      , Tooltip()
      , Dummy()
    {
    }

    const RowDataProvider& GetProvider(int role) const
    {
      switch (role)
      {
      case Qt::DisplayRole:
        return Display;
      case Qt::ToolTipRole:
        return Tooltip;
      default:
        return Dummy;
      }
    }
  private:
    const DisplayDataProvider Display;
    const TooltipDataProvider Tooltip;
    const DummyDataProvider Dummy;
  };

  class PlayitemsContainer
  {
    typedef std::list<PlayitemWrapper> ItemsContainer;
    typedef std::vector<ItemsContainer::iterator> IteratorsArray;
  public:
    void AddItem(Playitem::Ptr item)
    {
      Items.push_back(PlayitemWrapper(item));
      IteratorsArray::value_type it = Items.end();
      Iterators.push_back(--it);
    }

    std::size_t CountItems() const
    {
      return Items.size();
    }

    const PlayitemWrapper* GetItemByIndex(int idx) const
    {
      if (idx >= int(Iterators.size()))
      {
        return 0;
      }
      const IteratorsArray::value_type it = Iterators[idx];
      return &*it;
    }

    void Clear()
    {
      ItemsContainer tmpCont;
      IteratorsArray tmpIter;
      Items.swap(tmpCont);
      Iterators.swap(tmpIter);
    }

    void Remove(const QSet<unsigned>& indexes)
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

    class Comparer
    {
    public:
      virtual ~Comparer() {}

      virtual bool CompareItems(const PlayitemWrapper& lh, const PlayitemWrapper& rh) const = 0;
    };
 
    void Sort(const Comparer& cmp)
    {
      std::sort(Iterators.begin(), Iterators.end(),
        boost::bind(&Comparer::CompareItems, &cmp, 
          boost::bind(&IteratorsArray::value_type::operator *, _1),
          boost::bind(&IteratorsArray::value_type::operator *, _2)));
    }
  private:
    ItemsContainer Items;
    IteratorsArray Iterators;
  };

  template<class T>
  class TypedPlayitemsComparer : public PlayitemsContainer::Comparer
  {
  public:
    typedef T (PlayitemWrapper::*Functor)() const;
    TypedPlayitemsComparer(Functor fn, bool ascending)
      : Getter(fn)
      , Ascending(ascending)
    {
    }

    bool CompareItems(const PlayitemWrapper& lh, const PlayitemWrapper& rh) const
    {
      const T& val1 = (lh.*Getter)();
      const T& val2 = (rh.*Getter)();
      return Ascending
        ? val1 < val2
        : val1 > val2;
    }
  private:
    const Functor Getter;
    const bool Ascending;
  };

  class PlaylistModelImpl : public PlaylistModel
  {
  public:
    explicit PlaylistModelImpl(QObject* parent)
      : Providers()
      , Synchronizer(QMutex::Recursive)
      , FetchedItemsCount()
    {
      setParent(parent);
    }

    //new virtuals
    virtual Playitem::Ptr GetItem(unsigned index) const
    {
      QMutexLocker locker(&Synchronizer);
      if (const PlayitemWrapper* wrapper = Container.GetItemByIndex(index))
      {
        return wrapper->GetPlayitem();
      }
      return Playitem::Ptr();
    }

    virtual void AddItem(Playitem::Ptr item)
    {
      QMutexLocker locker(&Synchronizer);
      Container.AddItem(item);
    }

    virtual void Clear()
    {
      QMutexLocker locker(&Synchronizer);
      Container.Clear();
      FetchedItemsCount = 0;
      reset();
    }

    virtual void RemoveItems(const QSet<unsigned>& items)
    {
      QMutexLocker locker(&Synchronizer);
      Container.Remove(items);
      FetchedItemsCount = Container.CountItems();
      reset();
    }

    //base model virtuals
    virtual bool canFetchMore(const QModelIndex& /*index*/) const
    {
      QMutexLocker locker(&Synchronizer);
      return FetchedItemsCount < Container.CountItems();
    }

    virtual void fetchMore(const QModelIndex& /*index*/)
    {
      QMutexLocker locker(&Synchronizer);
      const std::size_t nextCount = Container.CountItems();
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
      Log::Debug(THIS_MODULE, "Create index row=%1% col=%2%",
        row, column);
      QMutexLocker locker(&Synchronizer);
      if (const PlayitemWrapper* item = Container.GetItemByIndex(row))
      {
        const Playitem::Ptr playitem = item->GetPlayitem();
        void* const data = static_cast<void*>(playitem.get());
        return createIndex(row, column, data);
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
        Log::Debug(THIS_MODULE, "Request header data section=%1% role=%2%",
          section, role);
        const RowDataProvider& provider = Providers.GetProvider(role);
        return provider.GetHeader(section);
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
      Log::Debug(THIS_MODULE, "Request data row=%1% col=%2% role=%3%",
        itemNum, fieldNum, role);
      QMutexLocker locker(&Synchronizer);
      if (const PlayitemWrapper* item = Container.GetItemByIndex(itemNum))
      {
        const RowDataProvider& provider = Providers.GetProvider(role);
        assert(static_cast<void*>(item->GetPlayitem().get()) == index.internalPointer());
        return provider.GetData(*item, fieldNum);
      }
      return QVariant();
    }

    virtual void sort(int column, Qt::SortOrder order)
    {
      Log::Debug(THIS_MODULE, "Sort data in column=%1% by order=%2%",
        column, order);
      const bool ascending = order == Qt::AscendingOrder;
      switch (column)
      {
      case COLUMN_TITLE:
        {
          QMutexLocker locker(&Synchronizer);
          const TypedPlayitemsComparer<String> comparer(&PlayitemWrapper::GetTitle, ascending);
          Container.Sort(comparer);
        }
        break;
      case COLUMN_DURATION:
        {
          QMutexLocker locker(&Synchronizer);
          const TypedPlayitemsComparer<uint_t> comparer(&PlayitemWrapper::GetDuration, ascending);
          Container.Sort(comparer);
        }
        break;
      default:
        break;
      }
      reset();
    }
  private:
    const DataProvidersSet Providers;
    mutable QMutex Synchronizer;
    std::size_t FetchedItemsCount;
    PlayitemsContainer Container;
  };
}

PlaylistModel* PlaylistModel::Create(QObject* parent)
{
  return new PlaylistModelImpl(parent);
}
