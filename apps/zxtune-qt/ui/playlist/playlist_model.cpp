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
  private:
    Playitem::Ptr Item;
    StringTemplate::Ptr TooltipTemplate;
  };

  class DataProvider
  {
  public:
    typedef std::auto_ptr<DataProvider> Ptr;
    virtual ~DataProvider() {}

    static Ptr Create(int_t role);

    virtual QVariant GetHeader(int_t column) const = 0;
    virtual QVariant GetData(const PlayitemWrapper& item, int_t column) const = 0;
  };

  class DummyDataProvider : public DataProvider
  {
  public:
    virtual QVariant GetHeader(int_t /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const PlayitemWrapper& /*item*/, int_t /*column*/) const
    {
      return QVariant();
    }
  };

  class DisplayDataProvider : public DataProvider
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

    virtual QVariant GetData(const PlayitemWrapper& item, int_t column) const
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

  class DecorationDataProvider : public DataProvider
  {
  public:
    virtual QVariant GetHeader(int_t /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const Playitem& item, int_t column) const
    {
      return QVariant();
    }
  };

  class TooltipDataProvider : public DataProvider
  {
  public:
    virtual QVariant GetHeader(int_t /*column*/) const
    {
      return QVariant();
    }

    virtual QVariant GetData(const PlayitemWrapper& item, int_t /*column*/) const
    {
      return ToQString(item.GetTooltip());
    }
  };

  DataProvider::Ptr DataProvider::Create(int_t role)
  {
    switch (role)
    {
    case Qt::DisplayRole:
      return Ptr(new DisplayDataProvider());
//    case Qt::DecorationRole:
//      return Ptr(new DecorationDataProvider());
    case Qt::ToolTipRole:
      return Ptr(new TooltipDataProvider());
    default:
      return Ptr(new DummyDataProvider());
    }
  }

  class PlayitemsContainer
  {
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
    std::vector<PlayitemWrapper> Container;
    std::vector<std::size_t> Indexes;
  };

  class PlaylistModelImpl : public PlaylistModel
  {
  public:
    explicit PlaylistModelImpl(QObject* parent)
      : FetchedItemsCount()
    {
      setParent(parent);
    }

    //new virtuals
    virtual void AddItem(Playitem::Ptr item)
    {
      Container.AddItem(item);
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
      if (index.isValid())
      {
        return 0;
      }
      return FetchedItemsCount;
    }

    virtual int columnCount(const QModelIndex& index) const
    {
      if (index.isValid())
      {
        return 0;
      }
      return COLUMNS_COUNT;
    }

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
      if (Qt::Horizontal == orientation)
      {
        Log::Debug(THIS_MODULE, "Request header data section=%1% role=%2%", 
          section, role);
        const DataProvider::Ptr provider = DataProvider::Create(role);
        return provider->GetHeader(section);
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
        const DataProvider::Ptr provider = DataProvider::Create(role);
        return provider->GetData(*item, fieldNum);
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
    std::size_t FetchedItemsCount;
    PlayitemsContainer Container;
  };
}

PlaylistModel* PlaylistModel::Create(QObject* parent)
{
  return new PlaylistModelImpl(parent);
}
