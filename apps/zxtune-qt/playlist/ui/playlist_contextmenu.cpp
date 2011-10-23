/*
Abstract:
  Playlist contextmenu implementation

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

//local includes
#include "playlist_contextmenu.h"
#include "table_view.h"
#include "ui/utils.h"
#include "no_items_contextmenu.ui.h"
#include "single_item_contextmenu.ui.h"
#include "multiple_items_contextmenu.ui.h"
#include "playlist/supp/storage.h"
//common includes
#include <format.h>
//library includes
#include <async/signals_collector.h>
#include <core/module_attrs.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/algorithm/string/join.hpp>
//qt includes
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
//text includes
#include "text/text.h"

namespace
{
  class NoItemsContextMenu : public QMenu
                           , private Ui::NoItemsContextMenu
  {
  public:
    NoItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver)
      : QMenu(&parent)
    {
      //setup self
      setupUi(this);

      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveAllDuplicates()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectAllRipOffs()));
      receiver.connect(ShowStatisticAction, SIGNAL(triggered()), SLOT(ShowAllStatistic()));
    }
  };

  class SingleItemContextMenu : public QMenu
                              , private Ui::SingleItemContextMenu
  {
  public:
    SingleItemContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver)
      : QMenu(&parent)
    {
      //setup self
      setupUi(this);

      receiver.connect(PlayAction, SIGNAL(triggered()), SLOT(PlaySelected()));
      receiver.connect(DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      receiver.connect(CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicatesOfSelected()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffsOfSelected()));
      receiver.connect(CopyToClipboardAction, SIGNAL(triggered()), SLOT(CopyPathToClipboard()));
    }
  };

  class MultipleItemsContextMenu : public QMenu
                                 , private Ui::MultipleItemsContextMenu
  {
  public:
    MultipleItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver, std::size_t count)
      : QMenu(&parent)
    {
      //setup self
      setupUi(this);
      InfoAction->setText(ToQString(Strings::Format(Text::CONTEXTMENU_STATUS, count)));

      receiver.connect(DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      receiver.connect(CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
      receiver.connect(GroupAction, SIGNAL(triggered()), SLOT(GroupSelected()));
      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicatesInSelected()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffsInSelected()));
      receiver.connect(CopyToClipboardAction, SIGNAL(triggered()), SLOT(CopyPathToClipboard()));
      receiver.connect(ShowStatisticAction, SIGNAL(triggered()), SLOT(ShowStatisticOfSelected()));
    }
  };

  template<class T>
  class PropertyModel
  {
  public:
    typedef T (Playlist::Item::Data::*GetFunctionType)() const;

    PropertyModel(const Playlist::Item::Storage& model, const GetFunctionType getter)
      : Model(model)
      , Getter(getter)
    {
    }

    class Visitor
    {
    public:
      virtual ~Visitor() {}

      virtual void OnItem(Playlist::Model::IndexType index, const T& val) = 0;
    };

    void ForAllItems(Visitor& visitor) const;

    void ForSpecifiedItems(const Playlist::Model::IndexSet& items, Visitor& visitor) const;
  private:
    const Playlist::Item::Storage& Model;
    const GetFunctionType Getter;
  }; 

  template<class T>
  class VisitorAdapter : public Playlist::Model::Visitor
  {
  public:
    VisitorAdapter(const typename PropertyModel<T>::GetFunctionType getter, typename PropertyModel<T>::Visitor& delegate)
      : Getter(getter)
      , Delegate(delegate)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, Playlist::Item::Data::Ptr data)
    {
      if (!data->IsValid())
      {
        return;
      }
      const T val = ((*data).*Getter)();
      Delegate.OnItem(index, val);
    }

  private:
    const typename PropertyModel<T>::GetFunctionType Getter;
    typename PropertyModel<T>::Visitor& Delegate;
  };

  class PathesCollector : public Playlist::Model::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      if (!data->IsValid())
      {
        return;
      }
      const String path = data->GetFullPath();
      Result.push_back(path);
    }

    String GetResult() const
    {
      static const Char PATHES_DELIMITER[] = {'\n', 0};
      return boost::algorithm::join(Result, PATHES_DELIMITER);
    }
  private:
    StringArray Result;
  };

  class StatisticSource
  {
  public:
    typedef boost::shared_ptr<const StatisticSource> Ptr;
    virtual ~StatisticSource() {}

    virtual String GetBasicStatistic() const = 0;
    virtual String GetDetailedStatistic() const = 0;
  };

  void ShowStatistic(const StatisticSource& src, QWidget& parent)
  {
    QMessageBox msgBox(QMessageBox::Information, QString::fromUtf8("Statistic:"), ToQString(src.GetBasicStatistic()),
      QMessageBox::Ok, &parent);
    msgBox.setDetailedText(ToQString(src.GetDetailedStatistic()));
    msgBox.exec();
  }

  class StatisticCollector : public Playlist::Model::Visitor
                           , public StatisticSource
  {
  public:
    StatisticCollector()
      : Processed()
      , Invalids()
      , Duration()
      , Size()
    {
    }

    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      //check for the data first to define is data valid or not
      const String type = data->GetType();
      if (!data->IsValid())
      {
        ++Invalids;
        return;
      }
      assert(!type.empty());
      ++Processed;
      Duration += data->GetDuration();
      Size += data->GetSize();
      ++Types[type];
    }

    virtual String GetBasicStatistic() const
    {
      const String duration = Strings::FormatTime(Duration.Get(), 1000);
      return Strings::Format(Text::BASIC_STATISTIC_TEMPLATE,
        Processed, Invalids,
        duration,
        Size,
        Types.size()
        );
    }

    virtual String GetDetailedStatistic() const
    {
      return std::accumulate(Types.begin(), Types.end(), String(), &TypeStatisticToString);
    }
  private:
    static String TypeStatisticToString(const String& prev, const std::pair<String, std::size_t>& item)
    {
      const String& next = Strings::Format(Text::TYPE_STATISTIC_TEMPLATE, item.first, item.second);
      return prev.empty()
        ? next
        : prev + '\n' + next;
    }
  private:
    std::size_t Processed;
    std::size_t Invalids;
    Time::Stamp<uint64_t, 1000> Duration;
    uint64_t Size;
    std::map<String, std::size_t> Types;
  };

  template<class T>
  void PropertyModel<T>::ForAllItems(typename PropertyModel<T>::Visitor& visitor) const
  {
    VisitorAdapter<T> adapter(Getter, visitor);
    Model.ForAllItems(adapter);
  }

  template<class T>
  void PropertyModel<T>::ForSpecifiedItems(const Playlist::Model::IndexSet& items, typename PropertyModel<T>::Visitor& visitor) const
  {
    assert(!items.empty());
    VisitorAdapter<T> adapter(Getter, visitor);
    Model.ForSpecifiedItems(items, adapter);
  }


  template<class T>
  class PropertiesFilter : public PropertyModel<T>::Visitor
  {
  public:
    PropertiesFilter(typename PropertyModel<T>::Visitor& delegate, const std::set<T>& filter)
      : Delegate(delegate)
      , Filter(filter)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      if (Filter.count(val))
      {
        Delegate.OnItem(index, val);
      }
    }
  private:
    typename PropertyModel<T>::Visitor& Delegate;
    const std::set<T>& Filter;
  };

  template<class T>
  class PropertiesCollector : public PropertyModel<T>::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType /*index*/, const T& val)
    {
      Result.insert(val);
    }

    const std::set<T>& GetResult() const
    {
      return Result;
    }
  private:
    std::set<T> Result;
  };

  template<class T>
  class DuplicatesCollector : public PropertyModel<T>::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      if (!Visited.insert(val).second)
      {
        Result.insert(index);
      }
    }

    const Playlist::Model::IndexSet& GetResult() const
    {
      return Result;
    }
  private:
    std::set<T> Visited;
    Playlist::Model::IndexSet Result;
  };

  template<class T>
  class RipOffsCollector : public PropertyModel<T>::Visitor
  {
  public:
    virtual void OnItem(Playlist::Model::IndexType index, const T& val)
    {
      const std::pair<typename PropToIndex::iterator, bool> result = Visited.insert(typename PropToIndex::value_type(val, index));
      if (!result.second)
      {
        Result.insert(result.first->second);
        Result.insert(index);
      }
    }

    const Playlist::Model::IndexSet& GetResult() const
    {
      return Result;
    }
  private:
    typedef typename std::map<T, Playlist::Model::IndexType> PropToIndex;
    PropToIndex Visited;
    Playlist::Model::IndexSet Result;
  };

  class RemoveAllDupsOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    virtual void Execute(Playlist::Item::Storage& stor)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
      DuplicatesCollector<uint32_t> dups;
      propertyModel.ForAllItems(dups);
      const Playlist::Model::IndexSet& toRemove = dups.GetResult();
      stor.RemoveItems(toRemove);
    }
  };

  class RemoveDupsOfSelectedOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    explicit RemoveDupsOfSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(Playlist::Item::Storage& stor)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
      //select all rips but delete only nonselected
      RipOffsCollector<uint32_t> dups;
      {
        PropertiesCollector<uint32_t> selectedProps;
        propertyModel.ForSpecifiedItems(SelectedItems, selectedProps);
        PropertiesFilter<uint32_t> filter(dups, selectedProps.GetResult());
        propertyModel.ForAllItems(filter);
      }
      Playlist::Model::IndexSet toRemove = dups.GetResult();
      std::for_each(SelectedItems.begin(), SelectedItems.end(), boost::bind<Playlist::Model::IndexSet::size_type>(&Playlist::Model::IndexSet::erase, &toRemove, _1));
      stor.RemoveItems(toRemove);
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
  };

  class RemoveDupsInSelectedOperation : public Playlist::Item::StorageModifyOperation
  {
  public:
    explicit RemoveDupsInSelectedOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(Playlist::Item::Storage& stor)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetChecksum);
      DuplicatesCollector<uint32_t> dups;
      propertyModel.ForSpecifiedItems(SelectedItems, dups);
      const Playlist::Model::IndexSet& toRemove = dups.GetResult();
      stor.RemoveItems(toRemove);
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
  };

  class SelectAllRipOffsOperation : public Playlist::Item::StorageAccessOperation
  {
  public:
    SelectAllRipOffsOperation(Playlist::UI::TableView& view)
      : View(view)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      propertyModel.ForAllItems(rips);
      const Playlist::Model::IndexSet& toSelect = rips.GetResult();
      View.SelectItems(toSelect);
    }
  private:
    Playlist::UI::TableView& View;
  };

  class SelectRipOffsOfSelectedOperation : public Playlist::Item::StorageAccessOperation
  {
  public:
    SelectRipOffsOfSelectedOperation(const Playlist::Model::IndexSet& items, Playlist::UI::TableView& view)
      : SelectedItems(items)
      , View(view)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      {
        PropertiesCollector<uint32_t> selectedProps;
        propertyModel.ForSpecifiedItems(SelectedItems, selectedProps);
        PropertiesFilter<uint32_t> filter(rips, selectedProps.GetResult());
        propertyModel.ForAllItems(filter);
      }
      const Playlist::Model::IndexSet toSelect = rips.GetResult();
      View.SelectItems(toSelect);
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
    Playlist::UI::TableView& View;
  };

  class SelectRipOffsInSelectedOperation : public Playlist::Item::StorageAccessOperation
  {
  public:
    SelectRipOffsInSelectedOperation(const Playlist::Model::IndexSet& items, Playlist::UI::TableView& view)
      : SelectedItems(items)
      , View(view)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor)
    {
      const PropertyModel<uint32_t> propertyModel(stor, &Playlist::Item::Data::GetCoreChecksum);
      RipOffsCollector<uint32_t> rips;
      propertyModel.ForSpecifiedItems(SelectedItems, rips);
      const Playlist::Model::IndexSet toSelect = rips.GetResult();
      View.SelectItems(toSelect);
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
    Playlist::UI::TableView& View;
  };

  class CollectClipboardPathsOperation : public Playlist::Item::StorageAccessOperation
  {
  public:
    explicit CollectClipboardPathsOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(items)
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor)
    {
      PathesCollector collector;
      stor.ForSpecifiedItems(SelectedItems, collector);
      const String result = collector.GetResult();
      QApplication::clipboard()->setText(ToQString(result));
    }
  private:
    const Playlist::Model::IndexSet& SelectedItems;
  };

  class CollectStatisticOperation : public Playlist::Item::StorageAccessOperation
  {
  public:
    typedef boost::shared_ptr<CollectStatisticOperation> Ptr;

    CollectStatisticOperation()
      : SelectedItems()
      , Signals(Async::Signals::Dispatcher::Create())
    {
    }

    explicit CollectStatisticOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(&items)
      , Signals(Async::Signals::Dispatcher::Create())
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor)
    {
      Collector.reset(new StatisticCollector());
      if (SelectedItems)
      {
        stor.ForSpecifiedItems(*SelectedItems, *Collector);
      }
      else
      {
        stor.ForAllItems(*Collector);
      }
      Signals->Notify(1);
    }

    StatisticSource::Ptr GetResult() const
    {
      //TODO: use promise/future
      const Async::Signals::Collector::Ptr collector = Signals->CreateCollector(1);
      while (!collector->WaitForSignals(100))
      {
        QCoreApplication::processEvents();
      }
      assert(Collector);
      return Collector;
    }
  private:
    const Playlist::Model::IndexSet* const SelectedItems;
    const Async::Signals::Dispatcher::Ptr Signals;
    boost::shared_ptr<StatisticCollector> Collector;
  };

  class ItemsContextMenuImpl : public Playlist::UI::ItemsContextMenu
  {
  public:
    ItemsContextMenuImpl(Playlist::UI::TableView& view, Playlist::Controller::Ptr playlist)
      : Playlist::UI::ItemsContextMenu(view)
      , View(view)
      , Controller(playlist)
    {
    }

    virtual void Exec(const QPoint& pos)
    {
      View.GetSelectedItems().swap(SelectedItems);
      const std::auto_ptr<QMenu> delegate = CreateMenu();
      delegate->exec(pos);
    }

    virtual void PlaySelected() const
    {
      assert(SelectedItems.size() == 1);
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      iter->Reset(*SelectedItems.begin());
    }

    virtual void RemoveSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->RemoveItems(SelectedItems);
    }

    virtual void CropSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      Playlist::Model::IndexSet unselected;
      for (unsigned idx = 0, total = model->CountItems(); idx < total; ++idx)
      {
        if (!SelectedItems.count(idx))
        {
          unselected.insert(idx);
        }
      }
      model->RemoveItems(unselected);
    }

    virtual void GroupSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const unsigned moveTo = *SelectedItems.begin();
      model->MoveItems(SelectedItems, moveTo);
      Playlist::Model::IndexSet toSelect;
      for (unsigned idx = 0; idx != SelectedItems.size(); ++idx)
      {
        toSelect.insert(moveTo + idx);
      }
      View.SelectItems(toSelect);
    }

    virtual void RemoveAllDuplicates() const
    {
      const Playlist::Item::StorageModifyOperation::Ptr op(new RemoveAllDupsOperation());
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
    }

    virtual void RemoveDuplicatesOfSelected() const
    {
      const Playlist::Item::StorageModifyOperation::Ptr op(new RemoveDupsOfSelectedOperation(SelectedItems));
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
    }

    virtual void RemoveDuplicatesInSelected() const
    {
      const Playlist::Item::StorageModifyOperation::Ptr op(new RemoveDupsInSelectedOperation(SelectedItems));
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
    }

    virtual void SelectAllRipOffs() const
    {
      const Playlist::Item::StorageAccessOperation::Ptr op(new SelectAllRipOffsOperation(View));
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
    }

    virtual void SelectRipOffsOfSelected() const
    {
      const Playlist::Item::StorageAccessOperation::Ptr op(new SelectRipOffsOfSelectedOperation(SelectedItems, View));
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
    }

    virtual void SelectRipOffsInSelected() const
    {
      const Playlist::Item::StorageAccessOperation::Ptr op(new SelectRipOffsInSelectedOperation(SelectedItems, View));
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
    }

    virtual void CopyPathToClipboard() const
    {
      const Playlist::Item::StorageAccessOperation::Ptr op(new CollectClipboardPathsOperation(SelectedItems));
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
    }

    virtual void ShowAllStatistic() const
    {
      const CollectStatisticOperation::Ptr op(new CollectStatisticOperation());
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
      const StatisticSource::Ptr res = op->GetResult();
      ShowStatistic(*res, View);
    }

    virtual void ShowStatisticOfSelected() const
    {
      const CollectStatisticOperation::Ptr op(new CollectStatisticOperation(SelectedItems));
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
      const StatisticSource::Ptr res = op->GetResult();
      ShowStatistic(*res, View);
    }
  private:
    std::auto_ptr<QMenu> CreateMenu()
    {
      switch (const std::size_t items = SelectedItems.size())
      {
      case 0:
        return std::auto_ptr<QMenu>(new NoItemsContextMenu(View, *this));
      case 1:
        return std::auto_ptr<QMenu>(new SingleItemContextMenu(View, *this));
      default:
        return std::auto_ptr<QMenu>(new MultipleItemsContextMenu(View, *this, items));
      }
    }
  private:
    Playlist::UI::TableView& View;
    const Playlist::Controller::Ptr Controller;
    //data
    Playlist::Model::IndexSet SelectedItems;
  };
}

namespace Playlist
{
  namespace UI
  {
    ItemsContextMenu::ItemsContextMenu(QObject& parent) : QObject(&parent)
    {
    }

    ItemsContextMenu* ItemsContextMenu::Create(TableView& view, Playlist::Controller::Ptr playlist)
    {
      return new ItemsContextMenuImpl(view, playlist);
    }
  }
}
