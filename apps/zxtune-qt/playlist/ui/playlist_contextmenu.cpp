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
  template<class T>
  void SelectItemsProperties(Playlist::Item::Data::Iterator::Ptr items, std::set<T>& result, T (Playlist::Item::Data::*getter)() const)
  {
    std::set<T> res;
    for (; items->IsValid(); items->Next())
    {
      const Playlist::Item::Data::Ptr item = items->Get();
      const T val = ((*item).*getter)();
      if (item->IsValid())
      {
        res.insert(val);
      }
    }
    res.swap(result);
  }

  template<class T>
  class SimilarItemsFilter : public Playlist::Item::Filter
  {
  public:
    SimilarItemsFilter(const std::set<T>& props, T (Playlist::Item::Data::*getter)() const)
      : Props(props)
      , Getter(getter)
    {
    }

    virtual bool OnItem(const Playlist::Item::Data& data) const
    {
      const T val = (data.*Getter)();
      if (!data.IsValid())
      {
        return false;
      }
      return 0 != Props.count(val);
    }
  private:
    const std::set<T>& Props;
    typedef T (Playlist::Item::Data::*GetFunc)() const;
    const GetFunc Getter;
  };

  template<class T>
  class DuplicatedItemsFilter : public Playlist::Item::Filter
  {
  public:
    DuplicatedItemsFilter(const std::set<T>& props, T (Playlist::Item::Data::*getter)() const)
      : Props(props)
      , Getter(getter)
    {
    }

    virtual bool OnItem(const Playlist::Item::Data& data) const
    {
      const T val = (data.*Getter)();
      if (!data.IsValid())
      {
        return false;
      }
      if (!Props.count(val))
      {
        return false;
      }
      if (Visited.count(val))
      {
        return true;
      }
      Visited.insert(val);
      return false;
    }
  private:
    const std::set<T>& Props;
    typedef T (Playlist::Item::Data::*GetFunc)() const;
    const GetFunc Getter;
    mutable std::set<T> Visited;
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
      , SelRipOffsAction(tr("Select rip-offs"), &OrganizeSubmenu)
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
      this->connect(&SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffs()));
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
      const unsigned moveTo = *SelectedItems.begin();
      model->MoveItems(SelectedItems, moveTo);
      Playlist::Model::IndexSet toSelect;
      for (unsigned idx = 0; idx != SelectedItems.size(); ++idx)
      {
        toSelect.insert(moveTo + idx);
      }
      View.SelectItems(toSelect);
    }

    virtual void RemoveDuplicates() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems(SelectedItems);
      std::set<uint32_t> checksumms;
      SelectItemsProperties(items, checksumms, &Playlist::Item::Data::GetChecksum);
      const DuplicatedItemsFilter<uint32_t> filter(checksumms, &Playlist::Item::Data::GetChecksum);
      const Playlist::Model::IndexSet toRemove = model->GetItemIndices(filter);
      model->RemoveItems(toRemove);
    }

    virtual void SelectRipOffs() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems(SelectedItems);
      std::set<uint32_t> checksumms;
      SelectItemsProperties(items, checksumms, &Playlist::Item::Data::GetCoreChecksum);
      const SimilarItemsFilter<uint32_t> filter(checksumms, &Playlist::Item::Data::GetCoreChecksum);
      const Playlist::Model::IndexSet toSelect = model->GetItemIndices(filter);
      View.SelectItems(toSelect);
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
      OrganizeSubmenu.addAction(&SelRipOffsAction);
    }

    void SetupMenu()
    {
      const std::size_t items = SelectedItems.size();
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
    QAction SelRipOffsAction;
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
