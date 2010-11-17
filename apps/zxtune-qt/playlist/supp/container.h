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

//common includes
#include <parameters.h>
//qt includes
#include <QtCore/QObject>

class PlaylistContainer : public QObject
{
  Q_OBJECT
protected:
  explicit PlaylistContainer(QObject& parent);
public:
  //creator
  static PlaylistContainer* Create(QObject& parent, Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams);

  virtual class PlaylistSupport* CreatePlaylist(const QString& name) = 0;
  virtual class PlaylistSupport* OpenPlaylist(const QString& filename) = 0;
};

#endif //ZXTUNE_QT_PLAYLIST_CONTAINER_H_DEFINED
