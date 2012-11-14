/*
Abstract:
  Playlist container

Last changed:
  $Id$

Author:
  (C) Vitamin/CAIG/2001

  This file is a part of zxtune-qt application based on zxtune library
*/

#pragma once
#ifndef ZXTUNE_QT_PLAYLIST_SUPP_CONTAINER_H_DEFINED
#define ZXTUNE_QT_PLAYLIST_SUPP_CONTAINER_H_DEFINED

//local includes
#include "controller.h"
//common includes
#include <parameters.h>
//qt includes
#include <QtCore/QObject>

namespace Playlist
{
  class Container : public QObject
  {
    Q_OBJECT
  public:
    typedef boost::shared_ptr<Container> Ptr;

    //creator
    static Ptr Create(Parameters::Accessor::Ptr parameters);

    virtual Controller::Ptr CreatePlaylist(const QString& name) const = 0;
    virtual void OpenPlaylist(const QString& filename) = 0;
  signals:
    void PlaylistCreated(Playlist::Controller::Ptr);
  };

  void Save(Controller::Ptr ctrl, const QString& filename, uint_t flags);
}

#endif //ZXTUNE_QT_PLAYLIST_SUPP_CONTAINER_H_DEFINED
