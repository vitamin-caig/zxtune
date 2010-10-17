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
#include <QtCore/QTime>
#include <QtGui/QIcon>
#include <QtGui/QFont>
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

    PlayitemWrapper& operator = (const PlayitemWrapper& rh)
    {
      Item = rh.Item;
      TooltipTemplate = StringTemplate::Create(Text::TOOLTIP_TEMPLATE);
      return *this;
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
    Playitem::Ptr Item;
    StringTemplate::Ptr TooltipTemplate;
  };

  class RowStateProvider
  {
  public:
    RowStateProvider(const QModelIndex& index, const PlayitemStateCallback& cb)
      : Index(index)
      , Callback(cb)
    {
    }

    bool IsPlaying() const
    {
      return Callback.IsPlaying(Index);
    }

    bool IsPaused() const
    {
      return Callback.IsPaused(Index);
    }
  private:
    const QModelIndex& Index;
    const PlayitemStateCallback& Callback;
  };

  class RowDataProvider
  {
  public:
    virtual ~RowDataProvider() {}

    virtual QVariant GetHeader(int_t column) const = 0;
    virtual QVariant GetData(const PlayitemWrapper& item, const RowStateProvider& state, int_t column) const = 0;
  };

  class DummyDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(int_t /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const PlayitemWrapper& /*item*/, const RowStateProvider& /*state*/, int_t /*column*/) const
    {
      return QVariant();
    }
  };

  class DisplayDataProvider : public RowDataProvider
  {
  public:
    virtual QVariant GetHeader(int_t column) const
    {
      switch (column)
      {
      case PlaylistModel::COLUMN_TITLE:
        return QString::fromUtf8("Author - Title");
      case PlaylistModel::COLUMN_DURATION:
        return QString::fromUtf8("Duration");
      default:
        return QVariant();
      };
    }

    virtual QVariant GetData(const PlayitemWrapper& item, const RowStateProvider& /*state*/, int_t column) const
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
    virtual QVariant GetHeader(int_t /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const PlayitemWrapper& item, const RowStateProvider& /*state*/, int_t /*column*/) const
    {
      return ToQString(item.GetTooltip());
    }
  };

  class FontDataProvider : public RowDataProvider
  {
  public:
    FontDataProvider()
      : Regular(QString::fromUtf8("Arial"), 8)
      , Playing(Regular)
      , Paused(Regular)
    {
      Playing.setBold(true);
      Paused.setBold(true);
      Paused.setItalic(true);
    }

    virtual QVariant GetHeader(int_t /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const PlayitemWrapper& /*item*/, const RowStateProvider& state, int_t /*column*/) const
    {
      if (state.IsPaused())
      {
        return Paused;
      }
      else if (state.IsPlaying())
      {
        return Playing;
      }
      else
      {
        return Regular;
      }
    }
  private:
    QFont Regular;
    QFont Playing;
    QFont Paused;
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

    const RowDataProvider& GetProvider(int_t role) const
    {
      switch (role)
      {
      case Qt::DisplayRole:
        return Display;
      case Qt::ToolTipRole:
        return Tooltip;
      case Qt::FontRole:
        return Font;
      default:
        return Dummy;
      }
    }
  private:
    const DisplayDataProvider Display;
    const TooltipDataProvider Tooltip;
    const FontDataProvider Font;
    const DummyDataProvider Dummy;
  };

  class PlayitemsContainer
  {
    typedef std::vector<PlayitemWrapper> ItemsArray;
    typedef std::vector<std::size_t> IdexesArray;
  public:
    void AddItem(Playitem::Ptr item)
    {
      const std::size_t newIndex = Container.size();
      Container.push_back(PlayitemWrapper(item));
      Indexes.push_back(newIndex);
    }

    std::size_t CountItems() const
    {
      return Container.size();
    }

    const PlayitemWrapper* GetItem(int_t idx) const
    {
      if (idx >= int_t(Container.size()))
      {
        return 0;
      }
      const std::size_t mappedIndex = Indexes[idx];
      return &Container[mappedIndex];
    }

    void Clear()
    {
      ItemsArray tmpCont;
      IdexesArray tmpInd;
      Container.swap(tmpCont);
      Indexes.swap(tmpInd);
    }

    template<class Func>
    void Sort(bool ascending, Func functor)
    {
      if (ascending)
      {
        std::sort(Indexes.begin(), Indexes.end(),
          boost::bind(&PlayitemsContainer::Compare<Func>, this, _1, _2, functor));
      }
      else
      {
        std::sort(Indexes.begin(), Indexes.end(),
          !boost::bind(&PlayitemsContainer::Compare<Func>, this, _1, _2, functor));
      }
    }

    template<class Func>
    bool Compare(std::size_t idx1, std::size_t idx2, Func functor) const
    {
      const PlayitemWrapper& item1 = Container[idx1];
      const PlayitemWrapper& item2 = Container[idx2];
      return functor(item1) < functor(item2);
    }
  private:
    ItemsArray Container;
    IdexesArray Indexes;
  };

  class PlaylistModelImpl : public PlaylistModel
  {
  public:
    PlaylistModelImpl(const PlayitemStateCallback& callback, QObject* parent)
      : Callback(callback)
      , Providers()
      , FetchedItemsCount()
    {
      setParent(parent);
    }

    //new virtuals
    virtual Playitem::Ptr GetItem(const QModelIndex& index) const
    {
      if (const PlayitemWrapper* wrapper = (index.isValid() ? Container.GetItem(index.row()) : 0))
      {
        return wrapper->GetPlayitem();
      }
      return Playitem::Ptr();
    }

    virtual void AddItem(Playitem::Ptr item)
    {
      Container.AddItem(item);
    }

    virtual void Clear()
    {
      beginRemoveRows(EMPTY_INDEX, 0, Container.CountItems());
      Container.Clear();
      endRemoveRows();
    }

    virtual void RemoveItems(const QModelIndexList& items)
    {
    }

    //base model virtuals
    virtual bool canFetchMore(const QModelIndex& /*index*/) const
    {
      return FetchedItemsCount < Container.CountItems();
    }

    virtual void fetchMore(const QModelIndex& /*index*/)
    {
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
      Log::Debug(THIS_MODULE, "Created index row=%1% col=%2%", row, column);
      return createIndex(row, column);
    }

    virtual QModelIndex parent(const QModelIndex& /*index*/) const
    {
      return EMPTY_INDEX;
    }

    virtual int rowCount(const QModelIndex& index) const
    {
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
      if (const PlayitemWrapper* item = Container.GetItem(itemNum))
      {
        const RowDataProvider& provider = Providers.GetProvider(role);
        const RowStateProvider state(index, Callback);
        return provider.GetData(*item, state, fieldNum);
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
        Container.Sort(ascending, std::mem_fun_ref(&PlayitemWrapper::GetTitle));
        break;
      case COLUMN_DURATION:
        Container.Sort(ascending, std::mem_fun_ref(&PlayitemWrapper::GetDuration));
        break;
      default:
        break;
      }
      reset();
    }
  private:
    const PlayitemStateCallback& Callback;
    const DataProvidersSet Providers;
    std::size_t FetchedItemsCount;
    PlayitemsContainer Container;
  };
}

PlaylistModel* PlaylistModel::Create(const PlayitemStateCallback& callback, QObject* parent)
{
  return new PlaylistModelImpl(callback, parent);
}
