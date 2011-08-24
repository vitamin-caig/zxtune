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
//common includes
#include <format.h>
//library includes
#include <core/module_attrs.h>
//text includes
#include "text/text.h"

namespace
{
  bool GetParam(const Parameters::Accessor& acc, const Parameters::NameType& propName, Parameters::IntType& val)
  {
    return acc.FindIntValue(propName, val);
  }

  bool GetParam(const Parameters::Accessor& acc, const Parameters::NameType& propName, Parameters::StringType& val)
  {
    return acc.FindStringValue(propName, val);
  }

  template<class T>
  void SelectItemsProperties(Playlist::Item::Data::Iterator::Ptr items, QSet<T>& res, const Parameters::NameType& propName)
  {
    res.clear();
    for (; items->IsValid(); items->Next())
    {
      const Playlist::Item::Data::Ptr item = items->Get();
      const ZXTune::Module::Holder::Ptr holder = item->GetModule();
      if (!holder)
      {
        continue;
      }
      const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
      T val = T();
      if (GetParam(*props, propName, val))
      {
        res.insert(val);
      }
    }
  }

  template<class T>
  class UniqueItemsFilter : public Playlist::Item::Filter
  {
  public:
    UniqueItemsFilter(const QSet<T>& props, const Parameters::NameType& propName)
      : Props(props)
      , PropName(propName)
    {
    }

    virtual bool OnItem(const Playlist::Item::Data& data) const
    {
      if (!data.IsValid())
      {
        return false;
      }
      const ZXTune::Module::Holder::Ptr holder = data.GetModule();
      const Parameters::Accessor::Ptr props = holder->GetModuleProperties();
      T val = T();
      if (!GetParam(*props, PropName, val) ||
          !Props.contains(val))
      {
        return false;
      }
      if (Visited.contains(val))
      {
        return true;
      }
      Visited.insert(val);
      return false;
    }
  private:
    const QSet<T>& Props;
    const Parameters::NameType PropName;
    mutable QSet<T> Visited;
  };

  class ItemsContextMenuImpl : public Playlist::UI::ItemsContextMenu
  {
  public:
    ItemsContextMenuImpl(Playlist::UI::TableView& view, Playlist::Controller::Ptr playlist)
      : Playlist::UI::ItemsContextMenu(view)
      , View(view)
      , Controller(playlist)
      , InfoAction(this)
      , PlayAction(QIcon(":/playback/play.png"), tr("Play"), this)
      , DeleteAction(QIcon(":/playlist/delete.png"), tr("Delete"), this)
      , CropAction(QIcon(":/playlist/crop.png"), tr("Crop"), this)
      , GroupAction(tr("Group together"), this)
      , OrganizeSubmenu(tr("Organize"))
      , DelDupsAction(tr("Remove duplicates"), &OrganizeSubmenu)
    {
      AddActions();

      //common
      InfoAction.setEnabled(false);
      this->connect(&PlayAction, SIGNAL(triggered()), SLOT(PlaySelected()));
      this->connect(&DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      this->connect(&CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
      this->connect(&GroupAction, SIGNAL(triggered()), SLOT(GroupSelected()));
      //organize
      this->connect(&DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicates()));
    }

    virtual void Exec(const QPoint& pos)
    {
      View.GetSelectedItems().swap(SelectedItems);
      if (SelectedItems.empty())
      {
        return;
      }
      SetupMenu();
      exec(pos);
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
      model->MoveItems(SelectedItems, *SelectedItems.begin());
    }

    virtual void RemoveDuplicates() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems(SelectedItems);
      QSet<Parameters::IntType> checksumms;
      SelectItemsProperties(items, checksumms, ZXTune::Module::ATTR_CRC);
      const UniqueItemsFilter<Parameters::IntType> filter(checksumms, ZXTune::Module::ATTR_CRC);
      const Playlist::Model::IndexSet toRemove = model->GetItemIndices(filter);
      model->RemoveItems(toRemove);
    }
  private:
    void AddActions()
    {
      addAction(&InfoAction);
      addAction(&PlayAction);
      addAction(&DeleteAction);
      addAction(&CropAction);
      addAction(&GroupAction);
      addMenu(&OrganizeSubmenu);
      OrganizeSubmenu.addAction(&DelDupsAction);
    }

    void SetupMenu()
    {
      const int items = SelectedItems.size();
      if (1 == items)
      {
        SetupSinglePlayitemMenu();
      }
      else
      {
        SetupMultiplePlayitemsMenu(items);
      }
    }

    void SetupSinglePlayitemMenu()
    {
      InfoAction.setVisible(false);
      PlayAction.setEnabled(true);
      GroupAction.setEnabled(false);
      SetupPlayitemMenu();
    }

    void SetupMultiplePlayitemsMenu(int count)
    {
      InfoAction.setVisible(true);
      InfoAction.setText(ToQString(Strings::Format(Text::CONTEXTMENU_STATUS, count)));
      PlayAction.setEnabled(false);
      GroupAction.setEnabled(true);
      SetupPlayitemMenu();
    }

    void SetupPlayitemMenu()
    {
      DeleteAction.setEnabled(true);
      CropAction.setEnabled(true);
    }
  private:
    Playlist::UI::TableView& View;
    const Playlist::Controller::Ptr Controller;
    QAction InfoAction;
    QAction PlayAction;
    QAction DeleteAction;
    QAction CropAction;
    QAction GroupAction;
    QMenu OrganizeSubmenu;
    QAction DelDupsAction;
    //data
    Playlist::Model::IndexSet SelectedItems;
  };
}

namespace Playlist
{
  namespace UI
  {
    ItemsContextMenu::ItemsContextMenu(QWidget& parent) : QMenu(&parent)
    {
    }

    ItemsContextMenu* ItemsContextMenu::Create(TableView& view, Playlist::Controller::Ptr playlist)
    {
      return new ItemsContextMenuImpl(view, playlist);
    }
  }
}
