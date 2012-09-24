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

#include <playlist/supp/operations.h>

class QWidget;

namespace Playlist
{
  namespace UI
  {
    Playlist::Item::SelectionOperation::Ptr ExecuteSearchDialog(QWidget& parent);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_SEARCH_H_DEFINED
