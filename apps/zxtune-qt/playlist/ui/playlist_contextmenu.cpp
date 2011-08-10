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
//text includes
#include "text/text.h"

namespace
{
  class ItemsContextMenuImpl : public Playlist::UI::ItemsContextMenu
  {
  public:
    ItemsContextMenuImpl(QWidget& parent, Playlist::Controller::Ptr playlist)
      : Playlist::UI::ItemsContextMenu(parent)
      , Controller(playlist)
      , InfoAction(this)
      , PlayAction(QIcon(":/playback/play.png"), tr("Play"), this)
      , DeleteAction(QIcon(":/playlist/delete.png"), tr("Delete"), this)
      , CropAction(QIcon(":/playlist/crop.png"), tr("Crop"), this)
    {
      AddActions();

      InfoAction.setEnabled(false);
      this->connect(&PlayAction, SIGNAL(triggered()), SLOT(PlaySelected()));
      this->connect(&DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      this->connect(&CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
    }

    virtual void Exec(const QSet<unsigned>& items, const QPoint& pos)
    {
      assert(items.size());
      SelectedItems = items;
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
      QSet<unsigned> unselected;
      for (unsigned idx = 0, total = model->CountItems(); idx < total; ++idx)
      {
        if (!SelectedItems.contains(idx))
        {
          unselected.insert(idx);
        }
      }
      model->RemoveItems(unselected);
    }
  private:
    void AddActions()
    {
      addAction(&InfoAction);
      addAction(&PlayAction);
      addAction(&DeleteAction);
      addAction(&CropAction);
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
      SetupPlayitemMenu();
    }

    void SetupMultiplePlayitemsMenu(int count)
    {
      InfoAction.setVisible(true);
      InfoAction.setText(ToQString(Strings::Format(Text::CONTEXTMENU_STATUS, count)));
      PlayAction.setEnabled(false);
      SetupPlayitemMenu();
    }

    void SetupPlayitemMenu()
    {
      DeleteAction.setEnabled(true);
      CropAction.setEnabled(true);
    }
  private:
    const Playlist::Controller::Ptr Controller;
    QAction InfoAction;
    QAction PlayAction;
    QAction DeleteAction;
    QAction CropAction;
    //data
    QSet<unsigned> SelectedItems;
  };
}

namespace Playlist
{
  namespace UI
  {
    ItemsContextMenu::ItemsContextMenu(QWidget& parent) : QMenu(&parent)
    {
    }

    ItemsContextMenu* ItemsContextMenu::Create(QWidget& parent, Playlist::Controller::Ptr playlist)
    {
      return new ItemsContextMenuImpl(parent, playlist);
    }
  }
}
