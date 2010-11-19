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

namespace Playlist
{
  class Container : public QObject
  {
    Q_OBJECT
  protected:
    explicit Container(QObject& parent);
  public:
    //creator
    static Container* Create(QObject& parent, Parameters::Accessor::Ptr ioParams, Parameters::Accessor::Ptr coreParams);

    virtual class Support* CreatePlaylist(const QString& name) = 0;
    virtual class Support* OpenPlaylist(const QString& filename) = 0;
  };
}

#endif //ZXTUNE_QT_PLAYLIST_CONTAINER_H_DEFINED
