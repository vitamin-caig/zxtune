/*
Abstract:
  Playlist table view interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_UI_TABLE_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_UI_TABLE_VIEW_H_DEFINED

//qt includes
#include <QtGui/QItemDelegate>
#include <QtGui/QTableView>

namespace Playlist
{
  class Model;

  namespace Item
  {
    class Data;
  }

  namespace UI
  {
    class TableViewStateCallback
    {
    public:
      virtual ~TableViewStateCallback() {}

      virtual bool IsPlaying(const QModelIndex& index) const = 0;
      virtual bool IsPaused(const QModelIndex& index) const = 0;
    };

    //table view
    class TableViewItem : public QItemDelegate
    {
      Q_OBJECT
    protected:
      explicit TableViewItem(QWidget& parent);
    public:
      static TableViewItem* Create(QWidget& parent, const TableViewStateCallback& callback);
    };

    class TableView : public QTableView
    {
      Q_OBJECT
    protected:
      explicit TableView(QWidget& parent);
    public:
      //creator
      static TableView* Create(QWidget& parent, const TableViewStateCallback& callback, Playlist::Model& model);

    public slots:
      virtual void ActivateItem(const QModelIndex&) = 0;
    signals:
      void OnItemActivated(unsigned index, const Playlist::Item::Data& item);
    };
  }
}

#endif //ZXTUNE_QT_PLAYLIST_UI_TABLE_VIEW_H_DEFINED
