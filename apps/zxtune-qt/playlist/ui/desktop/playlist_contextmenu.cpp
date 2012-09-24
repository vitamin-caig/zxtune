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
#include "properties_dialog.h"
#include "search_dialog.h"
#include "ui/utils.h"
#include "ui/conversion/filename_template.h"
#include "ui/conversion/setup_conversion.h"
#include "no_items_contextmenu.ui.h"
#include "single_item_contextmenu.ui.h"
#include "multiple_items_contextmenu.ui.h"
#include "playlist/supp/operations.h"
#include "playlist/supp/storage.h"
#include "playlist/ui/contextmenu.h"
#include "playlist/ui/table_view.h"
#include "supp/options.h"
//common includes
#include <contract.h>
#include <error.h>
#include <format.h>
//library includes
#include <core/module_attrs.h>
#include <sound/backends_parameters.h>
//boost includes
#include <boost/make_shared.hpp>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>

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

      Require(receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveAllDuplicates())));
      Require(receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectAllRipOffs())));
      Require(receiver.connect(SelFoundAction, SIGNAL(triggered()), SLOT(SelectFound())));
      Require(receiver.connect(ShowStatisticAction, SIGNAL(triggered()), SLOT(ShowAllStatistic())));
      Require(receiver.connect(ExportAction, SIGNAL(triggered()), SLOT(ExportAll())));
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

      Require(receiver.connect(PlayAction, SIGNAL(triggered()), SLOT(PlaySelected())));
      Require(receiver.connect(DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected())));
      Require(receiver.connect(CropAction, SIGNAL(triggered()), SLOT(CropSelected())));
      Require(receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicatesOfSelected())));
      Require(receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffsOfSelected())));
      Require(receiver.connect(SelSameTypesAction, SIGNAL(triggered()), SLOT(SelectSameTypesOfSelected())));
      Require(receiver.connect(CopyToClipboardAction, SIGNAL(triggered()), SLOT(CopyPathToClipboard())));
      Require(receiver.connect(ExportAction, SIGNAL(triggered()), SLOT(ExportSelected())));
      Require(receiver.connect(ConvertAction, SIGNAL(triggered()), SLOT(ConvertSelected())));
      Require(receiver.connect(PropertiesAction, SIGNAL(triggered()), SLOT(ShowPropertiesOfSelected())));
    }
  };

  QString ModulesCount(uint_t count)
  {
    return Playlist::UI::ItemsContextMenu::tr("%n item(s)", 0, static_cast<int>(count));
  }

  class MultipleItemsContextMenu : public QMenu
                                 , private Ui::MultipleItemsContextMenu
  {
  public:
    MultipleItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver, std::size_t count)
      : QMenu(&parent)
    {
      //setup self
      setupUi(this);
      InfoAction->setText(ModulesCount(count));

      Require(receiver.connect(DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected())));
      Require(receiver.connect(CropAction, SIGNAL(triggered()), SLOT(CropSelected())));
      Require(receiver.connect(GroupAction, SIGNAL(triggered()), SLOT(GroupSelected())));
      Require(receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicatesInSelected())));
      Require(receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffsInSelected())));
      Require(receiver.connect(SelSameTypesAction, SIGNAL(triggered()), SLOT(SelectSameTypesOfSelected())));
      Require(receiver.connect(SelFoundAction, SIGNAL(triggered()), SLOT(SelectFoundInSelected())));
      Require(receiver.connect(CopyToClipboardAction, SIGNAL(triggered()), SLOT(CopyPathToClipboard())));
      Require(receiver.connect(ShowStatisticAction, SIGNAL(triggered()), SLOT(ShowStatisticOfSelected())));
      Require(receiver.connect(ExportAction, SIGNAL(triggered()), SLOT(ExportSelected())));
      Require(receiver.connect(ConvertAction, SIGNAL(triggered()), SLOT(ConvertSelected())));
    }
  };

  static const QLatin1String LINE_BREAK("\n");

  class StatisticNotification : public Playlist::Item::StatisticTextNotification
  {
  public:
    StatisticNotification()
      : Processed()
      , Invalids()
      , Duration()
      , Size()
    {
      Duration.SetPeriod(Time::Milliseconds(1));
    }

    virtual QString Category() const
    {
      return Playlist::UI::ItemsContextMenu::tr("Statistic");
    }

    virtual QString Text() const
    {
      QStringList result;
      result.append(Playlist::UI::ItemsContextMenu::tr("Total: %1").arg(ModulesCount(Processed)));
      result.append(Playlist::UI::ItemsContextMenu::tr("Invalid: %1").arg(ModulesCount(Invalids)));
      result.append(Playlist::UI::ItemsContextMenu::tr("Total duration: %1").arg(ToQString(Duration.ToString())));
      result.append(Playlist::UI::ItemsContextMenu::tr("Total size: %n byte(s)", 0, Size));
      result.append(Playlist::UI::ItemsContextMenu::tr("%n diferent modules' type(s)", 0, Types.size()));
      return result.join(LINE_BREAK);
    }

    virtual QString Details() const
    {
      QStringList result;
      for (std::map<String, std::size_t>::const_iterator it = Types.begin(), lim = Types.end(); it != lim; ++it)
      {
        result.append(QString::fromAscii("%1: %2")
          .arg(ToQString(it->first)).arg(ModulesCount(it->second)));
      }
      return result.join(LINE_BREAK);
    }

    virtual void AddInvalid()
    {
      ++Invalids;
    }

    virtual void AddValid(const String& type, const Time::MillisecondsDuration& duration, std::size_t size)
    {
      ++Processed;
      Duration += duration;
      Size += size;
      ++Types[type];
    }
  private:
    std::size_t Processed;
    std::size_t Invalids;
    Time::Duration<uint64_t, Time::Milliseconds> Duration;
    uint64_t Size;
    std::map<String, std::size_t> Types;
  };

  Playlist::Item::StatisticTextNotification::Ptr CreateStatisticNotification()
  {
    return boost::make_shared<StatisticNotification>();
  }

  class ExportResult : public Playlist::Item::ConversionResultNotification
  {
  public:
    ExportResult()
      : Succeeds()
    {
    }

    virtual QString Category() const
    {
      return Playlist::UI::ItemsContextMenu::tr("Export");
    }

    virtual QString Text() const
    {
      return Playlist::UI::ItemsContextMenu::tr("Converted: %1<br/>Failed: %2")
        .arg(ModulesCount(Succeeds)).arg(ModulesCount(Errors.size()));
    }

    virtual QString Details() const
    {
      return Errors.join(LINE_BREAK);
    }

    virtual void AddSucceed()
    {
      ++Succeeds;
    }

    virtual void AddFailedToOpen(const String& path)
    {
      Errors.append(Playlist::UI::ItemsContextMenu::tr("Failed to open '%1' for conversion").arg(ToQString(path)));
    }

    virtual void AddFailedToConvert(const String& path, const Error& err)
    {
      Errors.append(Playlist::UI::ItemsContextMenu::tr("Failed to convert '%1': %2")
        .arg(ToQString(path)).arg(ToQString(Error::ToString(err))));
    }
  private:
    std::size_t Succeeds;
    QStringList Errors;
  };

  Playlist::Item::ConversionResultNotification::Ptr CreateConversionResultNotification()
  {
    return boost::make_shared<ExportResult>();
  }

  class ItemsContextMenuImpl : public Playlist::UI::ItemsContextMenu
  {
  public:
    ItemsContextMenuImpl(Playlist::UI::TableView& view, Playlist::Controller& playlist)
      : Playlist::UI::ItemsContextMenu(view)
      , View(view)
      , Controller(playlist)
    {
    }

    void Exec(const QPoint& pos)
    {
      SelectedItems = View.GetSelectedItems();
      const std::auto_ptr<QMenu> delegate = CreateMenu();
      delegate->exec(pos);
    }

    virtual void PlaySelected() const
    {
      assert(SelectedItems->size() == 1);
      const Playlist::Item::Iterator::Ptr iter = Controller.GetIterator();
      iter->Reset(*SelectedItems->begin());
    }

    virtual void RemoveSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      model->RemoveItems(*SelectedItems);
    }

    virtual void CropSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      Playlist::Model::IndexSet unselected;
      for (unsigned idx = 0, total = model->CountItems(); idx < total; ++idx)
      {
        if (!SelectedItems->count(idx))
        {
          unselected.insert(idx);
        }
      }
      model->RemoveItems(unselected);
    }

    virtual void GroupSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const unsigned moveTo = *SelectedItems->begin();
      model->MoveItems(*SelectedItems, moveTo);
      Playlist::Model::IndexSet toSelect;
      for (unsigned idx = 0, lim = SelectedItems->size(); idx != lim; ++idx)
      {
        toSelect.insert(moveTo + idx);
      }
      View.SelectItems(toSelect);
    }

    virtual void RemoveAllDuplicates() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectAllDuplicatesOperation(*model);
      Require(model->connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void RemoveDuplicatesOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectDuplicatesOfSelectedOperation(*model, SelectedItems);
      Require(model->connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void RemoveDuplicatesInSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectDuplicatesInSelectedOperation(*model, SelectedItems);
      Require(model->connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void SelectAllRipOffs() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectAllRipOffsOperation(*model);
      Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void SelectRipOffsOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectRipOffsOfSelectedOperation(*model, SelectedItems);
      Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void SelectRipOffsInSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectRipOffsInSelectedOperation(*model, SelectedItems);
      Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void SelectSameTypesOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectTypesOfSelectedOperation(*model, SelectedItems);
      Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void CopyPathToClipboard() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const QStringList paths =  model->GetItemsPaths(*SelectedItems);
      QApplication::clipboard()->setText(paths.join(LINE_BREAK));
    }

    virtual void ShowAllStatistic() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::StatisticTextNotification::Ptr result = CreateStatisticNotification();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation(*model, result);
      Require(Controller.connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
        SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
      model->PerformOperation(op);
    }

    virtual void ShowStatisticOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller.GetModel();
      const Playlist::Item::StatisticTextNotification::Ptr result = CreateStatisticNotification();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation(*model, SelectedItems, result);
      Require(Controller.connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
        SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
      model->PerformOperation(op);
    }

    virtual void ExportAll() const
    {
      QString nameTemplate;
      if (UI::GetFilenameTemplate(View, nameTemplate))
      {
        const Parameters::Accessor::Ptr allParams = GlobalOptions::Instance().Get();
        const Playlist::Model::Ptr model = Controller.GetModel();
        const Playlist::Item::ConversionResultNotification::Ptr result = CreateConversionResultNotification();
        const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateExportOperation(*model, FromQString(nameTemplate), allParams, result);
        Require(Controller.connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
          SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
        model->PerformOperation(op);
      }
    }

    virtual void ExportSelected() const
    {
      QString nameTemplate;
      if (UI::GetFilenameTemplate(View, nameTemplate))
      {
        const Parameters::Accessor::Ptr allParams = GlobalOptions::Instance().Get();
        const Playlist::Model::Ptr model = Controller.GetModel();
        const Playlist::Item::ConversionResultNotification::Ptr result = CreateConversionResultNotification();
        const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateExportOperation(*model, SelectedItems, FromQString(nameTemplate), allParams, result);
        Require(Controller.connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
          SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
        model->PerformOperation(op);
      }
    }

    virtual void ConvertSelected() const
    {
      String type;
      if (Parameters::Accessor::Ptr params = UI::GetConversionParameters(View, type))
      {
        //TODO: copy parameters
        const Parameters::Accessor::Ptr allParams = Parameters::CreateMergedAccessor(params, GlobalOptions::Instance().Get());
        const Playlist::Model::Ptr model = Controller.GetModel();
        const Playlist::Item::ConversionResultNotification::Ptr result = CreateConversionResultNotification();
        const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateConvertOperation(*model, SelectedItems, type, allParams, result);
        Require(Controller.connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
          SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
        model->PerformOperation(op);
      }
    }

    virtual void SelectFound() const
    {
      Playlist::UI::ExecuteSearchDialog(View, Controller);
    }

    virtual void SelectFoundInSelected() const
    {
      Playlist::UI::ExecuteSearchDialog(View, SelectedItems, Controller);
    }

    virtual void ShowPropertiesOfSelected() const
    {
      Playlist::UI::ExecutePropertiesDialog(View, SelectedItems, Controller);
    }
  private:
    std::auto_ptr<QMenu> CreateMenu()
    {
      switch (const std::size_t items = SelectedItems->size())
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
    Playlist::Controller& Controller;
    //data
    Playlist::Model::IndexSetPtr SelectedItems;
  };
}

namespace Playlist
{
  namespace UI
  {
    ItemsContextMenu::ItemsContextMenu(QObject& parent) : QObject(&parent)
    {
    }

    void ExecuteContextMenu(const QPoint& pos, TableView& view, Controller& playlist)
    {
      ItemsContextMenuImpl menu(view, playlist);
      menu.Exec(pos);
    }
  }
}
