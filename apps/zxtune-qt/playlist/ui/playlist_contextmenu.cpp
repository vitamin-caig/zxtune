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
#include "playlist/supp/operations.h"
#include "playlist/supp/storage.h"
//common includes
#include <format.h>
//library includes
#include <core/module_attrs.h>
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
      receiver.connect(SelSameTypesAction, SIGNAL(triggered()), SLOT(SelectSameTypesOfSelected()));
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
      receiver.connect(SelSameTypesAction, SIGNAL(triggered()), SLOT(SelectSameTypesOfSelected()));
      receiver.connect(CopyToClipboardAction, SIGNAL(triggered()), SLOT(CopyPathToClipboard()));
      receiver.connect(ShowStatisticAction, SIGNAL(triggered()), SLOT(ShowStatisticOfSelected()));
      receiver.connect(ExportAction, SIGNAL(triggered()), SLOT(ExportSelected()));
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
      const Playlist::Item::PromisedSelectionOperation::Ptr op = Playlist::Item::CreateSelectAllDuplicatesOperation();
      const Playlist::Item::SelectionPtr result = GetResult(op);
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->RemoveItems(*result);
    }

    virtual void RemoveDuplicatesOfSelected() const
    {
      const Playlist::Item::PromisedSelectionOperation::Ptr op = Playlist::Item::CreateSelectDuplicatesOfSelectedOperation(SelectedItems);
      const Playlist::Item::SelectionPtr result = GetResult(op);
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->RemoveItems(*result);
    }

    virtual void RemoveDuplicatesInSelected() const
    {
      const Playlist::Item::PromisedSelectionOperation::Ptr op = Playlist::Item::CreateSelectDuplicatesInSelectedOperation(SelectedItems);
      const Playlist::Item::SelectionPtr result = GetResult(op);
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->RemoveItems(*result);
    }

    virtual void SelectAllRipOffs() const
    {
      const Playlist::Item::PromisedSelectionOperation::Ptr op = Playlist::Item::CreateSelectAllRipOffsOperation();
      View.SelectItems(*GetResult(op));
    }

    virtual void SelectRipOffsOfSelected() const
    {
      const Playlist::Item::PromisedSelectionOperation::Ptr op = Playlist::Item::CreateSelectRipOffsOfSelectedOperation(SelectedItems);
      View.SelectItems(*GetResult(op));
    }

    virtual void SelectRipOffsInSelected() const
    {
      const Playlist::Item::PromisedSelectionOperation::Ptr op = Playlist::Item::CreateSelectRipOffsInSelectedOperation(SelectedItems);
      View.SelectItems(*GetResult(op));
    }

    virtual void SelectSameTypesOfSelected() const
    {
      const Playlist::Item::PromisedSelectionOperation::Ptr op = Playlist::Item::CreateSelectTypesOfSelectedOperation(SelectedItems);
      View.SelectItems(*GetResult(op));
    }

    virtual void CopyPathToClipboard() const
    {
      const Playlist::Item::PromisedTextResultOperation::Ptr op = Playlist::Item::CreateCollectPathsOperation(SelectedItems);
      const Playlist::Item::TextOperationResult::Ptr res = GetResult(op);
      //TODO: show msgbox
      QApplication::clipboard()->setText(ToQString(res->GetDetailedResult()));
    }

    virtual void ShowAllStatistic() const
    {
      const Playlist::Item::PromisedTextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation();
      ShowResult(QString::fromUtf8("Statistic:"), GetResult(op));
    }

    virtual void ShowStatisticOfSelected() const
    {
      const Playlist::Item::PromisedTextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation(SelectedItems);
      ShowResult(QString::fromUtf8("Statistic:"), GetResult(op));
    }

    virtual void ExportAll() const
    {
      QString nameTemplate;
      if (Playlist::UI::GetFilenameTemplate(View, nameTemplate))
      {
        const Playlist::Item::PromisedTextResultOperation::Ptr op = Playlist::Item::CreateExportOperation(FromQString(nameTemplate));
        ShowResult(QString::fromUtf8("Export"), GetResult(op));
      }
    }

    virtual void ExportSelected() const
    {
      QString nameTemplate;
      if (Playlist::UI::GetFilenameTemplate(View, nameTemplate))
      {
        const Playlist::Item::PromisedTextResultOperation::Ptr op = Playlist::Item::CreateExportOperation(SelectedItems, FromQString(nameTemplate));
        ShowResult(QString::fromUtf8("Export"), GetResult(op));
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

    void ShowResult(const QString& title, Playlist::Item::TextOperationResult::Ptr res) const
    {
      QMessageBox msgBox(QMessageBox::Information, title, ToQString(res->GetBasicResult()),
        QMessageBox::Ok, &View);
      msgBox.setDetailedText(ToQString(res->GetDetailedResult()));
      msgBox.exec();
    }

    Playlist::Item::SelectionPtr GetResult(Playlist::Item::PromisedSelectionOperation::Ptr op) const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
      return op->GetResult();
    }

    Playlist::Item::TextOperationResult::Ptr GetResult(Playlist::Item::PromisedTextResultOperation::Ptr op) const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->PerformOperation(op);
      return op->GetResult();
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
