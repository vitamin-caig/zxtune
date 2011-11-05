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
#include "filename_template.h"
#include "no_items_contextmenu.ui.h"
#include "single_item_contextmenu.ui.h"
#include "multiple_items_contextmenu.ui.h"
#include "playlist/supp/storage.h"
//common includes
#include <error.h>
#include <format.h>
#include <messages_collector.h>
#include <template.h>
#include <template_parameters.h>
//library includes
#include <async/signals_collector.h>
#include <core/module_attrs.h>
#include <core/convert_parameters.h>
#include <io/fs_tools.h>
//std includes
#include <numeric>
//boost includes
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
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
      receiver.connect(ExportAction, SIGNAL(triggered()), SLOT(ExportAll()));
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
      receiver.connect(ExportAction, SIGNAL(triggered()), SLOT(ExportSelected()));
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
      receiver.connect(ExportAction, SIGNAL(triggered()), SLOT(ExportSelected()));
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

  class OperationResult
  {
  public:
    typedef boost::shared_ptr<const OperationResult> Ptr;
    virtual ~OperationResult() {}

    virtual String GetBasicResult() const = 0;
    virtual String GetDetailedResult() const = 0;
  };

  class StatisticCollector : public Playlist::Model::Visitor
                           , public OperationResult
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

    virtual String GetBasicResult() const
    {
      const String duration = Strings::FormatTime(Duration.Get(), 1000);
      return Strings::Format(Text::BASIC_STATISTIC_TEMPLATE,
        Processed, Invalids,
        duration,
        Size,
        Types.size()
        );
    }

    virtual String GetDetailedResult() const
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

  class ExportVisitor : public Playlist::Model::Visitor
                      , public OperationResult
  {
  public:
    ExportVisitor(const String& nameTemplate, bool overwrite)
      : NameTemplate(StringTemplate::Create(nameTemplate))
      , Overwrite(overwrite)
      , Messages(Log::MessagesCollector::Create())
      , SucceedConvertions(0)
      , FailedConvertions(0)
    {
    }

    virtual void OnItem(Playlist::Model::IndexType /*index*/, Playlist::Item::Data::Ptr data)
    {
      const String path = data->GetFullPath();
      if (ZXTune::Module::Holder::Ptr holder = data->GetModule())
      {
        ExportItem(path, *holder);
      }
      else
      {
        Failed(Strings::Format(Text::CONVERT_FAILED_OPEN, path));
      }
    }

    virtual String GetBasicResult() const
    {
      return Strings::Format(Text::CONVERT_STATUS, SucceedConvertions, FailedConvertions);
    }

    virtual String GetDetailedResult() const
    {
      return Messages->GetMessages('\n');
    }
  private:
    class ModuleFieldsSource : public Parameters::FieldsSourceAdapter<SkipFieldsSource>
    {
    public:
      typedef Parameters::FieldsSourceAdapter<SkipFieldsSource> Parent;
      explicit ModuleFieldsSource(const Parameters::Accessor& params)
        : Parent(params)
      {
      }

      String GetFieldValue(const String& fieldName) const
      {
        return ZXTune::IO::MakePathFromString(Parent::GetFieldValue(fieldName), '_');
      }
    };

    void ExportItem(const String& path, const ZXTune::Module::Holder& item)
    {
      static const ZXTune::Module::Conversion::RawConvertParam RAW_CONVERSION;
      try
      {
        const Parameters::Accessor::Ptr props = item.GetModuleProperties();
        Dump result;
        ThrowIfError(item.Convert(RAW_CONVERSION, props, result));
        const String filename = NameTemplate->Instantiate(ModuleFieldsSource(*props));
        Save(result, filename);
        Succeed(Strings::Format(Text::CONVERT_SUCCEED, path, filename));
      }
      catch (const Error& err)
      {
        Failed(Strings::Format(Text::CONVERT_FAILED, path, err.GetText()));
      }
    }

    void Save(const Dump& data, const String& filename) const
    {
      const std::auto_ptr<std::ofstream> stream = ZXTune::IO::CreateFile(filename, Overwrite);
      stream->write(safe_ptr_cast<const char*>(&data[0]), static_cast<std::streamsize>(data.size() * sizeof(data.front())));
      //TODO: handle possible errors
    }

    void Failed(const String& status)
    {
      ++FailedConvertions;
      Messages->AddMessage(status);
    }

    void Succeed(const String& status)
    {
      ++SucceedConvertions;
      Messages->AddMessage(status);
    }

  private:
    const StringTemplate::Ptr NameTemplate;
    const bool Overwrite;
    const Log::MessagesCollector::Ptr Messages;
    std::size_t SucceedConvertions;
    std::size_t FailedConvertions;
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

  class PromisedResultOperation : public Playlist::Item::StorageAccessOperation
  {
  public:
    typedef boost::shared_ptr<PromisedResultOperation> Ptr;

    virtual OperationResult::Ptr GetResult() const = 0;
  };

  template<class ResultCollector>
  class TemplatePromisedResultOperation : public PromisedResultOperation
  {
  public:
    TemplatePromisedResultOperation()
      : SelectedItems()
      , Signals(Async::Signals::Dispatcher::Create())
    {
    }

    explicit TemplatePromisedResultOperation(const Playlist::Model::IndexSet& items)
      : SelectedItems(&items)
      , Signals(Async::Signals::Dispatcher::Create())
    {
    }

    virtual void Execute(const Playlist::Item::Storage& stor)
    {
      typename boost::shared_ptr<ResultCollector> tmp = CreateCollector();
      if (SelectedItems)
      {
        stor.ForSpecifiedItems(*SelectedItems, *tmp);
      }
      else
      {
        stor.ForAllItems(*tmp);
      }
      Collector.swap(tmp);
      Signals->Notify(1);
    }

    OperationResult::Ptr GetResult() const
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
  protected:
    virtual typename boost::shared_ptr<ResultCollector> CreateCollector() const = 0;
  private:
    const Playlist::Model::IndexSet* const SelectedItems;
    const Async::Signals::Dispatcher::Ptr Signals;
    typename boost::shared_ptr<ResultCollector> Collector;
  };

  class CollectStatisticOperation : public TemplatePromisedResultOperation<StatisticCollector>
  {
    typedef TemplatePromisedResultOperation<StatisticCollector> Parent;
  public:
    CollectStatisticOperation()
      : Parent()
    {
    }

    explicit CollectStatisticOperation(const Playlist::Model::IndexSet& items)
      : Parent(items)
    {
    }
  protected:
    virtual boost::shared_ptr<StatisticCollector> CreateCollector() const
    {
      return boost::make_shared<StatisticCollector>();
    }
  };

  class ExportOperation : public TemplatePromisedResultOperation<ExportVisitor>
  {
    typedef TemplatePromisedResultOperation<ExportVisitor> Parent;
  public:
    explicit ExportOperation(const String& nameTemplate)
      : Parent()
      , NameTemplate(nameTemplate)
    {
    }

    ExportOperation(const Playlist::Model::IndexSet& items, const String& nameTemplate)
      : Parent(items)
      , NameTemplate(nameTemplate)
    {
    }
  protected:
    virtual boost::shared_ptr<ExportVisitor> CreateCollector() const
    {
      return boost::make_shared<ExportVisitor>(NameTemplate, true);
    }
  private:
    const String NameTemplate;
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
      const PromisedResultOperation::Ptr op(new CollectStatisticOperation());
      ShowPromisedResult(QString::fromUtf8("Statistic:"), op);
    }

    virtual void ShowStatisticOfSelected() const
    {
      const PromisedResultOperation::Ptr op(new CollectStatisticOperation(SelectedItems));
      ShowPromisedResult(QString::fromUtf8("Statistic:"), op);
    }

    virtual void ExportAll() const
    {
      QString nameTemplate;
      if (Playlist::UI::GetFilenameTemplate(View, nameTemplate))
      {
        const PromisedResultOperation::Ptr op(new ExportOperation(FromQString(nameTemplate)));
        ShowPromisedResult(QString::fromUtf8("Export"), op);
      }
    }

    virtual void ExportSelected() const
    {
      QString nameTemplate;
      if (Playlist::UI::GetFilenameTemplate(View, nameTemplate))
      {
        const PromisedResultOperation::Ptr op(new ExportOperation(SelectedItems, FromQString(nameTemplate)));
        ShowPromisedResult(QString::fromUtf8("Export"), op);
      }
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

    void ShowPromisedResult(const QString& title, PromisedResultOperation::Ptr op) const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
      const OperationResult::Ptr res = op->GetResult();
      QMessageBox msgBox(QMessageBox::Information, title, ToQString(res->GetBasicResult()),
        QMessageBox::Ok, &View);
      msgBox.setDetailedText(ToQString(res->GetDetailedResult()));
      msgBox.exec();
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
