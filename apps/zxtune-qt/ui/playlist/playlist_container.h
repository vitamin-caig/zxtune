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

  virtual class Playlist* CreatePlaylist(const QString& name) = 0;
  virtual class Playlist* CreatePlaylist(const class QStringList& items) = 0;
};

class PlaylistContainerView : public QWidget
{
  Q_OBJECT
public:
  //creator
  static PlaylistContainerView* Create(class QMainWindow* parent);

  virtual void CreatePlaylist(const class QStringList& items) = 0;
signals:
  void OnItemSet(const Playitem&);
};

#endif //ZXTUNE_QT_PLAYLIST_CONTAINER_H_DEFINED
