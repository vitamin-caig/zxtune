/*
Abstract:
  Playlist model

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_MODEL_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_MODEL_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//qt includes
#include <QtCore/QAbstractItemModel>

class PlayitemStateCallback
{
public:
  virtual ~PlayitemStateCallback() {}

  virtual bool IsPlaying(const QModelIndex& index) const = 0;
  virtual bool IsPaused(const QModelIndex& index) const = 0;
};

class PlaylistModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  enum Columns
  {
    COLUMN_TITLE,
    COLUMN_DURATION,

    COLUMNS_COUNT
  };

  //creator
  static PlaylistModel* Create(const PlayitemStateCallback& callback, QObject* parent = 0);

  virtual Playitem::Ptr GetItem(const QModelIndex& index) const = 0;
  virtual void Clear() = 0;
  virtual void RemoveItems(const QModelIndexList& items) = 0;
public slots:
  virtual void AddItem(Playitem::Ptr item) = 0;
};

#endif //ZXTUNE_QT_PLAYLIST_MODEL_H_DEFINED
