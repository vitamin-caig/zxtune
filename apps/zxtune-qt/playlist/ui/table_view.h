/*
Abstract:
  Playlist table view interface

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_TABLE_VIEW_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_TABLE_VIEW_H_DEFINED

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

//table view
class PlaylistItemTableView : public QItemDelegate
{
  Q_OBJECT
protected:
  explicit PlaylistItemTableView(QWidget& parent);
public:
  static PlaylistItemTableView* Create(QWidget& parent, const PlayitemStateCallback& callback);
};

class PlaylistTableView : public QTableView
{
  Q_OBJECT
protected:
  explicit PlaylistTableView(QWidget& parent);
public:
  //creator
  static PlaylistTableView* Create(QWidget& parent, const PlayitemStateCallback& callback, class PlaylistModel& model);

public slots:
  virtual void ActivateItem(const QModelIndex&) = 0;
signals:
  void OnItemActivated(unsigned index, const class Playitem& item);
};

#endif //ZXTUNE_QT_PLAYLIST_VIEW_H_DEFINED
