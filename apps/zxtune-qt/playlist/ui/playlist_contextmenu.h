/*
Abstract:
  Playlist context menu

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_CONTEXTMENU_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_CONTEXTMENU_H_DEFINED

//local includes
#include "playlist/supp/controller.h"
//qt includes
#include <QtGui/QMenu>

namespace Playlist
{
  namespace UI
  {
    class TableView;

    class ContextMenu : public QMenu
    {
      Q_OBJECT
    protected:
      explicit ContextMenu(QWidget& parent);
    public:
      static ContextMenu* Create(TableView& view, Playlist::Controller::Ptr playlist);
    public slots:
      virtual void PlaySelected() const = 0;
      virtual void RemoveSelected() const = 0;
      virtual void CropSelected() const = 0;
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_CONTEXTMENU_H_DEFINED
