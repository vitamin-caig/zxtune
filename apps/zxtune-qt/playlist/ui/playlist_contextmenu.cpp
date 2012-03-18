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
#include "ui/conversion/filename_template.h"
#include "no_items_contextmenu.ui.h"
#include "single_item_contextmenu.ui.h"
#include "multiple_items_contextmenu.ui.h"
#include "playlist/supp/operations.h"
#include "playlist/supp/storage.h"
//common includes
#include <format.h>
//library includes
#include <core/module_attrs.h>
#include <sound/backends_parameters.h>
//qt includes
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
      receiver.connect(ConvertAction, SIGNAL(triggered()), SLOT(ConvertSelected()));
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
      receiver.connect(ConvertAction, SIGNAL(triggered()), SLOT(ConvertSelected()));
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
      model->connect(op.get(), SIGNAL(OnResult(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr)));
      model->PerformOperation(op);
    }

    virtual void RemoveDuplicatesOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectDuplicatesOfSelectedOperation(*model, SelectedItems);
      model->connect(op.get(), SIGNAL(OnResult(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr)));
      model->PerformOperation(op);
    }

    virtual void RemoveDuplicatesInSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectDuplicatesInSelectedOperation(*model, SelectedItems);
      model->connect(op.get(), SIGNAL(OnResult(Playlist::Model::IndexSetPtr)), SLOT(RemoveItems(Playlist::Model::IndexSetPtr)));
      model->PerformOperation(op);
    }

    virtual void SelectAllRipOffs() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectAllRipOffsOperation(*model);
      View.connect(op.get(), SIGNAL(OnResult(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr)));
      model->PerformOperation(op);
    }

    virtual void SelectRipOffsOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectRipOffsOfSelectedOperation(*model, SelectedItems);
      View.connect(op.get(), SIGNAL(OnResult(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr)));
      model->PerformOperation(op);
    }

    virtual void SelectRipOffsInSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectRipOffsInSelectedOperation(*model, SelectedItems);
      View.connect(op.get(), SIGNAL(OnResult(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr)));
      model->PerformOperation(op);
    }

    virtual void SelectSameTypesOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::SelectionOperation::Ptr op = Playlist::Item::CreateSelectTypesOfSelectedOperation(*model, SelectedItems);
      View.connect(op.get(), SIGNAL(OnResult(Playlist::Model::IndexSetPtr)), SLOT(SelectItems(Playlist::Model::IndexSetPtr)));
      model->PerformOperation(op);
    }

    virtual void CopyPathToClipboard() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateCollectPathsOperation(*model, SelectedItems);
      Controller->connect(op.get(), SIGNAL(OnResult(Playlist::TextNotification::Ptr)), SLOT(CopyDetailToClipboard(Playlist::TextNotification::Ptr)));
      model->PerformOperation(op);
    }

    virtual void ShowAllStatistic() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation(*model);
      Controller->connect(op.get(), SIGNAL(OnResult(Playlist::TextNotification::Ptr)), SLOT(ShowNotification(Playlist::TextNotification::Ptr)));
      model->PerformOperation(op);
    }

    virtual void ShowStatisticOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateCollectStatisticOperation(*model, SelectedItems);
      Controller->connect(op.get(), SIGNAL(OnResult(Playlist::TextNotification::Ptr)), SLOT(ShowNotification(Playlist::TextNotification::Ptr)));
      model->PerformOperation(op);
    }

    virtual void ExportAll() const
    {
      QString nameTemplate;
      if (UI::GetFilenameTemplate(View, nameTemplate))
      {
        const Playlist::Model::Ptr model = Controller->GetModel();
        const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateExportOperation(*model, FromQString(nameTemplate));
        Controller->connect(op.get(), SIGNAL(OnResult(Playlist::TextNotification::Ptr)), SLOT(ShowNotification(Playlist::TextNotification::Ptr)));
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
        Controller->connect(op.get(), SIGNAL(OnResult(Playlist::TextNotification::Ptr)), SLOT(ShowNotification(Playlist::TextNotification::Ptr)));
        model->PerformOperation(op);
      }
    }

    virtual void ConvertSelected() const
    {
      static const Char TYPE[] = {'w', 'a', 'v', '\0'};
      QString nameTemplate;
      if (UI::GetFilenameTemplate(View, nameTemplate))
      {
        const Parameters::Container::Ptr params = Parameters::Container::Create();
        params->SetStringValue(Parameters::ZXTune::Sound::Backends::File::FILENAME, FromQString(nameTemplate));
        params->SetIntValue(Parameters::ZXTune::Sound::Backends::File::OVERWRITE, 1/*TODO*/);
        const Playlist::Model::Ptr model = Controller->GetModel();
        const Playlist::Item::TextResultOperation::Ptr op = Playlist::Item::CreateConvertOperation(*model, SelectedItems, TYPE, params);
        Controller->connect(op.get(), SIGNAL(OnResult(Playlist::TextNotification::Ptr)), SLOT(ShowNotification(Playlist::TextNotification::Ptr)));
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

    ItemsContextMenu* ItemsContextMenu::Create(TableView& view, Playlist::Controller::Ptr playlist)
    {
      return new ItemsContextMenuImpl(view, playlist);
    }
  }
}
