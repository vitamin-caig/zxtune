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
#include "search_dialog.h"
#include "table_view.h"
#include "ui/utils.h"
#include "ui/conversion/filename_template.h"
#include "ui/conversion/setup_conversion.h"
#include "no_items_contextmenu.ui.h"
#include "single_item_contextmenu.ui.h"
#include "multiple_items_contextmenu.ui.h"
#include "playlist/supp/operations.h"
#include "playlist/supp/storage.h"
//common includes
#include <contract.h>
#include <format.h>
//library includes
#include <core/module_attrs.h>
#include <sound/backends_parameters.h>
//qt includes
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
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
      SelectedItems = View.GetSelectedItems();
      const std::auto_ptr<QMenu> delegate = CreateMenu();
      delegate->exec(pos);
    }

    virtual void PlaySelected() const
    {
      assert(SelectedItems->size() == 1);
      const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
      iter->Reset(*SelectedItems->begin());
    }

    virtual void RemoveSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->RemoveItems(*SelectedItems);
    }

    virtual void CropSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
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
      const Playlist::Model::Ptr model = Controller->GetModel();
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
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectAllDuplicatesOperation(*model);
      Require(model->connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void RemoveDuplicatesOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectDuplicatesOfSelectedOperation(*model, SelectedItems);
      Require(model->connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void RemoveDuplicatesInSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectDuplicatesInSelectedOperation(*model, SelectedItems);
      Require(model->connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void SelectAllRipOffs() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectAllRipOffsOperation(*model);
      Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void SelectRipOffsOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectRipOffsOfSelectedOperation(*model, SelectedItems);
      Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void SelectRipOffsInSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectRipOffsInSelectedOperation(*model, SelectedItems);
      Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void SelectSameTypesOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectTypesOfSelectedOperation(*model, SelectedItems);
      Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
      model->PerformOperation(op);
    }

    virtual void CopyPathToClipboard() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const QStringList paths =  model->GetItemsPaths(*SelectedItems);
      QApplication::clipboard()->setText(paths.join(QString('\n')));
    }

    virtual void ShowAllStatistic() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation(*model);
      Require(Controller->connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
        SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
      model->PerformOperation(op);
    }

    virtual void ShowStatisticOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation(*model, SelectedItems);
      Require(Controller->connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
        SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
      model->PerformOperation(op);
    }

    virtual void ExportAll() const
    {
      QString nameTemplate;
      if (UI::GetFilenameTemplate(View, nameTemplate))
      {
        const Playlist::Model::Ptr model = Controller->GetModel();
        const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateExportOperation(*model, FromQString(nameTemplate));
        Require(Controller->connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
          SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
        model->PerformOperation(op);
      }
    }

    virtual void ExportSelected() const
    {
      QString nameTemplate;
      if (UI::GetFilenameTemplate(View, nameTemplate))
      {
        const Playlist::Model::Ptr model = Controller->GetModel();
        const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateExportOperation(*model, SelectedItems, FromQString(nameTemplate));
        Require(Controller->connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
          SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
        model->PerformOperation(op);
      }
    }

    virtual void ConvertSelected() const
    {
      String type;
      if (Parameters::Accessor::Ptr params = UI::GetConversionParameters(View, type))
      {
        const Playlist::Model::Ptr model = Controller->GetModel();
        const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateConvertOperation(*model, SelectedItems, type, params);
        Require(Controller->connect(op.get(), SIGNAL(ResultAcquired(Playlist::TextNotification::Ptr)),
          SLOT(ShowNotification(Playlist::TextNotification::Ptr))));
        model->PerformOperation(op);
      }
    }

    virtual void SelectFound() const
    {
      Playlist::Item::Search::Data data;
      if (Playlist::UI::GetSearchParameters(View, data))
      {
        const Playlist::Model::Ptr model = Controller->GetModel();
        const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSearchOperation(*model, data);
        Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
        model->PerformOperation(op);
      }
    }

    virtual void SelectFoundInSelected() const
    {
      Playlist::Item::Search::Data data;
      if (Playlist::UI::GetSearchParameters(View, data))
      {
        const Playlist::Model::Ptr model = Controller->GetModel();
        const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSearchOperation(*model, SelectedItems, data);
        Require(View.connect(op.get(), SIGNAL(ResultAcquired(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr))));
        model->PerformOperation(op);
      }
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
    const Playlist::Controller::Ptr Controller;
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

    ItemsContextMenu::Ptr ItemsContextMenu::Create(TableView& view, Playlist::Controller::Ptr playlist)
    {
      return ItemsContextMenu::Ptr(new ItemsContextMenuImpl(view, playlist));
    }
  }
}
