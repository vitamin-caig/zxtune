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
//qt includes
#include <QtGui/QMenu>
//text includes
#include "text/text.h"

namespace
{
  //nofiles contextmenu
  // DeleteAllDups
  // SelectAllRipOffs
  class NoItemsContextMenu : public QMenu
  {
  public:
    NoItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver)
      : QMenu(&parent)
      , DelDupsAction(addAction(tr("Remove all duplicates")))
      , SelRipOffsAction(addAction(tr("Select all rip-offs")))
    {
      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveAllDuplicates()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectAllRipOffs()));
    }
  private:
    QAction* const DelDupsAction;
    QAction* const SelRipOffsAction;
  };

  //single item contextmenu
  // Play
  // Delete
  // Crop
  // DeleteDupsOf
  // SelectRipOffsOf
  class SingleItemContextMenu : public QMenu
  {
  public:
    SingleItemContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver)
      : QMenu(&parent)
      , PlayAction(addAction(QIcon(":/playback/play.png"), tr("Play")))
      , DeleteAction(addAction(QIcon(":/playlist/delete.png"), tr("Delete")))
      , CropAction(addAction(QIcon(":/playlist/crop.png"), tr("Crop")))
      , DelDupsAction(addAction(tr("Remove duplicates of")))
      , SelRipOffsAction(addAction(tr("Select rip-offs of")))
    {
      insertSeparator(DelDupsAction);
      receiver.connect(PlayAction, SIGNAL(triggered()), SLOT(PlaySelected()));
      receiver.connect(DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      receiver.connect(CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicatesOfSelected()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffsOfSelected()));
    }
  private:
    QAction* const PlayAction;
    QAction* const DeleteAction;
    QAction* const CropAction;
    QAction* const DelDupsAction;
    QAction* const SelRipOffsAction;
  };

  //multiple items contextmenu
  // <count>
  // Delete
  // Crop
  // Group
  // DeleteDupsIn
  // SelectRipOffsIn
  class MultipleItemsContextMenu : public QMenu
  {
  public:
    MultipleItemsContextMenu(QWidget& parent, Playlist::UI::ItemsContextMenu& receiver, std::size_t count)
      : QMenu(&parent)
      , InfoAction(addAction(ToQString(Strings::Format(Text::CONTEXTMENU_STATUS, count))))
      , DeleteAction(addAction(QIcon(":/playlist/delete.png"), tr("Delete")))
      , CropAction(addAction(QIcon(":/playlist/crop.png"), tr("Crop")))
      , GroupAction(addAction(tr("Group together")))
      , DelDupsAction(addAction(tr("Remove duplicates in")))
      , SelRipOffsAction(addAction(tr("Select rip-offs in")))
    {
      InfoAction->setEnabled(false);
      insertSeparator(DelDupsAction);
      receiver.connect(DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      receiver.connect(CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
      receiver.connect(GroupAction, SIGNAL(triggered()), SLOT(GroupSelected()));
      receiver.connect(DelDupsAction, SIGNAL(triggered()), SLOT(RemoveDuplicatesInSelected()));
      receiver.connect(SelRipOffsAction, SIGNAL(triggered()), SLOT(SelectRipOffsInSelected()));
    }
  private:
    QAction* const InfoAction;
    QAction* const DeleteAction;
    QAction* const CropAction;
    QAction* const GroupAction;
    QAction* const DelDupsAction;
    QAction* const SelRipOffsAction;
  };

  template<class T>
  void SelectItemsProperties(Playlist::Item::Data::Iterator::Ptr items, std::map<T, std::size_t>& result, T (Playlist::Item::Data::*getter)() const)
  {
    std::map<T, std::size_t> res;
    for (; items->IsValid(); items->Next())
    {
      const Playlist::Item::Data::Ptr item = items->Get();
      const T val = ((*item).*getter)();
      if (item->IsValid())
      {
        ++res[val];
      }
    }
    res.swap(result);
  }

  template<class T>
  class DuplicatedItemsFilter : public Playlist::Item::Filter
  {
  public:
    DuplicatedItemsFilter(const std::map<T, std::size_t>& props, T (Playlist::Item::Data::*getter)() const)
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
    const std::map<T, std::size_t>& Props;
    typedef T (Playlist::Item::Data::*GetFunc)() const;
    const GetFunc Getter;
    mutable std::set<T> Visited;
  };

  template<class T>
  class SimilarItemsFilter : public Playlist::Item::Filter
  {
  public:
    SimilarItemsFilter(const std::map<T, std::size_t>& props, T (Playlist::Item::Data::*getter)() const, std::size_t minCount)
      : Props(props)
      , Getter(getter)
      , MinCount(minCount)
    {
    }

    virtual bool OnItem(const Playlist::Item::Data& data) const
    {
      const T val = (data.*Getter)();
      if (!data.IsValid())
      {
        return false;
      }
      const typename std::map<T, std::size_t>::const_iterator it = Props.find(val);
      return it != Props.end() && it->second >= MinCount;
    }
  private:
    const std::map<T, std::size_t>& Props;
    typedef T (Playlist::Item::Data::*GetFunc)() const;
    const GetFunc Getter;
    const std::size_t MinCount;
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
      const Playlist::Model::Ptr model = Controller->GetModel();
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems();
      std::map<uint32_t, std::size_t> checksumms;
      SelectItemsProperties(items, checksumms, &Playlist::Item::Data::GetChecksum);
      const DuplicatedItemsFilter<uint32_t> filter(checksumms, &Playlist::Item::Data::GetChecksum);
      const Playlist::Model::IndexSet toRemove = model->GetItemIndices(filter);
      model->RemoveItems(toRemove);
    }

    virtual void RemoveDuplicatesOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      assert(!SelectedItems.empty());
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems(SelectedItems);
      std::map<uint32_t, std::size_t> checksumms;
      SelectItemsProperties(items, checksumms, &Playlist::Item::Data::GetChecksum);
      const DuplicatedItemsFilter<uint32_t> filter(checksumms, &Playlist::Item::Data::GetChecksum);
      const Playlist::Model::IndexSet toRemove = model->GetItemIndices(filter);
      model->RemoveItems(toRemove);
    }

    virtual void RemoveDuplicatesInSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      assert(!SelectedItems.empty());
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems(SelectedItems);
      std::map<uint32_t, std::size_t> checksumms;
      SelectItemsProperties(items, checksumms, &Playlist::Item::Data::GetChecksum);
      const DuplicatedItemsFilter<uint32_t> filter(checksumms, &Playlist::Item::Data::GetChecksum);
      const Playlist::Model::IndexSet toRemove = model->GetItemIndices(SelectedItems, filter);
      model->RemoveItems(toRemove);
    }

    virtual void SelectAllRipOffs() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems();
      std::map<uint32_t, std::size_t> checksumms;
      SelectItemsProperties(items, checksumms, &Playlist::Item::Data::GetCoreChecksum);
      const SimilarItemsFilter<uint32_t> filter(checksumms, &Playlist::Item::Data::GetCoreChecksum, 2);
      const Playlist::Model::IndexSet toSelect = model->GetItemIndices(filter);
      View.SelectItems(toSelect);
    }

    virtual void SelectRipOffsOfSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      assert(!SelectedItems.empty());
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems(SelectedItems);
      std::map<uint32_t, std::size_t> checksumms;
      SelectItemsProperties(items, checksumms, &Playlist::Item::Data::GetCoreChecksum);
      const SimilarItemsFilter<uint32_t> filter(checksumms, &Playlist::Item::Data::GetCoreChecksum, 1);
      const Playlist::Model::IndexSet toSelect = model->GetItemIndices(filter);
      View.SelectItems(toSelect);
    }

    virtual void SelectRipOffsInSelected() const
    {
      const Playlist::Model::Ptr model = Controller->GetModel();
      assert(!SelectedItems.empty());
      Playlist::Item::Data::Iterator::Ptr items = model->GetItems(SelectedItems);
      std::map<uint32_t, std::size_t> checksumms;
      SelectItemsProperties(items, checksumms, &Playlist::Item::Data::GetCoreChecksum);
      const SimilarItemsFilter<uint32_t> filter(checksumms, &Playlist::Item::Data::GetCoreChecksum, 1);
      const Playlist::Model::IndexSet toSelect = model->GetItemIndices(SelectedItems, filter);
      View.SelectItems(toSelect);
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
