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
#include <QtCore/QObject>

namespace Playlist
{
  namespace UI
  {
    class TableView;

    class ItemsContextMenu : public QObject
    {
      Q_OBJECT
    protected:
      explicit ItemsContextMenu(QObject& parent);
    public:
      static ItemsContextMenu* Create(TableView& view, Playlist::Controller::Ptr playlist);

      virtual void Exec(const QPoint& pos) = 0;
    private slots:
      virtual void PlaySelected() const = 0;
      virtual void RemoveSelected() const = 0;
      virtual void CropSelected() const = 0;
      virtual void GroupSelected() const = 0;
      virtual void RemoveAllDuplicates() const = 0;
      virtual void RemoveDuplicatesOfSelected() const = 0;
      virtual void RemoveDuplicatesInSelected() const = 0;
      virtual void SelectAllRipOffs() const = 0;
      virtual void SelectRipOffsOfSelected() const = 0;
      virtual void SelectRipOffsInSelected() const = 0;
      virtual void CopyPathToClipboard() const = 0;
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_CONTEXTMENU_H_DEFINED
