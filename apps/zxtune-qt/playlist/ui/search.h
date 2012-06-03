/*
Abstract:
  Playlist search

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_SEARCH_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_SEARCH_H_DEFINED

namespace Playlist
{
  class Controller;
  namespace UI
  {
    class TableView;
    void ExecuteSearchDialog(TableView& view, Controller& controller);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_SEARCH_H_DEFINED
