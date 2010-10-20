/*
Abstract:
  Playlist view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED

//qt includes
#include <QtGui/QItemDelegate>
#include <QtGui/QTableView>

class PlayitemStateCallback
{
public:
  virtual ~PlayitemStateCallback() {}

  virtual bool IsPlaying(const QModelIndex& index) const = 0;
  virtual bool IsPaused(const QModelIndex& index) const = 0;
};

class PlaylistItemView : public QItemDelegate
{
  Q_OBJECT
public:
  static PlaylistItemView* Create(const PlayitemStateCallback& callback, QWidget* parent = 0);
};

class PlaylistView : public QTableView
{
  Q_OBJECT
public:
  //creator
  static PlaylistView* Create(const PlayitemStateCallback& callback, QWidget* parent = 0);
};

#endif //ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED
