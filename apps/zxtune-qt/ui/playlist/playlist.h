/*
Abstract:
  Playlist entity and view

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//qt includes
#include <QtCore/QModelIndex>
#include <QtGui/QWidget>

enum PlayitemState
{
  STOPPED,
  PAUSED,
  PLAYING
};

class PlayitemIterator : public QObject
{
  Q_OBJECT
public:
  //access
  virtual const Playitem* Get() const = 0;
  virtual PlayitemState GetState() const = 0;
  //change
  virtual void SetState(PlayitemState state) = 0;
public slots:
  //navigate
  virtual void Reset(unsigned idx) = 0;
  virtual bool Next() = 0;
  virtual bool Prev() = 0;
signals:
  void OnItem(const Playitem&);
};

class PlaylistSupport : public QObject
{
  Q_OBJECT
public:
  static PlaylistSupport* Create(QObject* parent, const QString& name, PlayitemsProvider::Ptr provider);

  virtual class PlaylistScanner& GetScanner() const = 0;
  virtual class PlaylistModel& GetModel() const = 0;
  virtual PlayitemIterator& GetIterator() const = 0;
};

class PlaylistView : public QWidget
{
  Q_OBJECT
public:
  static PlaylistView* Create(QWidget* parent, const PlaylistSupport& playlist);

  virtual const PlaylistSupport& GetPlaylist() const = 0;
  virtual void Update() = 0;
public slots:
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Stop() = 0;
};

#endif //ZXTUNE_QT_PLAYLIST_H_DEFINED
