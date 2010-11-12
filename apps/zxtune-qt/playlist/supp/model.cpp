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
#include <formatter.h>
#include <logging.h>
#include <template_parameters.h>
//library includes
#include <core/module_attrs.h>
//boost includes
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
//qt includes
#include <QtCore/QMutex>
#include <QtCore/QSet>
#include <QtGui/QIcon>
//text includes
#include "text/text.h"

namespace
{
  const std::string THIS_MODULE("UI::PlaylistModel");

  const QModelIndex EMPTY_INDEX = QModelIndex();

  class PlayitemWrapper
  {
  public:
    PlayitemWrapper(Playitem::Ptr item, const StringTemplate& tooltipTemplate)
      : Item(item)
      , TooltipTemplate(tooltipTemplate)
    {
    }

    PlayitemWrapper(const PlayitemWrapper& rh)
      : Item(rh.Item)
      , TooltipTemplate(rh.TooltipTemplate)
    {
    }

    String GetType() const
    {
      const PlayitemAttributes& attrs = Item->GetAttributes();
      return attrs.GetType();
    }

    String GetTitle() const
    {
      const PlayitemAttributes& attrs = Item->GetAttributes();
      return attrs.GetTitle();
    }

    uint_t GetDuration() const
    {
      const PlayitemAttributes& attrs = Item->GetAttributes();
      return attrs.GetDuration();
    }

    String GetTooltip() const
    {
      const Parameters::Accessor::Ptr props = GetProperties();
      const Parameters::FieldsSourceAdapter<SkipFieldsSource> fields(*props);
      return TooltipTemplate.Instantiate(fields);
    }


    Playitem::Ptr GetPlayitem() const
    {
      return Item;
    }
  private:
    Parameters::Accessor::Ptr GetProperties() const
    {
      const ZXTune::Module::Holder::Ptr holder = Item->GetModule();
      const ZXTune::Module::Information::Ptr info = holder->GetModuleInformation();
      return info->Properties();
    }
  private:
    const Playitem::Ptr Item;
    const StringTemplate& TooltipTemplate;
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

  class DecorationDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(unsigned /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const PlayitemWrapper& item, unsigned column) const
    {
      switch (column)
      {
      case PlaylistModel::COLUMN_TYPEICON:
        {
          const String& iconPath = Text::TYPEICONS_RESOURCE_PREFIX + item.GetType();
          return QIcon(ToQString(iconPath));
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
      case PlaylistModel::COLUMN_TITLE:
        return PlaylistModel::tr("Author - Title");
      case PlaylistModel::COLUMN_DURATION:
        return PlaylistModel::tr("Duration");
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
          const String& title = item.GetTitle();
          return ToQString(title);
        }
      case PlaylistModel::COLUMN_DURATION:
        {
          const String& strFormat = FormatTime(item.GetDuration(), 20000);//TODO
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
      const String& tooltip = item.GetTooltip();
      return ToQString(tooltip);
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

  class PlayitemsContainer
  {
    typedef std::list<PlayitemWrapper> ItemsContainer;
    typedef std::vector<ItemsContainer::iterator> IteratorsArray;

    class PlayitemIteratorImpl : public Playitem::Iterator
    {
    public:
      explicit PlayitemIteratorImpl(const IteratorsArray& iters)
      {
        Items.reserve(iters.size());
        for (IteratorsArray::const_iterator it = iters.begin(), lim = iters.end();
          it != lim; ++it)
        {
          Items.push_back((*it)->GetPlayitem());
        }
        Current = Items.begin();
      }

      virtual bool IsValid() const
      {
        return Current != Items.end();
      }

      virtual Playitem::Ptr Get() const
      {
        if (Current != Items.end())
        {
          return *Current;
        }
        assert(!"Playitems iterator is out of range");
        return Playitem::Ptr();
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
      std::vector<Playitem::Ptr> Items;
      std::vector<Playitem::Ptr>::const_iterator Current;
    };
  public:
    PlayitemsContainer()
      : TooltipTemplate(StringTemplate::Create(Text::TOOLTIP_TEMPLATE))
    {
    }

    void AddItem(Playitem::Ptr item)
    {
      Items.push_back(PlayitemWrapper(item, *TooltipTemplate));
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

    Playitem::Iterator::Ptr GetAllItems() const
    {
      return Playitem::Iterator::Ptr(new PlayitemIteratorImpl(Iterators));
    }

    Playitem::Iterator::Ptr GetItems(const QSet<unsigned>& items) const
    {
      IteratorsArray choosenItems;
      choosenItems.reserve(items.count());
      for (QSet<unsigned>::const_iterator it = items.begin(), lim = items.end();
        it != lim; ++it)
      {
        choosenItems.push_back(Iterators[*it]);
      }
      return Playitem::Iterator::Ptr(new PlayitemIteratorImpl(choosenItems));
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

    class Comparer
    {
    public:
      virtual ~Comparer() {}

      virtual bool CompareItems(const PlayitemWrapper& lh, const PlayitemWrapper& rh) const = 0;
    };

    void Sort(const Comparer& cmp)
    {
      std::stable_sort(Iterators.begin(), Iterators.end(),
        boost::bind(&Comparer::CompareItems, &cmp,
          boost::bind(&IteratorsArray::value_type::operator *, _1),
          boost::bind(&IteratorsArray::value_type::operator *, _2)));
    }
  private:
    const StringTemplate::Ptr TooltipTemplate;
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
    explicit PlaylistModelImpl(QObject& parent)
      : PlaylistModel(parent)
      , Providers()
      , Synchronizer(QMutex::Recursive)
      , FetchedItemsCount()
      , Container(new PlayitemsContainer())
    {
    }

    //new virtuals
    virtual Playitem::Ptr GetItem(unsigned index) const
    {
      QMutexLocker locker(&Synchronizer);
      if (const PlayitemWrapper* wrapper = Container->GetItemByIndex(index))
      {
        return wrapper->GetPlayitem();
      }
      return Playitem::Ptr();
    }

    virtual Playitem::Iterator::Ptr GetItems() const
    {
      QMutexLocker locker(&Synchronizer);
      return Container->GetAllItems();
    }

    virtual Playitem::Iterator::Ptr GetItems(const QSet<unsigned>& items) const
    {
      QMutexLocker locker(&Synchronizer);
      return Container->GetItems(items);
    }

    virtual void AddItems(Playitem::Iterator::Ptr iter)
    {
      QMutexLocker locker(&Synchronizer);
      for (; iter->IsValid(); iter->Next())
      {
        const Playitem::Ptr item = iter->Get();
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

    virtual void AddItem(Playitem::Ptr item)
    {
      QMutexLocker locker(&Synchronizer);
      Container->AddItem(item);
    }


    //base model virtuals
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
      Log::Debug(THIS_MODULE, "Create index row=%1% col=%2%",
        row, column);
      QMutexLocker locker(&Synchronizer);
      if (const PlayitemWrapper* item = Container->GetItemByIndex(row))
      {
        const Playitem::Ptr playitem = item->GetPlayitem();
        const void* const data = static_cast<const void*>(playitem.get());
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
        Log::Debug(THIS_MODULE, "Request header data section=%1% role=%2%",
          section, role);
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
      Log::Debug(THIS_MODULE, "Request data row=%1% col=%2% role=%3%",
        itemNum, fieldNum, role);
      QMutexLocker locker(&Synchronizer);
      if (const PlayitemWrapper* item = Container->GetItemByIndex(itemNum))
      {
        const RowDataProvider& provider = Providers.GetProvider(role);
        assert(static_cast<const void*>(item->GetPlayitem().get()) == index.internalPointer());
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
        comparer.reset(new TypedPlayitemsComparer<String>(&PlayitemWrapper::GetType, ascending));
        break;
      case COLUMN_TITLE:
        comparer.reset(new TypedPlayitemsComparer<String>(&PlayitemWrapper::GetTitle, ascending));
        break;
      case COLUMN_DURATION:
        comparer.reset(new TypedPlayitemsComparer<uint_t>(&PlayitemWrapper::GetDuration, ascending));
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

PlaylistModel::PlaylistModel(QObject& parent) : QAbstractItemModel(&parent)
{
}

PlaylistModel* PlaylistModel::Create(QObject& parent)
{
  return new PlaylistModelImpl(parent);
}
