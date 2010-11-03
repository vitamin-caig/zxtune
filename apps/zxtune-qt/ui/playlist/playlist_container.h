/*
Abstract:
  Playlist container

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#ifndef ZXTUNE_QT_PLAYLIST_CONTAINER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_CONTAINER_H_DEFINED

//local includes
#include "supp/playitems_provider.h"
//qt includes
#include <QtGui/QWidget>

class PlaylistContainer : public QObject
{
  Q_OBJECT
public:
  //creator
  static PlaylistContainer* Create(QObject* parent);

  virtual class PlaylistSupport* CreatePlaylist(const QString& name) = 0;
};

class PlaylistContainerView : public QWidget
{
  Q_OBJECT
public:
  //creator
  static PlaylistContainerView* Create(QWidget* parent);

  virtual void CreatePlaylist(const class QStringList& items) = 0;
  virtual class QMenu* GetActionsMenu() const = 0;

public slots:
  //navigate
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void Stop() = 0;
  virtual void Finish() = 0;
  virtual void Next() = 0;
  virtual void Prev() = 0;
  //actions
  virtual void Clear() = 0;
  virtual void AddFiles() = 0;
  virtual void AddFolders() = 0;
signals:
  void OnItemActivated(const Playitem&);
};

#endif //ZXTUNE_QT_PLAYLIST_CONTAINER_H_DEFINED
