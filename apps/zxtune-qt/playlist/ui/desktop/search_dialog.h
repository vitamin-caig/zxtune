/*
Abstract:
  Playlist search dialog for desktop

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_DESKTOP_SEARCH_DIALOG_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_DESKTOP_SEARCH_DIALOG_H_DEFINED

//local includes
#include "playlist/ui/search.h"
#include "playlist/supp/model.h"
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

    void ExecuteSearchDialog(TableView& view, Playlist::Model::IndexSetPtr scope, Controller& controller);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_DESKTOP_SEARCH_DIALOG_H_DEFINED
