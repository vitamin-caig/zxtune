/*
Abstract:
  Playlist search dialog

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_SEARCH_DIALOG_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_SEARCH_DIALOG_H_DEFINED

//local includes
#include "playlist/supp/operations_search.h"
//qt includes
#include <QtGui/QWidget>

namespace Playlist
{
  namespace UI
  {
    class SearchWidget : public QWidget
    {
      Q_OBJECT
    protected:
      explicit SearchWidget(QWidget& parent);
    public:
      static SearchWidget* Create(QWidget& parent);

      virtual Playlist::Item::Search::Data GetData() const = 0;
    };

    bool ShowSearchDialog(QWidget& parent, Playlist::Item::Search::Data& data);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_SEARCH_DIALOG_H_DEFINED
