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

//qt includes
#include <QtCore/QObject>

class PlaylistContainer : public QObject
{
  Q_OBJECT
public:
  //creator
  static PlaylistContainer* Create(QObject* parent);

  virtual class PlaylistSupport* CreatePlaylist(const QString& name) = 0;
};

#endif //ZXTUNE_QT_PLAYLIST_CONTAINER_H_DEFINED
