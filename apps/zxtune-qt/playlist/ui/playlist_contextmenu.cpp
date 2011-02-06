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

namespace
{
  class ContextMenuImpl : public Playlist::UI::ContextMenu
  {
  public:
    ContextMenuImpl(Playlist::UI::TableView& view, Playlist::Controller::Ptr playlist)
      : Playlist::UI::ContextMenu(view)
      , View(view)
      , Controller(playlist)
      , PlayAction(QIcon(":/playback/play.png"), tr("Play"), this)
      , DeleteAction(QIcon(":/playlist/delete.png"), tr("Delete"), this)
      , CropAction(QIcon(":/playlist/crop.png"), tr("Crop"), this)
    {
      AddActions();

      this->connect(&PlayAction, SIGNAL(triggered()), SLOT(PlaySelected()));
      this->connect(&DeleteAction, SIGNAL(triggered()), SLOT(RemoveSelected()));
      this->connect(&CropAction, SIGNAL(triggered()), SLOT(CropSelected()));
    }


    virtual void PlaySelected() const
    {
      QSet<unsigned> selected;
      View.GetSelectedItems(selected);
      if (!selected.empty())
      {
        const Playlist::Item::Iterator::Ptr iter = Controller->GetIterator();
        iter->Reset(*selected.begin());
      }
    }

    virtual void RemoveSelected() const
    {
      QSet<unsigned> selected;
      View.GetSelectedItems(selected);
      const Playlist::Model::Ptr model = Controller->GetModel();
      model->RemoveItems(selected);
    }

    virtual void CropSelected() const
    {
      QSet<unsigned> selected;
      View.GetSelectedItems(selected);
      const Playlist::Model::Ptr model = Controller->GetModel();
      QSet<unsigned> unselected;
      for (unsigned idx = 0, total = model->CountItems(); idx < total; ++idx)
      {
        if (!selected.contains(idx))
        {
          unselected.insert(idx);
        }
      }
      model->RemoveItems(unselected);
    }
  private:
    void AddActions()
    {
      addAction(&PlayAction);
      addAction(&DeleteAction);
      addAction(&CropAction);
    }
  private:
    Playlist::UI::TableView& View;
    const Playlist::Controller::Ptr Controller;
    QAction PlayAction;
    QAction DeleteAction;
    QAction CropAction;
  };
}

namespace Playlist
{
  namespace UI
  {
    ContextMenu::ContextMenu(QWidget& parent) : QMenu(&parent)
    {
    }

    ContextMenu* ContextMenu::Create(TableView& view, Playlist::Controller::Ptr playlist)
    {
      return new ContextMenuImpl(view, playlist);
    }
  }
}
