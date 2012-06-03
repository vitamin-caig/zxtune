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

class QPoint;

namespace Playlist
{
  class Controller;
  namespace UI
  {
    class TableView;
    void ExecuteContextMenu(const QPoint& pos, TableView& view, Controller& playlist);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_CONTEXTMENU_H_DEFINED
