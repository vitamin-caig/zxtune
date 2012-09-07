/*
Abstract:
  Playlist table view interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_UI_TABLE_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_TABLE_VIEW_H_DEFINED

//local includes
#include "playlist/supp/model.h"
//std includes
#include <set>
//qt includes
#include <QtGui/QItemDelegate>
#include <QtGui/QTableView>

namespace Playlist
{
  namespace Item
  {
    class Data;
    class StateCallback;
  }

  namespace UI
  {
    //table view
    class TableViewItem : public QItemDelegate
    {
      Q_OBJECT
    protected:
      explicit TableViewItem(QWidget& parent);
    public:
      static TableViewItem* Create(QWidget& parent, const Item::StateCallback& callback);
    };

    class TableView : public QTableView
    {
      Q_OBJECT
    protected:
      explicit TableView(QWidget& parent);
    public:
      //creator
      static TableView* Create(QWidget& parent, const Item::StateCallback& callback, QAbstractItemModel& model);

      virtual Model::IndexSetPtr GetSelectedItems() const = 0;
      virtual void SelectItems(const Playlist::Model::IndexSet& indices) = 0;
      virtual void ActivateTableRow(unsigned index) = 0;
    public slots:
      virtual void SelectItems(Playlist::Model::IndexSetPtr indices) = 0;
    private slots:
      virtual void ActivateItem(const QModelIndex&) = 0;
    signals:
      void TableRowActivated(unsigned index);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_TABLE_VIEW_H_DEFINED
