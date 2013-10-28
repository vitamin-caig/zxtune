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
#include <QtGui/QDialog>

namespace Playlist
{
  namespace UI
  {
    class SearchDialog : public QDialog
    {
      Q_OBJECT
    protected:
      explicit SearchDialog(QWidget& parent);
    public:
      typedef boost::shared_ptr<SearchDialog> Ptr;
      static Ptr Create(QWidget& parent);

      virtual bool Execute(Playlist::Item::Search::Data& res) = 0;
    };

    Playlist::Item::SelectionOperation::Ptr ExecuteSearchDialog(QWidget& parent, Playlist::Model::IndexSetPtr scope);
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_PLAYLIST_DESKTOP_SEARCH_DIALOG_H_DEFINED
